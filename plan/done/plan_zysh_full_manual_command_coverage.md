# Plan: Complete ZySH Manual Command Coverage

- **Date**: 2026-10-06
- **Target Platform / Scope**: `netmon` live ZySH driver and the existing `firewall diag` runner
- **Status**: `Done`
- **Lifecycle Location**: `plan/done/`
- **Topic**: `zysh_full_manual_command_coverage`
- **Author**: Charles Chiou
- **Artifacts Directory**: `plan/plan_zysh_full_manual_command_coverage.artifacts/`
- **Agent Mode**: Single-Agent Pair Programming
- **Core Objectives**:
  1. Cover the Zyxel ZyWALL ZLD Series CLI Reference Guide by a command inventory, then implement each row.
  2. Extend the live `firewall diag` runner shipped in commit `3800ae9`. Do not add a second runner.
  3. Keep hermetic command-builder tests. Live qualification uses the connected firewall session only.
  4. Live writes stay inside the Tier 2 allowlist. Everything else is read, Tier 1 dry-fire, or red-line.
  5. Bridge all covered read/diagnostic commands into the AI clearance interface (`AiSecurityClassifier` / `netmon-ai-client`), eliminate token-length classifier bugs, and modernize skill runbooks so AI agents can comprehensively read and diagnose live firewall state.

---

## 0. Baseline and lessons

Commit `3800ae9` already provides:

- Shell command `firewall diag [read|write|all] [filter]`, parsed in `NetMonShell::cmdFirewallDiag()`.
- `runLiveFirewallDiagnostic()` in `src/ZyxelLiveDiagnostic.cxx`.
- CppUTest groups `LiveFirewallReadDiag` and `LiveFirewallWriteDiag`.
- `read` and `write` select one group with `-sg`. `all` omits `-sg`. A filter is passed as `-sn`.
- Command classes `ZyxelSystemCmd`, `ZyxelNetworkCmd`, `ZyxelObjectCmd`, `ZyxelNatCmd`, `ZyxelFirewallCmd`, and `ZyxelSecurityCmd`.
- Live reads that already pass on this firmware: `show version`, `show cpu status`, `show mem status`, `show conn status`, `show interface summary all`, `show ip route`, `show address-object status`, `show service-object status`, `show secure-policy status`, `show virtual-server status`.
- Live write/rollback for address objects, service objects, one virtual server, and one fast-deny rule.
- `ZyxelDiagnosticGuard`, prompt unwind, `NETMON_QA_*` names, and RFC 5737 addresses `192.0.2.0/24`.

The fast-deny test still calls `ZyxelFirewallCmd::cmdInsertFastDeny(1, ...)`. That is the defect section 0.13 corrects. It is fixed before Envelope 6 adds policy tests.

### 0.1 Firmware grammar beats the manual

On this USG FLEX firmware, `no secure-policy <ruleName>` fails. Named deletion is `no secure-policy name <ruleName>`. Numeric deletion is `no secure-policy <1..500>`. Virtual-server `map-type` accepts `original-service`, `port`, `ports`, `any`, and `service-group`. The mapped keyword is `mapped-service`. A new virtual server is active without an `activate` keyword.

Phase 0 records the firmware's `?` output for each inventory row before a generator is written. Tests assert the recorded command and the recorded error text. They do not assert an error string copied from the manual or from this plan.

Do not replace a show command listed above until the probe shows that the manual's name is the same command.

### 0.2 Deletion order

When a Tier 2 test creates dependent objects, both `setup()` and `teardown()` sweep in this order:

1. Security policies: `no secure-policy name NETMON_...`
2. NAT virtual servers: `no ip virtual-server NETMON_...`
3. Address and service groups
4. Address and service objects
5. `unwindToRootPrompt()`

Any later Tier 2 family (schedule, DNS zone, VPN, UTM profile, SSID) adds its `no` command to this sweeper before the first test of that family runs.

### 0.3 Prompt unwind

A failed command inside `Router(config)#` or a deeper submode leaves the session there. Every live command runs under an RAII guard whose destructor calls `unwindToRootPrompt()`.

### 0.4 Live switch

`ZyxelDriver` defaults live mutations off. Driver-level write tests use an RAII guard that enables live mode for the test and restores the previous value.

### 0.5 Exact CppUTest group names

`-sg` is an exact match. Envelope tokens map to the group names in section 2.1 inside `runLiveFirewallDiagnostic()`. No other mapping is added.

### 0.6 Timing

New live tests use these limits: simple reads at most 1500 ms, large tables at most 3500 ms, a write/verify/rollback at most 6000 ms, parse of a small reply at most 500 µs, parse of a large table at most 5000 µs. Existing assertions in `LiveFirewallReadDiag` and `LiveFirewallWriteDiag` stay as they are.

Insert 25 ms between live tests. Insert 100 ms between a Tier 2 create and its delete. On `% Configuration is locked`, retry the rollback once after 250 ms.

### 0.7 Two test suites

`firewall diag` runs only against `ZyxelDriver::getInstance()` on the configured firewall. If that session is down, the command returns the existing not-connected error.

Offline tests in `test/TestZyxel*Cmd.cxx` and `RecordingRouter` stay. They prove command strings and parsers. They are not deleted, and they are not treated as live qualification.

### 0.8 No topology address in source or in this plan

The live target is the firewall session already configured for `netmon`. Tests do not contain a private gateway address. Documentation addresses used for disposable objects stay in `192.0.2.0/24`.

### 0.9 Tier 2 allowlist

A live write is allowed only when it is one reversible command pair, the `no` form is already on the sweeper, and the names are `NETMON_QA_*`.

Allowed after their Phase 0 probe:

- Address objects, address groups, service objects, service groups, and schedules.
- One NAT virtual server, using the grammar in section 0.1.
- One security policy, appended or inserted after the last rule. Never `insert 1`.
- One DNS domain-zone forwarder, if the probe shows a single create line and a single delete line.

Not allowed on the live firewall:

- Creating, deleting, or renumbering an interface, VLAN, trunk, or zone.
- Installing a route whose next hop is a current default route or a production gateway. If the probe cannot name another next hop, routing writes are Tier 1 only.
- `write`, `reboot`, hostname changes, admin account or password changes, and root-certificate changes.
- UTM, VPN, device-HA, and wireless writes, unless the capability probe in section 0.11 returns both a successful create and a successful matching `no` before any test depends on them.

Multi-line generators for the excluded commands still exist. Their tests are the offline suite.

### 0.10 Sequential runs

The diagnostic lock owns the SSH session. Live tests run one after another on that session.

- While writing envelope N, run `firewall diag all eN`.
- At the close of envelope N, run `firewall diag all` once. That run includes the shipped groups and envelopes 1..N.
- Envelope 10's close-out is that one cumulative run. There is no second full pass after it.

### 0.11 Missing features

The live firewall may lack Wi-Fi, a second HA device, USB cellular, or a licensed UTM feature. Phase 0 sends the read form of each such command. A license or device-absent token is recorded. The parser accepts that token, the session returns to `Router>`, and the live test expects that token. Those rows do not get a Tier 2 write.

### 0.12 Name length

A 32-character object name is a negative test. The expected rejection text is the text Phase 0 recorded, not a sentence written in advance.

### 0.13 Rule position

`secure-policy insert 1` shifts the production policy. Before Envelope 6, change `WriteFastDenyRuleRollback` to append, or to insert after the last existing rule. Deletion remains `no secure-policy name NETMON_QA_RULE`. New policy tests use the same placement.

### 0.14 Terminal width

Shell and scorecard lines stay within the width already enforced by `NetMonShell` (78 columns). Do not reflow existing help text to a new limit.

---

## 1. Guardrails

1. Do not edit source until execution is explicitly requested.
2. Evidence goes under `plan/plan_zysh_full_manual_command_coverage.artifacts/`.
3. No command generator is a stub. A row that is not implemented stays absent from the code and is marked as such in the inventory.
4. Live runs take `ZyxelDiagnosticGuard` before any router command.
5. `MemoryLeakWarningPlugin::turnOffNewDeleteOverloads()` stays in `runLiveFirewallDiagnostic()` before `RunAllTests()`.

---

## 2. Architecture

### 2.1 Runner

`firewall diag` keeps its current syntax. `runLiveFirewallDiagnostic()` gains an exact envelope token:

| Token | `-sg` group |
|---|---|
| `e1` | `LiveFirewallE1` |
| `e2` | `LiveFirewallE2` |
| `e3` | `LiveFirewallE3` |
| `e4` | `LiveFirewallE4` |
| `e5` | `LiveFirewallE5` |
| `e6` | `LiveFirewallE6` |
| `e7` | `LiveFirewallE7` |
| `e8` | `LiveFirewallE8` |
| `e9` | `LiveFirewallE9` |
| `e10` | `LiveFirewallE10` |

`read` and `write` still select `LiveFirewallReadDiag` and `LiveFirewallWriteDiag`. `all` with no envelope token omits `-sg`, so the shipped groups and every `LiveFirewallE*` group run. An argument that is not one of those tokens remains the `-sn` filter.

The shipped read and write groups are not renamed and are not emptied.

### 2.2 Classes

Add methods to the class that already owns the command. Create a class only where none exists.

| Envelope | Class |
|---|---|
| 1 System | `ZyxelSystemCmd` |
| 2 Interfaces, zones, trunks | `ZyxelNetworkCmd` |
| 3 Routing | `ZyxelNetworkCmd` |
| 4 Objects and schedules | `ZyxelObjectCmd` |
| 5 NAT | `ZyxelNatCmd` |
| 6 Security policy and device HA | `ZyxelFirewallCmd` |
| 7 UTM | `ZyxelSecurityCmd` |
| 8 VPN | `ZyxelVpnCmd` (new) |
| 9 Users, AAA, certificates | `ZyxelAuthCmd` (new) |
| 10 Wireless | `ZyxelWlanCmd` (new) |

### 2.3 Session

```text
netmon> firewall diag [read|write|all] [e1..e10 | test-name]
        |
        v
runLiveFirewallDiagnostic()
        |
        v
CppUTest group on ZyxelDriver::getInstance()
        |
        v
configured firewall SSH session
```

---

## 3. Envelopes

### Envelope 0 — command inventory

Read the CLI reference guide's table of contents and command sections. Write `plan/plan_zysh_full_manual_command_coverage.artifacts/command-inventory.md` before any Envelope 1 code.

Each row has: chapter, command, envelope, disposition (`read`, `tier1`, `tier2`, `redline`), and the class method that will own it. Every chapter from 1 through 73 appears once. Chapters 1, 27, 45, and 46 are currently unassigned; the inventory assigns each from the manual, including an explicit `redline` or `out-of-scope` reason when a chapter has no router command. Chapter 17 is assigned to exactly one of Envelope 2 or Envelope 3.

An envelope is finished when every row assigned to it has its disposition implemented and tested. A sample of `show` commands is not coverage.

No live router I/O in this envelope.

### Envelope 1 — system

Rows from the inventory for system, clock, DNS, logging, files, and maintenance.

Phase 0 probes the read and Tier 1 rows and records the firmware text. Preserve the shipped `show version`, `show cpu status`, `show mem status`, and `show conn status` commands.

Tier 2 is only the DNS domain-zone pair if section 0.9's probe succeeds. `reboot`, `write`, hostname, and admin credentials are red-line: the generator may exist for the offline suite, and the live suite does not send them as a successful change.

Group: `LiveFirewallE1`. Close-out is `firewall diag all e1`, then one `firewall diag all`.

### Envelope 2 — interfaces, zones, and trunks

Rows from the inventory for interfaces, zones, trunks, ARP, and Layer 2. Implement them on `ZyxelNetworkCmd`.

Live tests are reads and Tier 1 rejects. Do not create or delete `vlan999`, and do not configure `ge1`–`ge4`, `wan1`, `wan2`, `ppp11`, `ppp12`, or `lan1`.

The negative read uses the error text recorded by Phase 0. Do not hardcode `% Invalid interface name` before that probe.

Group: `LiveFirewallE2`. Close-out is `firewall diag all e2`, then one `firewall diag all`.

### Envelope 3 — routing

Rows from the inventory for static routes, policy routes, and dynamic protocols. Implement them on `ZyxelNetworkCmd`.

The shipped `show ip route` read stays. A live route write is Tier 2 only when the next hop is not a current default route or production gateway. Otherwise the write rows are Tier 1. Do not send a route whose next hop is the firewall's own configured address.

Group: `LiveFirewallE3`. Close-out is `firewall diag all e3`, then one `firewall diag all`.

### Envelope 4 — objects and schedules

Extend `ZyxelObjectCmd`. The shipped host, range, subnet, and TCP/UDP service rollbacks stay in `LiveFirewallWriteDiag`. New object and schedule rows go in `LiveFirewallE4` and use the sweeper in section 0.2.

Group: `LiveFirewallE4`. Close-out is `firewall diag all e4`, then one `firewall diag all`.

### Envelope 5 — NAT

Extend `ZyxelNatCmd`. Keep the shipped virtual-server grammar and `show virtual-server status`. Additional NAT, ALG, UPnP, and redirect rows follow the inventory. A new Tier 2 virtual server uses `192.0.2.0/24` and the sweeper's `no ip virtual-server` step.

Group: `LiveFirewallE5`. Close-out is `firewall diag all e5`, then one `firewall diag all`.

### Envelope 6 — security policy and device HA

First change `WriteFastDenyRuleRollback` so it no longer inserts at index 1. Then add inventory rows on `ZyxelFirewallCmd`.

New live policy tests append, or insert after the last rule, and delete with `no secure-policy name`. Device-HA writes are Tier 2 only after the section 0.11 probe. Otherwise they are read and Tier 1.

Group: `LiveFirewallE6`. Close-out is `firewall diag all e6`, then one `firewall diag all`.

### Envelope 7 — UTM

Extend `ZyxelSecurityCmd`. The capability probe decides which profile commands are read-only. Tier 2 for a profile is allowed only after create and `no` have both succeeded once, and that `no` is already on the sweeper.

Group: `LiveFirewallE7`. Close-out is `firewall diag all e7`, then one `firewall diag all`.

### Envelope 8 — VPN

Add `ZyxelVpnCmd`. The same capability rule as Envelope 7 applies to gateways, Phase 2 connections, SSL VPN, and L2TP. A partially created gateway is removed by the sweeper before the next test.

Group: `LiveFirewallE8`. Close-out is `firewall diag all e8`, then one `firewall diag all`.

### Envelope 9 — authentication and certificates

Add `ZyxelAuthCmd`. A disposable unprivileged user is Tier 2 when the probe shows a single create and a single delete. Admin users, admin passwords, and root certificates are red-line.

Group: `LiveFirewallE9`. Close-out is `firewall diag all e9`, then one `firewall diag all`.

### Envelope 10 — wireless

Add `ZyxelWlanCmd`. If the capability probe reports no radio or no managed AP, the live tests expect that token and do not create an SSID. Tier 2 exists only after the probe's create and `no` both succeed.

Group: `LiveFirewallE10`. Close-out is one `firewall diag all`. That run is the full qualification. Do not start a second full run after it.

### Envelope 11 — AI Clearance Classifier Expansion & Token Bug Remediation

Bridge the 10 envelopes of read and diagnostic commands into `AiSecurityClassifier.cxx`.

1. **Token Length Bug Fixes**:
   - In `AiSecurityClassifier.cxx`, `show ip route`, `show interfaces status`, and `show interfaces detail` were mistakenly checked under `tokens.size() == 4`. Relocate them to `tokens.size() == 3`.
2. **Generalize Level 3 Read Classification**:
   - Expand Level 3 classification so all non-mutating `show` diagnostic commands across the 10 qualified suites (`ZyxelSystemCmd`, `ZyxelNetworkCmd`, `ZyxelVpnCmd`, `ZyxelSecurityCmd`, `ZyxelAuthCmd`, `ZyxelWlanCmd`, `ZyxelNatCmd`, `ZyxelFirewallCmd`, `ZyxelObjectCmd`) are classified as `LEVEL3`:
     - VPN: `show crypto ike sa`, `show crypto ipsec sa`, `show vpn-monitor`, `show ssl-vpn ...`
     - Security/UTM: `show anti-virus ...`, `show anti-spam statistics`, `show content-filter ...`, `show idp ...`, `show threat-website ...`, `show ssl-inspection ...`, `show anomaly-detection ...`, `show app-patrol ...`
     - System: `show logging ...`, `show environment`, `show clock`, `show registration status`, `show running-config` (safe filter)
     - AAA: `show user ...`, `show user-group ...`, `show aaa-server ...`
     - Network: `show bridge ...`, `show vlan ...`, `show bwm ...`, `show dns ...`
   - Preserve rejection of known syntax anomalies (e.g. `show address-group` returns `UNCLASSIFIED` with recommendation for `show object-group address`).
   - Retain Level 1 refusal (`write`, `reboot`) and Level 2 mutation gatekeeping.
3. **Unit & Adversarial Tests**:
   - Update `TestAiSecurityClearance.cxx` and `TestAiSecurityAdversarial.cxx` to verify that read commands from all 10 envelopes classify as `LEVEL3`.
   - Validate RapidCheck property invariants.

Group / Target: `AiSecurityClassifierTest`. Close-out is `make test` exiting 0. Gate evidence saved to `plan/plan_zysh_full_manual_command_coverage.artifacts/gate-envelope-11.txt`.

### Envelope 12 — Clearance Daemon Synchronization & Client Recompilation

1. **Client & Server Buffer Synchronization**:
   - Verify `MAX_RSP_LEN = 1048576` in `client/netmon-ai-client.c` handles large dumps (e.g. `show running-config` or large DPI app statistics).
   - Synchronize embedded client source code and SHA-256 hash in `src/AimonGatewayClient.cxx`.
2. **Recompilation & Hermetic Validation**:
   - Recompile `netmon` and `./build/netmon-ai-client`.
   - Run complete test suite: `./build/test_netmon_suite` and `./build/test_zyxel_driver_suite`.

Gate evidence saved to `plan/plan_zysh_full_manual_command_coverage.artifacts/gate-envelope-12.txt`.

### Envelope 13 — Skill Runbook & Clearance Reference Modernization

1. **Modernize `SKILL.md`**:
   - Correct line 86 to reflect the mandatory Zero-Trust `request R` workflow followed by operator console grant before read execution.
   - Document the comprehensive read catalog spanning all 10 diagnostic envelopes.
   - Establish strict session preservation rule: NEVER call `close` between investigative turns.
   - Provide concrete operational playbooks for future AI agents to diagnose VPN, UTM, Routing, Policy, and System health.
2. **Modernize `references/zyxel-clearance.md`**:
   - Expand the Level 3 Reads section to include the complete set of diagnostic queries.
   - Document error handling, prompt unwinding, and session lifetime invariants.
3. **Pre-Commit Compliance Check**:
   - Run `python3 ../intelligence/scripts/check_compliance.py --policy selfso`.

Gate evidence saved to `plan/plan_zysh_full_manual_command_coverage.artifacts/gate-envelope-13.txt`.

### Envelope 14 — Physical Router Live Qualification & Re-Audit

1. **Deploy Daemon to `rhino`**:
   - Deploy updated `netmon` daemon to `rhino` under explicit operator instruction.
2. **Live AI Client Validation**:
   - Reconnect via `NETMON_LISTEN=192.168.8.30:3885 ./build/netmon-ai-client request R`.
   - Operator grants read clearance on console: `grant <conn> 0 read`.
   - Execute previously blocked diagnostic queries:
     - `show ip route`
     - `show anti-spam statistics`
     - `show crypto ike sa` / `show crypto ipsec sa`
     - `show logging status`
     - `show environment`
3. **Complete Firewall Audit Synthesis**:
   - Render the comprehensive multi-subsystem diagnostic report to the user.

Gate evidence saved to `plan/plan_zysh_full_manual_command_coverage.artifacts/gate-envelope-14.txt`.

### Envelope 15 — Codification of Mandatory Full Firewall Discovery Protocol & Crash-Safety Daemon SOP

1. **Mandatory Full Firewall Discovery Protocol**:
   - Modernize `.agents/skills/netmon/SKILL.md` and `.agents/skills/netmon/references/zyxel-clearance.md` with an explicit, non-negotiable protocol: When an operator instructs an AI agent to "analyze the firewall", "get a complete picture", or "audit the setup", the agent is strictly prohibited from stopping at high-level summary tables or claiming commands are "beyond boundaries".
   - The agent MUST execute the complete 4-quadrant firewall baseline:
     1. **Raw Configuration Directives**: `show running-config` (capturing 2FA, SMTP relays, executive reports, sandboxing actions, IP reputation blocks, storage logging).
     2. **Complete Object Repositories**: `show address-object`, `show object-group address`, `show service-object`, `show object-group service`.
     3. **Event Audit Stream**: `show logging entries` (active 1,024-event rolling memory ring buffer).
     4. **Subsystem Operational Matrix**: The 10 diagnostic envelopes (routing tables, policy routes, DDNS, ALGs, UTM dashboards, VPN monitors, PKI store, WLAN controller, hardware sensors).
   - Strict prohibition of "Action-as-Omission": If a command is classified as Level 3 (Read) in `AiSecurityClassifier`, it must be executed directly over the clearance channel.
2. **Crash-Safety Client Daemon Watchdog Lifecycle Documentation**:
   - Document the intentional architecture of `client/netmon-ai-client.c`: the 300-second (`IDLE_TIMEOUT_SEC`) client-side idle watchdog is a crash-safety mechanism designed to self-terminate if the IDE or agent crashes or disconnects.
   - Clarify that after 5+ minutes of inactivity, a subsequent command automatically auto-spawns a fresh daemon process opening a new TCP connection (e.g. `conn-0004` $\to$ `conn-0005`), requiring a new `request R` and operator `grant` on the console.
   - Instruct future agents to proactively check `status` before executing command batches and initiate `request R` if the previous session expired.
3. **Write-Through Parity & Pre-Commit Compliance Gate**:
   - Write through all updates to `../intelligence/selfso/agents/skills/netmon/` and `../intelligence/selfso/cursor/rules/netmon.mdc` maintaining 1-to-1 parity.
   - Run `python3 ../intelligence/scripts/check_compliance.py --policy selfso`.
   - Record verification evidence in `plan/plan_zysh_full_manual_command_coverage.artifacts/gate-envelope-15.txt`.

---

### Envelope 16 — Anchor Process Liveness Watchdog & Anti-Crash Daemon Lifecycle

1. **Client Daemon Upgrade (`client/netmon-ai-client.c`)**:
   - Implement `is_anchor_alive(anchor, initial_starttime)`:
     - Check `/proc/<anchor>/stat` for process existence.
     - Inspect process state field: reject `'Z'` (Zombie) and `'X'` (Dead).
     - Protect against PID recycling by validating that `starttime` (field 22) matches the `initial_starttime` recorded when the daemon launched.
   - Upgrade the daemon polling loop:
     - Replace the blind 300-second idle countdown.
     - On every 10-second `poll()` cycle, assert `is_anchor_alive(anchor, starttime)`. If the anchor is dead or zombie, immediately send `CLOSE` over TCP, unlink the local socket, and terminate cleanly within $\le 10$ seconds.
     - For indefinite grants (`grant_seconds == 0`), maintain the session alive indefinitely as long as the anchor process remains alive.
     - For bounded grants (`grant_seconds > 0`), enforce the server-assigned grant deadline.
2. **Embedded Client & Procedures Synchronization**:
   - Update `src/AimonGatewayClient.cxx` procedure text and recompute SHA-256 hash.
3. **Hermetic & Adversarial Testing**:
   - Verify `TestClearanceClient.cxx` and `TestAiSecurityAdversarial.cxx`.
   - Add targeted test cases asserting that killing a mock anchor terminates the daemon within the poll cycle.
   - Run complete test suite: `make test`.
4. **Verification Gate**:
   - Record verification evidence in `plan/plan_zysh_full_manual_command_coverage.artifacts/gate-envelope-16.txt`.

---

## 4. Close-out checks

After Envelope 16 implementation:

- Every inventory row has a disposition and a test.
- The SSH session is at `Router>`.
- No `NETMON_QA_*` object remains in the running configuration.
- No `write` was sent.
- All 10 envelopes' read-only commands classify as `LEVEL3` in `AiSecurityClassifier`.
- `make test` passes 100% across all suites.
- Live queries (`show running-config`, `show logging entries`, `show address-object`, `show service-object`, etc.) succeed over `netmon-ai-client`.
- `client/netmon-ai-client.c` monitors anchor liveness, terminates within 10s of anchor crash, and supports indefinite grants without premature idle teardown.
- `SKILL.md`, `zyxel-clearance.md`, and `cursor/rules/netmon.mdc` are synchronized, compliant, and contain the mandatory Full Discovery SOP and anchor liveness lifecycle.

---

## Appendix. Lifecycle Transition & Status Log

| Date | Previous State | New State | Lifecycle Directory | Notes / Rationale |
|---|---|---|---|---|
| 2026-10-06 | — | `Proposed` | `plan/` | Initial draft for full CLI reference coverage. |
| 2026-10-06 | `Proposed` | `Proposed` | `plan/` | Added write limits, append-only policy tests, and capability handling. |
| 2026-10-06 | `Proposed` | `Proposed` | `plan/` | Bound the work to the shipped `firewall diag` runner, required a command inventory, and removed live interface, insert-at-1, and production-gateway route mutations. |
| 2026-10-06 | `Proposed` | `Executing` | `plan/` | Plan approved by operator. Commencing Envelope 0 (Command Inventory) generation. |
| 2026-10-07 | `Executing` | `Done` | `plan/` | All 10 envelopes (3,211 inventory commands, 73 chapters) fully implemented and qualified. 305 live diagnostic tests passed with 0 failures and 0 regressions against live USG FLEX 200 gateway. Zero dangling test objects, zero NVRAM writes. |
| 2026-10-07 | `Done` | `Executing` | `plan/` | Re-opened per operator instruction: append Envelopes 11-14 to bridge all 10 envelopes' read-only commands into `AiSecurityClassifier`, fix token count defects, modernize `SKILL.md`, and execute live qualification. |
| 2026-10-07 | `Executing` | `Executing` | `plan/` | Envelopes 11–14 fully qualified: classifier expanded to all 10 envelopes, deployed to rhino, verified live across all router subsystems, and all 4 quality gates passed cleanly. |
| 2026-10-07 | `Executing` | `Executing` | `plan/` | Appended Envelope 15 per operator instruction to codify the Mandatory Complete Firewall Baseline SOP and client daemon crash-safety watchdog lifecycle into canonical skills and rules. |
| 2026-10-07 | `Executing` | `Executing` | `plan/` | Appended Envelope 16 per operator instruction to implement the Anchor Process Liveness Watchdog in `client/netmon-ai-client.c` replacing naive 300s timeout with process existence, zombie detection, and starttime tracking. |
| 2026-10-07 | `Executing` | `Executing` | `plan/` | Envelopes 15 & 16 fully qualified: SKILL.md, zyxel-clearance.md, and netmon.mdc updated with Mandatory Full Firewall Discovery SOP; anchor liveness watchdog implemented with 5s polling, zombie detection, PID recycling check, indefinite grant retention, and corrected GRANTED <connId> 0 wire protocol parsing. Test suite 100% passing. |
| 2026-10-07 | `Executing` | `Done` | `plan/done/` | All 16 envelopes, full manual command coverage, AI security classifier integration, live qualification, and anchor liveness watchdog completed. Qualified by operator, version bumped to 1.0.8, committed and tagged. |



