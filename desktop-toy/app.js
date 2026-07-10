const canvas = document.querySelector("#screen");
const ctx = canvas.getContext("2d");

const modeLabel = document.querySelector("#modeLabel");
const tempLabel = document.querySelector("#tempLabel");
const focusLabel = document.querySelector("#focusLabel");
const clockText = document.querySelector("#clockText");
const dateText = document.querySelector("#dateText");
const dayLine = document.querySelector("#dayLine");
const tabs = [...document.querySelectorAll(".tab")];
const brightness = document.querySelector("#brightness");
const tempo = document.querySelector("#tempo");
const habit = document.querySelector("#habit");
const countdownName = document.querySelector("#countdownName");
const countdownDate = document.querySelector("#countdownDate");
const focusToggle = document.querySelector("#focusToggle");
const focusReset = document.querySelector("#focusReset");
const spark = document.querySelector("#spark");

const storageKey = "desk-orb-settings";
const defaultTarget = new Date();
defaultTarget.setDate(defaultTarget.getDate() + 21);

const socialAccounts = window.DESK_ORB_SOCIAL || [];
const saved = loadSettings();
const state = {
  mode: saved.mode || "orbit",
  brightness: saved.brightness ?? 0.82,
  tempo: saved.tempo ?? 0.46,
  habit: saved.habit ?? 64,
  countdownName: saved.countdownName || "旅行",
  countdownDate: saved.countdownDate || toDateInput(defaultTarget),
  focusSeconds: saved.focusSeconds ?? 25 * 60,
  focusRunning: false,
  lastTick: performance.now(),
  lineIndex: saved.lineIndex ?? 0,
  accountIndex: saved.accountIndex ?? 0,
  lastAccountTurn: performance.now(),
};

const lines = [
  "把大事缩小，把小事做完。",
  "账号状态不错，继续发光。",
  "今天只赢一小格，也算赢。",
  "少一点内耗，多一点动手。",
  "先喝水，再开工。",
  "不用等灵感，先摆好姿势。",
  "把今天调成好看的难度。",
];

function loadSettings() {
  try {
    return JSON.parse(localStorage.getItem(storageKey)) || {};
  } catch {
    return {};
  }
}

function saveSettings() {
  localStorage.setItem(
    storageKey,
    JSON.stringify({
      mode: state.mode,
      brightness: state.brightness,
      tempo: state.tempo,
      habit: state.habit,
      countdownName: state.countdownName,
      countdownDate: state.countdownDate,
      focusSeconds: state.focusSeconds,
      lineIndex: state.lineIndex,
      accountIndex: state.accountIndex,
    }),
  );
}

function pad(value) {
  return String(value).padStart(2, "0");
}

function toDateInput(date) {
  return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())}`;
}

function shortNumber(value) {
  if (!value || value <= 0) return "--";
  const abs = Math.abs(value);
  if (abs >= 10000) return `${(value / 10000).toFixed(abs >= 100000 ? 0 : 1)}w`;
  if (abs >= 1000) return `${(value / 1000).toFixed(1)}k`;
  return String(value);
}

function currentAccount() {
  if (!socialAccounts.length) return null;
  return socialAccounts[state.accountIndex % socialAccounts.length];
}

function nextAccount() {
  if (!socialAccounts.length) return;
  state.accountIndex = (state.accountIndex + 1) % socialAccounts.length;
  state.lastAccountTurn = performance.now();
  updateDayLine();
  saveSettings();
}

function countdownDays() {
  const target = new Date(`${state.countdownDate}T00:00:00`);
  const today = new Date();
  today.setHours(0, 0, 0, 0);
  return Math.ceil((target - today) / 86400000);
}

function setMode(nextMode) {
  state.mode = nextMode;
  tabs.forEach((tab) => tab.classList.toggle("is-active", tab.dataset.mode === nextMode));
  modeLabel.textContent = nextMode.toUpperCase();
  updateDayLine();
  saveSettings();
}

function updateDayLine() {
  const account = currentAccount();
  if (state.mode === "radar" && account) {
    dayLine.textContent = `${account.name}: ${account.nickname} / ${shortNumber(account.followers)} 粉丝`;
    return;
  }
  dayLine.textContent = lines[state.lineIndex];
}

function formatFocus() {
  const minutes = Math.floor(state.focusSeconds / 60);
  const seconds = state.focusSeconds % 60;
  return `${pad(minutes)}:${pad(seconds)}`;
}

function drawRoundRect(x, y, width, height, radius) {
  const r = Math.min(radius, width / 2, height / 2);
  ctx.beginPath();
  ctx.moveTo(x + r, y);
  ctx.arcTo(x + width, y, x + width, y + height, r);
  ctx.arcTo(x + width, y + height, x, y + height, r);
  ctx.arcTo(x, y + height, x, y, r);
  ctx.arcTo(x, y, x + width, y, r);
  ctx.closePath();
}

function background(now) {
  const b = state.brightness;
  const account = currentAccount();
  const hue = state.mode === "night" ? 226 : state.mode === "focus" ? 158 : state.mode === "radar" ? 12 : 198;
  const grd = ctx.createLinearGradient(0, 0, 240, 240);
  grd.addColorStop(0, `hsl(${hue}, 54%, ${92 * b}%)`);
  grd.addColorStop(1, `hsl(${hue + 34}, 48%, ${74 * b}%)`);
  ctx.fillStyle = grd;
  ctx.fillRect(0, 0, 240, 240);

  if (state.mode === "radar" && account) {
    ctx.globalAlpha = 0.12;
    ctx.fillStyle = account.color;
    ctx.beginPath();
    ctx.arc(198, 54, 54 + Math.sin(now / 700) * 3, 0, Math.PI * 2);
    ctx.fill();
    ctx.beginPath();
    ctx.arc(32, 202, 38 + Math.cos(now / 900) * 2, 0, Math.PI * 2);
    ctx.fill();
    ctx.globalAlpha = 1;
  }

  ctx.globalAlpha = 0.2;
  ctx.strokeStyle = "#ffffff";
  for (let y = 12; y < 240; y += 24) {
    ctx.beginPath();
    ctx.moveTo(0, y + Math.sin(now / 1100 + y) * 2);
    ctx.lineTo(240, y + Math.cos(now / 1300 + y) * 2);
    ctx.stroke();
  }
  ctx.globalAlpha = 1;
}

function drawHeader(date) {
  ctx.fillStyle = "rgba(255,255,255,.7)";
  drawRoundRect(12, 12, 216, 34, 7);
  ctx.fill();

  ctx.fillStyle = "#263445";
  ctx.font = "700 14px 'Segoe UI', sans-serif";
  ctx.textBaseline = "middle";
  ctx.fillText(`${pad(date.getHours())}:${pad(date.getMinutes())}`, 22, 29);

  ctx.font = "700 11px 'Segoe UI', sans-serif";
  ctx.textAlign = "right";
  ctx.fillText(state.mode.toUpperCase(), 218, 29);
  ctx.textAlign = "left";
}

function drawMiniStats(now) {
  const days = countdownDays();
  const temp = 22 + Math.round(Math.sin(now / 9000) * 3);
  const stats = [
    { label: state.countdownName.slice(0, 4), value: days >= 0 ? `${days}D` : "DONE", color: "#4f8ef7" },
    { label: "喝水", value: `${Math.floor((new Date().getHours() % 8) + 1)}/8`, color: "#49b883" },
    { label: "室温", value: `${temp}C`, color: "#f2b84b" },
  ];

  stats.forEach((item, index) => {
    const x = 18 + index * 70;
    ctx.fillStyle = "rgba(255,255,255,.52)";
    drawRoundRect(x, 155, 62, 34, 7);
    ctx.fill();
    ctx.fillStyle = item.color;
    ctx.fillRect(x + 8, 164, 4, 12);
    ctx.fillStyle = "#263445";
    ctx.font = "800 12px 'Segoe UI', sans-serif";
    ctx.fillText(item.value, x + 17, 166);
    ctx.fillStyle = "rgba(38,52,69,.66)";
    ctx.font = "700 9px 'Segoe UI', sans-serif";
    ctx.fillText(item.label, x + 17, 180);
  });
}

function drawHabitRing(x, y, radius) {
  const progress = Math.max(0, Math.min(1, state.habit / 100));
  ctx.strokeStyle = "rgba(38,52,69,.14)";
  ctx.lineWidth = 9;
  ctx.beginPath();
  ctx.arc(x, y, radius, 0, Math.PI * 2);
  ctx.stroke();
  ctx.strokeStyle = "#e46b78";
  ctx.beginPath();
  ctx.arc(x, y, radius, -Math.PI / 2, -Math.PI / 2 + Math.PI * 2 * progress);
  ctx.stroke();
  ctx.fillStyle = "#263445";
  ctx.font = "800 15px 'Segoe UI', sans-serif";
  ctx.textAlign = "center";
  ctx.fillText(`${state.habit}%`, x, y + 5);
  ctx.textAlign = "left";
}

function drawPlatformLogo(account, x, y, size) {
  const r = size / 2;
  ctx.save();
  ctx.translate(x, y);
  ctx.fillStyle = account.color;
  ctx.beginPath();
  ctx.arc(0, 0, r, 0, Math.PI * 2);
  ctx.fill();

  if (account.logo === "douyin") {
    ctx.lineCap = "round";
    ctx.lineJoin = "round";
    ctx.strokeStyle = "#25f4ee";
    ctx.lineWidth = 6;
    ctx.beginPath();
    ctx.moveTo(2, -14);
    ctx.lineTo(2, 8);
    ctx.arc(-6, 8, 8, 0, Math.PI * 1.7);
    ctx.stroke();
    ctx.strokeStyle = "#fe2c55";
    ctx.beginPath();
    ctx.moveTo(7, -12);
    ctx.quadraticCurveTo(11, -4, 18, -3);
    ctx.stroke();
    ctx.strokeStyle = "#ffffff";
    ctx.lineWidth = 4;
    ctx.beginPath();
    ctx.moveTo(4, -14);
    ctx.lineTo(4, 8);
    ctx.arc(-5, 8, 6, 0, Math.PI * 1.7);
    ctx.stroke();
  } else if (account.logo === "bilibili") {
    ctx.fillStyle = "#ffffff";
    drawRoundRect(-17, -11, 34, 24, 5);
    ctx.fill();
    ctx.strokeStyle = "#ffffff";
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.moveTo(-9, -12);
    ctx.lineTo(-14, -18);
    ctx.moveTo(9, -12);
    ctx.lineTo(14, -18);
    ctx.stroke();
    ctx.fillStyle = account.color;
    ctx.fillRect(-9, -2, 4, 7);
    ctx.fillRect(5, -2, 4, 7);
  } else if (account.logo === "weibo") {
    ctx.fillStyle = "#ffffff";
    ctx.beginPath();
    ctx.ellipse(-2, 5, 15, 10, -0.2, 0, Math.PI * 2);
    ctx.fill();
    ctx.fillStyle = account.color;
    ctx.beginPath();
    ctx.arc(-7, 4, 3, 0, Math.PI * 2);
    ctx.arc(4, 4, 3, 0, Math.PI * 2);
    ctx.fill();
    ctx.strokeStyle = account.accent;
    ctx.lineWidth = 3;
    ctx.beginPath();
    ctx.arc(9, -9, 8, -1.3, 0.5);
    ctx.stroke();
    ctx.beginPath();
    ctx.arc(12, -12, 13, -1.4, 0.45);
    ctx.stroke();
  } else if (account.logo === "xiaohongshu") {
    ctx.fillStyle = "#ffffff";
    ctx.font = "900 18px 'Microsoft YaHei', sans-serif";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText("小红", 0, -1);
  } else {
    ctx.fillStyle = "#ffffff";
    ctx.font = "900 17px 'Microsoft YaHei', sans-serif";
    ctx.textAlign = "center";
    ctx.textBaseline = "middle";
    ctx.fillText("头条", 0, -1);
  }

  ctx.restore();
  ctx.textAlign = "left";
}

function drawOrbit(now) {
  const cx = 102;
  const cy = 107;
  const pulse = Math.sin(now / (700 - state.tempo * 360)) * 4;

  ctx.strokeStyle = "rgba(38,52,69,.16)";
  ctx.lineWidth = 2;
  for (let r = 25; r <= 65; r += 20) {
    ctx.beginPath();
    ctx.ellipse(cx, cy, r + pulse, r * 0.58, now / 5000, 0, Math.PI * 2);
    ctx.stroke();
  }

  for (let i = 0; i < 5; i += 1) {
    const angle = now / (900 + i * 180) + i * 1.4;
    const rx = 34 + i * 8;
    const ry = 20 + i * 4;
    const x = cx + Math.cos(angle) * rx;
    const y = cy + Math.sin(angle) * ry;
    ctx.fillStyle = ["#4f8ef7", "#49b883", "#f2b84b", "#e46b78", "#263445"][i];
    ctx.beginPath();
    ctx.arc(x, y, i === 2 ? 5 : 4, 0, Math.PI * 2);
    ctx.fill();
  }

  ctx.fillStyle = "#fff8cf";
  ctx.beginPath();
  ctx.arc(cx, cy, 23 + pulse * 0.2, 0, Math.PI * 2);
  ctx.fill();
  ctx.fillStyle = "#263445";
  ctx.beginPath();
  ctx.arc(cx - 8, cy - 2, 3, 0, Math.PI * 2);
  ctx.arc(cx + 8, cy - 2, 3, 0, Math.PI * 2);
  ctx.fill();
  ctx.fillRect(cx - 8, cy + 9, 16, 2);

  drawHabitRing(184, 108, 29);
  drawMiniStats(now);
}

function drawRadar(now) {
  const account = currentAccount();
  if (!account) return;
  const pulse = Math.sin(now / 620) * 2;

  ctx.fillStyle = "rgba(255,255,255,.76)";
  drawRoundRect(18, 58, 204, 128, 8);
  ctx.fill();

  drawPlatformLogo(account, 58, 96, 50 + pulse);

  ctx.fillStyle = "#263445";
  ctx.font = "900 18px 'Microsoft YaHei', 'Segoe UI', sans-serif";
  ctx.fillText(account.name, 96, 82);
  ctx.font = "700 11px 'Microsoft YaHei', 'Segoe UI', sans-serif";
  ctx.fillStyle = "rgba(38,52,69,.62)";
  ctx.fillText(account.nickname.slice(0, 9), 96, 102);
  ctx.font = "800 10px 'Segoe UI', sans-serif";
  ctx.fillText(account.handle.slice(0, 14), 96, 119);

  ctx.fillStyle = "rgba(38,52,69,.08)";
  drawRoundRect(28, 138, 184, 36, 7);
  ctx.fill();
  ctx.fillStyle = account.color;
  ctx.font = "900 25px 'Segoe UI', sans-serif";
  ctx.fillText(shortNumber(account.followers), 42, 160);
  ctx.fillStyle = "rgba(38,52,69,.66)";
  ctx.font = "800 11px 'Microsoft YaHei', sans-serif";
  ctx.fillText("粉丝", 142, 159);

  ctx.fillStyle = "rgba(255,255,255,.62)";
  drawRoundRect(38, 192, 164, 14, 7);
  ctx.fill();
  const width = 164 / socialAccounts.length;
  socialAccounts.forEach((item, index) => {
    ctx.fillStyle = index === state.accountIndex ? item.color : "rgba(38,52,69,.18)";
    drawRoundRect(40 + index * width, 195, Math.max(8, width - 5), 8, 4);
    ctx.fill();
  });
}

function drawFocus() {
  const total = 25 * 60;
  const progress = 1 - state.focusSeconds / total;
  const start = -Math.PI / 2;
  const end = start + Math.PI * 2 * progress;

  ctx.strokeStyle = "rgba(38,52,69,.14)";
  ctx.lineWidth = 16;
  ctx.beginPath();
  ctx.arc(120, 115, 54, 0, Math.PI * 2);
  ctx.stroke();

  ctx.strokeStyle = "#49b883";
  ctx.beginPath();
  ctx.arc(120, 115, 54, start, end);
  ctx.stroke();

  ctx.fillStyle = "#263445";
  ctx.font = "800 32px 'Segoe UI', sans-serif";
  ctx.textAlign = "center";
  ctx.textBaseline = "middle";
  ctx.fillText(formatFocus(), 120, 112);
  ctx.font = "700 12px 'Segoe UI', sans-serif";
  ctx.fillText(state.focusRunning ? "FOCUS" : "READY", 120, 145);
  ctx.textAlign = "left";
  drawMiniStats(performance.now());
}

function drawGarden(now) {
  const baseY = 152;
  ctx.fillStyle = "rgba(255,255,255,.5)";
  drawRoundRect(24, 66, 192, 88, 8);
  ctx.fill();

  for (let i = 0; i < 7; i += 1) {
    const x = 42 + i * 26;
    const h = 25 + Math.sin(now / 900 + i) * 6 + i * 3;
    ctx.strokeStyle = i % 2 ? "#49b883" : "#4f8ef7";
    ctx.lineWidth = 4;
    ctx.beginPath();
    ctx.moveTo(x, baseY);
    ctx.lineTo(x + Math.sin(now / 800 + i) * 4, baseY - h);
    ctx.stroke();

    ctx.fillStyle = i % 3 === 0 ? "#f2b84b" : i % 3 === 1 ? "#e46b78" : "#263445";
    ctx.beginPath();
    ctx.arc(x + Math.sin(now / 800 + i) * 4, baseY - h, 6, 0, Math.PI * 2);
    ctx.fill();
  }

  drawHabitRing(120, 184, 21);
}

function drawNight(now) {
  ctx.fillStyle = "rgba(255,255,255,.55)";
  drawRoundRect(32, 76, 176, 88, 8);
  ctx.fill();
  ctx.fillStyle = "#263445";
  ctx.font = "800 22px 'Segoe UI', sans-serif";
  ctx.textAlign = "center";
  ctx.fillText("LOW LIGHT", 120, 112);
  ctx.font = "700 12px 'Segoe UI', sans-serif";
  ctx.fillText("breathing desk mode", 120, 138);
  ctx.textAlign = "left";

  for (let i = 0; i < 18; i += 1) {
    const x = 20 + ((i * 31) % 202);
    const y = 54 + ((i * 47) % 132);
    const a = 0.35 + Math.sin(now / 700 + i) * 0.25;
    ctx.globalAlpha = a;
    ctx.fillStyle = "#ffffff";
    ctx.fillRect(x, y, 2, 2);
  }
  ctx.globalAlpha = 1;
}

function drawFooter(date) {
  ctx.fillStyle = "rgba(255,255,255,.7)";
  drawRoundRect(14, 198, 212, 28, 7);
  ctx.fill();
  ctx.fillStyle = "#263445";
  ctx.font = "700 10px 'Microsoft YaHei', 'Segoe UI', sans-serif";
  ctx.textBaseline = "middle";
  const account = currentAccount();
  const footer = state.mode === "radar" && account ? `${account.nickname} ${shortNumber(account.followers)}粉` : lines[state.lineIndex];
  ctx.fillText(footer.slice(0, 15), 24, 212);

  const weekday = ["SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"][date.getDay()];
  ctx.textAlign = "right";
  ctx.fillText(weekday, 216, 212);
  ctx.textAlign = "left";
}

function render(now) {
  const date = new Date();
  if (state.mode === "radar" && now - state.lastAccountTurn > 5200) nextAccount();

  background(now);
  drawHeader(date);

  if (state.mode === "orbit") drawOrbit(now);
  if (state.mode === "radar") drawRadar(now);
  if (state.mode === "focus") drawFocus();
  if (state.mode === "garden") drawGarden(now);
  if (state.mode === "night") drawNight(now);

  drawFooter(date);

  const account = currentAccount();
  clockText.textContent = `${pad(date.getHours())}:${pad(date.getMinutes())}`;
  dateText.textContent = state.mode === "radar" && account ? account.name : `${date.getMonth() + 1}/${date.getDate()}`;
  focusLabel.textContent = state.mode === "radar" && account ? shortNumber(account.followers) : formatFocus();
  tempLabel.textContent = state.mode === "radar" ? "公开" : `${22 + Math.round(Math.sin(now / 9000) * 3)}C`;
  requestAnimationFrame(render);
}

function tickFocus(now) {
  if (state.focusRunning && now - state.lastTick >= 1000) {
    state.focusSeconds = Math.max(0, state.focusSeconds - 1);
    state.lastTick = now;
    if (state.focusSeconds === 0) state.focusRunning = false;
    saveSettings();
  }
  requestAnimationFrame(tickFocus);
}

tabs.forEach((tab) => {
  tab.addEventListener("click", () => setMode(tab.dataset.mode));
});

brightness.addEventListener("input", () => {
  state.brightness = Number(brightness.value) / 100;
  saveSettings();
});

tempo.addEventListener("input", () => {
  state.tempo = Number(tempo.value) / 100;
  saveSettings();
});

habit.addEventListener("input", () => {
  state.habit = Number(habit.value);
  saveSettings();
});

countdownName.addEventListener("input", () => {
  state.countdownName = countdownName.value || "目标";
  saveSettings();
});

countdownDate.addEventListener("input", () => {
  state.countdownDate = countdownDate.value;
  saveSettings();
});

focusToggle.addEventListener("click", () => {
  state.focusRunning = !state.focusRunning;
  focusToggle.textContent = state.focusRunning ? "Ⅱ" : "▶";
  state.lastTick = performance.now();
  setMode("focus");
});

focusReset.addEventListener("click", () => {
  state.focusSeconds = 25 * 60;
  state.focusRunning = false;
  focusToggle.textContent = "▶";
  saveSettings();
});

spark.addEventListener("click", () => {
  if (state.mode === "radar") {
    nextAccount();
    return;
  }
  state.lineIndex = (state.lineIndex + 1) % lines.length;
  updateDayLine();
  saveSettings();
});

setInterval(() => {
  if (state.mode === "orbit") {
    state.lineIndex = (state.lineIndex + 1) % lines.length;
    updateDayLine();
    saveSettings();
  }
}, 16000);

brightness.value = Math.round(state.brightness * 100);
tempo.value = Math.round(state.tempo * 100);
habit.value = state.habit;
countdownName.value = state.countdownName;
countdownDate.value = state.countdownDate;
setMode(state.mode);
requestAnimationFrame(render);
requestAnimationFrame(tickFocus);
