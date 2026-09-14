/*
 * app.js — NetMon Real-Time Client & Canvas Bandwidth Grapher
 * Copyright (C) 2026, Charles Chiou
 */

(function() {
    let currentWindowHours = 24;
    let allDevices = [];
    const canvasCharts = {}; // Map ifName -> canvas context and historical cache

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
        return d.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' });
    }

    // Initialize Timeframe Buttons
    const timeButtons = document.querySelectorAll('.time-btn');
    timeButtons.forEach(btn => {
        btn.addEventListener('click', () => {
            timeButtons.forEach(b => b.classList.remove('active'));
            btn.classList.add('active');
            currentWindowHours = parseInt(btn.getAttribute('data-hours'), 10) || 24;
            fetchAll();
        });
    });

    // Device Table Search
    const searchInput = document.getElementById('device-search');
    if (searchInput) {
        searchInput.addEventListener('input', (e) => {
            renderDevices(e.target.value.toLowerCase().trim());
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

            const container = document.getElementById('wan-container');
            if (!container) return;

            if (!data.wan_interfaces || data.wan_interfaces.length === 0) {
                container.innerHTML = '<div class="loading-placeholder">No WAN interfaces detected or configured.</div>';
                return;
            }

            // Render WAN cards if structure changed
            data.wan_interfaces.forEach((wan, idx) => {
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
                                <span class="wan-alias">${wan.alias || 'Internet Uplink'}</span>
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

                        <div class="chart-container">
                            <canvas class="chart-canvas" id="canvas-${wan.interface}"></canvas>
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
                    // Remove initial placeholder if first card
                    if (container.querySelector('.loading-placeholder')) {
                        container.innerHTML = '';
                    }
                    container.appendChild(card);
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
                fetchAndDrawChart(data.target_ip || '192.168.8.1', wan.interface);
            });
        } catch (err) {
            console.error('Error fetching WAN telemetry:', err);
        }
    }

    // 3. Historical Chart Drawing
    async function fetchAndDrawChart(targetIp, ifName) {
        try {
            const now = Math.floor(Date.now() / 1000);
            const start = now - (currentWindowHours * 3600);
            const url = `/api/snmp/history?target_ip=${encodeURIComponent(targetIp)}&iface=${encodeURIComponent(ifName)}&start=${start}&end=${now}&max_points=240`;
            const res = await fetch(url);
            if (!res.ok) return;
            const points = await res.json();

            const canvas = document.getElementById(`canvas-${ifName}`);
            if (!canvas) return;

            drawSmoothAreaChart(canvas, points);
        } catch (err) {
            console.error(`Error drawing chart for ${ifName}:`, err);
        }
    }

    function drawSmoothAreaChart(canvas, points) {
        const dpr = window.devicePixelRatio || 1;
        const rect = canvas.getBoundingClientRect();
        if (rect.width === 0 || rect.height === 0) return;

        canvas.width = rect.width * dpr;
        canvas.height = rect.height * dpr;
        const ctx = canvas.getContext('2d');
        ctx.scale(dpr, dpr);

        const w = rect.width;
        const h = rect.height;
        const padTop = 15;
        const padBottom = 20;
        const padLeft = 40;
        const padRight = 10;
        const plotW = w - padLeft - padRight;
        const plotH = h - padTop - padBottom;

        ctx.clearRect(0, 0, w, h);

        if (!points || points.length === 0) {
            ctx.fillStyle = '#64748b';
            ctx.font = '12px Inter';
            ctx.textAlign = 'center';
            ctx.fillText('Accumulating telemetry samples...', w / 2, h / 2);
            return;
        }

        // Find max rate
        let maxRate = 1.0;
        points.forEach(p => {
            if (p.in_mbps > maxRate) maxRate = p.in_mbps;
            if (p.out_mbps > maxRate) maxRate = p.out_mbps;
        });
        maxRate *= 1.15; // 15% head room

        // Draw gridlines
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.05)';
        ctx.lineWidth = 1;
        ctx.fillStyle = '#64748b';
        ctx.font = '10px JetBrains Mono';
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

        // Time axis
        ctx.textAlign = 'center';
        const startTs = points[0].timestamp;
        const endTs = points[points.length - 1].timestamp;
        const timeRange = (endTs - startTs) || 1;

        ctx.fillText(formatTime(startTs), padLeft + 20, h - 5);
        ctx.fillText(formatTime(endTs), w - padRight - 20, h - 5);

        function getX(ts) {
            return padLeft + ((ts - startTs) / timeRange) * plotW;
        }
        function getY(val) {
            return padTop + plotH - (val / maxRate) * plotH;
        }

        // 1. Draw Inbound (Cyan Gradient Area + Line)
        const gradIn = ctx.createLinearGradient(0, padTop, 0, padTop + plotH);
        gradIn.addColorStop(0, 'rgba(0, 242, 254, 0.35)');
        gradIn.addColorStop(1, 'rgba(0, 242, 254, 0.0)');

        ctx.beginPath();
        ctx.moveTo(getX(points[0].timestamp), getY(0));
        points.forEach(p => ctx.lineTo(getX(p.timestamp), getY(p.in_mbps)));
        ctx.lineTo(getX(points[points.length - 1].timestamp), getY(0));
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
        ctx.moveTo(getX(points[0].timestamp), getY(0));
        points.forEach(p => ctx.lineTo(getX(p.timestamp), getY(p.out_mbps)));
        ctx.lineTo(getX(points[points.length - 1].timestamp), getY(0));
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
            if (totalPacketsEl && data.total_packets) {
                totalPacketsEl.textContent = data.total_packets.toLocaleString();
            }

            // Protocol breakdown
            const protoBar = document.getElementById('protocol-bar');
            if (protoBar && data.protocols) {
                const p = data.protocols;
                const total = (p.https || 0) + (p.ssh || 0) + (p.dns || 0) + (p.arp || 0) + (p.other || 0) + (p.http || 0) || 1;
                const httpsPct = (((p.https || 0) / total) * 100).toFixed(1);
                const sshPct = (((p.ssh || 0) / total) * 100).toFixed(1);
                const dnsPct = (((p.dns || 0) / total) * 100).toFixed(1);
                const otherPct = Math.max(0, (100 - parseFloat(httpsPct) - parseFloat(sshPct) - parseFloat(dnsPct))).toFixed(1);

                protoBar.innerHTML = `
                    <div class="proto-segment proto-https" style="width: ${httpsPct}%" title="HTTPS: ${httpsPct}%"></div>
                    <div class="proto-segment proto-ssh" style="width: ${sshPct}%" title="SSH: ${sshPct}%"></div>
                    <div class="proto-segment proto-dns" style="width: ${dnsPct}%" title="DNS: ${dnsPct}%"></div>
                    <div class="proto-segment proto-other" style="width: ${otherPct}%" title="Other: ${otherPct}%"></div>
                `;
            }

            // Top talkers list
            const talkersList = document.getElementById('top-talkers-list');
            if (talkersList && data.top_talkers) {
                talkersList.innerHTML = '';
                if (data.top_talkers.length === 0) {
                    talkersList.innerHTML = '<li><span>No active flows in window</span></li>';
                } else {
                    data.top_talkers.slice(0, 5).forEach(t => {
                        const li = document.createElement('li');
                        li.innerHTML = `
                            <span>${t.name || t.ip}</span>
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

        let filtered = allDevices;
        if (query) {
            filtered = allDevices.filter(d => {
                return (d.name && d.name.toLowerCase().includes(query)) ||
                       (d.ip && d.ip.toLowerCase().includes(query)) ||
                       (d.mac && d.mac.toLowerCase().includes(query)) ||
                       (d.vendor && d.vendor.toLowerCase().includes(query)) ||
                       (d.category && d.category.toLowerCase().includes(query));
            });
        }

        if (filtered.length === 0) {
            tbody.innerHTML = '<tr><td colspan="6" class="text-center">No matching devices found</td></tr>';
            return;
        }

        tbody.innerHTML = filtered.map(d => {
            const catClass = `cat-${d.category || 'unregistered'}`;
            return `
                <tr>
                    <td class="device-name-cell">${d.name || '<span class="text-muted">Unnamed</span>'}</td>
                    <td class="mono-cell">${d.ip || '-'}</td>
                    <td class="mono-cell">${d.mac || '-'}</td>
                    <td>${d.vendor || '<span class="text-muted">Unknown</span>'}</td>
                    <td><span class="cat-badge ${catClass}">${d.category || 'unregistered'}</span></td>
                    <td class="mono-cell">${formatTime(d.last_seen)}</td>
                </tr>
            `;
        }).join('');
    }

    // Initial load and periodic interval
    fetchAll();
    setInterval(fetchAll, 3000);
})();
