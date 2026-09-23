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
    <link rel="preconnect" href="https://fonts.googleapis.com">
    <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@300;400;500;600;700&family=JetBrains+Mono:wght@400;500;600&display=swap" rel="stylesheet">
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <div class="app-container">
        <!-- Header -->
        <header class="navbar">
            <div class="brand">
                <div class="logo-icon">
                    <svg viewBox="0 0 24 24" width="22" height="22" fill="none" stroke="currentColor" stroke-width="2.2" stroke-linecap="round" stroke-linejoin="round">
                        <polyline points="22 12 18 12 15 21 9 3 6 12 2 12"></polyline>
                    </svg>
                </div>
                <div class="brand-text">
                    <h1>netmon</h1>
                    <span class="subtext">Telemetry & SNMP Gateway</span>
                </div>
            </div>

            <div class="status-indicators">
                <div class="pill-badge status-live">
                    <span class="pulse-dot"></span>
                    <span id="daemon-status">ONLINE</span>
                </div>
                <div class="pill-badge status-meta" id="uptime-pill">
                    <span>Uptime: <strong id="daemon-uptime">--</strong></span>
                </div>
                <div class="pill-badge status-meta" id="db-pill">
                    <span>DB: <strong id="db-size">--</strong></span>
                </div>
            </div>
        </header>

        <!-- Main Dashboard View -->
        <main class="content">
            <!-- WAN Telemetry Section (MRTG Modernization) -->
            <section class="section-container">
                <div class="section-header">
                    <div class="section-title">
                        <h2>WAN Uplink Telemetry</h2>
                        <span class="badge-tag">SNMP v2c / 64-bit HC</span>
                    </div>
                    <div class="timeframe-selector">
                        <button class="time-btn" data-hours="1">Hour</button>
                        <button class="time-btn active" data-hours="24">Day</button>
                        <button class="time-btn" data-hours="168">Week</button>
                        <button class="time-btn" data-hours="720">Month</button>
                        <button class="time-btn" data-hours="8760">Year</button>
                    </div>
                </div>

                <div class="wan-grid" id="wan-container">
                    <!-- Dynamically populated by app.js -->
                    <div class="loading-placeholder">Discovering WAN interfaces...</div>
                </div>
            </section>

            <!-- Secondary Metrics Grid: LAN Traffic & Top Talkers -->
            <section class="section-container">
                <div class="section-header">
                    <div class="section-title">
                        <h2>LAN Network Traffic & Sniffer</h2>
                        <span class="badge-tag">PCAP Live Capture</span>
                    </div>
                </div>

                <div class="metrics-grid">
                    <!-- Summary Card -->
                    <div class="glass-card stat-card">
                        <h3>LAN Throughput</h3>
                        <div class="stat-value" id="lan-current-rate">0.00 <span class="unit">Mbps</span></div>
                        <div class="stat-detail-row">
                            <span>Total Packets: <strong id="lan-total-packets">0</strong></span>
                            <span>Capture State: <strong class="text-success" id="pcap-state">Active</strong></span>
                        </div>
                    </div>

                    <!-- Protocol Distribution -->
                    <div class="glass-card stat-card">
                        <h3>Protocol Breakdown</h3>
                        <div class="protocol-bar" id="protocol-bar">
                            <div class="proto-segment proto-https" style="width: 60%"></div>
                            <div class="proto-segment proto-dns" style="width: 15%"></div>
                            <div class="proto-segment proto-ssh" style="width: 15%"></div>
                            <div class="proto-segment proto-other" style="width: 10%"></div>
                        </div>
                        <div class="protocol-legend" id="protocol-legend">
                            <span><i class="dot proto-https-dot"></i> HTTPS</span>
                            <span><i class="dot proto-dns-dot"></i> DNS</span>
                            <span><i class="dot proto-ssh-dot"></i> SSH</span>
                            <span><i class="dot proto-arp-dot"></i> ARP</span>
                            <span><i class="dot proto-other-dot"></i> Other</span>
                        </div>
                    </div>

                    <!-- Top Talker Card -->
                    <div class="glass-card stat-card talkers-card">
                        <h3>Top Bandwidth Hosts (15m)</h3>
                        <ul class="top-talker-list" id="top-talkers-list">
                            <li><span class="talker-empty-hint">Scanning LAN flows...</span></li>
                        </ul>
                    </div>
                </div>
            </section>

            <!-- LAN Devices Registry Section -->
            <section class="section-container">
                <div class="section-header">
                    <div class="section-title">
                        <h2>Discovered LAN Devices</h2>
                        <span class="badge-tag" id="devices-count-badge">0 Devices</span>
                    </div>
                    <div class="table-search">
                        <input type="text" id="device-search" placeholder="Search IP, MAC, Name, or Vendor...">
                    </div>
                </div>

                <div class="glass-card table-card">
                    <div class="table-scroll-container">
                        <table class="data-table">
                            <thead>
                                <tr>
                                    <th class="sortable" data-sort="name">Device Name <span class="sort-icon"></span></th>
                                    <th class="sortable active-sort asc" data-sort="ip">IPv4 Address <span class="sort-icon">▲</span></th>
                                    <th class="sortable" data-sort="mac">MAC Address <span class="sort-icon"></span></th>
                                    <th class="sortable" data-sort="vendor">OUI Vendor <span class="sort-icon"></span></th>
                                    <th class="sortable" data-sort="category">Category <span class="sort-icon"></span></th>
                                    <th class="sortable" data-sort="last_seen">Last Seen <span class="sort-icon"></span></th>
                                </tr>
                            </thead>
                            <tbody id="devices-tbody">
                                <tr><td colspan="6" class="text-center">Loading device registry...</td></tr>
                            </tbody>
                        </table>
                    </div>
                    <div class="pagination-bar" id="devices-pagination">
                        <div class="page-size-picker">
                            <label for="page-size-select">Show:</label>
                            <select id="page-size-select">
                                <option value="25" selected>25 / page</option>
                                <option value="50">50 / page</option>
                                <option value="100">100 / page</option>
                                <option value="0">All</option>
                            </select>
                        </div>
                        <div class="page-nav">
                            <button type="button" class="btn-page" id="btn-page-prev" disabled>&lsaquo; Prev</button>
                            <span class="page-info" id="devices-page-info">Page 1 of 1</span>
                            <button type="button" class="btn-page" id="btn-page-next" disabled>Next &rsaquo;</button>
                        </div>
                    </div>
                </div>
            </section>
        </main>

        <footer class="footer">
            <span>netmon v1.0.3 &bull; Copyright &copy; 2026, Charles Chiou &bull; Autonomous AI & Network Telemetry</span>
        </footer>
    </div>

    <script src="/app.js"></script>
</body>
</html>
)rawliteral";

inline constexpr const char* STYLE_CSS = R"rawliteral(
/*
 * style.css — netmon Dark Theme & Modern Glassmorphism Design System
 * Copyright (C) 2026, Charles Chiou
 */

:root {
    --bg-primary: #0a0d14;
    --bg-surface: rgba(18, 24, 38, 0.75);
    --bg-surface-elevated: rgba(26, 35, 54, 0.85);
    --border-color: rgba(255, 255, 255, 0.08);
    --border-highlight: rgba(79, 172, 254, 0.35);

    --text-primary: #f1f5f9;
    --text-secondary: #94a3b8;
    --text-muted: #64748b;

    --accent-cyan: #00f2fe;
    --accent-blue: #4facfe;
    --accent-emerald: #10b981;
    --accent-amber: #f59e0b;
    --accent-rose: #f43f5e;
    --accent-purple: #8b5cf6;
    --accent-teal: #06b6d4;

    --font-sans: 'Inter', -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
    --font-mono: 'JetBrains Mono', monospace;

    --radius-sm: 8px;
    --radius-md: 14px;
    --radius-lg: 20px;
    --shadow-glass: 0 8px 32px 0 rgba(0, 0, 0, 0.37);
}

* {
    box-sizing: border-box;
    margin: 0;
    padding: 0;
}

body {
    background-color: var(--bg-primary);
    background-image: 
        radial-gradient(at 0% 0%, rgba(79, 172, 254, 0.08) 0px, transparent 50%),
        radial-gradient(at 100% 100%, rgba(139, 92, 246, 0.06) 0px, transparent 50%);
    color: var(--text-primary);
    font-family: var(--font-sans);
    min-height: 100vh;
    display: flex;
    flex-direction: column;
    -webkit-font-smoothing: antialiased;
    overflow-x: hidden;
}

.app-container {
    display: flex;
    flex-direction: column;
    min-height: 100vh;
    width: 100%;
    max-width: 100%;
    overflow-x: hidden;
}

/* Navbar */
.navbar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 1rem 2rem;
    background: var(--bg-surface);
    backdrop-filter: blur(14px);
    -webkit-backdrop-filter: blur(14px);
    border-bottom: 1px solid var(--border-color);
    position: sticky;
    top: 0;
    z-index: 100;
    width: 100%;
}

.brand {
    display: flex;
    align-items: center;
    gap: 0.85rem;
    flex-shrink: 0;
}

.logo-icon {
    width: 38px;
    height: 38px;
    border-radius: var(--radius-sm);
    background: linear-gradient(135deg, var(--accent-blue) 0%, var(--accent-cyan) 100%);
    color: #050505;
    display: flex;
    align-items: center;
    justify-content: center;
    box-shadow: 0 0 16px rgba(0, 242, 254, 0.4);
    flex-shrink: 0;
}

.brand-text h1 {
    font-size: 1.25rem;
    font-weight: 700;
    letter-spacing: -0.02em;
    background: linear-gradient(135deg, #ffffff 40%, var(--accent-blue) 100%);
    -webkit-background-clip: text;
    background-clip: text;
    -webkit-text-fill-color: transparent;
}

.brand-text .subtext {
    font-size: 0.72rem;
    color: var(--text-muted);
    letter-spacing: 0.05em;
    text-transform: uppercase;
    font-weight: 500;
}

.status-indicators {
    display: flex;
    align-items: center;
    gap: 0.75rem;
    flex-wrap: wrap;
}

.pill-badge {
    display: flex;
    align-items: center;
    gap: 0.45rem;
    padding: 0.35rem 0.8rem;
    border-radius: 9999px;
    font-size: 0.78rem;
    font-family: var(--font-mono);
    background: var(--bg-surface-elevated);
    border: 1px solid var(--border-color);
    white-space: nowrap;
}

.status-live {
    color: var(--accent-emerald);
    border-color: rgba(16, 185, 129, 0.3);
    background: rgba(16, 185, 129, 0.08);
}

.pulse-dot {
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: var(--accent-emerald);
    box-shadow: 0 0 8px var(--accent-emerald);
    animation: pulse 2s infinite ease-in-out;
}

@keyframes pulse {
    0%, 100% { transform: scale(1); opacity: 1; }
    50% { transform: scale(1.3); opacity: 0.6; }
}

/* Content Container */
.content {
    flex: 1;
    padding: 2rem;
    max-width: 1400px;
    margin: 0 auto;
    width: 100%;
}

.section-container {
    margin-bottom: 2.5rem;
}

.section-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 1.25rem;
    gap: 1rem;
}

.section-title {
    display: flex;
    align-items: center;
    gap: 0.75rem;
    flex-wrap: wrap;
}

.section-title h2 {
    font-size: 1.18rem;
    font-weight: 600;
    color: var(--text-primary);
    white-space: nowrap;
}

.badge-tag {
    font-size: 0.72rem;
    font-weight: 500;
    padding: 0.2rem 0.6rem;
    background: rgba(79, 172, 254, 0.12);
    color: var(--accent-blue);
    border-radius: var(--radius-sm);
    border: 1px solid rgba(79, 172, 254, 0.25);
    white-space: nowrap;
}

/* Timeframe Buttons */
.timeframe-selector {
    display: flex;
    background: var(--bg-surface-elevated);
    border: 1px solid var(--border-color);
    border-radius: var(--radius-sm);
    padding: 2px;
    flex-shrink: 0;
}

.time-btn {
    background: transparent;
    border: none;
    color: var(--text-secondary);
    padding: 0.35rem 0.8rem;
    font-size: 0.78rem;
    font-weight: 500;
    border-radius: calc(var(--radius-sm) - 2px);
    cursor: pointer;
    transition: all 0.2s;
    white-space: nowrap;
}

.time-btn:hover {
    color: var(--text-primary);
}

.time-btn.active {
    background: var(--accent-blue);
    color: #050505;
    font-weight: 600;
}

/* Cards & Glassmorphism */
.glass-card {
    background: var(--bg-surface);
    backdrop-filter: blur(12px);
    -webkit-backdrop-filter: blur(12px);
    border: 1px solid var(--border-color);
    border-radius: var(--radius-md);
    box-shadow: var(--shadow-glass);
    overflow: hidden;
    transition: border-color 0.2s, transform 0.2s;
}

.glass-card:hover {
    border-color: var(--border-highlight);
}

/* WAN Grid */
.wan-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(min(100%, 480px), 1fr));
    gap: 1.5rem;
}

.wan-card {
    padding: 1.5rem;
    display: flex;
    flex-direction: column;
    gap: 1.25rem;
}

.wan-card-header {
    display: flex;
    justify-content: space-between;
    align-items: flex-start;
    gap: 0.75rem;
}

.wan-name h3 {
    font-size: 1.15rem;
    font-weight: 600;
    color: #ffffff;
    display: flex;
    align-items: center;
    gap: 0.5rem;
}

.wan-sub-info {
    display: flex;
    align-items: center;
    gap: 0.6rem;
    flex-wrap: wrap;
    margin-top: 0.25rem;
}

.wan-alias {
    font-size: 0.8rem;
    color: var(--text-muted);
}

.wan-ip-badge {
    display: inline-flex;
    align-items: center;
    gap: 0.35rem;
    padding: 0.12rem 0.55rem;
    border-radius: 9999px;
    font-size: 0.72rem;
    font-family: var(--font-mono);
    font-weight: 500;
    background: rgba(0, 242, 254, 0.08);
    color: var(--accent-cyan);
    border: 1px solid rgba(0, 242, 254, 0.25);
}

.wan-ip-badge svg {
    color: var(--accent-cyan);
    opacity: 0.9;
}

.link-status-badge {
    padding: 0.25rem 0.65rem;
    border-radius: 9999px;
    font-size: 0.75rem;
    font-family: var(--font-mono);
    font-weight: 600;
    white-space: nowrap;
    flex-shrink: 0;
}

.status-up {
    background: rgba(16, 185, 129, 0.15);
    color: var(--accent-emerald);
    border: 1px solid rgba(16, 185, 129, 0.35);
}

.status-down {
    background: rgba(244, 63, 94, 0.15);
    color: var(--accent-rose);
    border: 1px solid rgba(244, 63, 94, 0.35);
}

/* Rate Displays */
.wan-rate-row {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 1rem;
}

.rate-box {
    background: var(--bg-surface-elevated);
    border: 1px solid var(--border-color);
    border-radius: var(--radius-sm);
    padding: 1rem;
    display: flex;
    flex-direction: column;
    gap: 0.25rem;
    min-width: 0;
}

.rate-label {
    font-size: 0.75rem;
    text-transform: uppercase;
    color: var(--text-muted);
    font-weight: 600;
    display: flex;
    align-items: center;
    gap: 0.4rem;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
}

.rate-in-label i {
    color: var(--accent-cyan);
    font-style: normal;
}

.rate-out-label i {
    color: var(--accent-purple);
    font-style: normal;
}

.rate-val {
    font-size: 1.5rem;
    font-weight: 700;
    font-family: var(--font-mono);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
}

.rate-val-in {
    color: var(--accent-cyan);
}

.rate-val-out {
    color: var(--accent-purple);
}

.rate-val .unit {
    font-size: 0.85rem;
    font-weight: 500;
    color: var(--text-muted);
}

/* Summary stats list */
.wan-meta-grid {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 0.75rem;
    padding-top: 0.5rem;
    border-top: 1px solid var(--border-color);
}

.meta-item {
    display: flex;
    flex-direction: column;
    gap: 0.15rem;
    min-width: 0;
}

.meta-title {
    font-size: 0.7rem;
    color: var(--text-muted);
    text-transform: uppercase;
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
}

.meta-val {
    font-size: 0.85rem;
    font-weight: 600;
    font-family: var(--font-mono);
    color: var(--text-secondary);
    white-space: nowrap;
    overflow: hidden;
    text-overflow: ellipsis;
}

/* Chart Canvas Wrapper */
.chart-container {
    width: 100%;
    height: 180px;
    position: relative;
    background: rgba(10, 13, 20, 0.6);
    border-radius: var(--radius-sm);
    border: 1px solid var(--border-color);
    overflow: hidden;
    touch-action: none;
}

.chart-canvas {
    width: 100%;
    height: 100%;
    display: block;
    cursor: crosshair;
}

.chart-tooltip {
    position: absolute;
    display: none;
    pointer-events: none;
    background: rgba(15, 23, 42, 0.94);
    backdrop-filter: blur(8px);
    -webkit-backdrop-filter: blur(8px);
    border: 1px solid rgba(255, 255, 255, 0.15);
    border-radius: var(--radius-sm);
    padding: 0.45rem 0.75rem;
    font-size: 0.75rem;
    color: var(--text-primary);
    z-index: 30;
    box-shadow: 0 4px 16px rgba(0, 0, 0, 0.6);
    white-space: nowrap;
    transform: translate(-50%, -120%);
    transition: opacity 0.1s;
}

.chart-tooltip .tip-time {
    color: var(--text-muted);
    font-size: 0.7rem;
    margin-bottom: 0.25rem;
    font-family: var(--font-mono);
}

.chart-tooltip .tip-row {
    display: flex;
    justify-content: space-between;
    gap: 0.8rem;
    font-family: var(--font-mono);
    font-size: 0.76rem;
}

.chart-tooltip .tip-in {
    color: var(--accent-cyan);
}

.chart-tooltip .tip-out {
    color: var(--accent-purple);
}

/* Secondary Metrics Grid */
.metrics-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(min(100%, 320px), 1fr));
    gap: 1.5rem;
}

.stat-card {
    padding: 1.5rem;
    display: flex;
    flex-direction: column;
    gap: 1rem;
    min-width: 0;
}

.stat-card h3 {
    font-size: 0.95rem;
    font-weight: 600;
    color: var(--text-secondary);
}

.stat-value {
    font-size: 2.2rem;
    font-weight: 700;
    font-family: var(--font-mono);
    color: #ffffff;
}

.stat-value .unit {
    font-size: 1rem;
    font-weight: 500;
    color: var(--text-muted);
}

.stat-detail-row {
    display: flex;
    justify-content: space-between;
    font-size: 0.82rem;
    color: var(--text-muted);
    border-top: 1px solid var(--border-color);
    padding-top: 0.75rem;
    flex-wrap: wrap;
    gap: 0.5rem;
}

.text-success {
    color: var(--accent-emerald);
}

/* Protocol Bar */
.protocol-bar {
    height: 12px;
    border-radius: 9999px;
    background: var(--bg-surface-elevated);
    display: flex;
    overflow: hidden;
    border: 1px solid var(--border-color);
}

.proto-segment {
    height: 100%;
    transition: width 0.3s ease;
}

.proto-segment-idle {
    width: 100%;
    background: linear-gradient(90deg, rgba(79, 172, 254, 0.2), rgba(139, 92, 246, 0.2));
}

.proto-https { background: var(--accent-blue); }
.proto-dns { background: var(--accent-emerald); }
.proto-ssh { background: var(--accent-amber); }
.proto-arp { background: var(--accent-teal); }
.proto-http { background: #38bdf8; }
.proto-other { background: var(--accent-purple); }

.protocol-legend {
    display: flex;
    gap: 0.85rem;
    flex-wrap: wrap;
    font-size: 0.78rem;
    color: var(--text-secondary);
}

.dot {
    display: inline-block;
    width: 8px;
    height: 8px;
    border-radius: 50%;
    margin-right: 4px;
}

.proto-https-dot { background: var(--accent-blue); }
.proto-dns-dot { background: var(--accent-emerald); }
.proto-ssh-dot { background: var(--accent-amber); }
.proto-arp-dot { background: var(--accent-teal); }
.proto-http-dot { background: #38bdf8; }
.proto-other-dot { background: var(--accent-purple); }

/* Top Talkers List */
.talkers-card {
    max-height: 240px;
}

.top-talker-list {
    list-style: none;
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
    overflow-y: auto;
}

.top-talker-list li {
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-size: 0.82rem;
    padding: 0.4rem 0.65rem;
    background: var(--bg-surface-elevated);
    border-radius: var(--radius-sm);
    font-family: var(--font-mono);
    border: 1px solid transparent;
    transition: border-color 0.15s;
}

.top-talker-list li:hover {
    border-color: var(--border-highlight);
}

.talker-rank {
    display: inline-flex;
    align-items: center;
    justify-content: center;
    width: 18px;
    height: 18px;
    border-radius: 4px;
    font-size: 0.68rem;
    font-weight: 700;
    margin-right: 6px;
    background: rgba(79, 172, 254, 0.15);
    color: var(--accent-blue);
}

.talker-empty-hint {
    padding: 1.25rem;
    text-align: center;
    color: var(--text-muted);
    font-size: 0.8rem;
    font-style: italic;
}

/* Table Search & Data Table */
.table-search {
    position: relative;
    display: flex;
    align-items: center;
}

.table-search input {
    background: var(--bg-surface-elevated);
    border: 1px solid var(--border-color);
    border-radius: var(--radius-sm);
    padding: 0.45rem 0.9rem;
    color: var(--text-primary);
    font-size: 0.82rem;
    width: 280px;
    outline: none;
    transition: border-color 0.2s, box-shadow 0.2s;
    font-family: var(--font-sans);
}

.table-search input:focus {
    border-color: var(--accent-blue);
    box-shadow: 0 0 0 2px rgba(79, 172, 254, 0.2);
}

.table-card {
    padding: 0;
    overflow: hidden;
    display: flex;
    flex-direction: column;
}

.table-scroll-container {
    overflow-x: auto;
    max-height: 520px;
    overflow-y: auto;
    -webkit-overflow-scrolling: touch;
    scrollbar-width: thin;
}

.data-table {
    width: 100%;
    border-collapse: collapse;
    text-align: left;
    font-size: 0.85rem;
}

.data-table thead th {
    padding: 0.85rem 1rem;
    color: var(--text-muted);
    font-weight: 600;
    text-transform: uppercase;
    font-size: 0.72rem;
    letter-spacing: 0.05em;
    border-bottom: 1px solid var(--border-color);
    position: sticky;
    top: 0;
    background: #111726;
    z-index: 5;
    white-space: nowrap;
}

.data-table th.sortable {
    cursor: pointer;
    user-select: none;
    transition: color 0.15s, background-color 0.15s;
}

.data-table th.sortable:hover {
    color: var(--text-primary);
    background: rgba(255, 255, 255, 0.04);
}

.data-table th.sortable .sort-icon {
    display: inline-block;
    width: 10px;
    margin-left: 4px;
    font-size: 0.65rem;
    color: var(--text-muted);
}

.data-table th.sortable.active-sort {
    color: var(--accent-blue);
}

.data-table th.sortable.active-sort .sort-icon {
    color: var(--accent-blue);
    font-weight: 700;
}

.data-table td {
    padding: 0.75rem 1rem;
    border-bottom: 1px solid rgba(255, 255, 255, 0.04);
    color: var(--text-secondary);
    white-space: nowrap;
}

.data-table tr:hover td {
    background: rgba(255, 255, 255, 0.02);
    color: var(--text-primary);
}

.device-name-cell {
    font-weight: 600;
    color: #ffffff !important;
    max-width: 180px;
    overflow: hidden;
    text-overflow: ellipsis;
}

.mono-cell {
    font-family: var(--font-mono);
    font-size: 0.8rem;
}

.cat-badge {
    font-size: 0.72rem;
    padding: 0.15rem 0.5rem;
    border-radius: 4px;
    font-weight: 500;
    text-transform: capitalize;
    display: inline-block;
}

.cat-infrastructure { background: rgba(79, 172, 254, 0.15); color: var(--accent-blue); border: 1px solid rgba(79, 172, 254, 0.25); }
.cat-known { background: rgba(16, 185, 129, 0.15); color: var(--accent-emerald); border: 1px solid rgba(16, 185, 129, 0.25); }
.cat-visitor { background: rgba(245, 158, 11, 0.15); color: var(--accent-amber); border: 1px solid rgba(245, 158, 11, 0.25); }
.cat-unregistered { background: rgba(244, 63, 94, 0.15); color: var(--accent-rose); border: 1px solid rgba(244, 63, 94, 0.25); }

/* Footer */
.footer {
    padding: 1.5rem 2rem;
    text-align: center;
    font-size: 0.75rem;
    color: var(--text-muted);
    border-top: 1px solid var(--border-color);
    background: var(--bg-surface);
}

.loading-placeholder {
    padding: 3rem;
    text-align: center;
    color: var(--text-muted);
    font-style: italic;
}

/* Pagination Bar */
.pagination-bar {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 0.75rem 1rem;
    border-top: 1px solid var(--border-color);
    background: var(--bg-surface-elevated);
    font-size: 0.8rem;
    color: var(--text-secondary);
    flex-wrap: wrap;
    gap: 0.75rem;
}

.page-size-picker {
    display: flex;
    align-items: center;
    gap: 0.5rem;
}

.page-size-picker select {
    background: var(--bg-surface);
    border: 1px solid var(--border-color);
    border-radius: var(--radius-sm);
    color: var(--text-primary);
    padding: 0.25rem 0.5rem;
    font-size: 0.78rem;
    outline: none;
    font-family: var(--font-sans);
}

.page-nav {
    display: flex;
    align-items: center;
    gap: 0.75rem;
}

.btn-page {
    background: var(--bg-surface);
    border: 1px solid var(--border-color);
    border-radius: var(--radius-sm);
    color: var(--text-primary);
    padding: 0.25rem 0.65rem;
    font-size: 0.78rem;
    cursor: pointer;
    transition: all 0.15s;
    font-family: var(--font-sans);
}

.btn-page:hover:not(:disabled) {
    background: rgba(255, 255, 255, 0.08);
    border-color: var(--accent-blue);
}

.btn-page:disabled {
    opacity: 0.4;
    cursor: not-allowed;
}

.page-info {
    font-family: var(--font-mono);
    font-size: 0.75rem;
    color: var(--text-muted);
    white-space: nowrap;
}

/* ==========================================================================
   Responsive Breakpoints
   ========================================================================== */

@media (max-width: 1024px) {
    .content {
        padding: 1.25rem;
    }
    
    .wan-grid {
        grid-template-columns: 1fr;
    }
}

@media (max-width: 768px) {
    .navbar {
        padding: 0.75rem 1rem;
        flex-wrap: wrap;
        gap: 0.75rem;
    }

    .brand-text h1 {
        font-size: 1.1rem;
    }

    .status-indicators {
        gap: 0.4rem;
    }

    .pill-badge {
        font-size: 0.72rem;
        padding: 0.25rem 0.55rem;
    }

    .section-header {
        flex-direction: column;
        align-items: stretch;
        gap: 0.75rem;
    }

    .section-title {
        justify-content: space-between;
    }

    .timeframe-selector {
        width: 100%;
        display: grid;
        grid-template-columns: repeat(5, 1fr);
        text-align: center;
    }

    .time-btn {
        padding: 0.35rem 0.2rem;
        text-align: center;
        font-size: 0.75rem;
    }

    .table-search {
        width: 100%;
    }

    .table-search input {
        width: 100%;
    }

    .wan-card {
        padding: 1rem;
    }

    .wan-meta-grid {
        grid-template-columns: repeat(2, 1fr);
        gap: 0.5rem;
    }

    .metrics-grid {
        grid-template-columns: 1fr;
    }

    .stat-card {
        padding: 1.25rem;
    }

    .stat-value {
        font-size: 1.8rem;
    }

    .pagination-bar {
        flex-direction: column;
        align-items: stretch;
    }

    .page-nav {
        justify-content: space-between;
    }
}

@media (max-width: 480px) {
    .content {
        padding: 0.75rem;
    }

    .wan-card-header {
        flex-direction: column;
        align-items: flex-start;
    }

    .wan-rate-row {
        grid-template-columns: 1fr;
        gap: 0.6rem;
    }

    .rate-box {
        padding: 0.75rem;
    }

    .rate-val {
        font-size: 1.35rem;
    }

    .wan-meta-grid {
        grid-template-columns: 1fr 1fr;
    }

    .footer {
        padding: 1rem;
    }
}
)rawliteral";

inline constexpr const char* APP_JS = R"rawliteral(
/*
 * app.js — netmon Real-Time Client & High-DPI Canvas Bandwidth Grapher
 * Copyright (C) 2026, Charles Chiou
 */

(function() {
    let currentWindowHours = 24;
    let allDevices = [];
    let sortColumn = 'ip';
    let sortDirection = 'asc';
    let currentDevicePage = 1;
    let devicePageSize = 25;
    let activeWanData = null;

    // Cache of chart metadata and offscreen rendering buffers
    const chartState = {};

    // Format helpers
    function formatBytes(bytes) {
        if (!bytes || bytes === 0) return '0 B';
        const k = 1024;
        const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    }

    function formatMbps(mbps) {
        if (mbps === undefined || mbps === null || isNaN(mbps)) return '0.00';
        return parseFloat(mbps).toFixed(2);
    }

    function formatTime(epochSec) {
        if (!epochSec || epochSec === 0) return '--';
        const d = new Date(epochSec * 1000);
        return d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', hour12: false });
    }

    function formatAxisTick(epochSec, windowHours) {
        if (!epochSec) return '';
        const d = new Date(epochSec * 1000);
        if (windowHours <= 24) {
            return d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', hour12: false });
        } else if (windowHours <= 168) {
            const days = ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
            return `${days[d.getDay()]} ${d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', hour12: false })}`;
        } else if (windowHours <= 720) {
            const months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
            return `${months[d.getMonth()]} ${d.getDate()}`;
        } else {
            const months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
            return `${months[d.getMonth()]} '${String(d.getFullYear()).slice(-2)}`;
        }
    }

    function ipToLong(ip) {
        if (!ip) return 0;
        const parts = ip.split('.');
        if (parts.length !== 4) return 0;
        return ((parseInt(parts[0], 10) || 0) << 24) +
               ((parseInt(parts[1], 10) || 0) << 16) +
               ((parseInt(parts[2], 10) || 0) << 8) +
               ((parseInt(parts[3], 10) || 0));
    }

    // Initialize Timeframe Buttons
    const timeButtons = document.querySelectorAll('.time-btn');
    timeButtons.forEach(btn => {
        btn.addEventListener('click', () => {
            timeButtons.forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            currentWindowHours = parseInt(btn.getAttribute('data-hours'), 10) || 24;
            
            // Immediately redraw and fetch history for active interfaces
            if (activeWanData && activeWanData.wan_interfaces) {
                activeWanData.wan_interfaces.forEach(wan => {
                    fetchAndDrawChart(activeWanData.target_ip || '', wan.interface);
                });
            } else {
                fetchAll();
            }
        });
    });

    // Device Table Search
    const searchInput = document.getElementById('device-search');
    if (searchInput) {
        searchInput.addEventListener('input', (e) => {
            currentDevicePage = 1;
            renderDevices(e.target.value.toLowerCase().trim());
        });
    }

    // Device Table Column Sorting
    const sortHeaders = document.querySelectorAll('.data-table th.sortable');
    sortHeaders.forEach(th => {
        th.addEventListener('click', () => {
            const col = th.getAttribute('data-sort');
            if (sortColumn === col) {
                sortDirection = (sortDirection === 'asc') ? 'desc' : 'asc';
            } else {
                sortColumn = col;
                sortDirection = (col === 'last_seen') ? 'desc' : 'asc';
            }
            updateSortHeaders();
            renderDevices(searchInput ? searchInput.value.toLowerCase().trim() : '');
        });
    });

    function updateSortHeaders() {
        sortHeaders.forEach(th => {
            const col = th.getAttribute('data-sort');
            const icon = th.querySelector('.sort-icon');
            if (col === sortColumn) {
                th.classList.add('active-sort');
                th.classList.remove('asc', 'desc');
                th.classList.add(sortDirection);
                if (icon) icon.textContent = sortDirection === 'asc' ? '▲' : '▼';
            } else {
                th.classList.remove('active-sort', 'asc', 'desc');
                if (icon) icon.textContent = '';
            }
        });
    }

    // Device Pagination Controls
    const pageSizeSelect = document.getElementById('page-size-select');
    if (pageSizeSelect) {
        pageSizeSelect.addEventListener('change', (e) => {
            devicePageSize = parseInt(e.target.value, 10) || 0;
            currentDevicePage = 1;
            renderDevices(searchInput ? searchInput.value.toLowerCase().trim() : '');
        });
    }

    const btnPrev = document.getElementById('btn-page-prev');
    if (btnPrev) {
        btnPrev.addEventListener('click', () => {
            if (currentDevicePage > 1) {
                currentDevicePage--;
                renderDevices(searchInput ? searchInput.value.toLowerCase().trim() : '');
            }
        });
    }

    const btnNext = document.getElementById('btn-page-next');
    if (btnNext) {
        btnNext.addEventListener('click', () => {
            currentDevicePage++;
            renderDevices(searchInput ? searchInput.value.toLowerCase().trim() : '');
        });
    }

    // Main fetch loop
    async function fetchAll() {
        await Promise.all([
            fetchStatus(),
            fetchWanTelemetry(),
            fetchTraffic(),
            fetchDevices()
        ]);
    }

    // 1. Daemon Status
    async function fetchStatus() {
        try {
            const res = await fetch('/api/status');
            if (!res.ok) return;
            const data = await res.json();

            const statusEl = document.getElementById('daemon-status');
            if (statusEl) statusEl.textContent = data.status === 'ok' ? 'ONLINE' : 'DEGRADED';

            const uptimeEl = document.getElementById('daemon-uptime');
            if (uptimeEl && data.uptime_seconds) {
                const s = data.uptime_seconds;
                const h = Math.floor(s / 3600);
                const m = Math.floor((s % 3600) / 60);
                uptimeEl.textContent = `${h}h ${m}m`;
            }

            const dbSizeEl = document.getElementById('db-size');
            if (dbSizeEl && data.db_size_bytes !== undefined) {
                dbSizeEl.textContent = formatBytes(data.db_size_bytes);
            }
        } catch (err) {
            console.error('Error fetching status:', err);
        }
    }

    // 2. WAN Telemetry
    async function fetchWanTelemetry() {
        try {
            const res = await fetch('/api/snmp/wan');
            if (!res.ok) return;
            const data = await res.json();
            activeWanData = data;

            const container = document.getElementById('wan-container');
            if (!container) return;

            if (!data.wan_interfaces || data.wan_interfaces.length === 0) {
                container.innerHTML = '<div class="loading-placeholder">No WAN interfaces detected or configured.</div>';
                return;
            }

            // Render WAN cards if structure changed
            data.wan_interfaces.forEach((wan) => {
                let card = document.getElementById(`wan-card-${wan.interface}`);
                if (!card) {
                    card = document.createElement('div');
                    card.id = `wan-card-${wan.interface}`;
                    card.className = 'glass-card wan-card';
                    card.innerHTML = `
                        <div class="wan-card-header">
                            <div class="wan-name">
                                <h3>
                                    <svg viewBox="0 0 24 24" width="18" height="18" fill="none" stroke="currentColor" stroke-width="2">
                                        <rect x="2" y="2" width="20" height="8" rx="2" ry="2"></rect>
                                        <rect x="2" y="14" width="20" height="8" rx="2" ry="2"></rect>
                                        <line x1="6" y1="6" x2="6.01" y2="6"></line>
                                        <line x1="6" y1="18" x2="6.01" y2="18"></line>
                                    </svg>
                                    ${wan.interface.toUpperCase()}
                                </h3>
                                <div class="wan-sub-info">
                                    <span class="wan-alias">${wan.alias || 'Internet Uplink'}</span>
                                    <span class="wan-ip-badge" title="Outward WAN IP">
                                        <svg viewBox="0 0 24 24" width="12" height="12" fill="none" stroke="currentColor" stroke-width="2">
                                            <circle cx="12" cy="12" r="10"></circle>
                                            <line x1="2" y1="12" x2="22" y2="12"></line>
                                            <path d="M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z"></path>
                                        </svg>
                                        <span class="wan-ip-val" id="wan-ip-${wan.interface}">${wan.ip_address || 'No IP'}</span>
                                    </span>
                                </div>
                            </div>
                            <span class="link-status-badge ${wan.status === 'up' ? 'status-up' : 'status-down'}">
                                ${wan.status ? wan.status.toUpperCase() : 'UNKNOWN'} (${wan.speed_mbps || 0}M)
                            </span>
                        </div>

                        <div class="wan-rate-row">
                            <div class="rate-box">
                                <span class="rate-label rate-in-label"><i>&#x2193;</i> Inbound Rate</span>
                                <div class="rate-val rate-val-in" id="rate-in-${wan.interface}">0.00 <span class="unit">Mbps</span></div>
                            </div>
                            <div class="rate-box">
                                <span class="rate-label rate-out-label"><i>&#x2191;</i> Outbound Rate</span>
                                <div class="rate-val rate-val-out" id="rate-out-${wan.interface}">0.00 <span class="unit">Mbps</span></div>
                            </div>
                        </div>

                        <div class="chart-container" id="chart-wrap-${wan.interface}">
                            <canvas class="chart-canvas" id="canvas-${wan.interface}"></canvas>
                            <div class="chart-tooltip" id="tooltip-${wan.interface}"></div>
                        </div>

                        <div class="wan-meta-grid">
                            <div class="meta-item">
                                <span class="meta-title">5m Avg In</span>
                                <span class="meta-val" id="avg-in-${wan.interface}">0.00 M</span>
                            </div>
                            <div class="meta-item">
                                <span class="meta-title">5m Avg Out</span>
                                <span class="meta-val" id="avg-out-${wan.interface}">0.00 M</span>
                            </div>
                            <div class="meta-item">
                                <span class="meta-title">Peak In (24h)</span>
                                <span class="meta-val" id="peak-in-${wan.interface}">0.00 M</span>
                            </div>
                            <div class="meta-item">
                                <span class="meta-title">Total In/Out (24h)</span>
                                <span class="meta-val" id="daily-total-${wan.interface}">0 / 0 GB</span>
                            </div>
                        </div>
                    `;
                    if (container.querySelector('.loading-placeholder')) {
                        container.innerHTML = '';
                    }
                    container.appendChild(card);

                    // Setup ResizeObserver for dynamic DPI redraw
                    setupChartResizeObserver(wan.interface);
                }

                // Update text values
                const rateInEl = document.getElementById(`rate-in-${wan.interface}`);
                if (rateInEl) rateInEl.innerHTML = `${formatMbps(wan.rate_in_mbps)} <span class="unit">Mbps</span>`;

                const rateOutEl = document.getElementById(`rate-out-${wan.interface}`);
                if (rateOutEl) rateOutEl.innerHTML = `${formatMbps(wan.rate_out_mbps)} <span class="unit">Mbps</span>`;

                const avgInEl = document.getElementById(`avg-in-${wan.interface}`);
                if (avgInEl) avgInEl.textContent = `${formatMbps(wan.avg5min_in_mbps)} Mbps`;

                const avgOutEl = document.getElementById(`avg-out-${wan.interface}`);
                if (avgOutEl) avgOutEl.textContent = `${formatMbps(wan.avg5min_out_mbps)} Mbps`;

                const ipValEl = document.getElementById(`wan-ip-${wan.interface}`);
                if (ipValEl) ipValEl.textContent = wan.ip_address || 'No IP';

                const peakInEl = document.getElementById(`peak-in-${wan.interface}`);
                if (peakInEl) {
                    const peakTime = wan.peak_24h_in_timestamp ? ` @ ${formatTime(wan.peak_24h_in_timestamp)}` : '';
                    peakInEl.textContent = `${formatMbps(wan.peak_24h_in_mbps)} M${peakTime}`;
                }

                const dailyTotEl = document.getElementById(`daily-total-${wan.interface}`);
                if (dailyTotEl) {
                    dailyTotEl.textContent = `${(wan.daily_total_gb_in || 0).toFixed(1)} / ${(wan.daily_total_gb_out || 0).toFixed(1)} GB`;
                }

                // Fetch history and draw chart for this interface
                fetchAndDrawChart(data.target_ip || '', wan.interface);
            });
        } catch (err) {
            console.error('Error fetching WAN telemetry:', err);
        }
    }

    function setupChartResizeObserver(ifName) {
        const wrap = document.getElementById(`chart-wrap-${ifName}`);
        if (!wrap || wrap._hasObserver) return;
        wrap._hasObserver = true;

        if (window.ResizeObserver) {
            const ro = new ResizeObserver(() => {
                const state = chartState[ifName];
                if (state && state.points) {
                    const canvas = document.getElementById(`canvas-${ifName}`);
                    if (canvas) {
                        drawSmoothAreaChart(canvas, state.points, state.startTs, state.endTs, ifName);
                    }
                }
            });
            ro.observe(wrap);
        }
    }

    // 3. Historical Chart Drawing
    async function fetchAndDrawChart(targetIp, ifName) {
        try {
            const now = Math.floor(Date.now() / 1000);
            const start = now - (currentWindowHours * 3600);
            const maxPts = currentWindowHours > 720 ? 365 : (currentWindowHours > 168 ? 300 : 240);
            const url = `/api/snmp/history?target_ip=${encodeURIComponent(targetIp)}&iface=${encodeURIComponent(ifName)}&start=${start}&end=${now}&max_points=${maxPts}`;
            const res = await fetch(url);
            if (!res.ok) return;
            const points = await res.json();

            const canvas = document.getElementById(`canvas-${ifName}`);
            if (!canvas) return;

            drawSmoothAreaChart(canvas, points, start, now, ifName);
        } catch (err) {
            console.error(`Error drawing chart for ${ifName}:`, err);
        }
    }

    function drawSmoothAreaChart(canvas, points, startTs, endTs, ifName, activeCrosshairPoint = null) {
        const dpr = window.devicePixelRatio || 1;
        const rect = canvas.getBoundingClientRect();
        if (rect.width === 0 || rect.height === 0) return;

        // Set high-DPI canvas dimensions
        const pixelW = Math.round(rect.width * dpr);
        const pixelH = Math.round(rect.height * dpr);
        if (canvas.width !== pixelW || canvas.height !== pixelH) {
            canvas.width = pixelW;
            canvas.height = pixelH;
        }

        const ctx = canvas.getContext('2d');
        ctx.save();
        ctx.scale(dpr, dpr);

        const w = rect.width;
        const h = rect.height;
        const padTop = 15;
        const padBottom = 22;
        const padLeft = 42;
        const padRight = 14;
        const plotW = Math.max(10, w - padLeft - padRight);
        const plotH = Math.max(10, h - padTop - padBottom);

        ctx.clearRect(0, 0, w, h);

        if (!points || points.length === 0) {
            ctx.fillStyle = '#64748b';
            ctx.font = '12px Inter, sans-serif';
            ctx.textAlign = 'center';
            ctx.fillText('Accumulating telemetry samples...', w / 2, h / 2);
            ctx.restore();
            return;
        }

        // Find max rate
        let maxRate = 1.0;
        points.forEach(p => {
            if (p.in_mbps > maxRate) maxRate = p.in_mbps;
            if (p.out_mbps > maxRate) maxRate = p.out_mbps;
        });
        maxRate *= 1.15; // 15% headroom

        // Draw gridlines
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.06)';
        ctx.lineWidth = 1;
        ctx.fillStyle = '#64748b';
        ctx.font = '10px "JetBrains Mono", monospace';
        ctx.textAlign = 'right';

        for (let i = 0; i <= 3; i++) {
            const yVal = maxRate * (1 - i / 3);
            const yPos = padTop + (plotH * (i / 3));
            ctx.beginPath();
            ctx.moveTo(padLeft, yPos);
            ctx.lineTo(w - padRight, yPos);
            ctx.stroke();
            ctx.fillText(yVal.toFixed(1) + 'M', padLeft - 6, yPos + 3);
        }

        // Time axis: 5 ticks anchored to [startTs, endTs]
        const timeRange = (endTs - startTs) || 1;
        ctx.fillStyle = '#64748b';
        ctx.font = '10px "JetBrains Mono", monospace';

        for (let i = 0; i <= 4; i++) {
            const frac = i / 4.0;
            const tickTs = startTs + Math.round(timeRange * frac);
            const tickX = padLeft + (plotW * frac);

            ctx.beginPath();
            ctx.strokeStyle = 'rgba(255, 255, 255, 0.08)';
            ctx.moveTo(tickX, padTop + plotH);
            ctx.lineTo(tickX, padTop + plotH + 4);
            ctx.stroke();

            const label = (i === 4) ? 'Now' : formatAxisTick(tickTs, currentWindowHours);
            if (i === 0) ctx.textAlign = 'left';
            else if (i === 4) ctx.textAlign = 'right';
            else ctx.textAlign = 'center';

            ctx.fillText(label, tickX, h - 6);
        }

        function getX(ts) {
            return padLeft + Math.min(1, Math.max(0, (ts - startTs) / timeRange)) * plotW;
        }
        function getY(val) {
            return padTop + plotH - Math.min(1, Math.max(0, val / maxRate)) * plotH;
        }

        const firstPtX = getX(points[0].timestamp);
        const lastPtX = getX(points[points.length - 1].timestamp);

        // 1. Draw Inbound (Cyan Gradient Area + Line)
        const gradIn = ctx.createLinearGradient(0, padTop, 0, padTop + plotH);
        gradIn.addColorStop(0, 'rgba(0, 242, 254, 0.35)');
        gradIn.addColorStop(1, 'rgba(0, 242, 254, 0.0)');

        ctx.beginPath();
        ctx.moveTo(firstPtX, getY(0));
        points.forEach(p => ctx.lineTo(getX(p.timestamp), getY(p.in_mbps)));
        ctx.lineTo(lastPtX, getY(0));
        ctx.closePath();
        ctx.fillStyle = gradIn;
        ctx.fill();

        ctx.beginPath();
        points.forEach((p, idx) => {
            const x = getX(p.timestamp);
            const y = getY(p.in_mbps);
            if (idx === 0) ctx.moveTo(x, y);
            else ctx.lineTo(x, y);
        });
        ctx.strokeStyle = '#00f2fe';
        ctx.lineWidth = 1.8;
        ctx.stroke();

        // 2. Draw Outbound (Purple Gradient Area + Line)
        const gradOut = ctx.createLinearGradient(0, padTop, 0, padTop + plotH);
        gradOut.addColorStop(0, 'rgba(139, 92, 246, 0.35)');
        gradOut.addColorStop(1, 'rgba(139, 92, 246, 0.0)');

        ctx.beginPath();
        ctx.moveTo(firstPtX, getY(0));
        points.forEach(p => ctx.lineTo(getX(p.timestamp), getY(p.out_mbps)));
        ctx.lineTo(lastPtX, getY(0));
        ctx.closePath();
        ctx.fillStyle = gradOut;
        ctx.fill();

        ctx.beginPath();
        points.forEach((p, idx) => {
            const x = getX(p.timestamp);
            const y = getY(p.out_mbps);
            if (idx === 0) ctx.moveTo(x, y);
            else ctx.lineTo(x, y);
        });
        ctx.strokeStyle = '#8b5cf6';
        ctx.lineWidth = 1.8;
        ctx.stroke();

        // Draw active crosshair if hover/touch is active
        if (activeCrosshairPoint) {
            const cx = getX(activeCrosshairPoint.timestamp);
            const cyIn = getY(activeCrosshairPoint.in_mbps);
            const cyOut = getY(activeCrosshairPoint.out_mbps);

            // Vertical guideline
            ctx.save();
            ctx.beginPath();
            ctx.setLineDash([3, 3]);
            ctx.strokeStyle = 'rgba(255, 255, 255, 0.45)';
            ctx.lineWidth = 1;
            ctx.moveTo(cx, padTop);
            ctx.lineTo(cx, padTop + plotH);
            ctx.stroke();

            // Highlight dots
            ctx.setLineDash([]);
            ctx.beginPath();
            ctx.arc(cx, cyIn, 4.5, 0, 2 * Math.PI);
            ctx.fillStyle = '#00f2fe';
            ctx.fill();
            ctx.strokeStyle = '#ffffff';
            ctx.lineWidth = 1.5;
            ctx.stroke();

            ctx.beginPath();
            ctx.arc(cx, cyOut, 4.5, 0, 2 * Math.PI);
            ctx.fillStyle = '#8b5cf6';
            ctx.fill();
            ctx.strokeStyle = '#ffffff';
            ctx.lineWidth = 1.5;
            ctx.stroke();
            ctx.restore();
        }

        ctx.restore();

        // Cache chart metadata for interaction
        chartState[ifName] = {
            points,
            startTs,
            endTs,
            timeRange,
            maxRate,
            plotW,
            plotH,
            padLeft,
            padTop,
            padRight,
            padBottom,
            w,
            h,
            getX,
            getY
        };

        setupChartInteractivity(canvas, ifName);
    }

    function setupChartInteractivity(canvas, ifName) {
        if (canvas._hasInteractivity) return;
        canvas._hasInteractivity = true;

        const tooltip = document.getElementById(`tooltip-${ifName}`);

        function handlePointerMove(clientX, clientY) {
            const meta = chartState[ifName];
            if (!meta || !meta.points || meta.points.length === 0) return;

            const rect = canvas.getBoundingClientRect();
            const mouseX = clientX - rect.left;
            const mouseY = clientY - rect.top;

            if (mouseX < meta.padLeft || mouseX > meta.w - meta.padRight ||
                mouseY < meta.padTop || mouseY > meta.padTop + meta.plotH) {
                if (tooltip) tooltip.style.display = 'none';
                drawSmoothAreaChart(canvas, meta.points, meta.startTs, meta.endTs, ifName, null);
                return;
            }

            // Find closest data point
            let closest = meta.points[0];
            let minDiff = Infinity;
            meta.points.forEach(p => {
                const px = meta.getX(p.timestamp);
                const diff = Math.abs(px - mouseX);
                if (diff < minDiff) {
                    minDiff = diff;
                    closest = p;
                }
            });

            if (!closest) return;

            // Redraw with crosshair overlay
            drawSmoothAreaChart(canvas, meta.points, meta.startTs, meta.endTs, ifName, closest);

            // Show floating tooltip
            if (tooltip) {
                const cx = meta.getX(closest.timestamp);
                const cyIn = meta.getY(closest.in_mbps);
                const cyOut = meta.getY(closest.out_mbps);
                const d = new Date(closest.timestamp * 1000);
                const timeStr = d.toLocaleString([], {
                    month: 'short',
                    day: 'numeric',
                    hour: '2-digit',
                    minute: '2-digit',
                    hour12: false
                });

                tooltip.innerHTML = `
                    <div class="tip-time">${timeStr}</div>
                    <div class="tip-row tip-in"><span>&#x2193; In:</span> <strong>${formatMbps(closest.in_mbps)} Mbps</strong></div>
                    <div class="tip-row tip-out"><span>&#x2191; Out:</span> <strong>${formatMbps(closest.out_mbps)} Mbps</strong></div>
                `;
                tooltip.style.display = 'block';
                tooltip.style.left = `${Math.min(Math.max(cx, 80), meta.w - 80)}px`;
                tooltip.style.top = `${Math.max(Math.min(cyIn, cyOut) - 10, 45)}px`;
            }
        }

        function handlePointerLeave() {
            const meta = chartState[ifName];
            if (meta && meta.points) {
                drawSmoothAreaChart(canvas, meta.points, meta.startTs, meta.endTs, ifName, null);
            }
            if (tooltip) tooltip.style.display = 'none';
        }

        canvas.addEventListener('mousemove', (e) => handlePointerMove(e.clientX, e.clientY));
        canvas.addEventListener('mouseleave', handlePointerLeave);

        canvas.addEventListener('touchmove', (e) => {
            if (e.touches && e.touches[0]) {
                handlePointerMove(e.touches[0].clientX, e.touches[0].clientY);
            }
        }, { passive: true });
        canvas.addEventListener('touchend', handlePointerLeave);
    }

    // 4. LAN Traffic
    async function fetchTraffic() {
        try {
            const res = await fetch('/api/traffic');
            if (!res.ok) return;
            const data = await res.json();

            const curRateEl = document.getElementById('lan-current-rate');
            const rateMbps = data.current_rate_mbps !== undefined ? data.current_rate_mbps :
                (data.current_bytes_per_sec ? (data.current_bytes_per_sec * 8 / 1000000) : 0);
            if (curRateEl) {
                curRateEl.innerHTML = `${formatMbps(rateMbps)} <span class="unit">Mbps</span>`;
            }

            const totalPacketsEl = document.getElementById('lan-total-packets');
            if (totalPacketsEl && data.total_packets !== undefined) {
                totalPacketsEl.textContent = data.total_packets.toLocaleString();
            }

            // Protocol breakdown
            const protoBar = document.getElementById('protocol-bar');
            if (protoBar) {
                const p = data.protocols || {};
                const httpsCount = p.https || 0;
                const dnsCount = p.dns || 0;
                const sshCount = p.ssh || 0;
                const arpCount = p.arp || 0;
                const httpCount = p.http || 0;
                const otherCount = p.other || 0;
                const total = httpsCount + dnsCount + sshCount + arpCount + httpCount + otherCount;

                if (total === 0) {
                    protoBar.innerHTML = `<div class="proto-segment proto-segment-idle" title="Monitoring LAN traffic..."></div>`;
                } else {
                    const httpsPct = ((httpsCount / total) * 100).toFixed(1);
                    const dnsPct = ((dnsCount / total) * 100).toFixed(1);
                    const sshPct = ((sshCount / total) * 100).toFixed(1);
                    const arpPct = ((arpCount / total) * 100).toFixed(1);
                    const otherPct = Math.max(0, (100 - parseFloat(httpsPct) - parseFloat(dnsPct) - parseFloat(sshPct) - parseFloat(arpPct))).toFixed(1);

                    protoBar.innerHTML = `
                        <div class="proto-segment proto-https" style="width: ${httpsPct}%" title="HTTPS: ${httpsPct}% (${httpsCount} pkts)"></div>
                        <div class="proto-segment proto-dns" style="width: ${dnsPct}%" title="DNS: ${dnsPct}% (${dnsCount} pkts)"></div>
                        <div class="proto-segment proto-ssh" style="width: ${sshPct}%" title="SSH: ${sshPct}% (${sshCount} pkts)"></div>
                        <div class="proto-segment proto-arp" style="width: ${arpPct}%" title="ARP: ${arpPct}% (${arpCount} pkts)"></div>
                        <div class="proto-segment proto-other" style="width: ${otherPct}%" title="Other: ${otherPct}% (${otherCount} pkts)"></div>
                    `;
                }
            }

            // Top talkers list
            const talkersList = document.getElementById('top-talkers-list');
            if (talkersList) {
                talkersList.innerHTML = '';
                const talkers = data.top_talkers || [];
                if (talkers.length === 0) {
                    talkersList.innerHTML = '<li><span class="talker-empty-hint">No active high-bandwidth flows in last 15m</span></li>';
                } else {
                    talkers.slice(0, 5).forEach((t, idx) => {
                        const li = document.createElement('li');
                        li.innerHTML = `
                            <span><span class="talker-rank">#${idx + 1}</span>${t.name || t.ip}</span>
                            <strong>${formatBytes(t.bytes_total)} (${formatMbps(t.rate_mbps)}M)</strong>
                        `;
                        talkersList.appendChild(li);
                    });
                }
            }
        } catch (err) {
            console.error('Error fetching traffic summary:', err);
        }
    }

    // 5. LAN Devices
    async function fetchDevices() {
        try {
            const res = await fetch('/api/devices');
            if (!res.ok) return;
            const data = await res.json();
            allDevices = data.devices || [];

            const countBadge = document.getElementById('devices-count-badge');
            if (countBadge) countBadge.textContent = `${allDevices.length} Devices`;

            renderDevices(searchInput ? searchInput.value.toLowerCase().trim() : '');
        } catch (err) {
            console.error('Error fetching devices:', err);
        }
    }

    function renderDevices(query) {
        const tbody = document.getElementById('devices-tbody');
        if (!tbody) return;

        let filtered = allDevices.slice();
        if (query) {
            filtered = filtered.filter(d => {
                return (d.name && d.name.toLowerCase().includes(query)) ||
                       (d.ip && d.ip.toLowerCase().includes(query)) ||
                       (d.mac && d.mac.toLowerCase().includes(query)) ||
                       (d.vendor && d.vendor.toLowerCase().includes(query)) ||
                       (d.category && d.category.toLowerCase().includes(query));
            });
        }

        const pageInfoEl = document.getElementById('devices-page-info');
        const prevBtn = document.getElementById('btn-page-prev');
        const nextBtn = document.getElementById('btn-page-next');

        if (filtered.length === 0) {
            tbody.innerHTML = '<tr><td colspan="6" class="text-center">No matching devices found</td></tr>';
            if (pageInfoEl) pageInfoEl.textContent = '0 devices';
            if (prevBtn) prevBtn.disabled = true;
            if (nextBtn) nextBtn.disabled = true;
            return;
        }

        // Sort records
        filtered.sort((a, b) => {
            let cmp = 0;
            if (sortColumn === 'ip') {
                cmp = ipToLong(a.ip) - ipToLong(b.ip);
            } else if (sortColumn === 'last_seen') {
                cmp = (a.last_seen || 0) - (b.last_seen || 0);
            } else {
                const valA = (a[sortColumn] || '').toString().toLowerCase();
                const valB = (b[sortColumn] || '').toString().toLowerCase();
                cmp = valA.localeCompare(valB);
            }
            return sortDirection === 'asc' ? cmp : -cmp;
        });

        // Pagination slicing
        const totalItems = filtered.length;
        let displayList = filtered;

        if (devicePageSize > 0) {
            const totalPages = Math.ceil(totalItems / devicePageSize) || 1;
            if (currentDevicePage > totalPages) currentDevicePage = totalPages;
            if (currentDevicePage < 1) currentDevicePage = 1;

            const startIdx = (currentDevicePage - 1) * devicePageSize;
            displayList = filtered.slice(startIdx, startIdx + devicePageSize);

            if (pageInfoEl) {
                pageInfoEl.textContent = `Page ${currentDevicePage} of ${totalPages} (${totalItems} devices)`;
            }
            if (prevBtn) prevBtn.disabled = (currentDevicePage <= 1);
            if (nextBtn) nextBtn.disabled = (currentDevicePage >= totalPages);
        } else {
            if (pageInfoEl) pageInfoEl.textContent = `All ${totalItems} devices`;
            if (prevBtn) prevBtn.disabled = true;
            if (nextBtn) nextBtn.disabled = true;
        }

        tbody.innerHTML = displayList.map(d => {
            const catClass = `cat-${d.category || 'unregistered'}`;
            return `
                <tr>
                    <td class="device-name-cell" title="${d.name || ''}">${d.name || '<span class="text-muted">Unnamed</span>'}</td>
                    <td class="mono-cell">${d.ip || '-'}</td>
                    <td class="mono-cell">${d.mac || '-'}</td>
                    <td title="${d.vendor || ''}">${d.vendor || '<span class="text-muted">Unknown</span>'}</td>
                    <td><span class="cat-badge ${catClass}">${d.category || 'unregistered'}</span></td>
                    <td class="mono-cell">${formatTime(d.last_seen)}</td>
                </tr>
            `;
        }).join('');
    }

    // Global window resize listener to redraw canvas charts
    window.addEventListener('resize', () => {
        Object.keys(chartState).forEach(ifName => {
            const state = chartState[ifName];
            if (state && state.points) {
                const canvas = document.getElementById(`canvas-${ifName}`);
                if (canvas) {
                    drawSmoothAreaChart(canvas, state.points, state.startTs, state.endTs, ifName);
                }
            }
        });
    });

    // Initial load and periodic polling interval
    fetchAll();
    setInterval(fetchAll, 3000);
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
