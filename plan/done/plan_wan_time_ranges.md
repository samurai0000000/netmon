# Implementation Plan — Multi-Range WAN Histograph, Sortable LAN Devices & ppp11/ppp12 Dynamic WAN Outward IPs

## Overview
1. **WAN Uplink Histograph Time Ranges**: Support multiple selectable time ranges: **Hour**, **Day**, **Week** (7 days), **Month** (30 days), and **Year** (365 days) aligned with MRTG conventions, anchored to "now" on the right edge.
2. **True WAN Devices (`ppp11` & `ppp12`) & Dynamic IP Handling**:
   - Switch primary WAN monitoring from physical ports `eth1`/`eth2` to the true Layer-3 WAN interfaces **`ppp11`** and **`ppp12`**.
   - Poll `ipAdEntIfIndex` (`1.3.6.1.2.1.4.20.1.2`) every 30-second cycle to seamlessly detect and adapt to dynamic IP address rotations.
   - Decouple time-series metric storage (`snmp_samples` keyed by `(target_ip, if_name)`) from IP addresses so historical charts remain unbroken across IP renewals.
   - Display outward IPs (`220.132.136.6` for `ppp11`, `36.224.83.182` for `ppp12`) with live reactivity in the UI.
3. **Discovered LAN Devices Column Sorting**: Make every column in the "Discovered LAN Devices" table sortable (Device Name, IPv4 Address, MAC Address, OUI Vendor, Category, Last Seen) with visual sort indicators and smart numeric IP/timestamp comparisons.
4. **Multi-Tier SQLite Historical Aggregation**: Seamlessly bridge `snmp_samples` and `snmp_hourly_rollups` for long-duration time-series queries.

---

## User Review Required

> [!NOTE]
> - **Primary WAN Uplinks (`ppp11` & `ppp12`)**:
>   - `SnmpAggregator` will default to or prioritize `ppp11` (WAN 1) and `ppp12` (WAN 2) as the true WAN uplink devices.
>   - Physical interfaces `eth1` and `eth2` are preserved for underlying Layer-2 stats if requested.
> - **Dynamic IP Resilience**:
>   - Interface IP addresses are re-queried from SNMP MIB `ipAdEntIfIndex` on each 30s polling cycle.
>   - Any dynamic IP change is detected, logged, and propagated to the web UI instantly without service restart.
>   - SQLite samples remain keyed to persistent interface names (`ppp11`, `ppp12`), ensuring graphs never fracture or reset when an IP changes.
> - **MRTG Timeframe Range Selection**: Buttons configured as `Hour` (1h), `Day` (24h), `Week` (168h), `Month` (720h), and `Year` (8760h), anchored to "Now".
> - **Table Column Sorting**: Clicking any table header in "Discovered LAN Devices" sorts by that column, toggling ascending / descending with sort arrow indicators.

---

## Proposed Changes

### Backend C++ & SNMP Engine

#### [MODIFY] [SnmpAggregator.hxx](file:///home/samurai/work/netmon/include/SnmpAggregator.hxx)
- Add `std::string ipAddress;` to `struct InterfaceState`.

#### [MODIFY] [SnmpAggregator.cxx](file:///home/samurai/work/netmon/src/SnmpAggregator.cxx)
- In `SnmpAggregator::walkInterfaces`:
  - Walk `ipAdEntIfIndex` (`1.3.6.1.2.1.4.20.1.2`) to dynamically populate `indexMap[ifIdx].ipAddress`.
  - Detect dynamic IP changes: compare `scanned.ipAddress` with existing `st.ipAddress` and log when an IP rotation occurs.
  - Prioritize `ppp11` and `ppp12` in WAN uplink interface discovery.
- In `SnmpAggregator::getWanStatus`:
  - Default `wanNames` to `["ppp11", "ppp12"]` if present on the target device, with fallback to `eth1`/`eth2`.
  - Expose `w["ip_address"] = st.ipAddress;` and `w["dynamic_ip"] = true;` in each WAN interface record.

#### [MODIFY] [SnmpDatabase.cxx](file:///home/samurai/work/netmon/src/SnmpDatabase.cxx)
- In `SnmpDatabase::queryHistory`:
  - When downsampling with `bucketSec > 30`, use a `UNION ALL` query between `snmp_samples` and `snmp_hourly_rollups` (for timestamps earlier than the oldest raw sample in `snmp_samples`) to guarantee continuous multi-month and 1-year history.

---

### Web Interface & Chart Engine

#### [MODIFY] [index.html](file:///home/samurai/work/netmon/web/index.html)
- Update `.timeframe-selector` buttons:
  - `<button class="time-btn" data-hours="1">Hour</button>`
  - `<button class="time-btn active" data-hours="24">Day</button>`
  - `<button class="time-btn" data-hours="168">Week</button>`
  - `<button class="time-btn" data-hours="720">Month</button>`
  - `<button class="time-btn" data-hours="8760">Year</button>`
- Update `<thead>` of "Discovered LAN Devices" table with `data-sort` attributes:
  - `Device Name` (`name`)
  - `IPv4 Address` (`ip`)
  - `MAC Address` (`mac`)
  - `OUI Vendor` (`vendor`)
  - `Category` (`category`)
  - `Last Seen` (`last_seen`)

#### [MODIFY] [style.css](file:///home/samurai/work/netmon/web/style.css)
- Add styles for `.wan-ip-badge` (sleek dark pill badge with accent border and mono font for the outward IP).
- Add CSS styling for clickable sortable table headers (`th.sortable`, hover background, sort direction indicator icons).
- Add styles for histograph canvas crosshair line and floating tooltip.

#### [MODIFY] [app.js](file:///home/samurai/work/netmon/web/app.js)
- **WAN Outward IP**:
  - Render outward IP badge in the WAN card header (`<span class="wan-ip-badge">...</span>`), displaying `ppp11` and `ppp12` outward IPs and reacting to dynamic IP updates.
- **WAN Histograph**:
  - Enforce explicit timeline anchoring `[now - window, now]`.
  - Adaptive time axis ticks and formatting (Hour/Day: `HH:mm`, Week: `ddd HH:mm`, Month: `MMM D`, Year: `MMM YYYY`).
  - Interactive hover crosshair and floating tooltip.
- **LAN Devices Sorting**:
  - Maintain `sortColumn` and `sortDirection`.
  - Add click event listeners to table headers.
  - Implement smart numeric IP comparison, epoch timestamp comparison for `last_seen`, and alphanumeric sorting for text columns.

---

## Verification Plan

### Automated Build & Compilation
- Run `make -j$(nproc)` using top-level `Makefile` to ensure clean C++ compilation.

### Visual & API Verification
- Query `/api/snmp/wan` to verify `ppp11` and `ppp12` are returned as WAN interfaces with their outward IPs:
  - `ppp11`: `220.132.136.6`
  - `ppp12`: `36.224.83.182`
- Verify dynamic IP updates in `SnmpAggregator` loop.
- Inspect the web dashboard:
  - Verify WAN card header displays `ppp11` and `ppp12` with outward IP badges.
  - Verify WAN timeframe buttons (`Hour`, `Day`, `Week`, `Month`, `Year`) and canvas rendering anchored to "Now".
  - Test clicking table headers in "Discovered LAN Devices" to verify sorting by IP (numerical), Last Seen, and other columns.
