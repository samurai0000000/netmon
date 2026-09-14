/*
 * WebAssets.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_WEBASSETS_HXX
#define NETMON_WEBASSETS_HXX

namespace assets {

inline constexpr const char* INDEX_HTML = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>netmon — Real-Time Network & SNMP Telemetry</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <div class="app-container">
        <header class="navbar">
            <div class="brand">
                <div class="logo-icon">NM</div>
                <div class="brand-text">
                    <h1>netmon</h1>
                    <span class="subtext">Telemetry & SNMP Gateway</span>
                </div>
            </div>
            <div class="status-indicators">
                <div class="pill-badge status-live"><span id="daemon-status">ONLINE</span></div>
                <div class="pill-badge status-meta"><span>Uptime: <strong id="daemon-uptime">--</strong></span></div>
                <div class="pill-badge status-meta"><span>DB: <strong id="db-size">--</strong></span></div>
            </div>
        </header>
        <main class="content">
            <section class="section-container">
                <div class="section-header">
                    <h2>WAN Uplink Telemetry</h2>
                    <div class="timeframe-selector">
                        <button class="time-btn" data-hours="1">1 Hour</button>
                        <button class="time-btn active" data-hours="24">24 Hours</button>
                    </div>
                </div>
                <div class="wan-grid" id="wan-container">
                    <div class="loading-placeholder">Discovering WAN interfaces...</div>
                </div>
            </section>
        </main>
    </div>
    <script src="/app.js"></script>
</body>
</html>)rawliteral";

inline constexpr const char* STYLE_CSS = R"rawliteral(
body {
    background-color: #0a0d14;
    color: #f1f5f9;
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
    margin: 0;
    padding: 0;
}
.navbar {
    display: flex;
    justify-content: space-between;
    padding: 1rem 2rem;
    background: #121826;
    border-bottom: 1px solid rgba(255,255,255,0.08);
}
.brand-text h1 { margin: 0; font-size: 1.25rem; }
.subtext { font-size: 0.75rem; color: #64748b; }
.content { padding: 2rem; max-width: 1400px; margin: 0 auto; }
.wan-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(500px, 1fr)); gap: 1.5rem; }
.glass-card { background: #121826; border: 1px solid rgba(255,255,255,0.08); border-radius: 12px; padding: 1.5rem; }
.rate-val { font-size: 1.8rem; font-weight: bold; color: #00f2fe; }
.chart-container { width: 100%; height: 180px; background: rgba(0,0,0,0.4); border-radius: 8px; margin-top: 1rem; }
)rawliteral";

inline constexpr const char* APP_JS = R"rawliteral(
(async function() {
    async function update() {
        try {
            const res = await fetch('/api/snmp/wan');
            if (!res.ok) return;
            const data = await res.json();
            const container = document.getElementById('wan-container');
            if (!container || !data.wan_interfaces) return;
            container.innerHTML = data.wan_interfaces.map(w => `
                <div class="glass-card">
                    <h3>${w.interface.toUpperCase()} (${w.alias || 'WAN'})</h3>
                    <div class="rate-val">${(w.rate_in_mbps || 0).toFixed(2)} Mbps In / ${(w.rate_out_mbps || 0).toFixed(2)} Mbps Out</div>
                    <div style="font-size: 0.85rem; color: #94a3b8; margin-top: 0.5rem;">
                        5m Avg: ${(w.avg5min_in_mbps || 0).toFixed(2)}M / Peak: ${(w.peak_24h_in_mbps || 0).toFixed(2)}M
                    </div>
                </div>
            `).join('');
        } catch(e) {}
    }
    update();
    setInterval(update, 3000);
})();
)rawliteral";

} // namespace assets

#endif /* NETMON_WEBASSETS_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
