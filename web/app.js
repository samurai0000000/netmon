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
                    fetchAndDrawChart(activeWanData.target_ip || '192.168.8.1', wan.interface);
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
                fetchAndDrawChart(data.target_ip || '192.168.8.1', wan.interface);
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
