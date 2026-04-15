async function fetchJson(url, options) {
  const response = await fetch(url, options);
  if (!response.ok) {
    throw new Error(`HTTP ${response.status}`);
  }
  return response.json();
}

function pretty(value) {
  return JSON.stringify(value, null, 2);
}

async function refresh() {
  const status = await fetchJson('/api/status');
  const logs = await fetchJson('/api/logs');

  document.getElementById('status-output').textContent = pretty(status);
  document.getElementById('logs-output').textContent = pretty(logs);

  const form = document.getElementById('calibration-form');
  form.ds18b20TempOffsetC.value = status.calibration.ds18b20TempOffsetC;
  form.am2320TempOffsetC.value = status.calibration.am2320TempOffsetC;
  form.am2320HumidityOffsetPct.value = status.calibration.am2320HumidityOffsetPct;
}

document.getElementById('calibration-form').addEventListener('submit', async (event) => {
  event.preventDefault();
  const form = event.currentTarget;
  const payload = {
    ds18b20TempOffsetC: Number(form.ds18b20TempOffsetC.value || 0),
    am2320TempOffsetC: Number(form.am2320TempOffsetC.value || 0),
    am2320HumidityOffsetPct: Number(form.am2320HumidityOffsetPct.value || 0),
  };

  await fetchJson('/api/calibration', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(payload),
  });

  await refresh();
});

document.getElementById('poll-button').addEventListener('click', async () => {
  await fetchJson('/api/sensors/read', { method: 'POST' });
  await refresh();
});

refresh();
setInterval(refresh, 5000);
