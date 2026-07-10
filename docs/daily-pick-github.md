# Daily Pick GitHub Free Route

这个目录/脚本用于免费自动生成每日推荐 JSON，不需要你的电脑一直开着。

## 架构

```text
GitHub Actions 定时任务
  -> 运行 tools/generate_daily_pick.py
  -> 读取公开元数据源 R18.dev dump
  -> 按 daily-pick-preferences.json 偏好筛选
  -> 生成 daily-pick-site/today.json
  -> 发布到 GitHub Pages
  -> ESP32 读取 https://USER.github.io/REPO/today.json
```

## 文件

- `.github/workflows/daily-pick-pages.yml`：每天自动运行并部署 Pages
- `tools/generate_daily_pick.py`：生成今日推荐 JSON
- `daily-pick-preferences.json`：偏好配置
- `daily-pick-site/today.json`：生成结果，给 ESP32 读取

## GitHub 设置

1. 把项目推到 GitHub。
2. 打开仓库 Settings -> Pages。
3. Source 选择 GitHub Actions。
4. 打开 Actions，手动运行 `Daily Pick Pages` 一次。
5. 成功后 JSON 地址通常是：

```text
https://你的GitHub用户名.github.io/仓库名/today.json
```

然后把这个地址填进：

```text
firmware/desk-orb-social/daily_pick_config.h
```

## 本地测试

```powershell
python tools/generate_daily_pick.py --source-file tools/sample_daily_pick.jsonl --out daily-pick-site
```

正式 GitHub Actions 会使用默认源：

```text
https://r18.dev/dumps/latest
```

如果 R18.dev 在 GitHub Actions 里访问失败，`today.json` 会输出 `ok:false` 和错误信息，ESP32 可以继续显示上一次缓存。
