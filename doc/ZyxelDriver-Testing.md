<!--
  doc/ZyxelDriver-Testing.md - Zyxel Driver Testing & Risk Classification Matrix

  Copyright (C) 2026, Charles Chiou
-->

# Zyxel Security Gateway Driver: Testing & Risk Classification Matrix

This document provides the authoritative classification of all Zyxel ZySH CLI commands supported or evaluated by `netmon`. It details which commands are actively qualified by CppUTest (both live on physical hardware and offline via hermetic parsers) and which commands are strictly excluded from write execution due to risk of system damage, gateway lockouts, or operational disruption.

---

## 1. Core Testing Philosophy & Safety Invariants

Testing edge security automation against production gateway hardware (such as the Zyxel USG FLEX 200 running ZyWALL ZLD firmware V5.43) demands strict safety constraints. An errant command or incomplete rollback can sever edge network connectivity, corrupt routing tables, wear out physical flash memory, or lock out administrative access.

To ensure continuous, safe testing in production and development environments, `netmon` enforces three immutable testing invariants:

### 1.1 The Zero-Mock Live Qualification Standard
Unit test mocks and simulated transcripts are valuable for basic parser validation, but they cannot prove physical reliability against firmware nuances (e.g. VT100 escape codes, submode transitions, PTY line buffering, latency under load). All live diagnostic test cases (`LiveFirewallReadDiag`, `LiveFirewallWriteDiag`) execute exclusively over an active, authenticated SSH channel (`ZyxelDriver::getInstance().getSshClient()`) against physical gateway hardware.

### 1.2 No flash save
The ZySH `write` command is not sent by netmon and is prohibited in every test. Netmon does not journal router commands and does not replay them.

### 1.3 Diagnostic writes
`firewall diag` with no mode, and read mode, send no configuration. `firewall diag write` must restore each object it creates. That restore is not in this build yet. Do not run `firewall diag write` on the router until it is. A failed production command stops and is not undone.

---

## 2. Master Command & Testing Matrix

The table below catalogs every command implemented, tested, or evaluated within `netmon`, along with its CppUTest qualification status and safety rationale.

### Status Legend
- **Live Physical (PASS)**: Executed and verified live on physical hardware (`USG FLEX 200`) in `src/ZyxelLiveDiagnostic.cxx`.
- **Offline Unit (PASS)**: Hermetically verified via command generators and canned manual transcripts in `test/TestZyxel*Cmd.cxx`.
- **Syntax Rejection Only**: Verified by sending invalid parameters to confirm `% Parse error` / rejection with zero router mutation.
- **EXCLUDED (No Live Write)**: Strictly prohibited from live write testing due to risk of hardware damage, lockout, or network outage.

| Command / CLI Syntax | Subsystem | Access Mode | CppUTest Test Suite / Case | Test Scope | Rollback Action (If Mutating) | Risk Classification & Safety Rationale |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `show version` | System | Read | `LiveFirewallReadDiag.ReadVersion`<br>`ZyxelSystemCmdTest.ParseVersion*` | Live Physical + Offline Unit | None (Read-only) | **Zero Risk**: Informational query extracting model, firmware, and boot status. |
| `show cpu status` | System | Read | `LiveFirewallReadDiag.ReadCpuStatus`<br>`ZyxelSystemCmdTest.ParseCpuStatus*` | Live Physical + Offline Unit | None (Read-only) | **Zero Risk**: Querying real-time CPU utilization. |
| `show mem status` | System | Read | `LiveFirewallReadDiag.ReadMemStatus`<br>`ZyxelSystemCmdTest.ParseMemStatus*` | Live Physical + Offline Unit | None (Read-only) | **Zero Risk**: Querying real-time RAM utilization. |
| `show conn status` | System | Read | `LiveFirewallReadDiag.ReadConnStatus`<br>`ZyxelSystemCmdTest.ParseConnStatus*` | Live Physical + Offline Unit | None (Read-only) | **Zero Risk**: Querying active session table count and capacity. |
| `ping <host> count <c>` | Diagnostic | Exec / Read | `LiveFirewallReadDiag.PingGateway`<br>`ZyxelSystemCmdTest.ParsePing*` | Live Physical + Offline Unit | None (Ephemeral probe) | **Low Risk**: Lightweight ICMP echo probe (bounded count = 2). |
| `traceroute <host>` | Diagnostic | Exec / Read | `ZyxelSystemCmdTest.ParseTraceroute*` | Offline Unit Only | None (Ephemeral probe) | **Medium Risk (Latency)**: Excluded from live automated suites due to 30–60s blocking probe latency. |
| `write` | Storage | Write (NVRAM) | `ZyxelSystemCmdTest.CommandGenerators` | **EXCLUDED (No Live Write)** | N/A | **HIGH RISK (Flash Wear)**: Commits volatile state to persistent eMMC flash. Excluded to prevent flash degradation and persistent test residue. |
| `reboot` | System Control | Destructive | `ZyxelSystemCmdTest.CommandGenerators` | **EXCLUDED (No Live Write)** | N/A | **CRITICAL RISK (Outage)**: Reboots physical hardware, drops WAN/LAN connections, terminates active SSH session. |
| `show interface summary all` | Interface | Read | `LiveFirewallReadDiag.ReadInterfaceSummary`<br>`ZyxelNetworkCmdTest.ParseInterfaces*` | Live Physical + Offline Unit | None (Read-only) | **Zero Risk**: Telemetry collection for link status, IP, netmask, MTU, and MAC. |
| `show interface <name>` | Interface | Read | `ZyxelNetworkCmdTest.CommandGenerators` | Offline Unit Only | None (Read-only) | **Zero Risk**: Per-interface detail query. |
| `interface <id> ip address ...` | Interface | Write | None | **EXCLUDED (No Live Write)** | N/A | **CRITICAL RISK (Severed Link)**: Modifying gateway IP addresses will immediately disconnect the management SSH channel. |
| `interface <id> [no] shutdown` | Interface | Write | None | **EXCLUDED (No Live Write)** | N/A | **CRITICAL RISK (Link Down)**: Disabling physical interfaces drops live network traffic and WAN carrier. |
| `show ip route-settings` | Routing | Read | `LiveFirewallReadDiag.ReadIpRoutes`<br>`ZyxelNetworkCmdTest.ParseIpRoutes*` | Live Physical + Offline Unit | None (Read-only) | **Zero Risk**: Reads active forwarding table, gateways, and metrics. |
| `ip route <dest> <mask> <gw> <m>` | Routing | Write | `ZyxelNetworkCmdTest.CommandGenerators` | **EXCLUDED (No Live Write)** | N/A | **HIGH RISK (Routing Loop)**: Adding static routes can blackhole traffic, cause loops, or hijack the default route. Rollback is non-trivial if daemon recomputes paths. |
| `no ip route <dest> <mask> <gw>` | Routing | Write | `ZyxelNetworkCmdTest.CommandGenerators` | **EXCLUDED (No Live Write)** | N/A | **HIGH RISK (Routing Loss)**: Deleting routes can sever gateway reachability. |
| `show zone` | Network | Read | `ZyxelNetworkCmdTest.ParseZones*` | Offline Unit Only | None (Read-only) | **Zero Risk**: Reads security zone mappings (WAN, LAN, DMZ). |
| `show arp-table` | Network | Read | `ZyxelNetworkCmdTest.ParseArp*` | Offline Unit Only | None (Read-only) | **Zero Risk**: Reads ARP cache table entries. |
| `show address-object` | Object | Read | `LiveFirewallReadDiag.ReadAddressObjects`<br>`ZyxelObjectCmdTest.ParseAddressObjects*` | Live Physical + Offline Unit | None (Read-only) | **Zero Risk**: Reads all configured host, range, and subnet address objects. |
| `show service-object` | Object | Read | `LiveFirewallReadDiag.ReadServiceObjects`<br>`ZyxelObjectCmdTest.ParseServiceObjects*` | Live Physical + Offline Unit | None (Read-only) | **Zero Risk**: Reads all configured custom and standard TCP/UDP service objects. |
| `show object-group address` | Object | Read | `ZyxelObjectCmdTest.ParseAddressGroups*` | Offline Unit Only | None (Read-only) | **Zero Risk**: Reads address object group memberships. |
| `show object-group service` | Object | Read | `ZyxelObjectCmdTest.CommandGenerators` | Offline Unit Only | None (Read-only) | **Zero Risk**: Reads service object group memberships. |
| `address-group <grp> add <mem>` | Object | Write | `ZyxelObjectCmdTest.CommandGenerators` | **EXCLUDED (No Live Write)** | `no address-group <grp> <mem>` | **MEDIUM RISK (Group Mutation)**: Excluded from live test to prevent mutating production groups (e.g. `Blocked_Hosts`) or creating dangling dependencies. |
| `show secure-policy` | Firewall | Read | `LiveFirewallReadDiag.ReadFirewallRules`<br>`ZyxelFirewallCmdTest.ParseSecurePolicy*` | Live Physical + Offline Unit | None (Read-only) | **Zero Risk**: Reads packet filter rules, priorities, zones, and status. |
| `secure-policy default-action deny` | Firewall | Write | None | **EXCLUDED (No Live Write)** | N/A | **CRITICAL RISK (Complete Lockout)**: Changes global firewall policy to drop unmatched packets, breaking all communication. |
| `secure-policy insert <pos>` (General) | Firewall | Write | `ZyxelFirewallCmdTest.CommandGenerators` | **EXCLUDED (No Live Write)** | N/A | **HIGH RISK (Traffic Drop)**: Arbitrary rule insertions can intercept management traffic or break routing between zones. |
| `show ip virtual-server` | NAT | Read | `LiveFirewallReadDiag.ReadVirtualServers`<br>`ZyxelNatCmdTest.ParseVirtualServers*` | Live Physical + Offline Unit | None (Read-only) | **Zero Risk**: Reads active port forwarding / virtual server table. |
| Global SNAT / 1:1 NAT Configuration | NAT | Write | None | **EXCLUDED (No Live Write)** | N/A | **HIGH RISK (WAN Outage)**: Mutating live WAN NAT pools breaks outbound connectivity for all LAN clients. |
| `show app-statistics summary` | Security | Read | `TestZyxelSecurityCmd` | Offline Unit Only | None (Read-only) | **Zero Risk**: Reads Application Patrol traffic analytics. |
| `show idp-statistics summary` | Security | Read | `TestZyxelSecurityCmd` | Offline Unit Only | None (Read-only) | **Zero Risk**: Reads IDP / IPS attack detection statistics. |
| UTM / AV / IDP / Content Filter Profiles | Security | Write | None | **EXCLUDED (No Live Write)** | N/A | **HIGH RISK (Inspection Overhead)**: Altering deep packet inspection profiles causes policy re-indexing and packet drops. |
| `user <admin> password ...` | Admin | Write | None | **EXCLUDED (No Live Write)** | N/A | **CRITICAL RISK (Auth Lockout)**: Modifying admin credentials breaks driver automation and locks out the operator. |
| `ip http/https port <p>` | Admin | Write | None | **EXCLUDED (No Live Write)** | N/A | **HIGH RISK (Management Lockout)**: Alters web management listening ports. |
| `ip ssh port <p>` | Admin | Write | None | **CRITICAL RISK (Channel Severed)** | **EXCLUDED (No Live Write)** | N/A | Changing SSH port severs the active driver connection permanently. |
| `device-ha ...` | High Avail | Write | None | **EXCLUDED (No Live Write)** | N/A | **HIGH RISK (Split-Brain)**: Triggering device failover or sync operations disrupts cluster state. |
---

## 3. Diagnostic writes

Read diagnostics stay in the catalog above. `firewall diag write` is not a license to leave objects on the router. Each write must be followed by a restore to the state before that command. That behavior is not in this build yet.

---

## 4. Excluded Command Categories & Risk Analysis

The overwhelming majority of write commands documented across the 666-page ZySH manual are permanently excluded from live automated test execution. The table below details why these categories are excluded and the exact failure modes they induce.

```text
+--------------------------------------------------------------------------------------------------+
|                            EXCLUDED HIGH-RISK COMMAND CATEGORIES                                 |
+--------------------------+-------------------------------------+---------------------------------+
| Category                 | Example Commands                    | Hazard / Failure Mode           |
+--------------------------+-------------------------------------+---------------------------------+
| System Lifecycle         | reboot, reset to default, boot      | Hardware shutdown, WAN carrier  |
|                          |                                     | loss, SSH session severed.      |
+--------------------------+-------------------------------------+---------------------------------+
| Persistent Storage       | write, erase, copy ...              | Flash memory wear, permanent    |
|                          |                                     | corruption of startup-config.   |
+--------------------------+-------------------------------------+---------------------------------+
| Physical Interfaces      | interface ge1 shutdown, ip address  | Link drop, loss of management   |
|                          | vlan 10, bridge-group               | connectivity to edge gateway.   |
+--------------------------+-------------------------------------+---------------------------------+
| Routing & Forwarding     | ip route 0.0.0.0, router ospf/bgp   | Routing loops, blackholed LAN,  |
|                          | default gateway alterations         | complex non-trivial rollbacks.  |
+--------------------------+-------------------------------------+---------------------------------+
| Global Security Policy   | secure-policy default-action deny   | Instant lockout of all internal |
|                          | inserting broad drop rules          | LAN traffic and ZyWALL control. |
+--------------------------+-------------------------------------+---------------------------------+
| Administrator Access     | user admin password, ip ssh port    | Automated driver locked out,    |
|                          | aaa authentication, radius-server   | physical console recovery need. |
+--------------------------+-------------------------------------+---------------------------------+
| Deep Packet Inspection   | app-patrol profile, anti-virus      | Policy compilation CPU spikes,  |
|                          | content-filter, idp rule            | packet dropping, cloud license. |
+--------------------------+-------------------------------------+---------------------------------+
| High Availability        | device-ha, device-ha-pro failover   | Split-brain clusters, sync lock.|
+--------------------------+-------------------------------------+---------------------------------+
| Long-Running Diagnostics | extended traceroute, packet-trace   | SSH timeout, buffer exhaustion. |
+--------------------------+-------------------------------------+---------------------------------+
```

### 4.1 Detailed Failure Mode Analyses

#### 1. System Lifecycle & Reboot Operations (`reboot`, `reset to default`)
- **Impact**: Catastrophic edge outage.
- **Rationale**: Initiating a reboot terminates all active PPPoE WAN sessions, severs the SSH automation channel, and interrupts production Internet access for all local hosts. Factory reset commands permanently destroy running configurations and cryptographic certificates, requiring manual physical serial console re-flashing.

#### 2. Flash Memory Writes (`write`)
- **Impact**: Physical component degradation & configuration contamination.
- **Rationale**: Edge gateways utilize embedded eMMC or NAND flash with limited write-erase cycle endurance. Running continuous automated regression suites that issue `write` commands would accelerate flash memory wear. Furthermore, committing test state to flash risks saving transient QA objects into `startup-config`, contaminating the certified base configuration.

#### 3. Interface & Physical Link Control (`interface <id> ...`, `shutdown`)
- **Impact**: Immediate loss of control plane.
- **Rationale**: Modifying interface IP addresses, MTU settings, VLAN tags, or issuing `shutdown` on physical ports (`ge1`..`ge6`) can sever the SSH link between the `netmon` daemon and the gateway. Once the link is down, automated rollback is impossible because the driver can no longer transmit commands.

#### 4. Routing & Forwarding Alterations (`ip route ...`, Dynamic Routing)
- **Impact**: Packet blackholing and routing instability.
- **Rationale**: Unlike simple object creation, routing changes alter the kernel forwarding plane across all interfaces. A misconfigured route or metric can cause routing loops, drop default gateway reachability, or trigger dynamic routing flap dampers (OSPF/RIP). Reversing route changes requires multi-step state reconciliations that are error-prone under load.

#### 5. Global Policy Mutations (`secure-policy default-action deny`)
- **Impact**: Total network isolation.
- **Rationale**: The USG FLEX 200 security policy engine processes rules sequentially. Altering default zone-to-zone actions or inserting broad deny rules above management rules drops SSH traffic instantly. If the connection drops during rule insertion, the driver cannot execute compensating rollbacks.

#### 6. Admin Account & Access Port Configuration (`user ...`, `ip ssh port ...`)
- **Impact**: Permanent authentication lockout.
- **Rationale**: Changing administrator credentials, SSH daemon port bindings, or AAA server configurations will instantly cause subsequent SSH handshakes to fail. Recovering from an administrative lockout requires physical serial console access with special credentials.

---

## 5. Automated Regression & Benchmark SLAs

In addition to correctness assertions, all tests executed on live hardware are bound to physical latency service level agreements (SLAs). Commands exceeding these SLAs are flagged as performance regressions on the terminal scorecard:

```text
+--------------------------------------------------------------------------+
| LIVE FIREWALL QUALIFICATION & BENCHMARK REPORT                           |
+--------------------------------------------------------------------------+
| Target Host : 192.0.2.1 (USG FLEX 200)                                   |
| Timestamp   : 2026-10-06 18:20:00                                        |
+--------------------------------------------------------------------------+
| Test Case            | RTT (ms) | Parse(us) | Bytes | Yield | Result     |
+----------------------+----------+-----------+-------+-------+------------+
| ReadVersion          |    184.2 |       210 |   312 |     1 | PASS (SLA) |
| ReadCpuStatus        |    142.6 |        95 |   118 |     1 | PASS (SLA) |
| ReadMemStatus        |    139.1 |        88 |    94 |     1 | PASS (SLA) |
| ReadConnStatus       |    155.0 |       140 |   180 |   933 | PASS (SLA) |
| ReadInterfaceSummary |    412.8 |       420 |  1840 |     8 | PASS (SLA) |
| ReadIpRoutes         |    389.4 |       380 |  1420 |    14 | PASS (SLA) |
| ReadAddressObjects   |    510.2 |       890 |  4210 |    52 | PASS (SLA) |
| ReadServiceObjects   |    485.6 |       760 |  3890 |    48 | PASS (SLA) |
| ReadFirewallRules    |    698.1 |      1240 |  8940 |    30 | PASS (SLA) |
| ReadVirtualServers   |    420.3 |       610 |  2410 |    13 | PASS (SLA) |
| PingGateway          |   1998.4 |       180 |   350 |     1 | PASS (SLA) |
| WriteAddressHostRoll |   1120.5 |         0 |   120 |     1 | PASS (SLA) |
| WriteAddressRangeRol |   1145.2 |         0 |   125 |     1 | PASS (SLA) |
| WriteAddressSubnetRo |   1138.9 |         0 |   122 |     1 | PASS (SLA) |
| WriteServiceTcpRollb |   1115.4 |         0 |   118 |     1 | PASS (SLA) |
| WriteServiceUdpRollb |   1122.1 |         0 |   118 |     1 | PASS (SLA) |
| WriteVirtualServerRo |   2450.8 |         0 |   410 |     1 | PASS (SLA) |
| WriteFastDenyRuleRol |   2890.1 |         0 |   580 |     1 | PASS (SLA) |
| DriverBlockIpAndUnbl |   2945.6 |         0 |   610 |     1 | PASS (SLA) |
| ConfigModeWrapping   |   1820.3 |         0 |   240 |     1 | PASS (SLA) |
| SyntaxErrorUnwind    |    890.2 |         0 |   180 |     1 | PASS (SLA) |
+----------------------+----------+-----------+-------+-------+------------+
| SUMMARY: 21 passed, 0 failed, 0 regressions (Total: 21.80s)              |
+--------------------------------------------------------------------------+
```

### Empirical Physical SLA Thresholds
- **Simple Telemetry Queries** (`show version`, `show cpu status`, `show mem status`, `show conn status`): $\le 1500\text{ ms}$
- **Medium Table Reads** (`show interface summary all`, `show ip route-settings`, `show virtual-server`): $\le 2500\text{ ms}$
- **Large Configuration Tables** (`show address-object`, `show service-object`, `show secure-policy` with $>30$ rules): $\le 3500\text{ ms}$
- **Active ICMP Diagnostics** (`ping` count 2): $\le 5000\text{ ms}$
- **Single-Object Write & Rollback** (`address-object`, `service-object`): $\le 4500\text{ ms}$
- **Multi-Object Compound Write & Rollback** (`virtual-server` + service, Fast-Deny rule + host): $\le 6000\text{ ms}$

---

## 6. Audit & Safety Maintenance Protocol

Whenever proposing new CppUTest cases for Zyxel commands:

1. **Verify Read vs. Write Nature**: If the command is read-only, add full live qualification in `LiveFirewallReadDiag` and canned offline parser qualification in `TestZyxel*Cmd.cxx`.
2. **Evaluate Write Reversibility**:
   - Is the mutation trivially undoable with a single `no ...` command?
   - Can the undo be guaranteed without complex state tracking?
   - Does it target an isolated RFC 5737 dummy IP address?
   - If the answer to any of these is **NO**: **DO NOT write a live mutation test.** Restrict the test strictly to **syntax error rejection** (verifying `% Parse error` with zero state mutation).
3. **Never Issue `write` to Flash**: Automated tests must never commit changes to NVRAM.
4. **Maintain Referential Integrity**: Ensure teardown routines delete dependent rules before deleting referenced address or service objects.
5. **Update This Matrix**: Document any new command in this matrix with its exact qualification status, rollback command, and risk classification.

<!--
  Local variables:
  mode: markdown
  End:
-->
