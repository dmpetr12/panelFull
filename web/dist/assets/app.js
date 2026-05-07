const state = {
  cabinetMode: 0,
  systemState: 0,
  testRunning: false,
  testPlannedSec: 0,
  testRemainingSec: 0,
  programFireActive: false,
  relayStateKnown: true,
  busConnected: true,
  backendOnline: false,
  temperature: null,
  temperatureAvailable: false,
  serverTime: "",
  measurements: {
    power: null,
    powerAvailable: false,
    voltage: null,
    voltageAvailable: false,
    current: null,
    currentAvailable: false,
    freq: null,
    freqAvailable: false
  },
  battery: {
    percent: null,
    percentAvailable: false,
    stateText: "—",
    chargeStatus: "—",
    health: 92,
    voltage: null,
    voltageAvailable: false,
    chargeCurrent: null,
    chargeCurrentAvailable: false,
    temperature: null,
    temperatureAvailable: false,
    nominalVoltage: "—",
    nominalCapacity: "—",
    chemistry: "—",
    internalResistance: "—",
    floatVoltage: "—",
    autonomy: "—",
    lastService: "—",
    nextReplacement: "—",
    serial: "—",
    cycles: null
  },
  password: "",
  authenticated: false,
  selectedLineIndex: null,
  selectedScheduleIndex: null,
  lines: [
    { current: 0.465, description: "Линия 1", lastMeasuredTest: "2026-03-27T11:16:23", lineState: 0, mode: 1, mpower: 108.288, power: 108.345, status: 0, tolerance: 5, voltage: 233 },
    { current: 0.246, description: "Линия 2", lastMeasuredTest: "2026-03-27T11:16:33", lineState: 0, mode: 1, mpower: 56.84, power: 0, status: 0, tolerance: 5, voltage: 231 },
    { current: 0, description: "Линия 3", lastMeasuredTest: "2026-03-27T11:03:43", lineState: 0, mode: 1, mpower: 57.117, power: 0, status: 1, tolerance: 5, voltage: 233 },
    { current: 0, description: "Линия 4", lastMeasuredTest: "2026-03-27T11:03:53", lineState: 0, mode: 1, mpower: 0, power: 0, status: 1, tolerance: 5, voltage: 232 },
    { current: 0, description: "Линия 5", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 6", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 7", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 8", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 9", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 10", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 11", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 12", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 13", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 14", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 15", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 16", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 17", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 18", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 19", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 20", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 21", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 22", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 23", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 24", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 },
    { current: 0, description: "Линия 25", lastMeasuredTest: "", lineState: 0, mode: 2, mpower: 0, power: 0, status: 3, tolerance: 5, voltage: 0 }
  ],
  schedule: [
    { enabled: true, period: "один раз", startDate: "2026-03-27", startTime: "11:06", testType: "Функциональный", weekDays: [] },
    { enabled: true, period: "один раз", startDate: "2026-01-15", startTime: "00:00", testType: "Функциональный", weekDays: [] },
    { enabled: true, period: "ежедневно", startDate: "2026-01-15", startTime: "11:35", testType: "На время", weekDays: [] }
  ],
  logs: [
    "2026-04-24 11:58:02 INFO  Panel boot completed",
    "2026-04-24 11:58:18 INFO  Bus connected, 25 lines discovered",
    "2026-04-24 11:59:01 WARN  Line 3 fault detected during polling",
    "2026-04-24 11:59:07 WARN  Line 4 fault detected during polling",
    "2026-04-24 12:00:15 INFO  Scheduled functional test queued",
    "2026-04-24 12:01:44 INFO  Operator session closed by idle timeout"
  ]
};

const API_BASE = window.location.protocol === "file:" ? "http://127.0.0.1:8080" : "";
const STATUS_POLL_MS = 1000;
let backendPollTimerId = null;
let backendRequestInFlight = false;
let authToken = "";

const loginShell = document.getElementById("loginShell");
const loginForm = document.getElementById("loginForm");
const appRoot = document.getElementById("appRoot");
const loginPasswordInput = document.getElementById("loginPasswordInput");
const loginErrorText = document.getElementById("loginErrorText");
const loginSubmitBtn = document.getElementById("loginSubmitBtn");

const navButtons = document.querySelectorAll(".nav-btn");
const views = {
  dashboard: document.getElementById("dashboardView"),
  test: document.getElementById("testView"),
  battery: document.getElementById("batteryView"),
  schedule: document.getElementById("scheduleView"),
  logs: document.getElementById("logsView"),
  settings: document.getElementById("settingsView")
};

const linesTable = document.getElementById("linesTable");
const scheduleList = document.getElementById("scheduleList");
const logList = document.getElementById("logList");
const logSummary = document.getElementById("logSummary");
const toast = document.getElementById("toast");
const lineSettingsList = document.getElementById("lineSettingsList");
const lineModal = document.getElementById("lineModal");
const scheduleModal = document.getElementById("scheduleModal");
const timeModal = document.getElementById("timeModal");
const passwordModal = document.getElementById("passwordModal");

const refs = {
  dateValue: document.getElementById("dateValue"),
  timeValue: document.getElementById("timeValue"),
  temperatureValue: document.getElementById("temperatureValue"),
  modeDot: document.getElementById("modeDot"),
  modeText: document.getElementById("modeText"),
  systemDot: document.getElementById("systemDot"),
  systemText: document.getElementById("systemText"),
  networkDot: document.getElementById("networkDot"),
  networkText: document.getElementById("networkText"),
  batteryDot: document.getElementById("batteryDot"),
  batteryStateText: document.getElementById("batteryStateText"),
  powerValue: document.getElementById("powerValue"),
  voltageValue: document.getElementById("voltageValue"),
  currentValue: document.getElementById("currentValue"),
  freqValue: document.getElementById("freqValue"),
  systemActionBtn: document.getElementById("systemActionBtn"),
  manualTestBtn: document.getElementById("manualTestBtn"),
  durationTestBtn: document.getElementById("durationTestBtn"),
  stopTestBtn: document.getElementById("stopTestBtn"),
  testStatusText: document.getElementById("testStatusText"),
  testRemainingValue: document.getElementById("testRemainingValue"),
  testPlannedValue: document.getElementById("testPlannedValue"),
  batteryCap: document.getElementById("batteryCap"),
  batteryPercent: document.getElementById("batteryPercent"),
  batteryHealth: document.getElementById("batteryHealth"),
  batteryVoltage: document.getElementById("batteryVoltage"),
  batteryChargeCurrent: document.getElementById("batteryChargeCurrent"),
  batteryTemp: document.getElementById("batteryTemp"),
  batteryStatus: document.getElementById("batteryStatus"),
  batterySpecs: document.getElementById("batterySpecs"),
  systemDate: document.getElementById("systemDate"),
  systemTime: document.getElementById("systemTime"),
  newPassword: document.getElementById("newPassword"),
  repeatPassword: document.getElementById("repeatPassword"),
  openTimeModalBtn: document.getElementById("openTimeModalBtn"),
  openPasswordModalBtn: document.getElementById("openPasswordModalBtn"),
  syncNowBtn: document.getElementById("syncNowBtn"),
  closeTimeModal: document.getElementById("closeTimeModal"),
  savePasswordBtn: document.getElementById("savePasswordBtn"),
  closePasswordModal: document.getElementById("closePasswordModal"),
  lineModalTitle: document.getElementById("lineModalTitle"),
  closeLineModal: document.getElementById("closeLineModal"),
  lineNameInput: document.getElementById("lineNameInput"),
  lineBackendPower: document.getElementById("lineBackendPower"),
  lineManualPower: document.getElementById("lineManualPower"),
  lineToleranceInput: document.getElementById("lineToleranceInput"),
  lineModeInput: document.getElementById("lineModeInput"),
  lineMeasuredPowerDisplay: document.getElementById("lineMeasuredPowerDisplay"),
  lineVoltageDisplay: document.getElementById("lineVoltageDisplay"),
  lineCurrentDisplay: document.getElementById("lineCurrentDisplay"),
  runLineTestBtn: document.getElementById("runLineTestBtn"),
  applyMeasuredPowerBtn: document.getElementById("applyMeasuredPowerBtn"),
  saveLineBtn: document.getElementById("saveLineBtn")
};

refs.closeScheduleModal = document.getElementById("closeScheduleModal");
refs.scheduleModalTitle = document.getElementById("scheduleModalTitle");
refs.schedulePeriodInput = document.getElementById("schedulePeriodInput");
refs.scheduleTypeInput = document.getElementById("scheduleTypeInput");
refs.scheduleDateInput = document.getElementById("scheduleDateInput");
refs.scheduleTimeInput = document.getElementById("scheduleTimeInput");
refs.scheduleWeekdaysInput = document.getElementById("scheduleWeekdaysInput");
refs.saveScheduleBtn = document.getElementById("saveScheduleBtn");
refs.scheduleWeekdaysField = document.getElementById("scheduleWeekdaysField");

function pad(value) {
  return String(value).padStart(2, "0");
}

function formatDateTime(value) {
  if (!value) {
    return "—";
  }

  const date = new Date(value);
  if (Number.isNaN(date.getTime())) {
    return value;
  }

  return `${pad(date.getDate())}.${pad(date.getMonth() + 1)}.${date.getFullYear()} ${pad(date.getHours())}:${pad(date.getMinutes())}`;
}

function formatDateOnly(value) {
  if (!value) {
    return "—";
  }

  const date = new Date(value);
  if (Number.isNaN(date.getTime())) {
    return value;
  }

  return `${pad(date.getDate())}.${pad(date.getMonth() + 1)}.${date.getFullYear()}`;
}

function formatWeekDays(days) {
  const names = {
    Mon: "Пн",
    Tue: "Вт",
    Wed: "Ср",
    Thu: "Чт",
    Fri: "Пт",
    Sat: "Сб",
    Sun: "Вс"
  };

  if (!days || days.length === 0) {
    return "-";
  }

  return days.map((day) => names[day] || day).join(", ");
}

function formatValue(value, unit = "", digits = 1) {
  if (typeof value !== "number" || Number.isNaN(value)) {
    return "—";
  }

  const formatted = digits === 0 ? String(Math.round(value)) : value.toFixed(digits);
  return unit ? `${formatted} ${unit}` : formatted;
}

function formatDurationSeconds(totalSeconds) {
  if (!Number.isFinite(totalSeconds) || totalSeconds <= 0) {
    return "—";
  }

  const hours = Math.floor(totalSeconds / 3600);
  const minutes = Math.floor((totalSeconds % 3600) / 60);
  const seconds = totalSeconds % 60;

  if (hours > 0) {
    return `${pad(hours)}:${pad(minutes)}:${pad(seconds)}`;
  }

  return `${pad(minutes)}:${pad(seconds)}`;
}

function isLineTestRunning(line, index) {
  if (!line || index === null || index === undefined) {
    return false;
  }

  return line.status === 2;
}

function clearLineModalInputs() {
  refs.lineNameInput.value = "";
  refs.lineBackendPower.value = "—";
  refs.lineManualPower.value = "";
  refs.lineToleranceInput.value = "";
  refs.lineModeInput.value = "1";
  refs.lineMeasuredPowerDisplay.textContent = "—";
  refs.lineVoltageDisplay.textContent = "—";
  refs.lineCurrentDisplay.textContent = "—";
  refs.runLineTestBtn.textContent = "Запуск теста";
  refs.runLineTestBtn.className = "action-btn warning";
}

function renderLineModal(index = state.selectedLineIndex) {
  if (index === null || index === undefined || !lineModal.classList.contains("open")) {
    return;
  }

  const line = state.lines[index];
  if (!line) {
    return;
  }

  const lineTestRunning = isLineTestRunning(line, index);

  refs.lineModalTitle.textContent = `Настройка линии ${index + 1}`;
  refs.lineNameInput.value = line.description;
  refs.lineBackendPower.value = typeof line.mpower === "number" && line.mpower > 0 ? `${line.mpower.toFixed(1)} Вт` : "—";
  refs.lineToleranceInput.value = typeof line.tolerance === "number" ? String(line.tolerance) : "";
  refs.lineModeInput.value = String(line.mode);
  refs.lineMeasuredPowerDisplay.textContent = typeof line.power === "number" && line.power > 0 ? `${line.power.toFixed(1)} Вт` : "—";
  refs.lineVoltageDisplay.textContent = typeof line.voltage === "number" && line.voltage > 0 ? `${line.voltage.toFixed(0)} В` : "—";
  refs.lineCurrentDisplay.textContent = typeof line.current === "number" && line.current > 0 ? `${line.current.toFixed(3)} А` : "—";
  refs.runLineTestBtn.textContent = lineTestRunning ? "Стоп теста" : "Запуск теста";
  refs.runLineTestBtn.className = lineTestRunning ? "action-btn danger" : "action-btn warning";
}

async function stopLineTestIfNeeded(index) {
  const line = index === null || index === undefined ? null : state.lines[index];
  if (!line || line.status !== 2) {
    return;
  }

  try {
    await apiRequest(`/api/lines/${index}/test/stop`, {
      method: "POST",
      auth: true
    });
    await pollBackendStatus();
  } catch (error) {
    // Закрытие окна не должно блокироваться из-за ошибки остановки теста.
  }
}

function showToast(message) {
  toast.textContent = message;
  toast.classList.add("visible");
  clearTimeout(showToast.timeoutId);
  showToast.timeoutId = setTimeout(() => {
    toast.classList.remove("visible");
  }, 2200);
}

function setLoginError(message = "") {
  loginErrorText.textContent = message;
}

function setAuthenticated(isAuthenticated) {
  state.authenticated = isAuthenticated;
  loginShell.classList.toggle("is-hidden", isAuthenticated);
  appRoot.classList.toggle("is-locked", !isAuthenticated);

  if (isAuthenticated) {
    setLoginError("");
    loginPasswordInput.value = "";
    return;
  }

  authToken = "";
  loginSubmitBtn.disabled = false;
  loginPasswordInput.focus();
}

async function loginWithPassword(password) {
  const response = await fetch(`${API_BASE}/api/login`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      Accept: "application/json"
    },
    body: JSON.stringify({ password })
  });

  const payload = await response.json().catch(() => null);
  if (!response.ok || !payload || payload.ok !== true || !payload.token) {
    throw new Error(payload?.error || `HTTP ${response.status}`);
  }

  state.password = password;
  authToken = payload.token;
  return authToken;
}

async function ensureAuthToken(forceRenew = false) {
  if (authToken && !forceRenew) {
    return authToken;
  }

  if (!state.authenticated || !state.password) {
    setAuthenticated(false);
    throw new Error("Требуется вход в систему");
  }

  return loginWithPassword(state.password);
}

async function apiRequest(path, options = {}) {
  const {
    method = "GET",
    body = null,
    auth = false,
    rawResponse = false
  } = options;

  const headers = {
    Accept: "application/json"
  };

  if (body !== null) {
    headers["Content-Type"] = "application/json";
  }

  if (auth) {
    headers.Authorization = `Bearer ${await ensureAuthToken()}`;
  }

  let response = await fetch(`${API_BASE}${path}`, {
    method,
    cache: "no-store",
    headers,
    body: body !== null ? JSON.stringify(body) : undefined
  });

  if (response.status === 401 && auth) {
    try {
      headers.Authorization = `Bearer ${await ensureAuthToken(true)}`;
      response = await fetch(`${API_BASE}${path}`, {
        method,
        cache: "no-store",
        headers,
        body: body !== null ? JSON.stringify(body) : undefined
      });
    } catch (error) {
      setAuthenticated(false);
      throw error;
    }
  }

  if (rawResponse) {
    if (!response.ok) {
      const text = await response.text().catch(() => "");
      throw new Error(text || `HTTP ${response.status}`);
    }
    return response;
  }

  const payload = await response.json().catch(() => null);
  if (!response.ok || !payload || payload.ok !== true) {
    if (response.status === 401 && auth) {
      setAuthenticated(false);
    }
    throw new Error(payload?.error || `HTTP ${response.status}`);
  }

  return payload;
}

function switchView(name) {
  navButtons.forEach((button) => {
    button.classList.toggle("active", button.dataset.view === name);
  });

  Object.entries(views).forEach(([key, view]) => {
    view.classList.toggle("active", key === name);
  });

  if (name === "schedule") {
    void refreshSchedule();
  }
  if (name === "logs") {
    void refreshLogs();
  }
}

function getModeMeta() {
  if (state.cabinetMode === 2) {
    return { text: "Тест", className: "warning" };
  }
  if (state.testRunning) {
    return { text: "Тест запускается", className: "warning" };
  }
  if (state.cabinetMode === 1) {
    return { text: "Пожар", className: "danger" };
  }
  return { text: "Нормальный", className: "work" };
}

function getSystemMeta() {
  if (state.systemState === 1) {
    return { text: "Авария", className: "alarm" };
  }
  return { text: "OK", className: "ok" };
}

function getLineStatusMeta(status) {
  if (status === 1) {
    return { text: "АВАРИЯ", className: "alarm" };
  }
  if (status === 2) {
    return { text: "ТЕСТ", className: "test" };
  }
  if (status === 0) {
    return { text: "OK", className: "ok" };
  }
  return { text: "РЕЗЕРВ", className: "idle" };
}

function renderHeaderClock() {
  const now = state.serverTime ? new Date(state.serverTime) : new Date();
  refs.dateValue.textContent = `${pad(now.getDate())}.${pad(now.getMonth() + 1)}.${now.getFullYear()}`;
  refs.timeValue.textContent = `${pad(now.getHours())}:${pad(now.getMinutes())}`;
}

function renderOverview() {
  const mode = getModeMeta();
  const system = getSystemMeta();
  const batteryClass = !state.battery.percentAvailable
    ? "warning"
    : state.battery.percent > 60 ? "ok" : state.battery.percent > 30 ? "warning" : "alarm";

  refs.modeText.textContent = mode.text;
  refs.systemText.textContent = system.text;
  refs.networkText.textContent = state.backendOnline
    ? (state.busConnected ? "Шина доступна" : "Связь потеряна")
    : "Нет связи с backend";
  refs.batteryStateText.textContent = state.battery.percentAvailable
    ? `${state.battery.stateText}, ${state.battery.percent}%`
    : state.battery.stateText;
  const showRealtimeValues = state.backendOnline && state.busConnected;
  refs.temperatureValue.textContent = showRealtimeValues && state.temperatureAvailable ? formatValue(state.temperature, "°C", 0) : "—";
  refs.powerValue.textContent = showRealtimeValues && state.measurements.powerAvailable ? formatValue(state.measurements.power, "Вт", 1) : "—";
  refs.voltageValue.textContent = showRealtimeValues && state.measurements.voltageAvailable ? formatValue(state.measurements.voltage, "В", 1) : "—";
  refs.currentValue.textContent = showRealtimeValues && state.measurements.currentAvailable ? formatValue(state.measurements.current, "А", 2) : "—";
  refs.freqValue.textContent = showRealtimeValues && state.measurements.freqAvailable ? formatValue(state.measurements.freq, "Гц", 1) : "—";

  refs.modeDot.className = `status-dot ${mode.className}`;
  refs.systemDot.className = `status-dot ${system.className}`;
  refs.networkDot.className = `status-dot ${state.busConnected ? "ok" : "alarm"}`;
  refs.batteryDot.className = `status-dot ${batteryClass}`;
}

function applyBackendState(data) {
  if (!data || typeof data !== "object") {
    return;
  }

  state.backendOnline = true;
  state.serverTime = data.serverTime || state.serverTime;
  state.cabinetMode = Number.isFinite(data.cabinetMode) ? data.cabinetMode : state.cabinetMode;
  state.systemState = Number.isFinite(data.systemState) ? data.systemState : state.systemState;
  state.testRunning = Boolean(data.testRunning);
  state.testPlannedSec = Number.isFinite(data.testPlannedSec) ? Math.max(0, data.testPlannedSec) : 0;
  state.testRemainingSec = Number.isFinite(data.testRemainingSec) ? Math.max(0, data.testRemainingSec) : 0;
  state.programFireActive = Boolean(data.programFireActive);
  state.relayStateKnown = data.relayStateKnown !== false;
  state.busConnected = Boolean(data.busConnected);

  state.temperatureAvailable = Boolean(data.temperatureAvailable);
  state.temperature = state.temperatureAvailable && typeof data.temperature === "number"
    ? data.temperature
    : null;

  state.measurements.powerAvailable = Boolean(data.inletPAvailable);
  state.measurements.power = state.measurements.powerAvailable && typeof data.inletP === "number"
    ? data.inletP
    : null;
  state.measurements.voltageAvailable = Boolean(data.inletUAvailable);
  state.measurements.voltage = state.measurements.voltageAvailable && typeof data.inletU === "number"
    ? data.inletU
    : null;
  state.measurements.currentAvailable = Boolean(data.inletIAvailable);
  state.measurements.current = state.measurements.currentAvailable && typeof data.inletI === "number"
    ? data.inletI
    : null;
  state.measurements.freqAvailable = Boolean(data.inletFAvailable);
  state.measurements.freq = state.measurements.freqAvailable && typeof data.inletF === "number"
    ? data.inletF
    : null;

  if (data.battery && typeof data.battery === "object") {
    const battery = data.battery;
    state.battery.percentAvailable = typeof battery.chargePercent === "number";
    state.battery.percent = state.battery.percentAvailable
      ? Math.max(0, Math.min(100, Math.round(battery.chargePercent)))
      : null;
    state.battery.voltageAvailable = typeof battery.voltage === "number";
    state.battery.voltage = state.battery.voltageAvailable ? battery.voltage : null;
    state.battery.chargeCurrentAvailable = typeof battery.current === "number";
    state.battery.chargeCurrent = state.battery.chargeCurrentAvailable ? battery.current : null;

    if (!state.battery.percentAvailable && !state.battery.voltageAvailable && !state.battery.chargeCurrentAvailable) {
      state.battery.stateText = "—";
      state.battery.chargeStatus = "—";
    } else if (battery.batteryFault) {
      state.battery.stateText = "Авария";
      state.battery.chargeStatus = "Неисправность АКБ";
    } else if (battery.batteryLow) {
      state.battery.stateText = "Низкий заряд";
      state.battery.chargeStatus = "Требуется заряд";
    } else if (battery.onBattery) {
      state.battery.stateText = "Работа от АКБ";
      state.battery.chargeStatus = "Разряд";
    } else if (battery.charging) {
      state.battery.stateText = "Норма";
      state.battery.chargeStatus = "Заряд";
    } else {
      state.battery.stateText = "Норма";
      state.battery.chargeStatus = "Буферный заряд";
    }
  }

  if (Array.isArray(data.lines)) {
    state.lines = data.lines.map((line, index) => ({
      ...state.lines[index],
      current: typeof line.current === "number" ? line.current : 0,
      description: line.description || `Линия ${index + 1}`,
      lastMeasuredTest: line.lastMeasuredTest || "",
      lineState: Number.isFinite(line.lineState) ? line.lineState : 0,
      mode: Number.isFinite(line.mode) ? line.mode : 2,
      mpower: typeof line.mpower === "number" ? line.mpower : 0,
      power: typeof line.power === "number" ? line.power : 0,
      status: Number.isFinite(line.status) ? line.status : 3,
      tolerance: typeof line.tolerance === "number" ? line.tolerance : 5,
      voltage: typeof line.voltage === "number" ? line.voltage : 0
    }));
  }
}

async function refreshSchedule() {
  try {
    const payload = await apiRequest("/api/schedule");
    state.schedule = Array.isArray(payload.data) ? payload.data.map((item) => ({
      enabled: item.enabled !== false,
      period: item.period || "один раз",
      startDate: item.startDate || "",
      startTime: item.startTime || "",
      testType: item.testType || "Функциональный",
      weekDays: Array.isArray(item.weekDays) ? item.weekDays : []
    })) : [];
    renderSchedule();
  } catch (error) {
    showToast(`Не удалось загрузить расписание: ${error.message}`);
  }
}

async function refreshLogs() {
  try {
    const batchSize = 200;
    let offset = 0;
    const allLogs = [];

    while (true) {
      const payload = await apiRequest(`/api/logs?offset=${offset}&limit=${batchSize}`);
      const chunk = Array.isArray(payload.data) ? payload.data : [];
      allLogs.push(...chunk);

      if (chunk.length < batchSize) {
        break;
      }

      offset += chunk.length;
      if (offset >= 10000) {
        break;
      }
    }

    state.logs = allLogs;
    renderLogs();
  } catch (error) {
    showToast(`Не удалось загрузить журнал: ${error.message}`);
  }
}

async function pollBackendStatus() {
  if (backendRequestInFlight) {
    return;
  }

  backendRequestInFlight = true;

  try {
    const response = await fetch(`${API_BASE}/api/status`, {
      method: "GET",
      cache: "no-store",
      headers: {
        Accept: "application/json"
      }
    });

    if (!response.ok) {
      throw new Error(`HTTP ${response.status}`);
    }

    const payload = await response.json();
    if (!payload || payload.ok !== true || !payload.data) {
      throw new Error("Invalid payload");
    }

    applyBackendState(payload.data);
    renderAll();
  } catch (error) {
    state.backendOnline = false;
    renderOverview();
    renderTestPanel();
  } finally {
    backendRequestInFlight = false;
  }
}

function startBackendPolling() {
  if (backendPollTimerId !== null) {
    return;
  }

  pollBackendStatus();
  backendPollTimerId = window.setInterval(pollBackendStatus, STATUS_POLL_MS);
  refreshSchedule();
  refreshLogs();
}

function renderLines() {
  const usedLines = state.lines.filter((line) => line.mode !== 2);

  linesTable.innerHTML = usedLines.map((line) => {
    const index = state.lines.indexOf(line);
    const status = state.relayStateKnown
      ? getLineStatusMeta(line.status)
      : { text: "—", className: "idle" };
    const outputStateText = state.relayStateKnown
      ? (line.lineState === 1 ? "вкл" : "выкл")
      : "—";
    return `
      <article class="line-row">
        <strong>${index + 1}</strong>
        <strong>${line.description}</strong>
        <div class="line-state">
          <span class="state-pill ${status.className}">${status.text}</span>
          <span>${outputStateText}</span>
        </div>
        <span>${line.mode === 0 ? "пост." : line.mode === 1 ? "непост." : "откл."}</span>
        <span>${formatDateOnly(line.lastMeasuredTest)}</span>
      </article>
    `;
  }).join("");
}

function renderSystemAction() {
  if (state.testRunning) {
    refs.systemActionBtn.textContent = "Остановить тест";
    refs.systemActionBtn.className = "action-btn danger nav-action-btn";
    return;
  }

  if (state.programFireActive) {
    refs.systemActionBtn.textContent = "Сброс \"Пожар\"";
    refs.systemActionBtn.className = "action-btn danger nav-action-btn";
    return;
  }

  refs.systemActionBtn.textContent = "Пуск \"Пожар\"";
  refs.systemActionBtn.className = "action-btn nav-action-btn fire-start-btn";
}

function renderTestPanel() {
  let statusText = "Ожидание";

  if (!state.backendOnline) {
    statusText = "Нет связи с backend";
  } else if (state.cabinetMode === 2) {
    statusText = "Тест подтверждён";
  } else if (state.testRunning) {
    statusText = "Тест запускается";
  }

  refs.testStatusText.textContent = statusText;
  refs.testRemainingValue.textContent = state.backendOnline
    ? formatDurationSeconds(state.testRemainingSec)
    : "—";
  refs.testPlannedValue.textContent = state.backendOnline
    ? formatDurationSeconds(state.testPlannedSec)
    : "—";
}

function renderSchedule() {
  scheduleList.innerHTML = state.schedule.map((item, index) => `
    <article class="schedule-item">
      <strong>${item.period}</strong>
      <span>${item.startDate}</span>
      <span>${item.startTime}</span>
      <span>${formatWeekDays(item.weekDays)}</span>
      <strong>${item.testType}</strong>
      <div class="action-row">
        <button class="action-btn neutral" data-edit-schedule="${index}" type="button">Настроить</button>
        <button class="action-btn neutral" data-remove-schedule="${index}" type="button">Удалить</button>
      </div>
    </article>
  `).join("");
}

function renderLogs() {
  const lastLongTest = state.schedule.find((item) => item.testType === "На время");
  logSummary.innerHTML = `
    <article>
      <span>Тест длительности</span>
      <strong>${lastLongTest ? `${lastLongTest.startDate} ${lastLongTest.startTime}` : "—"}</strong>
    </article>
  `;

  logList.innerHTML = state.logs.map((entry) => `<article class="log-entry">${entry}</article>`).join("");
}

function renderBattery() {
  refs.batteryPercent.textContent = state.battery.percentAvailable ? `${state.battery.percent}%` : "—";
  refs.batteryHealth.textContent = state.battery.percentAvailable
    ? `Заряд в норме, ресурс ${state.battery.health}%`
    : "Данные АКБ недоступны";
  refs.batteryVoltage.textContent = state.battery.voltageAvailable ? formatValue(state.battery.voltage, "В", 1) : "—";
  refs.batteryChargeCurrent.textContent = state.battery.chargeCurrentAvailable ? formatValue(state.battery.chargeCurrent, "А", 1) : "—";
  refs.batteryTemp.textContent = state.battery.temperatureAvailable ? formatValue(state.battery.temperature, "°C", 0) : "—";
  refs.batteryStatus.textContent = state.battery.chargeStatus;
  refs.batteryCap.style.width = `${state.battery.percentAvailable ? state.battery.percent : 0}%`;

  refs.stopTestBtn.disabled = !state.testRunning;

  const specs = [
    ["Состояние", state.battery.stateText],
    ["Серийный номер", state.battery.serial],
    ["Тип батареи", state.battery.chemistry],
    ["Номинальное напряжение", state.battery.nominalVoltage],
    ["Номинальная ёмкость", state.battery.nominalCapacity],
    ["Внутреннее сопротивление", state.battery.internalResistance],
    ["Буферное напряжение", state.battery.floatVoltage],
    ["Остаточная автономия", state.battery.autonomy],
    ["Циклы заряд/разряд", state.battery.cycles ?? "—"],
    ["Последнее обслуживание", state.battery.lastService],
    ["Плановая замена", state.battery.nextReplacement]
  ];

  refs.batterySpecs.innerHTML = specs.map(([label, value]) => `
    <article class="spec-row">
      <span>${label}</span>
      <strong>${value}</strong>
    </article>
  `).join("");
}

function renderSettings() {
  renderLineSettingsList();
}

function renderLineSettingsList() {
  lineSettingsList.innerHTML = state.lines.map((line) => {
    const index = state.lines.indexOf(line);
    const modeText = line.mode === 0 ? "постоянная" : line.mode === 1 ? "непостоянная" : "линия отключена";
    return `
    <article class="line-settings-row">
      <strong>${line.description}</strong>
      <span>${modeText}</span>
      <button class="action-btn neutral" data-edit-line="${index}" type="button">Настроить</button>
    </article>
  `;
  }).join("");
}

function openLineModal(index) {
  state.selectedLineIndex = index;
  refs.lineManualPower.value = "";
  lineModal.classList.add("open");
  renderLineModal(index);
}

async function closeLineModal() {
  const index = state.selectedLineIndex;
  await stopLineTestIfNeeded(index);
  lineModal.classList.remove("open");
  clearLineModalInputs();
  state.selectedLineIndex = null;
}

function openScheduleModal(index) {
  state.selectedScheduleIndex = index;
  const item = state.schedule[index];
  refs.scheduleModalTitle.textContent = `Настройка записи ${index + 1}`;
  refs.schedulePeriodInput.value = item.period;
  refs.scheduleTypeInput.value = item.testType;
  refs.scheduleDateInput.value = item.startDate;
  refs.scheduleTimeInput.value = item.startTime;
  Array.from(refs.scheduleWeekdaysInput.querySelectorAll("input[type='checkbox']")).forEach((input) => {
    input.checked = item.weekDays.includes(input.value);
  });
  toggleWeekdaysVisibility();
  scheduleModal.classList.add("open");
}

function closeScheduleModal() {
  scheduleModal.classList.remove("open");
  state.selectedScheduleIndex = null;
}

function openTimeModal() {
  const now = new Date();
  refs.systemDate.value = `${now.getFullYear()}-${pad(now.getMonth() + 1)}-${pad(now.getDate())}`;
  refs.systemTime.value = `${pad(now.getHours())}:${pad(now.getMinutes())}`;
  timeModal.classList.add("open");
}

function closeTimeModal() {
  timeModal.classList.remove("open");
}

function openPasswordModal() {
  passwordModal.classList.add("open");
}

function closePasswordModal() {
  passwordModal.classList.remove("open");
}

function toggleWeekdaysVisibility() {
  refs.scheduleWeekdaysField.classList.toggle(
    "is-hidden",
    refs.schedulePeriodInput.value !== "дни недели"
  );
}

function bindEvents() {
  loginForm.addEventListener("submit", async (event) => {
    event.preventDefault();

    const password = loginPasswordInput.value.trim();
    if (!password) {
      setLoginError("Введите пароль шкафа");
      loginPasswordInput.focus();
      return;
    }

    loginSubmitBtn.disabled = true;
    setLoginError("");

    try {
      await loginWithPassword(password);
      await pollBackendStatus();
      setAuthenticated(true);
      renderAll();
      startBackendPolling();
      showToast("Вход выполнен");
    } catch (error) {
      setAuthenticated(false);
      setLoginError("Неверный пароль или backend недоступен");
    } finally {
      loginSubmitBtn.disabled = false;
    }
  });

  navButtons.forEach((button) => {
    button.addEventListener("click", () => switchView(button.dataset.view));
  });

  refs.manualTestBtn.addEventListener("click", async () => {
    try {
      await apiRequest("/api/test/start-functional", { method: "POST", auth: true });
      await pollBackendStatus();
      showToast("Тест линий запущен");
    } catch (error) {
      showToast(`Не удалось запустить тест: ${error.message}`);
    }
  });

  refs.durationTestBtn.addEventListener("click", async () => {
    try {
      await apiRequest("/api/test/start-duration", { method: "POST", auth: true });
      await pollBackendStatus();
      showToast("Тест на время запущен");
    } catch (error) {
      showToast(`Не удалось запустить тест: ${error.message}`);
    }
  });

  refs.stopTestBtn.addEventListener("click", async () => {
    if (refs.stopTestBtn.disabled) {
      return;
    }

    try {
      await apiRequest("/api/test/stop", { method: "POST", auth: true });
      await pollBackendStatus();
      showToast("Тест остановлен");
    } catch (error) {
      showToast(`Не удалось остановить тест: ${error.message}`);
    }
  });

  refs.systemActionBtn.addEventListener("click", async () => {
    try {
      if (state.programFireActive) {
        await apiRequest("/api/fire/off", { method: "POST", auth: true });
        await pollBackendStatus();
        showToast("Выполнен сброс \"Пожар\"");
        return;
      }

      if (state.testRunning) {
        await apiRequest("/api/test/stop", { method: "POST", auth: true });
        await pollBackendStatus();
        showToast("Тест остановлен");
        return;
      }

      await apiRequest("/api/fire/on", { method: "POST", auth: true });
      await pollBackendStatus();
      showToast("Выполнен пуск \"Пожар\"");
    } catch (error) {
      showToast(`Команда не выполнена: ${error.message}`);
    }
  });

  document.getElementById("addScheduleBtn").addEventListener("click", async () => {
    try {
      await apiRequest("/api/schedule/add", {
        method: "POST",
        auth: true,
        body: {
          enabled: true,
          period: "один раз",
          startDate: new Date().toISOString().slice(0, 10),
          startTime: "09:00",
          testType: "Функциональный",
          weekDays: []
        }
      });
      await refreshSchedule();
      showToast("Запись расписания добавлена");
    } catch (error) {
      showToast(`Не удалось добавить запись: ${error.message}`);
    }
  });

  scheduleList.addEventListener("click", (event) => {
    const editButton = event.target.closest("[data-edit-schedule]");
    if (editButton) {
      openScheduleModal(Number(editButton.dataset.editSchedule));
      return;
    }

    const button = event.target.closest("[data-remove-schedule]");
    if (!button) {
      return;
    }

    const index = Number(button.dataset.removeSchedule);
    void (async () => {
      try {
        await apiRequest(`/api/schedule/${index}/remove`, {
          method: "POST",
          auth: true
        });
        await refreshSchedule();
        showToast(`Запись ${index + 1} удалена`);
      } catch (error) {
        showToast(`Не удалось удалить запись: ${error.message}`);
      }
    })();
  });

  refs.closeScheduleModal.addEventListener("click", closeScheduleModal);
  refs.schedulePeriodInput.addEventListener("change", toggleWeekdaysVisibility);
  scheduleModal.addEventListener("click", (event) => {
    if (event.target === scheduleModal) {
      closeScheduleModal();
    }
  });

  refs.saveScheduleBtn.addEventListener("click", async () => {
    if (state.selectedScheduleIndex === null) {
      return;
    }

    const index = state.selectedScheduleIndex;
    const weekDays = Array.from(
      refs.scheduleWeekdaysInput.querySelectorAll("input[type='checkbox']:checked")
    ).map((input) => input.value);

    try {
      await apiRequest(`/api/schedule/${index}/update`, {
        method: "POST",
        auth: true,
        body: {
          period: refs.schedulePeriodInput.value.trim(),
          testType: refs.scheduleTypeInput.value.trim(),
          startDate: refs.scheduleDateInput.value,
          startTime: refs.scheduleTimeInput.value,
          weekDays
        }
      });
      await refreshSchedule();
      closeScheduleModal();
      showToast(`Расписание ${index + 1} сохранено`);
    } catch (error) {
      showToast(`Не удалось сохранить расписание: ${error.message}`);
    }
  });

  document.getElementById("exportLogsBtn").addEventListener("click", async () => {
    try {
      const response = await apiRequest("/api/logs/download-all", {
        method: "GET",
        auth: true,
        rawResponse: true
      });
      const content = await response.text();
      const blob = new Blob([content], { type: "text/plain;charset=utf-8" });
      const url = URL.createObjectURL(blob);
      const link = document.createElement("a");
      const stamp = new Date().toISOString().slice(0, 19).replace(/[:T]/g, "-");

      link.href = url;
      link.download = `panel-logs-${stamp}.txt`;
      document.body.appendChild(link);
      link.click();
      document.body.removeChild(link);
      URL.revokeObjectURL(url);
      showToast("Файл логов скачан");
    } catch (error) {
      showToast(`Не удалось скачать журнал: ${error.message}`);
    }
  });

  refs.syncNowBtn.addEventListener("click", async () => {
    const now = new Date();
    refs.systemDate.value = `${now.getFullYear()}-${pad(now.getMonth() + 1)}-${pad(now.getDate())}`;
    refs.systemTime.value = `${pad(now.getHours())}:${pad(now.getMinutes())}`;

    try {
      await apiRequest("/api/system/time", {
        method: "POST",
        auth: true,
        body: { msec: now.getTime() }
      });
      await pollBackendStatus();
      showToast("Системное время синхронизировано");
    } catch (error) {
      showToast(`Не удалось синхронизировать время: ${error.message}`);
    }
  });

  refs.openTimeModalBtn.addEventListener("click", openTimeModal);
  refs.openPasswordModalBtn.addEventListener("click", openPasswordModal);
  refs.closeTimeModal.addEventListener("click", closeTimeModal);
  refs.closePasswordModal.addEventListener("click", closePasswordModal);
  refs.savePasswordBtn.addEventListener("click", async () => {
    const newPassword = refs.newPassword.value.trim();
    const repeatPassword = refs.repeatPassword.value.trim();

    if (!newPassword || !repeatPassword) {
      showToast("Введите новый пароль дважды");
      return;
    }
    if (newPassword !== repeatPassword) {
      showToast("Новый пароль и повтор не совпадают");
      return;
    }

    try {
      await apiRequest("/api/password/change", {
        method: "POST",
        auth: true,
        body: { newPassword }
      });
      state.password = newPassword;
      refs.newPassword.value = "";
      refs.repeatPassword.value = "";
      closePasswordModal();
      showToast("Пароль сохранён");
    } catch (error) {
      showToast(`Не удалось сменить пароль: ${error.message}`);
    }
  });

  timeModal.addEventListener("click", (event) => {
    if (event.target === timeModal) {
      closeTimeModal();
    }
  });
  passwordModal.addEventListener("click", (event) => {
    if (event.target === passwordModal) {
      closePasswordModal();
    }
  });

  lineSettingsList.addEventListener("click", (event) => {
    const button = event.target.closest("[data-edit-line]");
    if (!button) {
      return;
    }

    openLineModal(Number(button.dataset.editLine));
  });

  refs.closeLineModal.addEventListener("click", () => {
    void closeLineModal();
  });
  lineModal.addEventListener("click", (event) => {
    if (event.target === lineModal) {
      void closeLineModal();
    }
  });

  refs.runLineTestBtn.addEventListener("click", async () => {
    if (state.selectedLineIndex === null) {
      return;
    }

    const index = state.selectedLineIndex;
    const line = state.lines[index];
    const lineTestRunning = isLineTestRunning(line, index);

    try {
      if (lineTestRunning) {
        await apiRequest(`/api/lines/${index}/test/stop`, {
          method: "POST",
          auth: true
        });
      } else {
        await apiRequest(`/api/lines/${index}/test/start`, {
          method: "POST",
          auth: true,
          body: { durationSec: 60 }
        });
      }
      await pollBackendStatus();
      renderLineModal(index);
      showToast(lineTestRunning
        ? `Тест линии ${index + 1} остановлен`
        : `Тест линии ${index + 1} запущен`);
    } catch (error) {
      showToast(`Не удалось выполнить команду теста линии: ${error.message}`);
    }
  });

  refs.applyMeasuredPowerBtn.addEventListener("click", async () => {
    if (state.selectedLineIndex === null) {
      return;
    }

    const line = state.lines[state.selectedLineIndex];
    if (typeof line.power !== "number" || line.power <= 0) {
      showToast("Нет измеренной мощности для записи");
      return;
    }

    try {
      await apiRequest(`/api/lines/${state.selectedLineIndex}/update`, {
        method: "POST",
        auth: true,
        body: {
          mpower: line.power
        }
      });
      await pollBackendStatus();
      renderLineModal(state.selectedLineIndex);
      showToast(`Измеренное значение записано в мощность линии ${state.selectedLineIndex + 1}`);
    } catch (error) {
      showToast(`Не удалось записать измеренное: ${error.message}`);
    }
  });

  refs.saveLineBtn.addEventListener("click", async () => {
    if (state.selectedLineIndex === null) {
      return;
    }

    const index = state.selectedLineIndex;
    const line = state.lines[index];
    const manualPowerValue = refs.lineManualPower.value.trim();
    const payload = {
      description: refs.lineNameInput.value.trim() || line.description,
      tolerance: Number(refs.lineToleranceInput.value),
      mode: Number(refs.lineModeInput.value)
    };

    if (manualPowerValue !== "") {
      payload.mpower = Number(manualPowerValue);
    }

    if (!Number.isFinite(payload.tolerance)) {
      payload.tolerance = typeof line.tolerance === "number" ? line.tolerance : 0;
    }

    try {
      await apiRequest(`/api/lines/${index}/update`, {
        method: "POST",
        auth: true,
        body: payload
      });
      await pollBackendStatus();
      await closeLineModal();
      showToast(`Линия ${index + 1} сохранена`);
    } catch (error) {
      showToast(`Не удалось сохранить линию: ${error.message}`);
    }
  });

  refs.lineModeInput.addEventListener("change", async () => {
    if (state.selectedLineIndex === null) {
      return;
    }

    const index = state.selectedLineIndex;

    try {
      await apiRequest(`/api/lines/${index}/update`, {
        method: "POST",
        auth: true,
        body: {
          mode: Number(refs.lineModeInput.value)
        }
      });
      await pollBackendStatus();
      renderLineModal(index);
      showToast(`Режим линии ${index + 1} сохранён`);
    } catch (error) {
      refs.lineModeInput.value = String(state.lines[index].mode);
      showToast(`Не удалось сохранить режим линии: ${error.message}`);
    }
  });

}

function renderAll() {
  renderHeaderClock();
  renderOverview();
  renderLines();
  renderBattery();
  renderTestPanel();
  renderSystemAction();
  renderSchedule();
  renderLogs();
  renderSettings();
}

bindEvents();
renderAll();
toggleWeekdaysVisibility();
setInterval(renderHeaderClock, 1000);
setAuthenticated(false);
