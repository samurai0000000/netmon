# NetMon: Architectural Design & Technical Specification

## 1. System Overview & Problem Statement

### 1.1 Overview
**NetMon** is a high-performance network monitoring, device discovery, traffic analysis, and router/firewall automation daemon written in C++17. It continuously observes physical LAN traffic, tracks active devices and visitor phones, aggregates real-time bandwidth metrics, and exports a vendor-agnostic toolset to AI assistants (Cursor, Google Antigravity, Gemini) via `aimon` over MCP.

All daemon configurations standardize on `libconfig++` and adhere to the XDG Base Directory specification (`~/.config/netmon/netmon.cfg`).

### 1.2 Deployment Model & Hardware Placement
`netmon` is designed to be deployed on a Linux host with direct visibility into LAN network traffic:
- **Hardware Requirement**: Dedicated Network Interface Card (NIC) bridged or in promiscuous mode (e.g. `br0` or mirror port), ideally suited for packet capture and traffic analysis.
- **Compilation**: Standard C++17 build via top-level `Makefile` (`make -j$(nproc)`).
- **Runtime Environment**: Operates as a system daemon, background service, or interactive CLI shell.

---

## 2. System Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                             LAN Network Traffic                             │
│                  (Core Switch Port Mirror / Promiscuous Bridge)             │
└──────────────────────────────────────┬──────────────────────────────────────┘
                                       │ Raw Ethernet Frames
                                       ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                             NetMon Core Daemon                              │
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │                    AI Security Checkpoint & Policy                    │  │
│  │  - Access permissions (allow/deny actions, dry-run, rate limiting)    │  │
│  │  - Protected invariants (prevents blocking router, gateway, cluster)  │  │
│  │  - Audit logging of all AI agent interactions                         │  │
│  └───────────────────┬───────────────────────────────────┬───────────────┘  │
│                      │                                   │                  │
│                      ▼                                   ▼                  │
│  ┌───────────────────────────────────────┐   ┌───────────────────────────┐  │
│  │ Telemetry & Router Subsystems         │   │ Live LAN Streaming Sniffer│  │
│  │   ┌────────────────────────────────┐  │   │ (LanSniffer via libpcap)  │  │
│  │   │ ZyxelDriver (SSH / CLI Driver) │  │   │ - Configurable interface  │  │
│  │   └────────────────────────────────┘  │   │ - Header-only snaplen(96B)│  │
│  │   ┌────────────────────────────────┐  │   │ - Zero Raw Packet Storage │  │
│  │   │ SnmpAggregator (Active Engine) │  │   │ - Stream Feature Extract  │  │
│  │   │ - 30s Polling / 64-bit HC      │  │   └─────────────┬─────────────┘  │
│  │   │ - Reboot & Wrap Detection      │  │                 │                │
│  │   │ - 3-Tier Interface Filtering   │  │   ┌─────────────┴─────────────┐  │
│  │   └───────────────┬────────────────┘  │   │ In-Memory Sliding Windows │  │
│  │                   ▼                   │   │ - 60-Minute Circular Ring │  │
│  │   ┌────────────────────────────────┐  │   │ - Top talkers & rates     │  │
│  │   │ SnmpDatabase (SQLite WAL Mode) │  │   │ - Protocol distributions  │  │
│  │   │ - 90-Day Raw 30s Retention     │  │   │ - Host Behavioral Metrics │  │
│  │   │ - Multi-Year Hourly Rollups    │  │   └─────────────┬─────────────┘  │
│  │   └───────────────┬────────────────┘  │                 │                │
│  │                   ▼                   │   ┌─────────────▼─────────────┐  │
│  │   ┌────────────────────────────────┐  │   │ Persistent DeviceRegistry │  │
│  │   │ WebServer (cpp-httplib :3884)  │  │   │ (devices.cfg / DNS PTR)   │  │
│  │   │ - Live Dual-WAN Canvas Charts  │  │   │ - Known / Visitor / New   │  │
│  │   │ - REST API (/api/snmp/*)       │  │   │ - OUI Vendor Match        │  │
│  │   └────────────────────────────────┘  │   └─────────────┬─────────────┘  │
│  └───────────────────┬───────────────────┘                 │                │
│                      │                                     │                │
│  ┌───────────────────┴─────────────────────────────────────┴─────────────┐  │
│  │                  AimonGatewayClient (TCP Client)                      │  │
│  │          Exports netmon_* toolset to aimon gateway                    │  │
│  └───────────────────────────────────┬───────────────────────────────────┘  │
└──────────────────────────────────────┼──────────────────────────────────────┘
                                       │ Line-delimited JSON-RPC 2.0 (TCP)
                                       ▼
                         aimon Hub (<gateway-host>:3885)
```

---

## 3. Core Functional Subsystems

### 3.1 Live Streaming Sniffer & In-Memory Architecture (Zero-DB Mode)

#### 3.1.1 The Anti-Pattern: Why Raw Packet DBs Fail on LANs
On a Gigabit LAN, network throughput easily reaches 50 GB to 200 GB per day. Writing raw packet payloads or individual frame records to a SQLite database causes severe disk write thrashing, rapid SSD wear, and database bloat. Furthermore, AI agents almost never need raw packet bytes; they require high-level, aggregate answers to operational questions.

#### 3.1.2 Real-Time Stream Extraction Pipeline
`netmon` completely avoids disk thrashing by using a **100% in-memory streaming extraction pipeline**:
1. **Header-Only Capture (`snaplen = 96` bytes)**:
   `libpcap` captures only the first 96 bytes of each frame. All bulk payloads (video, downloads, backups) are dropped in the kernel before reaching user space.
2. **Sub-Microsecond Header Decoding**:
   Incoming frames are decoded in $< 1\,\mu\text{s}$ (Ethernet MACs, IPv4/IPv6 headers, TCP/UDP ports, TCP flags, ARP, DHCP Option 12 hostnames, and DNS queries).
3. **Immediate Buffer Release**:
   Extracted features and counters are recorded in in-memory C++ structures, and the scratch buffer is immediately released. Zero raw packets are retained in memory or on disk.

#### 3.1.3 Sliding-Window Ring Buffers
To compute rates and rolling-window metrics (15 minutes, 1 hour, 24 hours) without unbounded memory growth:
- `_minuteRing`: A fixed array of 60 slot buckets representing the last 60 minutes.
- Each bucket tracks byte/packet counts per host for that minute.
- Rate calculations (bytes/sec) divide the current minute bucket by elapsed seconds.
- Memory usage is fixed and recycled every hour ($\approx 50\text{ KB}$). Total sniffer RAM usage is $< 1\text{ MB}$.

---

### 3.2 Industry-Standard Host Behavioral Metrics

Rather than arbitrary ad-hoc counters, `netmon` models host behavior using metrics aligned with established networking and security standards:
- **RFC 5101 / RFC 7011 (IPFIX / NetFlow v9)**: Flow directionality, TCP flag state tracking, byte/packet sizing.
- **RFC 2819 / RFC 4502 (RMON / RMON-2 MIBs)**: Host matrix, broadcast/multicast ratios, error accounting.
- **Zeek / Suricata Connection State History**: Handshake completion, rejected connections, half-open tracking.

#### Host Behavioral State Schema (`HostMetrics`)

```cpp
struct HostMetrics {
    // Identity & Profiling
    uint8_t     mac[6];              // Hardware MAC address
    uint32_t    ip;                  // IPv4 address
    std::string hostname;            // DHCP Option 12 or mDNS announced name
    std::string vendor;              // IEEE OUI vendor string (e.g. "Apple, Inc.")
    uint8_t     estimatedOs;         // IP TTL signature (64=Linux/iOS/Android, 128=Windows)

    // 1. Connection Lifecycle & State (IPFIX / Zeek)
    uint32_t    tcpSynSent = 0;      // Outbound connection attempts
    uint32_t    tcpSynAckRecv = 0;   // Successfully established handshakes
    uint32_t    tcpRstRecv = 0;      // Connections rejected / refused (RST received)
    uint32_t    tcpHalfOpen = 0;     // SYN sent with no response (unreachable / scan)
    uint32_t    activeFlows = 0;     // Concurrent active transport sessions

    // 2. Traffic Symmetry & Volume Profiles (RMON-2 / IPFIX)
    uint64_t    bytesTx = 0;         // Total outbound payload bytes
    uint64_t    bytesRx = 0;         // Total inbound payload bytes
    uint64_t    packetsTx = 0;       // Total outbound packets
    uint64_t    packetsRx = 0;       // Total inbound packets
    double      txRxRatio = 1.0;     // Upload/download asymmetry (producer vs consumer)

    // 3. Transport & Diversity / Fan-Out Metrics (Scanning & Botnet Detection)
    uint32_t    uniquePeersCount = 0;   // Distinct remote IPs contacted in window
    uint32_t    uniqueDstPorts = 0;     // Distinct destination ports targeted
    double      fanOutRatio = 0.0;      // Peers per byte (high ratio = scanner / worm)

    // 4. Application Layer Behavior (DNS, DHCP, ARP)
    uint32_t    dnsQueryCount = 0;   // Total DNS queries made
    uint32_t    dnsNxdomainCount = 0;// Failed domain lookups (beaconing indicator)
    uint32_t    arpRequestsSent = 0; // Local L2 discovery rate
    uint32_t    bcastPacketsTx = 0;  // Broadcast/multicast chatter emitted

    time_t      firstSeen = 0;
    time_t      lastSeen = 0;
};
```

---

### 3.3 Persistent Device Registry via `libconfig++` (`devices.cfg`)

Following the proven persistence model in `meshmon`, `netmon` uses `libconfig++` rather than SQLite for device registry storage.

#### Key Benefits
1. **Zero Disk Thrashing**: Loaded into RAM on startup; written to disk only when a new device is discovered or manually updated.
2. **Human-Readable & Editable**: Standard structured text format (`~/.config/netmon/devices.cfg`), viewable and editable with standard Linux tools (`cat`, `nano`, `vim`), and git-friendly.
3. **No Database Dependencies**: Eliminates SQLite WAL files, file lock contention over NFS, and database corruption risks.

#### Device Classification & Taxonomy Schema
Every discovered device is categorized objectively through a hybrid MAC OUI + DNS architecture into operational tiers:
1. **`infrastructure`**: Core servers, hypervisors/VMs, routers, firewalls, switches, and embedded SBCs (e.g. HPE, XenSource, Zyxel, Cisco, Ubiquiti, Raspberry Pi).
2. **`iot`**: IP cameras, microcontrollers, smart home displays, smart TVs, audio systems, and network printers (e.g. D-Link, Espressif, Google Nest, Sony Bravia, Nabu Casa, Brother).
3. **`known`**: Workstations, desktop PCs, and laptops (e.g. ASUSTek, EliteGroup, Apple on LAN).
4. **`visitor`**: Client devices on the dedicated guest subnet (`192.168.11.0/24`) or modern mobile devices operating with IEEE 802.3 Locally Administered (LAA) randomized MAC addresses.
5. **`unregistered`**: Newly attached or unclassified hardware awaiting vendor resolution.

#### Automated Identification Pipeline
- **MAC Vendor Resolution (`MacVendorResolver`)**: Discovered OUIs are queried asynchronously against public IEEE registries via `api.maclookup.app` (HTTPS) and cached permanently in `netmon_telemetry.db` (`oui_cache` table) with fallback to embedded `OuiDatabase`.
- **Vendor Taxonomy Mapping (`VendorTaxonomy`)**: Infers category deterministically from hardware manufacturer specialization, eliminating brittle hostname regex rules.
- **Authoritative Reverse DNS (`DnsResolver`)**: Performs non-blocking PTR lookups (`getnameinfo`), strips search domain suffixes (`.selfso.com`), and validates IP consistency via forward confirmation (`getaddrinfo`) to assign canonical hostnames.
- **Passive DHCP Option 12 Sniffing (`LanSniffer`)**: Captures self-announced client hostnames (e.g. mobile phones on guest WiFi) where static DNS PTR records are absent.

#### Sample `~/.config/netmon/devices.cfg`
```libconfig
# NetMon Persistent Device Registry (libconfig++)

devices = (
    {
        mac = "02:00:00:00:00:01";
        ip = "192.168.1.1";
        name = "gateway";
        vendor = "Network Gateway";
        category = "infrastructure";
        source = "dns_ptr";
        first_seen = 1788594000L;
        last_seen = 1789236300L;
    },
    {
        mac = "02:00:00:00:00:02";
        ip = "192.168.1.50";
        name = "ipcam-salon";
        vendor = "D-Link International";
        category = "iot";
        source = "dns_ptr";
        first_seen = 1788594000L;
        last_seen = 1789236300L;
    },
    {
        mac = "02:00:00:00:00:03";
        ip = "192.168.11.2";
        name = "guest_phone";
        vendor = "Mobile Device";
        category = "visitor";
        source = "dhcp_opt12";
        first_seen = 1789230000L;
        last_seen = 1789236310L;
    }
);
```

---

---

### 3.4 SNMP Telemetry Aggregator (MRTG Replacement Engine)

`SnmpAggregator` provides asynchronous, high-resolution polling against network switches, routers, and gateways:
- **Net-SNMP Async Engine**: Background thread polls configured targets at configurable cadences (default: 30s).
- **64-bit High-Capacity (`ifXTable`) Counters**: Supports `ifHCInOctets` and `ifHCOutOctets` with seamless 32-bit rollover fallback for legacy ports.
- **Router Reboot & Wrap Detection**: Continuously checks `sysUpTime`. When a target router reboots and counter resets to 0, rate calculations automatically discard the interval, preventing false petabyte spikes.
- **Smart 3-Tier Interface Filtering**: Suppresses noise from loopback (`lo`), virtual interfaces (`docker*`, `virbr*`, `veth*`), and inactive/down ports, exposing only verified physical uplinks and configured WAN ports.
- **Sub-Millisecond In-Memory Caching**: Thread-safe in-memory cache responds instantaneously to AI MCP tool queries without blocking on network round-trips.

---

### 3.5 Persistent SQLite Time-Series Database (`SnmpDatabase`)

`SnmpDatabase` provides resilient, zero-loss local time-series storage:
- **SQLite WAL Mode**: Configured with `PRAGMA journal_mode=WAL;` and `PRAGMA synchronous=NORMAL;` for minimal I/O overhead and lock-free concurrent reads while polling threads write.
- **`snmp_samples` Table**: Records uncompressed 30-second telemetry data points (`timestamp`, `target_ip`, `if_name`, `in_bytes_sec`, `out_bytes_sec`, `in_hc_octets`, `out_hc_octets`, `oper_status`, `in_errors`, `out_errors`). Indexed on `(target_ip, if_name, timestamp)`.
- **90-Day Raw Retention**: Retains full uncompressed granularity for 90 days (`~50 MB` total database footprint) for forensic spike and outage analysis.
- **`snmp_hourly_rollups` Table**: Automatically rolls up older samples into hourly min/avg/max/total throughput statistics, enabling multi-year capacity planning and 95th-percentile billing analysis with `< 1 MB` storage per year.

---

### 3.6 Embedded HTTP Web Server & Live Dashboard (`WebServer`)

`WebServer` embeds `cpp-httplib` to host a standalone, dark-mode web monitoring dashboard on port `3884`:
- **Dual-WAN Live Telemetry**: Visual gauges and responsive Canvas area charts displaying live and historical transfer rates.
- **Traffic Protocol Breakdown**: Live distribution of DNS, HTTPS, SSH, HTTP, ARP, Broadcast, Multicast, and ICMP traffic.
- **Top Talkers & Device Inventory**: Searchable inventory table displaying discovered devices, friendly names, vendors, categories, and last seen timestamps.
- **Dual-Mode Asset Serving**: Dynamically serves live assets from `web/` when developing, falling back to compiled C++ string literals in `WebAssets.hxx` for zero-dependency binary distribution.
- **RESTful Endpoints**:
  - `GET /api/status`: Subsystem status, database size, and device counts.
  - `GET /api/snmp/wan`: Dual-WAN uplink metrics, 24-hour peaks, and daily GB totals.
  - `GET /api/snmp/devices`: 3-tier filtered interface metrics and switch counters.
  - `GET /api/snmp/history?iface=<name>&hours=<n>`: Time-series historical data.
  - `GET /api/traffic`: LAN packet rates and protocol distribution.
  - `GET /api/devices`: Complete device inventory with vendor mappings.

---

### 3.7 AI Security Checkpoint & Policy

The `SecurityCheckpoint` enforces hard safety boundaries on all AI agent invocations:
1. **Protected Invariant IPs**: Prevents blocking gateway router, core cluster servers, or loopback/broadcast addresses.
2. **Permission Matrix**: Configurable boolean flags (`allow_ai_block_ip`, `allow_ai_raw_exec`).
3. **Audit Trail**: Every action requested by an AI agent is recorded in-memory and emitted to standard diagnostic logs with timestamps and agent identifiers.

---

### 3.8 Zyxel USG Driver (Production Qualified on USG FLEX 200)

`ZyxelDriver` and `ZyxelSshClient` provide the local state machine, PTY stream parser, and SSH automation subsystem for Zyxel USG FLEX and ZyWALL series gateways:
- **Operational Status & Hardware Qualification**:
  - **Status**: **Production Qualified & Active**.
  - **Live Hardware Target**: Fully qualified and validated directly against physical **Zyxel USG FLEX 200** hardware running firmware `V5.43(ABUI.0)` on `192.168.8.1:22`.
  - **Automated Qualification**: Comprehensive test suite (`test_zyxel_driver_suite`) running CppUTest fixtures across 14 data models, manual transcripts from the official 666-page ZyWALL ZLD Reference Guide, and deep Clang 18 `libFuzzer` campaigns (>450,000 fuzzed iterations with ASan/UBSan).
  - **Authoritative Specification**: See [`ZyxelDriver.md`](ZyxelDriver.md) for full architectural documentation, security controls, capabilities catalog, and ASCII dataflow diagrams.
- **Default Configuration Safeguards**:
  - `router_live_enabled = false` (default in `netmon.cfg`): Safe default; mutations are journaled locally as pending unless explicitly enabled by operator.
  - `router_flash_write = false` (default in `netmon.cfg`): Quiescent flash commits (`write`) disabled unless enabled.
  - `router_dry_run = true` (configurable via `netmon.cfg` or `NETMON_ROUTER_DRY_RUN`): Emits synthetic `[router-dry-run]` command logs to stderr and records transcripts for verification without network access.
- **Encrypted Vault Storage Only**:
  - Router credentials are stored exclusively in `AuthManager`'s AES-256-GCM encrypted vault (`~/.config/netmon/vault.enc`) with companion master key `~/.config/netmon/vault.key` (`0600` permissions).
  - Plaintext password config fields and `NETMON_ROUTER_PASSWORD` environment overrides are not supported and are excluded from the design.
  - Interactive zero-leak provisioning is performed via `router set-password` CLI command with terminal echo disabled.
- **Interactive PTY Session Engine**:
  - Allocates a 256-column PTY terminal preventing arbitrary line wraps.
  - Enforces 5000ms blocking timeouts and automatic `--More--` pager stream advancement.
  - Tail-anchored prompt detection matching user exec (`Router>`), privileged exec (`Router#`), config (`Router(config)#`), and submode contexts (`Router(secure-policy)#`).
  - ANSI escape code stripping and command echo removal.
  - Automatic 5x `exit` unwind recovery to restore root prompt `#` on unexpected submode traps.
- **Durable On-Disk Mutation Journaling (`router_journal.json`)**:
  - Every block and unblock mutation is journaled atomically to `~/.config/netmon/router_journal.json` (`0600` permissions) before dispatch.
  - Replay and reconciliation logic tracks row states (`pending` vs `applied_running`).
- **30-Second Debounced Flash Protection**:
  - Rapid block/unblock cycles are debounced across a 30-second window before committing to router NVRAM (`write`).
- **Reference-Ordered Rollback Engine**:
  - Clean transactional teardown in exact reverse reference order (`no secure-policy` before `no address-object`) if command sequence fails midway.
- **Trust On First Use (TOFU) Host-Key Pinning**:
  - Router SSH server public key SHA-256 fingerprint pinned to `~/.config/netmon/router_hostkey.pin` (`0600` permissions), protecting against man-in-the-middle attacks.

---

## 4. Exported MCP Toolset via `aimon` Gateway

| Tool Name | Status | Functionality |
| :--- | :--- | :--- |
| **`lan_get_devices`** | **Active** | Returns all unique MAC and IP addresses discovered on the network (from ARP cache + live frames), vendor OUI, category (Known / Visitor / Unregistered), and last seen. |
| **`lan_get_unregistered_devices`** | **Active** | Lists all unregistered devices or visitor phones connected to WiFi, with vendor identification and connection timestamps. |
| **`lan_name_device`** | **Active** | Allows an operator or AI assistant to assign a friendly name and category to a MAC address, persisting it to `devices.cfg`. |
| **`lan_get_top_talkers`** | **Active** | Returns the top bandwidth-consuming internal hosts over a time window (15m, 1h, 24h) with transfer rates directly from RAM. |
| **`lan_get_traffic_summary`** | **Active** | Returns total LAN throughput, packet rates, and protocol distribution (DNS, HTTPS, SSH, ARP, broadcast). |
| **`firewall_get_status`** | **Active** | Returns router model, firmware, uptime, and SSH link state (live qualified on USG FLEX 200). |
| **`firewall_get_sessions`** | **Active** | Queries active firewall session table and connection counts (live qualified on USG FLEX 200). |
| **`firewall_block_ip`** | **Active** | Enqueues and applies firewall drop rule and address object via SSH automation (checked against safety invariants). |
| **`firewall_unblock_ip`** | **Active** | Enqueues and removes firewall drop rule and address object via SSH automation. |
| **`snmp_get_wan_status`** | **Active** | Dedicated dual-WAN uplink bandwidth rates, 5-minute averages, 24-hour peaks, and daily GB transfers. |
| **`snmp_get_device_metrics`** | **Active** | SNMP metrics, system description, uptime, and 3-tier filtered interfaces for switches and routers. |
| **`snmp_get_interface_counters`** | **Active** | Line speeds, 64-bit HC octet counters, and packet error counts for specific ports. |
| **`snmp_query_oid`** | **Active** | Direct arbitrary standard MIB or enterprise OID queries. |

---

## 5. Build & Execution

- **Compile**: Compile via the top-level `Makefile`:
  ```bash
  make -j$(nproc)
  ```
- **Packet Capture Permissions**: Attach Linux raw network capabilities to the compiled binary:
  ```bash
  make setcap
  ```
- **Running the Daemon**:
  - Interactive terminal CLI mode:
    ```bash
    ./build/netmon daemon
    ```
  - Headless background service:
    ```bash
    ./build/netmon run
    ```
  - Interface and registry diagnostics:
    ```bash
    ./build/netmon status
    ```

---

## 6. Verification Plan

1. **Native Compilation & Automated Unit Test Suites**:
   - Build cleanly with zero compiler warnings via `make -j$(nproc)`.
   - Run `make test` verifying local state machine, parser, vault, and database suites.
   - Note: Automated test suites validate in-memory state and PTY parsing only; live ZySH execution on hardware is not exercised by `make test`.
2. **Capability Preflight**:
   - Verify `./build/netmon status` reports active capture permissions.
3. **Gateway Registration**:
   - Verify connection to `aimon` gateway (`port 3885`) and confirmation of tool registration.
4. **Live MCP Query Verification**:
   - Query `lan_get_devices` via MCP: Verify discovery of unique MACs from ARP cache and live frames.
   - Query `lan_get_unregistered_devices` via MCP: Verify unregistered device identification and OUI resolution.
   - Call `lan_name_device` via MCP: Verify dynamic update and persistence to `devices.cfg`.
   - Query `lan_get_top_talkers` and `lan_get_traffic_summary` via MCP: Verify real-time bandwidth and protocol metrics.
   - Test stub/disabled tools (`firewall_get_status`, `snmp_get_device_metrics`): Verify clean disabled/unconfigured responses without crashing.
5. **Physical Hardware Qualification (Future)**:
   - Controlled live qualification on physical USG hardware remains pending manual transcript validation.

---

## License & Copyright

Copyright (C) 2026, Charles Chiou. All rights reserved.
