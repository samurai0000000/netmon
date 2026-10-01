# netmon: LAN Streaming Monitor & AI Telemetry Gateway

`netmon` is a high-performance network monitoring, device discovery, traffic analysis, and router/firewall automation daemon written in C++17. It continuously observes physical LAN traffic, tracks active devices and visitor phones, aggregates real-time bandwidth metrics, and exports a vendor-agnostic toolset to AI assistants (Cursor, Google Antigravity, Gemini) via `aimon` over the Model Context Protocol (MCP).

All daemon configurations standardize on `libconfig++` and adhere to the XDG Base Directory specification (`~/.config/netmon/netmon.cfg`).

---

## Key Features

- **Zero-DB In-Memory Streaming Sniffer**:
  - Uses `libpcap` with header-only capture (`snaplen = 96` bytes).
  - Bulk payloads (video, downloads, backups) are dropped in kernel space; zero raw packets are retained in RAM or written to disk, preventing SSD thrashing.
  - Sub-microsecond protocol decoding for Ethernet, IPv4, TCP/UDP, ARP, DNS, DHCP, and ICMP.
- **Sliding-Window Ring Buffers**:
  - Fixed 60-minute circular ring buffer tracking host throughput, burst rates, and protocol breakdowns.
  - Memory consumption is bounded ($\approx 50\text{ KB}$ ring buffer, $< 1\text{ MB}$ total sniffer footprint).
- **Persistent Device Registry & OUI Identification**:
  - Tracks discovered hosts in `~/.config/netmon/devices.cfg` using `libconfig++`.
  - IEEE OUI vendor database matching with automatic classification of Known, Infrastructure, Visitor/Guest, and Unregistered devices (including mobile randomized MAC detection).
- **AI Security Checkpoint & Policy Enforcement**:
  - Hard safety invariants prevent AI agents from blocking essential network infrastructure (default gateway router, local cluster servers, or broadcast ranges).
  - Configurable safety flags (`allow_ai_block_ip`, `allow_ai_raw_exec`) and full audit logging.
- **Modern SNMP Telemetry Engine (MRTG Replacement)**:
  - Continuously polls network switches, routers, and gateways every 30 seconds.
  - Full 64-bit High-Capacity counter support (`ifHCInOctets`/`ifHCOutOctets`) with automatic 32-bit rollover fallback.
  - `sysUpTime` tracking to detect device reboots and eliminate false delta spikes.
  - 3-tier interface filtering suppresses virtual, down, and loopback noise, isolating active physical ports and WAN uplinks.
- **Persistent SQLite Time-Series Database (`SnmpDatabase`)**:
  - High-performance WAL mode (`PRAGMA journal_mode=WAL; PRAGMA synchronous=NORMAL;`).
  - Retains uncompressed raw 30-second samples for 90 days (`~50 MB` total footprint) for forensic rate spike analysis.
  - Automated hourly rollups (`snmp_hourly_rollups`) for multi-year trend analysis and capacity planning.
- **Embedded Web Server & Live Dashboard**:
  - Built-in asynchronous HTTP server running on port `3884` powered by `cpp-httplib`.
  - Modern responsive dark-mode web dashboard featuring live dual-WAN canvas throughput meters, LAN protocol gauges, and device inventory.
  - Complete RESTful JSON API (`/api/status`, `/api/snmp/wan`, `/api/snmp/devices`, `/api/snmp/history`, `/api/traffic`, `/api/devices`).
  - Dual-mode asset serving: reads live files from `web/` when present, falling back to compiled C++ string literals for standalone single-binary deployment.
- **MCP Gateway Integration (`aimon`)**:
  - Dynamically registers network diagnostics, device discovery, firewall automation, and SNMP telemetry tools with `aimon` over a persistent TCP JSON-RPC 2.0 stream.
- **Least-Privilege Security & Credential Model**:
  - Runs as a standard unprivileged user using Linux POSIX capability (`CAP_NET_RAW`).
  - Omits `CAP_NET_ADMIN` to ensure the process cannot alter routing tables, interface IPs, or host firewall rules.
  - Configuration directory is enforced to `0700` (`drwx------`) and files to `0600` (`-rw-------`).
  - Router credentials reside solely in `AuthManager`'s AES-256-GCM encrypted vault (`~/.config/netmon/vault.enc`), provisioned interactively via `router set-password`. Plaintext config passwords and `NETMON_ROUTER_PASSWORD` environment overrides are not supported.
  - Zyxel USG/ATP firewall integration is **Production Qualified on USG FLEX 200** (see [`ZyxelDriver.md`](ZyxelDriver.md) for full architecture and command catalog). Defaults to safe mode (`router_live_enabled = false`, `router_flash_write = false`) unless explicitly enabled by the operator.
  - Supports 12-factor environment variable configuration injection (`NETMON_ROUTER_USER`, `NETMON_ROUTER_KEY_PATH`, `NETMON_ROUTER_DRY_RUN`, `NETMON_SNMP_*`, `NETMON_DB_*`, `NETMON_WEB_*`).
- **Interactive Diagnostic Shell**:
  - Embedded interactive terminal CLI with real-time status, device, and traffic reports.

---

## Architecture Overview

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
│  │ RouterDriver Abstraction Interface    │   │ Live LAN Streaming Sniffer│  │
│  │   ┌────────────────────────────────┐  │   │ (LanSniffer via libpcap)  │  │
│  │   │ ZyxelDriver (USG SSH / API)    │  │   │ - Configurable interface  │  │
│  │   └────────────────────────────────┘  │   │ - Header-only snaplen(96B)│  │
│  │   ┌────────────────────────────────┐  │   │ - Zero Raw Packet Storage │  │
│  │   │ SnmpAggregator (Switches / APs)│  │   │ - Stream Feature Extract  │  │
│  │   └────────────────────────────────┘  │   └─────────────┬─────────────┘  │
│  └───────────────────┬───────────────────┘                 │                │
│                      │                       ┌─────────────┴─────────────┐  │
│                      │                       │ In-Memory Sliding Windows │  │
│                      │                       │ - 60-Minute Circular Ring │  │
│                      │                       │ - Top talkers & rates     │  │
│                      │                       │ - Protocol distributions  │  │
│                      │                       │ - Host Behavioral Metrics │  │
│                      │                       └─────────────┬─────────────┘  │
│                      │                                     │                │
│                      │                       ┌─────────────▼─────────────┐  │
│                      │                       │ Persistent DeviceRegistry │  │
│                      │                       │ (libconfig++ storage in   │  │
│                      │                       │  ~/.config/netmon/        │  │
│                      │                       │  devices.cfg)             │  │
│                      │                       │ - Known / Visitor / New   │  │
│                      │                       │ - OUI Vendor Match        │  │
│                      │                       └─────────────┬─────────────┘  │
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

## Deployment Model

`netmon` is designed to be deployed on a Linux host with direct visibility into LAN network traffic:

- **Network Interface Placement**: Dedicated Network Interface Card (NIC) bridged or configured in promiscuous mode (e.g. `br0` or a core switch mirror port) for packet capture and traffic analysis.
- **Compilation**: Standard native build via top-level `Makefile` (`make -j$(nproc)`).
- **Execution Modes**: Runs as an interactive terminal CLI (`./build/netmon`) or as a headless background service (`./build/netmon run`).

---

## Exported MCP Toolset via `aimon`

| Tool Name | Type | Description |
| :--- | :--- | :--- |
| **`lan_get_devices`** | Read | Discovered network inventory (MAC, IP, device name, category, OUI vendor, last seen). |
| **`lan_get_unregistered_devices`** | Read | Lists unclassified or visitor mobile devices connected to the LAN. |
| **`lan_name_device`** | Write | Assigns friendly names and categories (`known`, `visitor`, `iot`, etc.) persisted to `devices.cfg`. |
| **`lan_get_top_talkers`** | Read | Top bandwidth consumers over rolling windows (default: 15m, configurable). |
| **`lan_get_traffic_summary`** | Read | Real-time throughput, packet rates, active capture engine status, and protocol breakdown. |
| **`firewall_get_status`** | Read | Router hardware model, firmware, link state (live qualified on USG FLEX 200). |
| **`firewall_get_sessions`** | Read | Active router state table / connection sessions (live qualified on USG FLEX 200). |
| **`firewall_block_ip`** | Write | Drops IP address via router firewall (when live mode enabled; checked against invariants). |
| **`firewall_unblock_ip`** | Write | Removes router drop rule for specified IP (when live mode enabled). |
| **`snmp_get_wan_status`** | Read | Dedicated dual-WAN uplink rates, 5-min averages, 24-hr peaks, and daily GB transfers. |
| **`snmp_get_device_metrics`** | Read | SNMP system metrics, uptime, and 3-tier filtered active interfaces. |
| **`snmp_get_interface_counters`** | Read | Line speeds, 64-bit HC octet counters, and packet error counts for specific ports. |
| **`snmp_query_oid`** | Read | Direct arbitrary standard MIB or enterprise OID queries. |

---

## Embedded Web Dashboard

`netmon` includes an embedded dark-mode web dashboard running on port `3884` (`http://<host>:3884`), modeled after `aimon` and `meshmon`:

- **Dual-WAN Meters & Canvas Charts**: Visual gauges and high-resolution historical throughput charts powered by the SQLite database.
- **Protocol Distribution**: Live breakdown of DNS, HTTPS, SSH, HTTP, ARP, Broadcast, Multicast, and ICMP traffic.
- **Top Talkers**: Real-time identification of highest bandwidth-consuming internal hosts.
- **Device Inventory**: Searchable, filterable table of discovered devices, vendors, and online states.
- **RESTful Endpoints**:
  - `GET /api/status`: Overall daemon health, database size, and subsystem state.
  - `GET /api/snmp/wan`: Real-time dual-WAN bandwidth rates and 24-hour peaks.
  - `GET /api/snmp/devices`: Monitored interfaces, link speeds, and octet counters.
  - `GET /api/snmp/history?iface=<name>&hours=<n>`: Historical time-series points.
  - `GET /api/traffic`: LAN packet rates and protocol distribution.
  - `GET /api/devices`: Discovered device inventory and vendor mappings.

---

## Automated Device Classification & Taxonomy Engine

`netmon` combines hardware OUI vendor discovery with local network telemetry to classify devices into operational categories without manual configuration:

- **Asynchronous MAC OUI Resolution (`MacVendorResolver`)**: Observed MAC prefixes are queried asynchronously against public IEEE registries via `api.maclookup.app` (HTTPS) and cached permanently in `netmon_telemetry.db` (`oui_cache` table) with graceful offline fallback to `OuiDatabase`.
- **Deterministic Hardware Taxonomy (`VendorTaxonomy`)**: Maps manufacturers into standardized operational categories (`infrastructure`, `iot`, `known`, `visitor`, `unregistered`) based on hardware specialization (e.g. D-Link cameras, Espressif sensors, and Google Nest Hubs &rarr; `iot`; HPE servers and Xen hypervisors &rarr; `infrastructure`).
- **Authoritative Reverse DNS (`DnsResolver`)**: Non-blocking background worker queries local reverse DNS PTR records (`getnameinfo`), validates IP consistency via forward confirmation (`getaddrinfo`), and strips local search domains (`.selfso.com`) to assign canonical friendly hostnames.
- **Passive DHCP Option 12 Sniffing (`LanSniffer`)**: Captures hostnames announced in DHCP requests on UDP ports 67/68 for dynamic guest clients where PTR records are absent.
- **Guest Subnet Topology Isolation**: Enforces guest network semantics (`192.168.11.0/24`), ensuring visitor mobile devices remain categorized as `visitor`.

---

## Prerequisites & Dependencies

To build and run `netmon`:

- **Compiler**: GCC (`g++` 9+) or Clang (`clang++` 10+) with C++17 support.
- **Build System**: CMake 3.16+ and GNU Make.
- **System Libraries**:
  - `libpcap-dev`
  - `libconfig++-dev`
  - `libsnmp-dev`
  - `libsqlite3-dev`
  - `libssl-dev`
  - `libssh2-1-dev`
  - `pkg-config`
- **Submodules** (managed under `third_party/`):
  - `third_party/json` (nlohmann/json)
  - `third_party/cpp-httplib` (yhirose/cpp-httplib)

### Installing Build Dependencies (Debian / Ubuntu)

```bash
sudo apt update
sudo apt install -y build-essential cmake libpcap-dev libconfig++-dev libsnmp-dev libsqlite3-dev libssl-dev libssh2-1-dev pkg-config
```

---

## Building & Enabling PCAP Capture on the NIC

To capture live Ethernet frames and analyze protocol distributions, `netmon` requires access to the physical network interface card (NIC) or bridge. Follow these steps to ensure live PCAP streaming functions properly.

### 1. Compile the Binary
Always compile via the top-level `Makefile`:

```bash
make -j$(nproc)
```

### 2. Configure Linux Capabilities (Least Privilege Sniffing)
On Linux, opening raw packet capture sockets (`socket(PF_PACKET, SOCK_RAW)`) and capturing promiscuous frames requires the `CAP_NET_RAW` capability. 

To enforce the **Principle of Least Privilege**, `netmon` requires **only `CAP_NET_RAW`** and explicitly avoids `CAP_NET_ADMIN`. This guarantees that the binary has permission strictly to sniff packet headers—it is physically incapable of altering kernel routing tables, modifying interface IP configurations, or changing firewall rules.

Attach the capability directly to the binary:

```bash
make setcap
# Or run manually:
sudo setcap cap_net_raw=eip build/netmon
```

Verify that the capability was attached:
```bash
getcap build/netmon
# Expected output:
# build/netmon cap_net_raw=eip
```

> [!NOTE]
> **Rebuild Notice**: In Linux, whenever `make` relinks the executable, the kernel clears extended file attributes (`security.capability`).
>
> **Optional One-Time Sudoers Rule (Automated Builds)**:
> To allow `make` (or `make setcap`) to re-apply capabilities automatically without prompting for a password, pin the exact binary path (never use wildcards, to prevent privilege escalation).
>
> **Option A: One-line command**:
> ```bash
> echo "$USER ALL=(ALL) NOPASSWD: /usr/sbin/setcap cap_net_raw=eip $(pwd)/build/netmon" | sudo tee /etc/sudoers.d/netmon-setcap
> sudo chmod 0440 /etc/sudoers.d/netmon-setcap
> ```
>
> **Option B: Edit with `vi` (via `visudo`)**:
> Always use `visudo` with `vi` so sudoers syntax is validated before saving:
> ```bash
> sudo EDITOR=vi visudo -f /etc/sudoers.d/netmon-setcap
> ```
> Add the rule line (replace `<user>` and `<repo-path>` with your user and repository path):
> ```sudoers
> <user> ALL=(ALL) NOPASSWD: /usr/sbin/setcap cap_net_raw=eip <repo-path>/build/netmon
> ```
> Save and exit in `vi` with `:wq`.
>
> With this rule installed, `make` will automatically and silently re-apply `setcap` on every build.

### 3. Verify NIC Link & Promiscuous State
Ensure the monitored network interface is in the `UP` state:

```bash
ip link show br0
# Expected: <BROADCAST,MULTICAST,UP,LOWER_UP>
```

- **Interface Matching**: By default, `netmon` listens on `br0`. If your system uses a different interface (e.g. `eth0`, `eno1`, `enp3s0`), set `interface = "<iface>";` in `~/.config/netmon/netmon.cfg` or supply `-i <iface>` on the command line.
- **Switch Port Mirroring / Promiscuous Visibility**:
  To capture all LAN traffic (and not just traffic destined to or emitted from the host's own MAC), ensure the monitored physical interface is either:
  1. Connected to a managed switch port configured with **Port Mirroring / SPAN** forwarding all segment packets to the host NIC, OR
  2. Member of a network bridge (`br0`) where promiscuous mode is enabled.

### 4. Run Preflight Diagnostics
Before launching the continuous daemon, run the built-in preflight status check:

```bash
./build/netmon status
```

Confirm that the output reports:
```text
--- PCAP Permission Preflight ---
  Raw Capture Socket:  PERMITTED (CAP_NET_RAW active)
```
If permissions are missing or denied, `netmon` will print an explicit `[WARNING]` banner detailing the exact socket error and remediation command.

### 5. Running the Application

```bash
# Launch interactive CLI shell (default)
./build/netmon

# Or run one-shot preflight status diagnostics
./build/netmon status

# Or start non-interactive in background
./build/netmon run
```

---

## Running & CLI Usage

### Interactive CLI Shell (Default)
Starts the streaming sniffer, connects to the `aimon` gateway, and launches the interactive terminal CLI shell:

```bash
./build/netmon
```

### Status & Preflight Check Mode
Performs an immediate preflight socket check, prints service status, outputs the device registry, and exits:

```bash
./build/netmon status
```

### Non-Interactive Background Mode
Runs continuously without an interactive terminal shell (e.g. when run under systemd or a background service):

```bash
./build/netmon run
```

### Interactive CLI Shell Commands

When running the interactive CLI shell (`./build/netmon`):

```text
status                       Show sniffer, interface, and gateway status
devices [filter]             List devices (optional: known, visitor, unregistered, all)
unregistered                 List unregistered and visitor mobile devices
visitors                     Alias for unregistered
toptalkers [limit] [mins]    Show top bandwidth consumers (default: 10 hosts, 15 mins)
traffic                      Show LAN throughput and protocol breakdown
name <mac> <name> [category] Assign friendly name and category to a device
router set-password          Interactively set encrypted router password in vault
router clear-password        Clear encrypted router password from vault
reload                       Reload configuration and devices registry
help                         Show available commands
quit / exit                  Exit the shell
```

---

## Configuration

Default configuration is automatically created at `~/.config/netmon/netmon.cfg` with mode `0600`:

```libconfig
interface = "br0";
gateway_host = "127.0.0.1";
gateway_port = 3885;
reconnect_interval_sec = 5;
devices_file = "~/.config/netmon/devices.cfg";
log_level = "info";
allow_ai_block_ip = false;
allow_ai_raw_exec = false;

router_live_enabled = false;
router_flash_write = false;
router_dry_run = false;

web = {
    enabled = true;
    port = 3884;
    bind_address = "0.0.0.0";
};

snmp = {
    poll_interval_sec = 30;
    targets = (
        {
            name = "gateway";
            ip = "<router-ip>";
            community = "public";
            version = "2c";
            port = 161;
            wan_interfaces = [ "eth1", "eth2" ];
        }
    );
};
```

### Environment Variable Overrides (12-Factor Support)

All parameters can be injected or overridden via environment variables:
- `NETMON_WEB_PORT` / `NETMON_WEB_ENABLED` / `NETMON_WEB_BIND`
- `NETMON_SNMP_TARGET` / `NETMON_SNMP_COMMUNITY` / `NETMON_SNMP_VERSION` / `NETMON_SNMP_PORT`
- `NETMON_DB_PATH` / `NETMON_DB_RETENTION_DAYS`
- `NETMON_ROUTER_USER` / `NETMON_ROUTER_KEY_PATH` / `NETMON_ROUTER_DRY_RUN`

---

## Repository Structure

```text
netmon/
├── CMakeLists.txt                 # CMake configuration and version generation
├── Design.md                      # Complete architectural specification & schemas
├── Makefile                       # Top-level build wrapper (all, setcap, clean, distclean)
├── README.md                      # Project overview and quickstart guide
├── Version.hxx.in                 # Metadata template (version, host, build timestamp)
├── include/
│   ├── AimonGatewayClient.hxx     # TCP client exporting netmon tools to aimon
│   ├── AuthManager.hxx            # Encrypted AES-256-GCM vault & session manager
│   ├── Config.hxx                 # libconfig++ configuration manager
│   ├── DeviceRegistry.hxx         # Device registry and persistence engine
│   ├── HostMetrics.hxx            # Host behavioral state & IPFIX/RMON metrics
│   ├── LanSniffer.hxx             # Zero-DB streaming sniffer via libpcap
│   ├── NetMonShell.hxx            # Interactive diagnostic shell for GNU screen
│   ├── OuiDatabase.hxx            # IEEE OUI vendor resolution database
│   ├── RouterDriver.hxx           # Abstract router/firewall interface
│   ├── SecurityCheckpoint.hxx     # AI safety invariants and permission gating
│   ├── SnmpAggregator.hxx         # SNMP metrics engine (64-bit HC counters, 30s poll)
│   ├── SnmpDatabase.hxx           # SQLite WAL time-series storage & hourly rollups
│   ├── SyslogServer.hxx           # Syslog UDP receiver and audit ingest
│   ├── WebAssets.hxx              # Embedded fallback HTML/CSS/JS string literals
│   ├── WebServer.hxx              # Asynchronous HTTP server (cpp-httplib)
│   ├── ZyxelDriver.hxx            # Concrete Zyxel USG router driver
│   └── ZyxelSshClient.hxx         # libssh2 PTY client & ZySH stream parser
├── src/
│   ├── AimonGatewayClient.cxx     # AimonGatewayClient implementation
│   ├── AuthManager.cxx            # Vault encryption and auth token manager
│   ├── Config.cxx                 # Config manager implementation
│   ├── DeviceRegistry.cxx         # Device registry implementation
│   ├── LanSniffer.cxx             # LanSniffer & packet stream decoding
│   ├── Main.cxx                   # Entrypoint & CLI subcommand routing
│   ├── NetMonShell.cxx            # Interactive shell commands implementation
│   ├── OuiDatabase.cxx            # OUI database implementation
│   ├── SecurityCheckpoint.cxx     # Security checkpoint implementation
│   ├── SnmpAggregator.cxx         # SNMP aggregator implementation
│   ├── SnmpDatabase.cxx           # SQLite time-series database implementation
│   ├── SyslogServer.cxx           # Syslog receiver implementation
│   ├── WebServer.cxx              # Embedded WebServer & REST API implementation
│   ├── ZyxelDriver.cxx            # Zyxel router driver implementation
│   └── ZyxelSshClient.cxx         # Zyxel SSH client & PTY driver implementation
├── web/                           # Embedded web dashboard frontend assets
│   ├── app.js                     # Live Canvas chart rendering & API polling
│   ├── index.html                 # Dark-mode dashboard layout
│   └── style.css                  # Responsive dark-theme styling
└── third_party/
    ├── cpp-httplib/               # Git submodule (yhirose/cpp-httplib)
    └── json/                      # Git submodule (nlohmann/json)
```

---

## License & Copyright

Copyright (C) 2026, Charles Chiou. All rights reserved.
