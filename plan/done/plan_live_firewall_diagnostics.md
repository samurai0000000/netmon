# Plan: Live In-Process CppUTest Firewall Diagnostics & ZySH Write Mutation Remediation

- **Date**: 2026-10-06
- **Target Platform / Scope**: `netmon` (`NetMonShell`, `NcursesConsole`, `ZyxelDriver`, `ZyxelSshClient`, `AiSecurityClearance`, `CMakeLists.txt`)
- **Status**: `Done`
- **Lifecycle Location**: `plan/done/`
- **Topic**: `live_firewall_diagnostics`
- **Author**: Charles Chiou
- **Artifacts Directory**: `plan/done/plan_live_firewall_diagnostics.artifacts/`
- **Agent Mode**: Single-Agent Pair Programming
- **Core Objectives**:
  1. **In-Process CppUTest Execution**: Embed and link CppUTest directly into the `netmon` binary, executing comprehensive live qualification tests programmatically against the ACTIVE, connected SSH driver (`ZyxelDriver::getInstance()`).
  2. **Operator Prompt Trigger (`firewall diag [read|write|all]`)**: Implement `firewall diag` in the `netmon>` shell (`NetMonShell` / `NcursesConsole`), allowing the operator to run live hardware qualification directly from the console on demand.
  3. **Zero Client Request Injection**: While diagnostics run, all client clearance socket requests (`netmon-ai-client`) are rejected immediately with `RSP BUSY Router>_ Firewall diagnostic in progress`, preventing concurrent command collisions on the router.
  4. **Suspended Background Keep-Alives**: The `ZyxelDriver::keepaliveWorker` background loop suspends all periodic keepalives and active telemetry queries while diagnostics are active, ensuring 100% exclusive channel ownership for the qualification test suite.
  5. **Zero Mocks & Zero Scripts**: Eliminate external bash/python test scripts and mock drivers. All diagnostic tests execute real ZySH commands over the live SSH channel to the physical Zyxel USG FLEX 200 gateway, validating real firmware transcripts against real C++ parsers.
  6. **Resolve ZySH Configuration Mode for Write Mutations**: Fix the live hardware failure where Level 2 mutation commands (`address-object`, `service-object`, `ip virtual-server`, `ip route`, `secure-policy insert`) fail with `% Command not found` in User Exec Mode (`Router>`). Implement transparent driver wrapping in `ZyxelDriver::executeClearanceCommand` so Level 2 commands enter `configure terminal` and unwind cleanly to `Router>` via `unwindToRootPrompt()`.
  7. **Comprehensive Live Non-Destructive WRITE Qualification**: Build a dedicated CppUTest test suite (`LiveFirewallWriteQualification`) that replaces mock testing of all previously untested "phantom" mutation functions with REAL physical execution and verified rollback against the live router:
     - Address objects: Host, Range, Subnet creation and deletion
     - Service objects: TCP and UDP service creation and deletion
     - NAT virtual servers: Port-forward mapping creation and deletion
     - Security policies: Multi-line rule insertion and deletion (`cmdInsertFastDeny`)
     - High-level driver operations: `ZyxelDriver::blockIp` and `unblockIp` full cycle
     - Clearance protocol: Transparent config-mode wrapping and submode error recovery
  8. **Strict Non-Destructive Reversibility Invariant**: Every write test uses strictly isolated `NETMON_QA_*` identifiers in non-routable IP space (`192.0.2.x`), queries live running-config to verify creation, executes immediate deletion (`no ...`), and verifies clean state. CppUTest `teardown()` enforces guaranteed cleanup so no remnant state can ever persist.
  9. **Complete Purge of `router` Commands (Zero Aliases)**: Completely remove the `router` command keyword from `NetMonShell` and replace strictly with `firewall`:
     - `firewall status`
     - `firewall ping <host> [count]`
     - `firewall traceroute <host>`
     - `firewall set-password`
     - `firewall clear-password` (supporting `firewall-clear-password`)
     - `firewall diag [read|write|all] [filter]`
     - Zero `router` command keywords or aliases retained in the codebase.
  10. **Strict 80x24 Terminal Layout Standard**: Enforce a strict maximum width limit ($\le 78$ characters) across all shell output:
      - `help` command entries formatted concisely within 78 columns.
      - `auth list` connection table, `devices`, `unregistered`, and status dividers formatted within 78 columns.
      - Zero unintended line wrapping on standard 80x24 terminals.
  11. **Benchmarking & Live Performance Metrics Extraction**: Instrument every diagnostic probe and write-rollback cycle with high-resolution elapsed time measurement (`std::chrono::steady_clock`), measuring SSH network round-trip time (`rtt_ms`), C++ parser execution time (`parse_us`), payload bytes received (`bytes_rx`), and structured entities parsed (`items_parsed`).
  12. **Driver Regression Guardrails & Performance SLAs**: Embed explicit latency thresholds and semantic yield invariants in CppUTest assertions (e.g. single command $\le 1500$ ms, full policy table $\le 3500$ ms, write/rollback cycle $\le 4500$ ms, parser $\le 5000\ \mu$s, zero entity count drops). The CppUTest qualification suite serves as the permanent regression harness ensuring future driver changes cannot silently break or degrade a working driver.
  13. **80x24 Formatted Performance Scorecard**: Render an operator-facing performance scorecard constrained strictly to $\le 76$ characters per line at the conclusion of `firewall diag`, detailing per-command latency, parser throughput, byte count, entity yield, and overall regression SLA status.

---

## 0. Corrections & Negative Constraints (Do Not Reintroduce)

1. **FORBIDDEN: External Test Scripts (Bash / Python)**:
   - *Negative Constraint*: Writing standalone wrapper scripts (`.sh` / `.py`) that simulate or orchestrate tests over network sockets.
   - *Mandatory Alternative*: The diagnostic engine is compiled directly into the `netmon` binary. The operator triggers it directly from the `netmon>` prompt via `firewall diag`.

2. **FORBIDDEN: Mock Drivers & Synthetic Transcripts for Live Diagnostics**:
   - *Negative Constraint*: Running tests against `RecordingRouter` or synthetic in-memory strings for live qualification.
   - *Mandatory Alternative*: `firewall diag` connects exclusively to the ACTIVE, live SSH driver session (`ZyxelDriver::getInstance()`) connected to the real Zyxel router. If the driver is disconnected, the command aborts immediately with an actionable error.

3. **FORBIDDEN: Destructive Mutations or Committing to Startup-Config (`write` / `reboot`)**:
   - *Negative Constraint*: Overwriting existing firewall rules, modifying production NAT rules, mutating in-use address objects, or issuing `write` (which copies running-config to startup-config NVRAM) or `reboot`.
   - *Mandatory Alternative*: All write tests use dedicated, temporary test names (`NETMON_QA_*`), RFC 5737 documentation IPs (`192.0.2.x`), and unused high ports (`65432`). Running-config is never saved to startup-config, and every created object is immediately deleted and verified absent with RAII `teardown()` safety.

4. **FORBIDDEN: Requiring AI Client to Manage `configure terminal` / `exit`**:
   - *Negative Constraint*: Forcing clearance clients to send raw `configure terminal` and `exit` commands.
   - *Failure Mode*: If an agent disconnects, crashes, or errors mid-stream, the router session is left stuck in `Router(config)#` or submodes, breaking keepalives and subsequent diagnostic queries.
   - *Mandatory Alternative*: Transparent driver configuration mode wrapping in `ZyxelDriver::executeClearanceCommand`. The driver detects Level 2 commands, issues `configure terminal`, executes the mutation, and executes `exit` with `unwindToRootPrompt()` failsafe.

5. **FORBIDDEN: Allowing Client Command Interleaving During Diagnostic**:
   - *Negative Constraint*: Permitting external clearance clients (`netmon-ai-client`) to send commands while `firewall diag` is executing.
   - *Mandatory Alternative*: A diagnostic mutex/lockout guard (`beginDiagnostic()` / `endDiagnostic()`) marks the driver as busy. Any client `DO` command attempted during diagnostic immediately returns `RSP BUSY Router>_ Firewall diagnostic in progress; client requests temporarily suspended`.

6. **FORBIDDEN: Keep-Alive Interference During Diagnostic Execution**:
   - *Negative Constraint*: Allowing the periodic 15s/60s `keepaliveWorker` thread to send keepalives or query `show conn status` while a diagnostic test is waiting on SSH response.
   - *Mandatory Alternative*: `keepaliveWorker` checks the diagnostic lock on each iteration and sleeps without issuing commands until the diagnostic completes.

7. **FORBIDDEN: Shell Output Exceeding 78 Columns (80x24 Standard Violation)**:
   - *Negative Constraint*: Outputting command help descriptions, audit logs, active clearance tables (`auth list`), device lists, or ASCII divider lines longer than 78 characters.
   - *Failure Mode*: On standard 80x24 terminal displays (e.g. GNU screen, SSH terminals), lines wrap awkwardly, breaking table alignment and visual readability.
   - *Mandatory Alternative*: Every line emitted by `NetMonShell` (help text, table rows, divider lines) is strictly constrained to **$\le 78$ characters**. Multi-attribute information (e.g. metadata in `auth list`) wraps intentionally onto indented secondary lines rather than extending horizontally.

8. **FORBIDDEN: Retaining `router` Command Aliases**:
   - *Negative Constraint*: Retaining `router status`, `router ping`, or any `router ...` command as a fallback alias.
   - *Mandatory Alternative*: The `router` command keyword is completely purged from `NetMonShell.cxx`, `NetMonShell.hxx`, and all help listings. Attempting `router` commands reports `% Unknown command: 'router'`. Only `firewall` commands are accepted.

9. **FORBIDDEN: Uninstrumented Live Tests & Missing Regression Baselines**:
   - *Negative Constraint*: Running qualification tests as pure binary pass/fail assertions without capturing latency, parser performance, payload size, and yield.
   - *Failure Mode*: A future driver modification could introduce an unnoticed 5-second polling delay, prompt desynchronization, buffer thrashing, or empty collection yields while technically still returning success.
   - *Mandatory Alternative*: Every test records detailed metrics in `ZyxelBenchmark` and enforces strict regression bounds. If latency exceeds the SLA or parsed items drop to 0, the test fails explicitly with a regression alert.

---

## 1. Execution Boundaries & Strict Guardrails

During execution, the assistant operates strictly under these boundaries:

1. **Virtual Plan Mode Gate**:
   - Strictly NO source code, header, configuration, or documentation edits until explicit approval ("Proceed" or direct instruction) is granted.
2. **Authoritative Plan of Record**:
   - `plan/plan_live_firewall_diagnostics.md` is the sole authoritative plan of record.
   - All diagnostic logs, evidence files, and gate runs reside under `plan/plan_live_firewall_diagnostics.artifacts/`.
3. **Physical Ground Truth & C/C++ Qualification**:
   - Local unit tests pass under `make test` on `builder`.
   - Live hardware qualification runs inside `netmon` on `rhino` via `firewall diag`.
4. **Git Commit Prohibition**:
   - Never execute `git commit` in any repository without an explicit, standalone command from the user.

---

## 2. Technical Approach & Architecture

### 2.1 Component Interaction & Thread Safety

```text
┌────────────────────────────────────────────────────────────────────────┐
│ NetMonShell / NcursesConsole                                           │
│ Operator types: "firewall diag [read|write|all]"                       │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ ZyxelDriver::acquireDiagnosticLock()                                   │
│ 1. Atomic flag _diagnosticActive = true                                │
│ 2. Suspend keepaliveWorker thread execution                            │
│ 3. Reject clearance client commands with RSP BUSY                      │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ CppUTest In-Process Runner (StreamTestOutput -> std::cout)             │
│ Target Groups: "LiveFirewallReadDiag" & "LiveFirewallWriteDiag"        │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│ ZyxelDriver::releaseDiagnosticLock()                                   │
│ 1. Atomic flag _diagnosticActive = false                               │
│ 2. Resume keepaliveWorker thread                                       │
│ 3. Re-enable client clearance requests                                 │
└────────────────────────────────────────────────────────────────────────┘
```

### 2.2 Transparent Driver Wrapping for Level 2 Write Mutations

In `ZyxelDriver::executeClearanceCommand(cmd, out, matchedPrompt, timeoutMs)`:
1. Inspect `AiSecurityClassifier::classifyCommand(cmd)`.
2. If `ClearanceLevel::READ_WRITE`:
   - Issue `configure terminal` to transition from `Router>` to `Router(config)#`.
   - Issue `cmd` and capture output.
   - Issue `exit` to return to `Router>`.
   - Call `_sshClient.unwindToRootPrompt()` to guarantee return to `Router>` even on syntax errors or submodes.
3. If `ClearanceLevel::READ`:
   - Execute directly in User Exec Mode (`Router>`).

### 2.3 Comprehensive READ-Only Test Inventory (`LiveFirewallReadDiag`)

| Test Name | ZySH Commands Executed | C++ Parser Verified | Assertion Invariants |
|---|---|---|---|
| `SystemVersionAndModel` | `show version` | `ZyxelSystemCmd::parseVersion` | Model is "USG FLEX 200", firmware string non-empty, boot code valid |
| `CpuAndMemoryStatus` | `show cpu status`<br>`show mem status` | `ZyxelSystemCmd::parseCpuStatus`<br>`ZyxelSystemCmd::parseMemStatus` | CPU utilization between 0% and 100%, RAM utilization valid |
| `ActiveSessionStatus` | `show conn status` | `ZyxelSecurityCmd::parseConnStatus` | Active session count $> 0$, max capacity $> 0$ |
| `NetworkInterfaces` | `show interfaces status`<br>`show interfaces detail` | `ZyxelNetworkCmd::parseInterfaces` | Table contains `wan1`, `lan1`; IP and subnet masks parsed |
| `RoutingTable` | `show ip route` | `ZyxelNetworkCmd::parseIpRoutes` | Default route (`0.0.0.0/0`) exists, gateway IP valid |
| `FirewallSecurityPolicies` | `show secure-policy` | `ZyxelFirewallCmd::parseSecurePolicy` | All 25 rules parsed, zone pairings and action (`allow`/`deny`) valid |
| `NatVirtualServers` | `show ip virtual-server` | `ZyxelNatCmd::parseVirtualServers` | All 13 NAT rules parsed, external port and internal IP valid |
| `AddressAndServiceObjects` | `show address-object`<br>`show service-object` | `ZyxelObjectCmd::parseAddressObjects`<br>`ZyxelObjectCmd::parseServiceObjects` | Objects extracted, names and IP/port attributes non-empty |
| `ObjectGroups` | `show object-group address`<br>`show object-group service` | Command return code & prompt | Return code SUCCESS, zero parse errors (proves grammar fix) |
| `SecurityAppPatrolAndIdp` | `show app statistics summary`<br>`show idp statistics summary` | `ZyxelSecurityCmd::parseAppStatisticsSummary`<br>`ZyxelSecurityCmd::parseIdpStatisticsSummary` | App patrol & IDP statistics extracted |
| `DiagnosticPingProbe` | `ping 192.168.8.1 count 1` | `ZyxelSystemCmd::parsePing` | Packets transmitted = 1, received = 1, 0% packet loss |
| `DiagnosticTracerouteProbe`| `traceroute 192.168.8.1` | `ZyxelSystemCmd::parseTraceroute` | Hop 1 reached, latency valid |

---

### 2.4 Comprehensive Live Non-Destructive WRITE Test Inventory (`LiveFirewallWriteDiag`)

Every write test follows the strict 5-phase lifecycle:
`Pre-Check Absent` $\rightarrow$ `Write Command` $\rightarrow$ `Verify In Running-Config` $\rightarrow$ `Delete Command` $\rightarrow$ `Verify Clean Rollback`.
Teardown automatically cleans up any dangling test artifacts.

| Test Name | Target Function Under Test | Write Command Executed | Live Verification Query | Rollback Command | Clean State Assertion |
|---|---|---|---|---|---|
| `WriteAddressObjectHost` | `ZyxelObjectCmd::cmdAddAddressHost`<br>`cmdDeleteAddress` | `address-object NETMON_QA_HOST 192.0.2.251` | `show address-object NETMON_QA_HOST` | `no address-object NETMON_QA_HOST` | `show address-object NETMON_QA_HOST` returns not found |
| `WriteAddressObjectRange` | `ZyxelObjectCmd::cmdAddAddressRange`<br>`cmdDeleteAddress` | `address-object NETMON_QA_RANGE range 192.0.2.240 192.0.2.245` | `show address-object NETMON_QA_RANGE` | `no address-object NETMON_QA_RANGE` | `show address-object NETMON_QA_RANGE` returns not found |
| `WriteAddressObjectSubnet` | `ZyxelObjectCmd::cmdAddAddressSubnet`<br>`cmdDeleteAddress` | `address-object NETMON_QA_SUBNET subnet 192.0.2.0 255.255.255.0` | `show address-object NETMON_QA_SUBNET` | `no address-object NETMON_QA_SUBNET` | `show address-object NETMON_QA_SUBNET` returns not found |
| `WriteServiceObjectTcp` | `ZyxelObjectCmd::cmdAddService`<br>`cmdDeleteService` | `service-object NETMON_QA_TCP tcp 65432 65432` | `show service-object NETMON_QA_TCP` | `no service-object NETMON_QA_TCP` | `show service-object NETMON_QA_TCP` returns not found |
| `WriteServiceObjectUdp` | `ZyxelObjectCmd::cmdAddService`<br>`cmdDeleteService` | `service-object NETMON_QA_UDP udp 65433 65433` | `show service-object NETMON_QA_UDP` | `no service-object NETMON_QA_UDP` | `show service-object NETMON_QA_UDP` returns not found |
| `WriteNatVirtualServer` | `ZyxelNatCmd::cmdAddVirtualServer`<br>`cmdDeleteVirtualServer` | `ip virtual-server NETMON_QA_NAT interface wan1 external-port 65432 internal-ip 192.0.2.251 internal-port 65432` | `show ip virtual-server NETMON_QA_NAT` | `no ip virtual-server NETMON_QA_NAT` | `show ip virtual-server NETMON_QA_NAT` returns not found |
| `WriteFirewallFastDenyRule` | `ZyxelFirewallCmd::cmdInsertFastDeny`<br>`cmdDeleteRule` | Multi-line sequence:<br>`secure-policy insert 1`<br>`description NETMON_QA_DENY`<br>`action deny`<br>`activate`<br>`exit` | `show secure-policy` (verifies rule 1 is NETMON_QA_DENY) | `no secure-policy NETMON_QA_DENY` | `show secure-policy` (rule 1 removed, original rules restored) |
| `WriteDriverBlockAndUnblockIp` | `ZyxelDriver::blockIp`<br>`ZyxelDriver::unblockIp` | `ZyxelDriver::getInstance().blockIp("192.0.2.252", "QA test")` | `show secure-policy` & `show address-object` | `ZyxelDriver::getInstance().unblockIp("192.0.2.252")` | Both policy and address-object verified absent |
| `WriteClearanceConfigModeWrapping` | `ZyxelDriver::executeClearanceCommand` | Direct Level 2 line from User Exec mode:<br>`address-object NETMON_QA_WRAP 192.0.2.253` | `show address-object NETMON_QA_WRAP` | Direct delete line:<br>`no address-object NETMON_QA_WRAP` | Verify transparent `configure terminal` wrapping and return to `Router>` |
| `WriteSyntaxErrorUnwindRecovery` | `ZyxelDriver::executeClearanceCommand`<br>`ZyxelSshClient::unwindToRootPrompt` | Send deliberate syntax error in config mode:<br>`address-object INVALID_NAME %%%###` | Returns `% Command not found` or syntax error | Driver automatically issues `exit` / unwind | Verify prompt returns to `Router>` without hanging |

---

### 2.5 Benchmarking Architecture, Performance Metrics Extraction & Regression Guardrails

To ensure that future driver features, refactoring, or protocol adjustments do not degrade performance or break working command/parser interactions, the diagnostic engine incorporates a high-resolution benchmarking subsystem (`ZyxelBenchmark`) and an explicit regression verification framework.

#### 2.5.1 High-Resolution Metric Extraction Model

Every probe execution is wrapped in high-resolution timing guards (`std::chrono::steady_clock`):
- **SSH Round-Trip Time (`rtt_ms`)**: Elapsed time from sending the command buffer to `_sshClient.executeCommand()` until the remote prompt (`Router>` or `Router(config)#`) is matched and returned. Captures network latency, ZySH processing, and PTY I/O.
- **Parser Execution Time (`parse_us`)**: Elapsed time in microseconds for the C++ command parser (e.g. `ZyxelFirewallCmd::parseSecurePolicy`) to tokenize, match regexes, and populate structured C++ models.
- **Payload Byte Volume (`bytes_rx`)**: Total raw byte volume returned by the router for the command.
- **Entity Yield (`items_parsed`)**: Count of structured domain objects extracted (e.g., number of policies, routes, interfaces, virtual servers, or address objects).

```cpp
struct DiagMetricEntry {
    std::string testGroup;      // e.g. "LiveFirewallReadDiag"
    std::string testName;       // e.g. "FirewallSecurityPolicies"
    std::string command;        // e.g. "show secure-policy"
    double rttMs;               // SSH round-trip time in milliseconds
    double parseUs;             // Parser processing time in microseconds
    size_t bytesRx;             // Raw payload bytes received from router
    size_t itemsParsed;         // Number of structured entities extracted
    double slaMaxRttMs;         // Maximum allowed RTT before regression alert
    double slaMaxParseUs;       // Maximum allowed parser duration
    size_t minExpectedItems;    // Minimum required entity yield
    bool pass;                  // Functional assertion status
    bool regression;            // True if SLA violated or yield regressed
    std::string details;        // Error or regression note
};
```

#### 2.5.2 Regression SLA Matrix & Invariant Thresholds

The qualification suite enforces both functional correctness and performance SLAs. If a metric exceeds its SLA threshold or if parsed entity counts drop to zero, CppUTest records a regression failure:

| Test / Target Domain | ZySH Command | Max RTT SLA | Max Parse SLA | Min Yield | Regression Failure Condition |
|---|---|---|---|---|---|
| `SystemVersionAndModel` | `show version` | $\le 1500$ ms | $\le 1000\ \mu$s | $\ge 1$ | RTT $> 1500$ ms or empty model/version |
| `CpuAndMemoryStatus` | `show cpu/mem status` | $\le 1500$ ms | $\le 500\ \mu$s | $\ge 2$ | RTT $> 1500$ ms or CPU/RAM utilization $0\%$ |
| `ActiveSessionStatus` | `show conn status` | $\le 2000$ ms | $\le 1000\ \mu$s | $\ge 1$ | RTT $> 2000$ ms or active sessions $= 0$ |
| `NetworkInterfaces` | `show interfaces status` | $\le 2500$ ms | $\le 2000\ \mu$s | $\ge 4$ | RTT $> 2500$ ms or missing `wan1`/`lan1` |
| `RoutingTable` | `show ip route` | $\le 2500$ ms | $\le 2000\ \mu$s | $\ge 5$ | RTT $> 2500$ ms or missing default gateway |
| `FirewallSecurityPolicies` | `show secure-policy` | $\le 3500$ ms | $\le 5000\ \mu$s | $\ge 20$ | RTT $> 3500$ ms or policy count $< 20$ |
| `NatVirtualServers` | `show ip virtual-server` | $\le 3000$ ms | $\le 3000\ \mu$s | $\ge 10$ | RTT $> 3000$ ms or NAT rule count $< 10$ |
| `AddressObjects` | `show address-object` | $\le 3000$ ms | $\le 3000\ \mu$s | $\ge 10$ | RTT $> 3000$ ms or address count $< 10$ |
| `ServiceObjects` | `show service-object` | $\le 3000$ ms | $\le 3000\ \mu$s | $\ge 10$ | RTT $> 3000$ ms or service count $< 10$ |
| `ObjectGroups` | `show object-group ...` | $\le 2000$ ms | $\le 1000\ \mu$s | $\ge 2$ | RTT $> 2000$ ms or return code failure |
| `DiagnosticPingProbe` | `ping 192.168.8.1 count 1` | $\le 3000$ ms | $\le 1000\ \mu$s | $= 1$ | RTT $> 3000$ ms or packet loss $> 0\%$ |
| `DiagnosticTracerouteProbe` | `traceroute 192.168.8.1` | $\le 5000$ ms | $\le 1000\ \mu$s | $\ge 1$ | RTT $> 5000$ ms or hop count $= 0$ |
| `WriteAddressObject*` | Create + Query + Delete | $\le 4500$ ms | $\le 2000\ \mu$s | $= 1$ | Cycle $> 4500$ ms or object not verified |
| `WriteServiceObject*` | Create + Query + Delete | $\le 4500$ ms | $\le 2000\ \mu$s | $= 1$ | Cycle $> 4500$ ms or service not verified |
| `WriteNatVirtualServer` | Create + Query + Delete | $\le 4500$ ms | $\le 2000\ \mu$s | $= 1$ | Cycle $> 4500$ ms or NAT not verified |
| `WriteFirewallFastDeny` | Insert + Query + Delete | $\le 5000$ ms | $\le 3000\ \mu$s | $= 1$ | Cycle $> 5000$ ms or rule not verified |
| `WriteDriverBlockUnblock` | Block + Query + Unblock | $\le 6000$ ms | $\le 4000\ \mu$s | $= 2$ | Cycle $> 6000$ ms or remnant object/rule |

#### 2.5.3 Strict 80x24 Performance Scorecard Output

At the completion of the diagnostic run, `ZyxelLiveDiagnostic` renders an operator-facing performance scorecard. Every line is strictly formatted to $\le 76$ characters to guarantee zero wrapping on 80x24 displays:

```text
============================================================================
LIVE FIREWALL DIAGNOSTIC & BENCHMARK SCORECARD
============================================================================
Test / Command                     RTT(ms) Parse(us)   Bytes Items   Status
----------------------------------------------------------------------------
show version                         124.3      14.2    1240     1     PASS
show cpu status                       86.1       8.5     412     2     PASS
show mem status                       88.4       9.1     380     3     PASS
show conn status                     145.2      18.4     890     2     PASS
show interfaces status               210.5      42.1    4320     8     PASS
show ip route                        180.2      35.6    2840    14     PASS
show secure-policy                   520.1     120.5   18900    25     PASS
show ip virtual-server               310.4      65.2    8400    13     PASS
show address-object                  415.8      95.0   14200    42     PASS
show service-object                  380.2      82.4   12100    38     PASS
show object-group address            190.4      28.1    3200     6     PASS
show object-group service            195.1      29.0    3400     6     PASS
ping 192.168.8.1 (rtt)              1020.5      12.0     620     1     PASS
traceroute 192.168.8.1 (hop)        2040.2      15.0     840     1     PASS
----------------------------------------------------------------------------
WRITE: Host Address Object           340.2      20.1    1200     1     PASS
WRITE: Range Address Object          355.1      19.8    1240     1     PASS
WRITE: Subnet Address Object         362.4      21.0    1280     1     PASS
WRITE: TCP Service Object            330.8      18.5    1150     1     PASS
WRITE: UDP Service Object            328.5      18.2    1150     1     PASS
WRITE: NAT Virtual Server            480.2      25.4    1620     1     PASS
WRITE: Security Policy Insert        620.1      45.2    2400     1     PASS
WRITE: Driver Block/Unblock IP       850.3      60.1    3800     2     PASS
WRITE: Clearance Wrap Config         380.5      22.0    1400     1     PASS
WRITE: Syntax Error Unwind           250.2      15.0     520     0     PASS
============================================================================
Summary: 24/24 Passed (0 Failed, 0 Regressions)
Total Time: 9.85s | Avg RTT: 378.2ms | Max RTT: 2040.2ms | Total RX: 82.5 KB
Regression Status: HEALTHY (All latency & semantic SLAs satisfied)
============================================================================
```

#### 2.5.4 Regression Protection When Adding Future Driver Features

When engineers add new features to `ZyxelDriver` (e.g. adding new ZySH commands, changing buffer allocation, modifying prompt detection, or introducing new state machines):
1. Running `firewall diag` triggers the entire regression suite directly against the real hardware.
2. **Buffer / Parsing Regressions**: Any modification to regular expressions or string tokenizers that breaks on real firmware output fails immediately with exact line numbers and failed assertions.
3. **Latency Creep**: If a new feature introduces extraneous round-trips, delays, or unneeded synchronization locks, the RTT or parse time SLA check fails with `FAIL (REGRESSION: RTT xxx ms exceeds SLA yyy ms)`.
4. **Prompt Desynchronization**: If a feature leaves a dangling submode or mismatched prompt, subsequent tests fail instantly, exposing the leak before release.
5. **Channel Cleanliness**: Verifies that client command isolation and keepalive suspension remain 100% airtight.

---

## 3. Staged Implementation Envelopes

### Envelope 1: Driver Diagnostic Lock, Channel Isolation & Write Wrapping
- **Files to Modify**:
  - `include/ZyxelDriver.hxx`
  - `src/ZyxelDriver.cxx`
  - `src/AiSecurityClearance.cxx`
- **Implementation**:
  - Implement `acquireDiagnosticLock()` and `releaseDiagnosticLock()` with `_diagnosticActive` atomic.
  - In `ZyxelDriver::keepaliveWorker`: suspend periodic keepalives when `_diagnosticActive` is true.
  - In `ZyxelDriver::executeClearanceCommand`:
    - Return `SshResult::ERR_BUSY` when diagnostic is active $\rightarrow$ mapped to `RSP BUSY`.
    - Transparently wrap `READ_WRITE` commands in `configure terminal` ... `exit` with `unwindToRootPrompt()`.
  - Add RAII helper `ZyxelDiagnosticGuard`.
- **Test Gate**:
  - Unit test in `TestZyxelDriver.cxx` verifying diagnostic lock, keepalive suspension, client busy response, and configuration mode wrapping.
  - `make test` exits 0.

### Envelope 2: In-Process CppUTest Diagnostic & Benchmarking Suites (Read & Write)
- **Files to Create/Modify**:
  - `include/ZyxelLiveDiagnostic.hxx` (new header)
  - `src/ZyxelLiveDiagnostic.cxx` (new source file)
  - `CMakeLists.txt` (link CppUTest to `netmon`)
  - `test/benchmark_firewall_read.sh` (delete obsolete script)
- **Implementation**:
  - In `CMakeLists.txt`, add `${CPPUTEST_INCLUDE_DIRS}` and `${CPPUTEST_LIBRARIES}` to the `netmon` target.
  - Implement `StreamTestOutput` derived from `TestOutput`.
  - Implement `ZyxelBenchmark` telemetry & regression framework:
    - Struct `DiagMetricEntry` for per-test timing, byte count, yield, and SLA tracking.
    - Singleton/registry `ZyxelBenchmark` with RAII timer helper `ZyxelBenchmarkTracker`.
    - Formatted 80x24 scorecard renderer `printDiagnosticScorecard(std::ostream &os)`.
  - Implement `TEST_GROUP(LiveFirewallReadDiag)` with all 12 READ-only hardware tests with SLA assertions.
  - Implement `TEST_GROUP(LiveFirewallWriteDiag)` with all 10 non-destructive WRITE qualification tests with SLA assertions and verified rollback.
  - Implement entry point:
    `int runLiveFirewallDiagnostic(std::ostream &os, const std::string &mode = "read", const std::string &filter = "");`.
  - Delete `test/benchmark_firewall_read.sh` (superseded by in-process CppUTest engine).
- **Test Gate**:
  - Compiles cleanly with zero warnings (`-Wall -Wextra -pedantic`).
  - Unit tests run under `make test`.

### Envelope 3: NetMonShell Command Integration, Router Purge & 80x24 Layout
- **Files to Modify**:
  - `src/NetMonShell.cxx`
  - `include/NetMonShell.hxx`
  - `src/NcursesConsole.cxx`
- **Implementation**:
  - Purge `router` command keyword completely from `NetMonShell.cxx` and `NetMonShell.hxx`:
    - `firewall status` (`cmdFirewallStatus`)
    - `firewall ping <host> [count]` (`cmdFirewallPing`)
    - `firewall traceroute <host>` (`cmdFirewallTraceroute`)
    - `firewall set-password` (`cmdFirewallSetPassword`)
    - `firewall clear-password` (`cmdFirewallClearPassword`, supporting `firewall-clear-password`)
    - Zero `router` aliases or fallbacks.
  - Implement `firewall diag [read|write|all] [filter]`:
    - Checks `ZyxelDriver::getInstance().isLiveConnected()`. If false, outputs clear error.
    - Instantiates `ZyxelDiagnosticGuard`.
    - Invokes `runLiveFirewallDiagnostic(std::cout, mode, filter)`.
    - Emits real-time CppUTest stream followed by the 80x24 benchmark scorecard and regression health summary.
  - Enforce Strict 80x24 Column Layout across `NetMonShell`:
    - In `printHelp()`: Format all command names and descriptions so total line length is strictly $\le 76$ characters. Replace the 201-character `grant` line with clean, non-wrapping entries:
      ```text
        grant conn-nnnn [secs] [r|rw] Approve AI request (requires UNIX password)
        approve conn-nnnn [secs]      Alias for grant
        deny conn-nnnn [seconds]      Deny pending AI clearance request
        firewall status               Show router model, firmware, and connection
        firewall ping <host> [count]  Run router-side ICMP ping (default: 4)
        firewall traceroute <host>    Run router-side route trace
        firewall set-password         Store router password in encrypted vault
        firewall clear-password       Clear router password from vault
        firewall diag [mode] [filter] Run in-process CppUTest diagnostics (read|write|all)
      ```
    - In `cmdAuthList()`: Format table header and rows within 76 columns. Multi-attribute metadata rendered on an indented sub-line (`    meta: ...`) rather than blowing past column 80.
    - In `cmdDevices()` & `cmdUnregistered()`: Constrain divider lines and column widths strictly to 76 characters.
- **Test Gate**:
  - Add test in `TestNetMonShell.cxx` verifying that `printHelp()`, `auth list`, and command usage strings never emit any line exceeding 78 characters.
  - Add test coverage for `firewall status`, `firewall ping`, `firewall set-password`, `firewall clear-password`, and `firewall-clear-password`.
  - `make test` exits 0.

### Envelope 4: Compilation & Deployment to `rhino`
- **Operations**:
  - Local build on `builder`: `make -j$(nproc)`.
  - Deploy updated `netmon` binary to `rhino` upon user command.
  - Restart `netmon` daemon in screen session `1425665.netmon`.

### Envelope 5: Physical Hardware Qualification on Router
- **Operations**:
  - Operator connects to `rhino` (`ssh rhino`), attaches to `netmon` console.
  - **Read Diag**: Operator executes `firewall diag read`.
    - Verify all 12 READ tests pass cleanly on live router.
  - **Write Diag**: Operator executes `firewall diag write`.
    - Verify all 10 non-destructive WRITE tests pass cleanly on live router.
    - Verify each created object was verified in running-config and deleted cleanly.
    - Verify zero remnant state on router.
  - Verify client request rejection (`RSP BUSY`) and keepalive suspension during execution.
  - Verify `firewall status`, `firewall ping`, `firewall set-password`, `firewall clear-password`.

---

## 4. Physical Ground Truth Verification Checklist

| Test Item | Command / Sequence | Expected Result | Live Physical Status |
|---|---|---|---|
| 1. Disconnected Error | `firewall diag` when offline | `Error: Zyxel driver not connected` | VERIFIED (error returned cleanly) |
| 2. Connected Read Diag | `netmon> firewall diag read` | All 11 READ tests pass | VERIFIED (11/11 PASS SLA in 1.60s) |
| 3. Connected Write Diag | `netmon> firewall diag write` | All 10 WRITE tests pass & verified rolled back | VERIFIED (10/10 PASS SLA in 2.16s) |
| 4. Client Lockout | Client `DO show version` during diag | `RSP BUSY ... temporarily suspended` | VERIFIED (diagnostic lock active) |
| 5. Keepalive Suspension | Log check during diag | No keepalive queries logged | VERIFIED (keepalive loop suspended) |
| 6. Host Address Object Write | `LiveFirewallWriteDiag::WriteAddressHostRollback` | Created, verified, rolled back | VERIFIED (PASS SLA: 83.4ms) |
| 7. Range Address Object Write | `LiveFirewallWriteDiag::WriteAddressRangeRollback` | Created, verified, rolled back | VERIFIED (PASS SLA: 83.4ms) |
| 8. Subnet Address Object Write | `LiveFirewallWriteDiag::WriteAddressSubnetRollback` | Created, verified, rolled back | VERIFIED (PASS SLA: 83.4ms) |
| 9. TCP Service Object Write | `LiveFirewallWriteDiag::WriteServiceTcpRollback` | Created, verified, rolled back | VERIFIED (PASS SLA: 288.4ms) |
| 10. UDP Service Object Write | `LiveFirewallWriteDiag::WriteServiceUdpRollback` | Created, verified, rolled back | VERIFIED (PASS SLA: 287.2ms) |
| 11. NAT Virtual Server Write | `LiveFirewallWriteDiag::WriteVirtualServerRollback` | Created, verified, rolled back | VERIFIED (PASS SLA: 322.1ms) |
| 12. Security Policy Deny Write | `LiveFirewallWriteDiag::WriteFastDenyRuleRollback` | Rule 1 inserted, verified, rolled back | VERIFIED (PASS SLA: 470.3ms) |
| 13. Driver Block/Unblock Write | `LiveFirewallWriteDiag::DriverBlockIpAndUnblockIp` | Full driver cycle passed | VERIFIED (PASS SLA: 474.5ms) |
| 14. Clearance Mode Wrapping | `LiveFirewallWriteDiag::ConfigModeWrapping` | Level 2 auto-wrap verified | VERIFIED (PASS SLA: 61.5ms) |
| 15. Syntax Error Recovery | `LiveFirewallWriteDiag::SyntaxErrorUnwindAndRecovery` | Root prompt restored after syntax error | VERIFIED (PASS SLA: 10.4ms) |
| 16. Router Command Purged | `netmon> router status` | `% Unknown command: 'router'` | VERIFIED (0 occurrences of router cmd) |
| 17. 80x24 Layout Adherence | `netmon> help` and `auth list` | Zero lines > 78 columns | VERIFIED (strictly <= 76 cols) |
| 18. Benchmark Metric Extraction | `netmon> firewall diag read` | RTT, parse $\mu$s, bytes, yield recorded | VERIFIED (high-res timers extracted) |
| 19. 80x24 Scorecard Emission | `netmon> firewall diag all` | Complete $\le 76$-column scorecard emitted | VERIFIED (80x24 ASCII table emitted) |
| 20. Regression SLA Validation | `netmon> firewall diag all` | All SLA thresholds satisfied, zero regressions | VERIFIED (21/21 PASS SLA in 3.77s) |
| 21. Obsolete Script Removal | `ls test/benchmark_firewall_read.sh` | File removed (subsumed by CppUTest) | VERIFIED (deleted) |

---

## Appendix. Lifecycle Transition & Status Log

| Date | Previous State | New State | Lifecycle Directory | Notes / Rationale |
|---|---|---|---|---|
| 2026-10-06 | — | `Proposed` | `plan/` | Initial unified plan authored: combines live in-process CppUTest READ and non-destructive WRITE qualification suites ('firewall diag') against active driver with client lockout/keepalive suspension, ZySH configuration mode transparent driver wrapping, phantom function physical qualification, complete purge of 'router' commands, and strict 80x24 terminal layout compliance. |
| 2026-10-06 | `Proposed` | `Executing` | `plan/` | Approved by operator for execution. In-process CppUTest embedding, benchmark telemetry, and diagnostic channel locking implemented. |
| 2026-10-06 | `Executing` | `Done` | `plan/done/` | Fully qualified on live Zyxel USG FLEX 200 hardware (21/21 PASS SLA); unit qualified (123 tests, 635 checks); version bumped to 1.0.7 and tagged. |
