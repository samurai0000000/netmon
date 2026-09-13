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
│  │ RouterDriver Abstraction Interface    │   │ Live LAN Streaming Sniffer│  │
│  │   ┌────────────────────────────────┐  │   │ (LanSniffer via libpcap)  │  │
│  │   │ ZyxelDriver (Stub / Unconfig)  │  │   │ - Configurable interface  │  │
│  │   └────────────────────────────────┘  │   │ - Header-only snaplen(96B)│  │
│  │   ┌────────────────────────────────┐  │   │ - Zero Raw Packet Storage │  │
│  │   │ SnmpAggregator (Stub / Unconfig│  │   │ - Stream Feature Extraction│ │
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
│                      │                       │ - OUI Vendor Identification│ │
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

#### Device Classification Schema
Every discovered device is categorized into one of three operational tiers:
1. **`Known / Infrastructure`**: Fixed infrastructure defined in `devices.cfg` (servers, routers, workstations, fixed IoT).
2. **`Visitor / Guest`**: Mobile phones, tablets, or visitor laptops (recognized by vendor OUI like Apple, Samsung, Google Pixel, or assigned to the guest WiFi VLAN `192.168.11.x`).
3. **`Unregistered`**: Newly observed MAC addresses awaiting identification.

#### Sample `~/.config/netmon/devices.cfg`
```libconfig
# NetMon Persistent Device Registry (libconfig++)

devices = (
    {
        mac = "02:00:00:00:00:01";
        ip = "192.168.8.1";
        name = "gateway";
        vendor = "Network Gateway";
        category = "infrastructure";
        first_seen = 1788594000;
        last_seen = 1789236300;
    },
    {
        mac = "02:00:00:00:00:02";
        ip = "192.168.8.39";
        name = "server_node";
        vendor = "Virtual Machine NIC";
        category = "infrastructure";
        first_seen = 1788594000;
        last_seen = 1789236300;
    },
    {
        mac = "02:00:00:00:00:03";
        ip = "192.168.8.215";
        name = "mobile_client";
        vendor = "Mobile Device";
        category = "visitor";
        first_seen = 1789230000;
        last_seen = 1789236310;
    }
);
```

---

### 3.4 Decoupled Hardware Drivers & Clean Stubs

To ensure `netmon` compiles, connects to `aimon`, and functions immediately without requiring external credentials:
- **`RouterDriver` & `ZyxelDriver`**: Defined as clean abstract interfaces. Unimplemented methods return structured `"Driver unconfigured in netmon.cfg"` messages over MCP.
- **`SnmpAggregator`**: Defined as a clean abstract interface. Unimplemented methods return `"No SNMP targets configured"`.
- Concrete SSH commands for Zyxel and SNMP OIDs will be implemented when hardware details are provided, with zero impact on the sniffer or gateway architecture.

---

### 3.5 AI Security Checkpoint & Policy

The `SecurityCheckpoint` enforces hard safety boundaries on all AI agent invocations:
1. **Protected Invariant IPs**: Prevents blocking gateway router, core cluster servers, or loopback/broadcast addresses.
2. **Permission Matrix**: Configurable boolean flags (`allow_ai_block_ip`, `allow_ai_raw_exec`).
3. **Audit Trail**: Every action requested by an AI agent is recorded in-memory and emitted to standard diagnostic logs with timestamps and agent identifiers.

---

## 4. Exported MCP Toolset via `aimon` Gateway

| Tool Name | Status | Functionality |
| :--- | :--- | :--- |
| **`lan_get_devices`** | **Active** | Returns all unique MAC and IP addresses discovered on the network (from ARP cache + live frames), vendor OUI, category (Known / Visitor / Unregistered), and last seen. |
| **`lan_get_unregistered_devices`** | **Active** | Lists all unregistered devices or visitor phones connected to WiFi, with vendor identification and connection timestamps. |
| **`lan_name_device`** | **Active** | Allows an operator or AI assistant to assign a friendly name and category to a MAC address, persisting it to `devices.cfg`. |
| **`lan_get_top_talkers`** | **Active** | Returns the top bandwidth-consuming internal hosts over a time window (15m, 1h, 24h) with transfer rates directly from RAM. |
| **`lan_get_traffic_summary`** | **Active** | Returns total LAN throughput, packet rates, and protocol distribution (DNS, HTTPS, SSH, ARP, broadcast). |
| **`firewall_get_status`** | **Stub** | Returns router model, firmware, link states (currently stubbed: *"Router not configured in netmon.cfg"*). |
| **`firewall_get_sessions`** | **Stub** | Queries active firewall session table (currently stubbed). |
| **`firewall_block_ip`** | **Stub (Gated)** | Inserts a firewall drop rule, validated against protected subnets (currently stubbed). |
| **`firewall_unblock_ip`** | **Stub (Gated)** | Removes a firewall drop rule (currently stubbed). |
| **`snmp_get_device_metrics`** | **Stub** | Returns switch/AP interface metrics (currently stubbed: *"No SNMP targets configured"*). |

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

1. **Native Compilation**:
   - Build cleanly with zero compiler warnings via `make -j$(nproc)`.
2. **Capability Preflight**:
   - Verify `./build/netmon status` reports active capture permissions.
3. **Gateway Registration**:
   - Verify connection to `aimon` gateway (`port 3885`) and confirmation of tool registration.
4. **Live MCP Query Verification**:
   - Query `lan_get_devices` via MCP: Verify discovery of unique MACs from ARP cache and live frames.
   - Query `lan_get_unregistered_devices` via MCP: Verify unregistered device identification and OUI resolution.
   - Call `lan_name_device` via MCP: Verify dynamic update and persistence to `devices.cfg`.
   - Query `lan_get_top_talkers` and `lan_get_traffic_summary` via MCP: Verify real-time bandwidth and protocol metrics.
   - Test stub tools (`firewall_get_status`, `snmp_get_device_metrics`): Verify clean `"Not configured"` responses without crashing.

---

## License & Copyright

Copyright (C) 2026, Charles Chiou. All rights reserved.
