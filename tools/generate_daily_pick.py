#!/usr/bin/env python3
import argparse
import datetime as dt
import gzip
import io
import json
import os
import random
import tempfile
import urllib.request
import zipfile
from pathlib import Path

DEFAULT_SOURCE_URL = "https://r18.dev/dumps/latest"
DEFAULT_PREFERENCES = {"tags": [], "makers": [], "actresses": [], "avoid_tags": []}


def read_json(path, fallback):
    if not path.exists():
        return fallback
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def text(value):
    if value is None:
        return ""
    if isinstance(value, str):
        return value
    return str(value)


def first(record, *keys):
    for key in keys:
        value = record
        for part in key.split("."):
            if not isinstance(value, dict) or part not in value:
                value = None
                break
            value = value[part]
        if value not in (None, "", []):
            return value
    return None


def as_list(value):
    if not value:
        return []
    if isinstance(value, list):
        return [text(item.get("name", item) if isinstance(item, dict) else item) for item in value]
    if isinstance(value, str):
        return [part.strip() for part in value.replace("、", ",").split(",") if part.strip()]
    return [text(value)]


def download(url, target):
    request = urllib.request.Request(
        url,
        headers={
            "User-Agent": "DeskOrbDailyPick/1.0 (+https://github.com/)",
            "Accept": "application/json, application/gzip, application/zip, */*",
        },
    )
    with urllib.request.urlopen(request, timeout=180) as response:
        with target.open("wb") as handle:
            while True:
                chunk = response.read(1024 * 512)
                if not chunk:
                    break
                handle.write(chunk)


def iter_lines_from_dump(path):
    raw = path.read_bytes()[:4]
    if zipfile.is_zipfile(path):
        with zipfile.ZipFile(path) as archive:
            names = [name for name in archive.namelist() if name.endswith((".json", ".jsonl"))]
            if not names:
                return
            with archive.open(names[0]) as handle:
                for line in io.TextIOWrapper(handle, encoding="utf-8", errors="ignore"):
                    yield line
        return

    opener = gzip.open if raw.startswith(b"\x1f\x8b") else open
    with opener(path, "rt", encoding="utf-8", errors="ignore") as handle:
        start = handle.read(1)
        handle.seek(0)
        if start == "[":
            data = json.load(handle)
            for item in data:
                yield json.dumps(item, ensure_ascii=False)
        else:
            yield from handle


def normalize(record):
    code = text(first(record, "dvd_id", "dvdId", "content_id", "contentId", "id", "product_id")).upper()
    title = text(first(record, "title", "name", "display_title", "japanese_title", "original_title"))
    cover = text(first(record, "jacket_full_url", "jacket.large", "image.large", "imageURL.large", "cover", "cover_url"))
    release_date = text(first(record, "release_date", "date", "released_at", "publication_date"))
    maker = text(first(record, "maker.name", "studio.name", "label.name", "maker", "studio", "label"))
    actresses = as_list(first(record, "actresses", "performers", "casts", "stars"))
    tags = as_list(first(record, "genres", "categories", "tags"))
    if not code or not title:
        return None
    return {
        "code": code,
        "title": title,
        "cover": cover,
        "releaseDate": release_date,
        "maker": maker,
        "actresses": actresses[:5],
        "tags": tags[:8],
        "source": "R18.dev",
    }


def score(item, preferences):
    haystack = " ".join([item["title"], item["maker"], " ".join(item["actresses"]), " ".join(item["tags"])]).lower()
    value = 1
    for tag in preferences.get("tags", []):
        if tag.lower() in haystack:
            value += 8
    for maker in preferences.get("makers", []):
        if maker.lower() in haystack:
            value += 5
    for actress in preferences.get("actresses", []):
        if actress.lower() in haystack:
            value += 10
    for tag in preferences.get("avoid_tags", []):
        if tag.lower() in haystack:
            value -= 100
    return value


def choose_daily(items, preferences):
    today = dt.date.today().isoformat()
    random.seed(today)
    scored = [(score(item, preferences), random.random(), item) for item in items]
    scored = [entry for entry in scored if entry[0] > 0]
    if not scored:
        return None
    scored.sort(key=lambda entry: (entry[0], entry[1]), reverse=True)
    return scored[0][2]


def write_site(out_dir, payload):
    out_dir.mkdir(parents=True, exist_ok=True)
    (out_dir / "today.json").write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding="utf-8")
    (out_dir / "index.html").write_text(
        """<!doctype html>
<html lang=\"zh-CN\">
<head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>Desk Orb Daily Pick</title></head>
<body><pre id=\"out\">loading...</pre><script>
fetch('./today.json').then(r=>r.json()).then(j=>out.textContent=JSON.stringify(j,null,2));
</script></body></html>
""",
        encoding="utf-8",
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", default="daily-pick-site")
    parser.add_argument("--preferences", default="daily-pick-preferences.json")
    parser.add_argument("--limit", type=int, default=4000)
    parser.add_argument("--source-url", default=None)
    parser.add_argument("--source-file", default=None)
    args = parser.parse_args()

    root = Path.cwd()
    out_dir = root / args.out
    preferences = read_json(root / args.preferences, DEFAULT_PREFERENCES)
    source_url = args.source_url or os.environ.get("DAILY_PICK_SOURCE_URL", DEFAULT_SOURCE_URL)
    generated_at = dt.datetime.now(dt.timezone.utc).isoformat()

    try:
        temp_holder = None
        if args.source_file:
            dump_path = Path(args.source_file)
            source_label = str(dump_path)
        else:
            temp_holder = tempfile.TemporaryDirectory(ignore_cleanup_errors=True)
            dump_path = Path(temp_holder.name) / "r18-dump"
            download(source_url, dump_path)
            source_label = source_url

        items = []
        for line in iter_lines_from_dump(dump_path):
            line = line.strip()
            if not line:
                continue
            try:
                record = json.loads(line)
            except json.JSONDecodeError:
                continue
            item = normalize(record)
            if item:
                items.append(item)
            if len(items) >= args.limit:
                break

        pick = choose_daily(items, preferences)
        if not pick:
            raise RuntimeError("No valid items found in source data")

        payload = {
            "ok": True,
            "generatedAt": generated_at,
            "date": dt.date.today().isoformat(),
            "source": source_label,
            "pick": pick,
            "reason": "Selected by preference tags and daily seed.",
        }
    except Exception as exc:
        payload = {
            "ok": False,
            "generatedAt": generated_at,
            "date": dt.date.today().isoformat(),
            "source": source_url,
            "error": str(exc),
            "pick": None,
        }

    write_site(out_dir, payload)


if __name__ == "__main__":
    main()



