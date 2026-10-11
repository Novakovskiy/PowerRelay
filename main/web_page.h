#pragma once

const char INDEX_HTML[] = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <link rel="icon" href="/favicon.ico" sizes="any">
  <title>PowerRelay</title>
  <style>
    :root {
      --bg1: #16223f;
      --bg2: #233045;
      --card: #1a243b;
      --text: #e5e7eb;
      --muted: #9ca3af;
      --green: #22c55e;
      --red: #ef4444;
      --btn: #142f6b;
      --btn-hover: #163ca5;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      font-family: "Segoe UI", Tahoma, sans-serif;
      color: var(--text);
      background: radial-gradient(1200px 600px at 20% 20%, #1d4ed8 0%, transparent 60%),
                  linear-gradient(135deg, var(--bg1), var(--bg2));
      display: grid;
      place-items: center;
      padding: 20px;
    }
    .card {
      width: min(460px, 100%);
      background: color-mix(in srgb, var(--card) 88%, black 12%);
      border: 1px solid #334155;
      border-radius: 18px;
      padding: 24px;
      box-shadow: 0 20px 45px rgba(0, 0, 0, 0.45);
    }
    h1 {
      margin: 0 0 16px;
      font-size: 1.4rem;
      letter-spacing: 0.02em;
    }
    .status {
      display: flex;
      align-items: center;
      gap: 10px;
      margin-bottom: 20px;
      color: var(--muted);
    }
    .dot {
      width: 16px;
      height: 16px;
      border-radius: 50%;
      background: var(--red);
      box-shadow: 0 0 0 0 rgba(239, 68, 68, 0.7);
      transition: 0.2s;
    }
    .dot.on {
      background: var(--green);
      box-shadow: 0 0 16px rgba(34, 197, 94, 0.9);
    }
    .grid {
      display: grid;
      grid-template-columns: 1fr;
      gap: 10px;
    }
    button {
      border: 0;
      border-radius: 12px;
      padding: 12px;
      color: #fff;
      background: var(--btn);
      font-size: 1rem;
      cursor: pointer;
      transition: transform 0.08s ease, background 0.2s ease;
    }
    button:hover { background: var(--btn-hover); }
    button:active { transform: scale(0.98); }
    button:disabled {
      opacity: 0.6;
      cursor: not-allowed;
      background: #475569;
    }
    .hint {
      margin-top: 14px;
      font-size: 0.86rem;
      color: var(--muted);
      line-height: 1.35;
    }
  </style>
</head>
<body>
  <main class="card">
    <h1>Voron Power Control</h1>
    <div class="status">
      <span id="dot" class="dot"></span>
      <span>Power: <strong id="stateText">Off</strong></span>
    </div>
    <div class="grid">
      <button id="powerBtn" onclick="sendCmd('on')">On/Off</button>
    </div>
    <p class="hint"><a href="/settings" style="color:#93c5fd; text-decoration:none;">Settings</a></p>
  </main>

  <script>
    function applyHostNameTitle(hostName) {
      const trimmed = (hostName || '').trim();
      if (trimmed.length > 0) {
        document.title = trimmed;
      }
    }

    async function loadHostNameTitle() {
      try {
        const r = await fetch('/api/settings');
        const data = await r.json();
        applyHostNameTitle(data.hostName);
      } catch (_) {
        // Keep default title when settings are temporarily unavailable.
      }
    }

    async function refreshStatus() {
      try {
        const r = await fetch('/status');
        const data = await r.json();
        const on = !!data.powerOn;
        const pendingOff = !!data.pendingOff;
        const pendingOffMs = Number(data.pendingOffMs || 0);
        document.getElementById('dot').classList.toggle('on', on);
        const stateText = document.getElementById('stateText');
        const powerBtn = document.getElementById('powerBtn');
        powerBtn.disabled = pendingOff;

        if (pendingOff) {
          const seconds = Math.max(1, Math.ceil(pendingOffMs / 1000));
          stateText.textContent = 'Off in ' + seconds + 's';
        } else {
          stateText.textContent = on ? 'On' : 'Off';
        }
      } catch (_) {
        document.getElementById('stateText').textContent = 'N/A';
        document.getElementById('powerBtn').disabled = true;
      }
    }

    async function sendCmd(cmd) {
      await fetch('/action?cmd=' + encodeURIComponent(cmd), { method: 'POST' });
      setTimeout(refreshStatus, 400);
    }

    loadHostNameTitle();
    refreshStatus();
    setInterval(refreshStatus, 1500);
  </script>
</body>
</html>
)rawliteral";

const char SETTINGS_HTML[] = R"rawliteral(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width,initial-scale=1">
  <link rel="icon" href="/favicon.ico" sizes="any">
  <title>PowerRelay</title>
  <style>
    :root {
      --bg1: #16223f;
      --bg2: #233045;
      --card: #1a243b;
      --text: #e5e7eb;
      --muted: #9ca3af;
      --btn: #142f6b;
      --btn-hover: #163ca5;
      --ok: #22c55e;
    }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      font-family: "Segoe UI", Tahoma, sans-serif;
      color: var(--text);
      background: radial-gradient(1200px 600px at 20% 20%, #1d4ed8 0%, transparent 60%),
                  linear-gradient(135deg, var(--bg1), var(--bg2));
      display: grid;
      place-items: center;
      padding: 20px;
    }
    .card {
      width: min(640px, 100%);
      background: color-mix(in srgb, var(--card) 88%, black 12%);
      border: 1px solid #334155;
      border-radius: 18px;
      padding: 24px;
      box-shadow: 0 20px 45px rgba(0, 0, 0, 0.45);
    }
    h1 {
      margin: 0 0 16px;
      font-size: 1.35rem;
      letter-spacing: 0.02em;
    }
    .grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 12px;
    }
    .field {
      display: flex;
      flex-direction: column;
      gap: 6px;
    }
    .field.full { grid-column: 1 / -1; }
    .divider {
      grid-column: 1 / -1;
      margin-top: 6px;
      padding-top: 10px;
      border-top: 1px solid #334155;
      color: var(--muted);
      font-size: 0.82rem;
      letter-spacing: 0.05em;
      text-transform: uppercase;
    }
    label {
      font-size: 0.9rem;
      color: var(--muted);
    }
    input[type="text"], input[type="password"], input[type="number"] {
      border: 1px solid #334155;
      background: #0b1220;
      color: var(--text);
      border-radius: 10px;
      padding: 10px 12px;
      width: 100%;
    }
    .check {
      display: flex;
      align-items: center;
      gap: 8px;
      margin-top: 4px;
    }
    .actions {
      display: flex;
      gap: 10px;
      margin-top: 16px;
      flex-wrap: wrap;
    }
    button, .link {
      border: 0;
      border-radius: 10px;
      padding: 10px 14px;
      color: #fff;
      background: var(--btn);
      font-size: 0.95rem;
      cursor: pointer;
      text-decoration: none;
      transition: background 0.2s ease;
    }
    button:hover, .link:hover { background: var(--btn-hover); }
    .hint {
      margin-top: 14px;
      font-size: 0.86rem;
      color: var(--muted);
      line-height: 1.35;
    }
    #msg {
      margin-top: 12px;
      min-height: 1.2em;
      color: var(--ok);
      font-size: 0.9rem;
    }
    .note {
      margin-top: 12px;
      color: var(--muted);
      font-size: 0.85rem;
      line-height: 1.35;
    }
    @media (max-width: 640px) {
      .grid { grid-template-columns: 1fr; }
    }
  </style>
</head>
<body>
  <main class="card">
    <h1>Settings</h1>
    <form id="cfgForm">
      <div class="grid">
        <div class="divider">Network</div>
        <div class="field">
          <label for="hostName" data-base-label="DHCP Hostname">Hostname</label>
          <input id="hostName" name="hostName" type="text" maxlength="24">
        </div>
        <div class="field">
          <label for="wifiReconnectSec">Wi-Fi reconnect interval (sec)</label>
          <input id="wifiReconnectSec" name="wifiReconnectSec" type="number" min="10" max="600">
        </div>
        <div class="field full">
          <label for="wifiSsid" data-base-label="Wi-Fi SSID">Wi-Fi SSID</label>
          <input id="wifiSsid" name="wifiSsid" type="text" maxlength="24">
        </div>
        <div class="field full">
          <label for="wifiPassword" data-base-label="Wi-Fi Password">Wi-Fi Password</label>
          <input id="wifiPassword" name="wifiPassword" type="password" maxlength="24">
        </div>
        <div class="divider">MQTT</div>
        <div class="field">
          <label class="check"><input id="mqttEnabled" name="mqttEnabled" type="checkbox"> Enable MQTT</label>
        </div>
        <div class="field">
          <label class="check"><input id="mqttHaDiscovery" name="mqttHaDiscovery" type="checkbox"> Home Assistant MQTT discovery</label>
        </div>
        <div class="field">
          <label for="mqttHost" data-base-label="MQTT Host/IP">MQTT Host/IP</label>
          <input id="mqttHost" name="mqttHost" type="text" maxlength="64">
        </div>
        <div class="field">
          <label for="mqttPort">MQTT Port</label>
          <input id="mqttPort" name="mqttPort" type="number" min="1" max="65535">
        </div>
        <div class="field">
          <label for="mqttUser" data-base-label="MQTT Username">MQTT Username</label>
          <input id="mqttUser" name="mqttUser" type="text" maxlength="24">
        </div>
        <div class="field">
          <label for="mqttPassword" data-base-label="MQTT Password">MQTT Password</label>
          <input id="mqttPassword" name="mqttPassword" type="password" maxlength="24">
        </div>
        <div class="field full">
          <label for="mqttBaseTopic" data-base-label="MQTT Base Topic">MQTT Base Topic</label>
          <input id="mqttBaseTopic" name="mqttBaseTopic" type="text" maxlength="48">
        </div>
        <div class="divider">Relay</div>
        <div class="field full">
          <label class="check"><input id="powerOnByDefault" name="powerOnByDefault" type="checkbox"> Power enabled by default on startup</label>
        </div>
        <div class="divider">UART</div>
        <div class="field full">
          <label class="check"><input id="uartControl" name="uartControl" type="checkbox"> Use UART control</label>
        </div>
        <div class="field">
          <label for="uartTimeout">Delay before shutdown (sec)</label>
          <input id="uartTimeout" name="uartTimeout" type="number" min="10" max="600">
        </div>
      </div>
      <div class="actions">
        <button type="submit">Save Settings</button>
        <button type="button" id="rebootBtn">Reboot</button>
        <a class="link" href="/">Back</a>
      </div>
      <p class="hint"><a href="#" id="resetDefaultsLink" style="color:#93c5fd; text-decoration:none;">Reset to default</a></p>
      <div id="msg"></div>
    </form>
  </main>

  <script>
    function applyFieldLimits(data) {
      const maxLens = data.maxLens || {};
      const fields = ['wifiSsid', 'wifiPassword', 'hostName', 'mqttHost', 'mqttUser', 'mqttPassword', 'mqttBaseTopic'];

      for (const id of fields) {
        const input = document.getElementById(id);
        if (!input) continue;

        const fromApi = Number(maxLens[id]);
        const maxLen = Number.isFinite(fromApi) && fromApi > 0 ? fromApi : Number(input.maxLength);
        if (Number.isFinite(maxLen) && maxLen > 0) {
          input.maxLength = maxLen;
        }
      }
    }

    function applyHostNameTitle(hostName) {
      const trimmed = (hostName || '').trim();
      if (trimmed.length > 0) {
        document.title = trimmed + ' - Settings';
      } else {
        document.title = 'Settings';
      }
    }

    function fillForm(data) {
      applyFieldLimits(data);
      document.getElementById('wifiSsid').value = data.wifiSsid || '';
      document.getElementById('wifiPassword').value = data.wifiPassword || '';
      document.getElementById('hostName').value = data.hostName || '';
      const reconnectMs = Number(data.wifiReconnectMs || 30000);
      document.getElementById('wifiReconnectSec').value = Math.max(10, Math.round(reconnectMs / 1000));
      document.getElementById('mqttEnabled').checked = !!data.mqttEnabled;
      document.getElementById('mqttHost').value = data.mqttHost || '';
      document.getElementById('mqttPort').value = data.mqttPort || 1883;
      document.getElementById('mqttUser').value = data.mqttUser || '';
      document.getElementById('mqttPassword').value = data.mqttPassword || '';
      document.getElementById('mqttBaseTopic').value = data.mqttBaseTopic || 'pc_powerctrl';
      document.getElementById('mqttHaDiscovery').checked = data.mqttHaDiscovery !== false;
      document.getElementById('powerOnByDefault').checked = !!data.powerOnByDefault;
      document.getElementById('uartControl').checked = !!data.uartControl;
      const uartTimeoutMs = Number(data.uartTimeout || 30000);
      const uartTimeoutSec = Math.max(10, Math.min(600, Math.round(uartTimeoutMs / 1000)));
      document.getElementById('uartTimeout').value = uartTimeoutSec;
      applyHostNameTitle(data.hostName);
    }

    async function loadSettings() {
      const r = await fetch('/api/settings');
      const data = await r.json();
      fillForm(data);
    }

    function startRebootCountdown(msgEl, seconds) {
      let left = seconds;
      msgEl.textContent = 'Rebooting. Wait ' + left + ' seconds.';

      const timer = setInterval(() => {
        left -= 1;
        if (left <= 0) {
          clearInterval(timer);
          window.location.reload();
          return;
        }
        msgEl.textContent = 'Rebooting. Wait ' + left + ' seconds.';
      }, 1000);
    }

    document.getElementById('cfgForm').addEventListener('submit', async (e) => {
      e.preventDefault();
      const msg = document.getElementById('msg');
      msg.textContent = 'Saving...';

      const body = new URLSearchParams();
      const reconnectSec = Number(document.getElementById('wifiReconnectSec').value || 30);
      body.set('wifiSsid', document.getElementById('wifiSsid').value);
      body.set('wifiPassword', document.getElementById('wifiPassword').value);
      body.set('hostName', document.getElementById('hostName').value);
      body.set('wifiReconnectMs', String(Math.max(10, Math.round(reconnectSec)) * 1000));
      body.set('mqttEnabled', document.getElementById('mqttEnabled').checked ? '1' : '0');
      body.set('mqttHost', document.getElementById('mqttHost').value);
      body.set('mqttPort', document.getElementById('mqttPort').value);
      body.set('mqttUser', document.getElementById('mqttUser').value);
      body.set('mqttPassword', document.getElementById('mqttPassword').value);
      body.set('mqttBaseTopic', document.getElementById('mqttBaseTopic').value);
      body.set('mqttHaDiscovery', document.getElementById('mqttHaDiscovery').checked ? '1' : '0');
      body.set('powerOnByDefault', document.getElementById('powerOnByDefault').checked ? '1' : '0');
      body.set('uartControl', document.getElementById('uartControl').checked ? '1' : '0');
      const uartTimeoutSec = Number(document.getElementById('uartTimeout').value || 30);
      body.set('uartTimeout', String(Math.max(10, Math.min(600, Math.round(uartTimeoutSec))) * 1000));

      const r = await fetch('/api/settings', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: body.toString()
      });

      if (!r.ok) {
        msg.textContent = 'Save error';
        return;
      }

      msg.textContent = 'Saved. Reloading...';
      setTimeout(() => {
        window.location.reload();
      }, 300);
    });

    document.getElementById('rebootBtn').addEventListener('click', () => {
      const msg = document.getElementById('msg');
      msg.textContent = 'Rebooting...';
      startRebootCountdown(msg, 5);

      void fetch('/api/reboot', { method: 'POST' }).catch(() => {
        // Connection may drop because the device is already rebooting.
      });
    });

    document.getElementById('resetDefaultsLink').addEventListener('click', (e) => {
      e.preventDefault();

      const confirmed = confirm('Reset settings to default and reboot?');
      if (!confirmed) {
        return;
      }

      const msg = document.getElementById('msg');
      msg.textContent = 'Resetting to default...';
      startRebootCountdown(msg, 10);

      void fetch('/api/reset', { method: 'POST' }).catch(() => {
        // Connection may drop because the device is already rebooting.
      });
    });

    loadSettings().catch(() => {
      document.getElementById('msg').textContent = 'Load error';
    });
  </script>
</body>
</html>
)rawliteral";
