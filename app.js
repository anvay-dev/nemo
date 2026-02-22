import * as DB from "./db.js";
import { ref, set } from "https://www.gstatic.com/firebasejs/12.9.0/firebase-database.js";


// Use the RTDB instance created in firebase-config.js
// const db = window.rtdb;


// ----------------- Local storage keys -----------------
const LS_GOAL = "goalOz";
const LS_RTDB_ROOT = "rtdbRootPath";

// ----------------- App state -----------------
let useMock = false;
let latestUnsub = null;
let connected = false;

let goalOz = 64;
let percentDrunk = NaN; // from RTDB
let lastEvent = "";
let lastTimestampMs = null;

let currentRootPath = "waterbuddy";

// Mock state
let mockPercent = 0;

// ----------------- DOM -----------------
const remainingText = document.getElementById("remainingText");
const connectionText = document.getElementById("connectionText");
const progressFill = document.getElementById("progressFill");
const progressText = document.getElementById("progressText");

const toggleMockBtn = document.getElementById("toggleMockBtn");
const logDrinkBtn = document.getElementById("logDrinkBtn");
const resetDrinkBtn = document.getElementById("resetDrinkBtn");
const goalInput = document.getElementById("goalInput");
const deviceUrlInput = document.getElementById("deviceUrlInput"); // repurposed as RTDB root path
const connectBtn = document.getElementById("connectBtn");

// ----------------- Init UI -----------------
const savedGoal = loadNumber(LS_GOAL, 64);
if (Number.isFinite(savedGoal) && savedGoal > 0) goalOz = savedGoal;

goalInput.value = goalOz;

const savedRoot = loadString(LS_RTDB_ROOT, "waterbuddy");
deviceUrlInput.value = savedRoot;
currentRootPath = DB.normalizeRootPath(deviceUrlInput.value);

toggleMockBtn.textContent = `Use Mock: ${useMock ? "ON" : "OFF"}`;
setMockControlsEnabled(useMock);

render();

// Auto-connect on load (unless mock is on)
if (!useMock) connectRTDB();

// ----------------- RTDB connect / disconnect -----------------
function disconnectRTDB() {
  if (latestUnsub) latestUnsub();
  latestUnsub = null;
  connected = false;
}

function connectRTDB() {
  const rootPath = DB.normalizeRootPath(deviceUrlInput.value);
  currentRootPath = rootPath;
  saveString(LS_RTDB_ROOT, rootPath);

  disconnectRTDB();

  connectionText.textContent = `Connecting to /${rootPath}/latest…`;

  try {
    latestUnsub = DB.subscribeLatest(rootPath, (latest, err) => {
      if (err) {
        connected = false;
        connectionText.textContent = `Disconnected ❌ (${err.message ?? err})`;
        return;
      }

      if (!latest) {
        connected = true; // connected, just no data yet
        connectionText.textContent = `Connected ✅ (no data at /${rootPath}/latest yet)`;
        return;
      }

      connected = true;

      // Expected fields from ESP32:
      //   percent_drunk (number)
      //   event (string)
      //   timestamp_ms (number)
      percentDrunk = clampNumber(Number(latest.percent_drunk), 0, 100);
      lastEvent = typeof latest.event === "string" ? latest.event : "";
      lastTimestampMs =
        latest.timestamp_ms !== undefined && latest.timestamp_ms !== null
          ? Number(latest.timestamp_ms)
          : null;

      const eventSuffix = lastEvent ? ` • ${lastEvent}` : "";
      connectionText.textContent = `Connected ✅${eventSuffix}`;

      render();
    });
  } catch (e) {
    connected = false;
    connectionText.textContent = `Could not connect ❌ (${e.message})`;
    console.error(e);
  }
}

async function writeLatest(percent, event) {
  if (!window.rtdb) throw new Error("window.rtdb missing");
  const path = DB.latestPath(currentRootPath);
  const payload = {
    percent_drunk: clampNumber(Number(percent), 0, 100),
    event: String(event ?? ""),
    timestamp_ms: Date.now(),
  };
  await set(ref(window.rtdb, path), payload);
}

// ----------------- Buttons / handlers -----------------

toggleMockBtn.addEventListener("click", () => {
  useMock = !useMock;
  toggleMockBtn.textContent = `Use Mock: ${useMock ? "ON" : "OFF"}`;
  setMockControlsEnabled(useMock);

  if (useMock) {
    disconnectRTDB();
    connected = true;
    connectionText.textContent = "Mock mode ✅";
    if (!Number.isFinite(mockPercent)) mockPercent = 0;
    percentDrunk = mockPercent;
    lastEvent = "mock";
    lastTimestampMs = Date.now();
    render();
  } else {
    connected = false;
    connectionText.textContent = "Not connected";
    percentDrunk = NaN;
    lastEvent = "";
    lastTimestampMs = null;
    render();
  }
});

connectBtn.addEventListener("click", () => {
  if (useMock) {
    connectionText.textContent = "Mock mode is ON — turn it OFF to connect to RTDB.";
    return;
  }
  connectRTDB();
});

goalInput.addEventListener("change", () => {
  const nextGoal = Number(goalInput.value);
  if (!Number.isFinite(nextGoal) || nextGoal <= 0) {
    goalInput.value = goalOz;
    return;
  }
  goalOz = nextGoal;
  saveNumber(LS_GOAL, goalOz);
  render();
});


logDrinkBtn.addEventListener("click", async () => {
  // Mock mode = local UI-only
  if (useMock) {
    const deltaPct = goalOz > 0 ? (8 / goalOz) * 100 : 0;
    mockPercent = clampNumber((mockPercent ?? 0) + deltaPct, 0, 100);
    percentDrunk = mockPercent;
    lastEvent = `mock: drank 8 oz`;
    lastTimestampMs = Date.now();
    render();
    return;
  }

  // RTDB mode = write demo data if connected
  if (!connected) return;

  try {
    const deltaPct = goalOz > 0 ? (8 / goalOz) * 100 : 0;
    const next = clampNumber((Number.isFinite(percentDrunk) ? percentDrunk : 0) + deltaPct, 0, 100);
    await writeLatest(next, "web: drank 8 oz");
  } catch (e) {
    console.error(e);
    connectionText.textContent = `Write failed ❌ (${e.message ?? e})`;
  }
});

resetDrinkBtn.addEventListener("click", async () => {
  if (!confirm("Reset progress?")) return;

  if (useMock) {
    mockPercent = 0;
    percentDrunk = 0;
    lastEvent = "mock: reset";
    lastTimestampMs = Date.now();
    render();
    return;
  }

  if (!connected) return;

  try {
    await writeLatest(0, "web: reset");
  } catch (e) {
    console.error(e);
    connectionText.textContent = `Write failed ❌ (${e.message ?? e})`;
  }
});

function setMockControlsEnabled(enabled) {
  logDrinkBtn.disabled = !enabled;
  resetDrinkBtn.disabled = !enabled;

  // visually indicate disabled (optional)
  logDrinkBtn.style.opacity = enabled ? "1" : "0.5";
  resetDrinkBtn.style.opacity = enabled ? "1" : "0.5";
}

// ----------------- Rendering -----------------
function render() {
  // Big percent display
  if (Number.isFinite(percentDrunk)) {
    remainingText.textContent = `${Math.round(percentDrunk)}%`;
  } else {
    remainingText.textContent = "--%";
  }

  // Progress bar + oz text
  const consumedOz =
    Number.isFinite(percentDrunk) && goalOz > 0 ? (percentDrunk / 100) * goalOz : 0;

  const pct =
    Number.isFinite(percentDrunk) && percentDrunk >= 0
      ? Math.max(0, Math.min(100, percentDrunk))
      : 0;

  progressFill.style.width = `${pct}%`;

  // Add a little debug hint from ESP32 if present
  const tsHint = Number.isFinite(lastTimestampMs)
    ? ` • t=${Math.round(lastTimestampMs)}ms`
    : "";

  progressText.textContent = `${Math.round(consumedOz)} / ${Math.round(goalOz)} oz${tsHint}`;

  if (!useMock && !connected) {
    // keep whatever connectionText says; don't overwrite
  }
}

// ----------------- Helpers -----------------
function loadString(key, fallback) {
  const v = localStorage.getItem(key);
  return v ?? fallback;
}
function saveString(key, value) {
  localStorage.setItem(key, String(value));
}
function loadNumber(key, fallback) {
  const v = localStorage.getItem(key);
  const n = v === null ? NaN : Number(v);
  return Number.isFinite(n) ? n : fallback;
}
function saveNumber(key, value) {
  localStorage.setItem(key, String(value));
}
function clampNumber(n, min, max) {
  if (!Number.isFinite(n)) return NaN;
  return Math.max(min, Math.min(max, n));
}

