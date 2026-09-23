# Implementation Plan: Robust Parameterized SNMP Telemetry Engine, SQLite Database & Web Dashboard in NetMon

## Goal Description
Implement a production-grade, headless SNMP telemetry engine, persistent **SQLite time-series storage**, and an embedded HTTP web dashboard in `netmon` (following the proven architecture of `aimon` and `meshmon`).

The implementation accomplishes three primary objectives:
1. **Modernizes and Replaces Classic MRTG (`/etc/mrtg.cfg`) without Lossy Compression**:
   - **Continuous Background Polling**: Collects interface octets on a periodic cadence (e.g. 15–30s) so accurate bandwidth rates ($\Delta \text{Octets}/\Delta t$) and rolling averages are always pre-calculated.
   - **Reboot & Rollover Protection**: Tracks `sysUpTime` to detect router reboots, gracefully handling counter resets without false petabyte spikes. Supports both 64-bit High Capacity (`ifXTable`) and 32-bit (`ifTable`) rollover.
   - **Index-Drift Immunity**: Indexes all historical time-series buffers by persistent interface name (`ifName`/`ifDescr`) rather than volatile numerical `ifIndex`.
   - **Robust WAN Classification**: Prioritizes explicit configuration (`wan_interfaces = [ "eth1", "eth2" ]`), falling back to `ifAlias`/`ifName` heuristics and route tables if available.
   - **Smart 3-Tier Interface Filtering**: Suppresses inactive, unplugged, loopback, and virtual container interfaces so reports and dashboards only present meaningful network interfaces.

2. **Persistent SQLite Telemetry Database (`SnmpDatabase`)**:
   - Stores raw, uncompressed 30-second samples into a dedicated SQLite database (`~/.config/netmon/netmon_telemetry.db`) using WAL mode (`PRAGMA journal_mode=WAL;`, `PRAGMA synchronous=NORMAL;`).
   - Preserves exact peak spikes, failover events, and outage timestamps without MRTG's lossy downsampling.
   - **Tiered Retention Policy**: Retains 90 days of raw 30-second data (~50 MB disk footprint) for high-resolution forensic investigation, and rolls older data into compact hourly summaries (`snmp_hourly_rollups`, <1 MB/year) for multi-year capacity planning and 95th-percentile queries.

3. **Embedded Web Server & Live Modern Dashboard (like `aimon` and `meshmon`)**:
   - Embeds `cpp-httplib` (git submodule in `third_party/cpp-httplib`) running on port `3884` (default).
   - Serves an interactive, rich dark-mode web dashboard featuring live WAN throughput meters, zoomable historical bandwidth charts powered by the SQLite database, LAN traffic breakdown, and device inventory.
   - Dual-mode asset serving: loads live frontend assets from `web/` on disk when present, with embedded compiled C++ string fallbacks (`WebAssets.hxx`) for standalone binary deployment.
   - Exposes RESTful JSON endpoints (`/api/status`, `/api/snmp/wan`, `/api/snmp/devices`, `/api/snmp/history`, `/api/traffic`, `/api/devices`).

All targets, community strings, protocol versions, ports, interface lists, database paths, and web server ports are **100% parameterized** via `netmon.cfg` and injectable via 12-factor environment variables (`NETMON_SNMP_*`, `NETMON_DB_*`, `NETMON_WEB_*`).

---

## 1. Resolved Architectural Challenges

| Challenge / Requirement | Failure Mode in Classic MRTG / Naive Design | Production Fix in NetMon |
|---|---|---|
| **Downsampling / Lossy Compression** | MRTG averages past samples into flatlines; short spikes & outage minutes are erased | Store uncompressed 30s samples in SQLite (`snmp_samples`) for 90 days; query exact historical spikes anytime |
| **Rate Computation** | On-demand RPC cannot compute B/s without sleeping | Dedicated background polling thread (`SnmpAggregator::pollLoop`) pre-calculates rates into memory & database |
| **Router Reboot** | Counter reset from $4 \times 10^9 \to 100$ produces false petabyte/s spike | Track `sysUpTime`; discard delta on reboot and re-baseline smoothly |
| **Counter Rollover** | 32-bit counters wrap in 34s at 1 Gbps | Auto-detect `ifXTable` 64-bit HC counters; fallback to 32-bit with modulo wrap math |
| **Index Drift** | Router reboots renumber `ifIndex` (e.g. `eth1` moves from 2 to 4) | Key internal sample buffers & DB records by persistent string `ifName`, never numerical `ifIndex` |
| **Restricted SNMP ACL** | Routers restrict `ipRouteTable` (.1.3.6.1.2.1.4); queries fail with auth error | WAN detection uses explicit `wan_interfaces` config first, then `ifAlias`/name tokens |
| **SSD Wear / I/O Overhead** | High-frequency disk writes can wear flash storage | SQLite WAL mode + `synchronous=NORMAL` + batched transaction per poll cycle (<4 KB write / 30s) |
| **Target Security** | Arbitrary IP in MCP tool allows SSRF / external SNMP reflection | Validate target IP through `SecurityCheckpoint` (RFC1918 / configured targets only) |

---

## 2. SQLite Database Schema & Retention Architecture (`SnmpDatabase`)

### Database Connection & Pragma Setup
- Database path: `~/.config/netmon/netmon_telemetry.db` (configurable via `database_file`).
- Connection flags:
  ```sql
  PRAGMA journal_mode = WAL;
  PRAGMA synchronous = NORMAL;
  PRAGMA foreign_keys = ON;
  ```

### Table 1: Raw Samples (`snmp_samples`)
Stores exact 30-second polled rates and counters:
```sql
CREATE TABLE IF NOT EXISTS snmp_samples (
    timestamp       INTEGER NOT NULL,   -- UNIX epoch (seconds)
    target_ip       TEXT NOT NULL,      -- e.g. "192.168.8.1"
    if_name         TEXT NOT NULL,      -- e.g. "eth1"
    in_bytes_sec    REAL NOT NULL,      -- Computed bandwidth rate (B/s)
    out_bytes_sec   REAL NOT NULL,      -- Computed bandwidth rate (B/s)
    in_hc_octets    INTEGER NOT NULL,   -- Raw cumulative 64-bit counter
    out_hc_octets   INTEGER NOT NULL,   -- Raw cumulative 64-bit counter
    oper_status     INTEGER NOT NULL,   -- 1 = up, 2 = down
    in_errors       INTEGER DEFAULT 0,
    out_errors      INTEGER DEFAULT 0
);
CREATE INDEX IF NOT EXISTS idx_snmp_samples_lookup 
ON snmp_samples(target_ip, if_name, timestamp);
```

### Table 2: Long-Term Hourly Rollups (`snmp_hourly_rollups`)
For historical multi-year capacity planning and billing verification:
```sql
CREATE TABLE IF NOT EXISTS snmp_hourly_rollups (
    timestamp       INTEGER NOT NULL,   -- Start of hour epoch
    target_ip       TEXT NOT NULL,
    if_name         TEXT NOT NULL,
    avg_in_bytes    REAL NOT NULL,      -- Average B/s across hour
    avg_out_bytes   REAL NOT NULL,      -- Average B/s across hour
    max_in_bytes    REAL NOT NULL,      -- Peak spike B/s in hour
    max_out_bytes   REAL NOT NULL,      -- Peak spike B/s in hour
    total_gb_in     REAL NOT NULL,      -- Total gigabytes in
    total_gb_out    REAL NOT NULL,      -- Total gigabytes out
    PRIMARY KEY (target_ip, if_name, timestamp)
);
```

### Automated Pruning & Rollup Maintenance
- Runs daily in a lightweight background task:
  1. Aggregates records older than 90 days into `snmp_hourly_rollups`.
  2. Deletes raw records older than 90 days from `snmp_samples`.
  3. Total steady-state database size: **~50 MB** for 2 WAN ports across 90 days of raw 30-second data.

---

## 3. Zero-Hardcoding Design Principles

| Parameter | Configuration (`netmon.cfg`) | Environment Variable | Fallback / Default |
|---|---|---|---|
| **Target IP** | `targets = ( { ip = "..."; } )` | `NETMON_SNMP_TARGET` | Required / Configured target |
| **Community** | `community = "..."` | `NETMON_SNMP_COMMUNITY` | Configured default (`"public"`) |
| **Protocol Version** | `version = "2c"` | `NETMON_SNMP_VERSION` | `"2c"` |
| **UDP Port** | `port = 161` | `NETMON_SNMP_PORT` | `161` |
| **Monitored Interfaces** | `interfaces = [ "eth1", ... ]` | `NETMON_SNMP_INTERFACES` | Auto-filtered |
| **WAN Interfaces** | `wan_interfaces = [ "eth1", ... ]` | `NETMON_SNMP_WAN_INTERFACES` | Auto-detect via `ifAlias` / route |
| **Poll Interval** | `poll_interval_sec = 30` | `NETMON_SNMP_INTERVAL` | `30` seconds |
| **Database File** | `database_file = "..."` | `NETMON_DB_PATH` | `~/.config/netmon/netmon_telemetry.db` |
| **Retention Days (Raw)**| `raw_retention_days = 90` | `NETMON_DB_RETENTION_DAYS` | `90` days |
| **Web Server Port** | `web.port = 3884` | `NETMON_WEB_PORT` | `3884` |
| **Web Server Enabled** | `web.enabled = true` | `NETMON_WEB_ENABLED` | `true` |

---

## 4. Exported AI MCP Toolset (via `aimon`)

`netmon` will register a clean **4-tool SNMP telemetry suite** with `aimon`:

### Tool 1: `snmp_get_device_metrics`
- **Purpose**: Device health, system information, uptime, and filtered interface inventory.
- **Parameters**: `target_ip` (required), `filter` (optional: `"monitored"`, `"active"`, `"all"`), `community` (optional).
- **Returns**: System description (`sysDescr`), uptime (`sysUpTime` formatted as days/hours/mins), device name (`sysName`), and a structured table of filtered interfaces with line speed, administrative/operational status, MTU, 64-bit HC octet counters, and current transfer rates.

### Tool 2: `snmp_get_wan_status`
- **Purpose**: Dedicated WAN internet uplink bandwidth rates, 24-hour trends, and throughput health (MRTG replacement).
- **Parameters**: `target_ip` (optional), `wan_interfaces` (optional), `history_hours` (optional, default 24).
- **Returns**:
  - Current real-time throughput (B/s, Kbps, Mbps)
  - 5-min rolling average (`avin`/`avout`)
  - 24-hour peak rate (`maxin`/`maxout`) with exact timestamp
  - Total data transferred in window (GB in / GB out from SQLite)
  - Link speed and operational state for each active WAN uplink

### Tool 3: `snmp_get_interface_counters`
- **Purpose**: In-depth port diagnostics and error isolation on a specific interface.
- **Parameters**: `target_ip` (required), `interface_name` (required), `community` (optional).
- **Returns**: Detailed unicast vs. broadcast vs. multicast packet ratios, in/out discards, and CRC/frame error rates.

### Tool 4: `snmp_query_oid`
- **Purpose**: Extensible query tool for arbitrary standard MIB or vendor enterprise OIDs.
- **Parameters**: `target_ip` (required), `oid` (required), `community` (optional).
- **Returns**: ASN.1 data type, raw numeric/string value, and formatted human-readable output.

---

## 5. Embedded Web Dashboard & REST API Architecture

### REST API Endpoints
- **`GET /api/status`**: Overall NetMon status, daemon uptime, PCAP sniffer stats, active gateway client state, SQLite database size.
- **`GET /api/snmp/wan`**: Live WAN bandwidth meters, 5-min averages, 24-hour peak rates, total GB in/out.
- **`GET /api/snmp/devices`**: Filtered hardware inventory, system uptime, and interface counters.
- **`GET /api/snmp/history?iface=<name>&start=<ts>&end=<ts>&resolution=raw|hourly`**: Direct SQLite time-series points for zoomable interactive charting.
- **`GET /api/traffic`**: LAN throughput summary, protocol breakdown, top talkers.
- **`GET /api/devices`**: Discovered LAN device registry with OUI vendors and categories.

### Modern Web Dashboard (`web/`)
- **Rich Aesthetics**: Premium dark mode, glassmorphism cards, responsive flex layout.
- **Interactive Bandwidth Charts**: High-performance SVG/Canvas charting displaying live Mbps in/out and historical area curves with pan/zoom (powered by SQLite).
- **Status Indicators**: Real-time connection indicators, WAN link speed badges, and live counter tickers.

---

## 6. Proposed Code Changes

### Component 1: Build System & Dependencies
#### [MODIFY] [CMakeLists.txt](file:///home/samurai/work/netmon/CMakeLists.txt)
- Add `pkg_check_modules(NETSNMP REQUIRED netsnmp)`.
- Add `pkg_check_modules(SQLITE3 REQUIRED sqlite3)`.
- Add `third_party/cpp-httplib` include path.
- Add `src/SnmpDatabase.cxx` and `src/WebServer.cxx` to sources.
- Link `${NETSNMP_LIBRARIES}`, `${SQLITE3_LIBRARIES}`, and `Threads::Threads`.

---

### Component 2: Configuration Subsystem
#### [MODIFY] [include/Config.hxx](file:///home/samurai/work/netmon/include/Config.hxx)
#### [MODIFY] [src/Config.cxx](file:///home/samurai/work/netmon/src/Config.cxx)
- Add `SnmpTargetConfig`, `WebConfig`, `databaseFile`, and `rawRetentionDays`.
- Support 12-factor environment variables (`NETMON_SNMP_*`, `NETMON_DB_*`, `NETMON_WEB_*`).
- Enforce strict `0600` POSIX permissions on configuration and database files.

---

### Component 3: SQLite Telemetry Database Subsystem
#### [NEW] [include/SnmpDatabase.hxx](file:///home/samurai/work/netmon/include/SnmpDatabase.hxx)
#### [NEW] [src/SnmpDatabase.cxx](file:///home/samurai/work/netmon/src/SnmpDatabase.cxx)
- Manage SQLite connection lifecycle (`sqlite3_open`, `sqlite3_close`).
- Set WAL mode and normal synchronous pragma.
- Prepare reusable statements for `insertSample`, `queryHistory`, `queryPeak`, `queryTotals`, `pruneOldSamples`.
- Thread-safe access via `std::mutex`.

---

### Component 4: SNMP Aggregator Engine
#### [MODIFY] [include/SnmpAggregator.hxx](file:///home/samurai/work/netmon/include/SnmpAggregator.hxx)
#### [MODIFY] [src/SnmpAggregator.cxx](file:///home/samurai/work/netmon/src/SnmpAggregator.cxx)
- Dedicated background polling thread (`pollLoop`).
- Net-SNMP session management (`init_snmp("netmon")`, `snmp_open`, `snmp_synch_response`, `snmp_close`).
- Compute rates, detect reboots via `sysUpTime`, handle 64-bit HC counters and 32-bit rollover.
- Insert computed rates into `SnmpDatabase` after each poll cycle.
- In-memory ring buffer for instant 0 ms dashboard reads.
- 3-tier interface filtering (Tier 1 allowlist, Tier 2 auto-filter, Tier 3 MCP filter).

---

### Component 5: Embedded Web Server & UI Assets
#### [NEW] [include/WebServer.hxx](file:///home/samurai/work/netmon/include/WebServer.hxx)
#### [NEW] [include/WebAssets.hxx](file:///home/samurai/work/netmon/include/WebAssets.hxx)
#### [NEW] [src/WebServer.cxx](file:///home/samurai/work/netmon/src/WebServer.cxx)
#### [NEW] `web/index.html`, `web/style.css`, `web/app.js`
- Create `WebServer` using `cpp-httplib` with async background listening thread on port `3884`.
- Setup REST API routes querying `SnmpAggregator` and `SnmpDatabase`.
- Dual-mode asset serving (disk `web/` with compiled `WebAssets.hxx` fallback).

---

### Component 6: Daemon Startup & Main Integration
#### [MODIFY] [src/Main.cxx](file:///home/samurai/work/netmon/src/Main.cxx)
- Initialize and start `SnmpDatabase`, `SnmpAggregator`, and `WebServer` on boot.
- Cleanly stop subsystems on shutdown signal (`SIGINT`, `SIGTERM`).
- Add command-line options (`-w <port>`, `--db <path>`).

---

### Component 7: AI MCP Gateway Integration (`aimon`)
#### [MODIFY] [src/AimonGatewayClient.cxx](file:///home/samurai/work/netmon/src/AimonGatewayClient.cxx)
- Register the 4 tools in `gateway/register`.
- Dispatch RPC calls returning real-time metrics and SQLite historical aggregations with 0 ms query latency.

---

## 7. Verification Plan

### 1. Build Verification
```bash
make clean && make -j$(nproc)
```
- Verify clean compilation with zero warnings on `builder` / `rhino`.

### 2. SQLite Database Verification
- Inspect generated database via `sqlite3 ~/.config/netmon/netmon_telemetry.db`:
  - Verify tables exist (`snmp_samples`, `snmp_hourly_rollups`).
  - Verify WAL mode is active (`PRAGMA journal_mode;` returns `wal`).
  - Verify records are inserted every 30 seconds with correct rates and counters.

### 3. Live Telemetry & Filter Verification (via `aimon` MCP tools)
- Query target router (`192.168.8.1`):
  - `snmp_get_device_metrics`: Verify system uptime, 64-bit counters, and suppression of inactive interfaces.
  - `snmp_get_wan_status`: Verify real-time Mbps throughput, 5-minute rolling average, 24-hour peak rate, and link state for active WAN ports (`eth1`, `eth2`).
  - `snmp_get_interface_counters`: Verify error stats on specific ports.
  - `snmp_query_oid`: Test arbitrary OID query (`sysDescr.0`).

### 4. Web Dashboard Verification
- Launch daemon with webserver enabled on port `3884`.
- Verify REST endpoints:
  ```bash
  curl -s http://localhost:3884/api/status | jq .
  curl -s http://localhost:3884/api/snmp/wan | jq .
  curl -s "http://localhost:3884/api/snmp/history?iface=eth1&hours=24" | jq .
  ```
- Verify interactive web dashboard loads in browser at `http://localhost:3884/` with live charts and real-time bandwidth metrics.
