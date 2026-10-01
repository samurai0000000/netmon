# Plan: Interactive Router Shell Subcommands (`status`, `ping`, `traceroute`)

- **Date**: 2026-10-01
- **Target Platform / Scope**: `netmon` — Interactive terminal shell (`NetMonShell`) and gateway driver (`ZyxelDriver`)
- **Status**: `Done`
- **Lifecycle Location**: `plan/done/`
- **Artifacts Directory**: `plan/plan_router_shell_commands.artifacts/`
- **Agent Mode**: Single-Agent Pair Programming
- **Core Objectives**:
  1. Restrict and implement interactive `router` subcommands in `NetMonShell` strictly to: `status`, `ping`, `traceroute`, `set-password`, and `clear-password`.
  2. Implement `ping(target, count)` and `traceroute(target)` in `ZyxelDriver` using `ZyxelSystemCmd` command generators and parsers with dry-run and offline handling.
  3. Provide formatted, human-readable terminal output for `router status`, `router ping <host> [count]`, and `router traceroute <host>`.
  4. Qualify changes with CppUTest fixtures in `test/TestZyxelDriver.cxx` and regression verification under `make test`.

---

## 0. Corrections & Negative Constraints (Do Not Reintroduce)

1. **Forbidden Arbitrary Subcommand Bloat**:
   - *Negative Constraint*: Do not introduce broad mutation subcommands (`block`, `unblock`, `nat`, `routes`, `arp`, `policies`) into the shell.
   - *Mandatory Alternative*: In accordance with operator directive, strictly confine router shell commands to diagnostics: `status`, `ping`, `traceroute`, alongside existing vault credentials (`set-password`, `clear-password`).
2. **Forbidden Unsanitized Host / Target Input**:
   - *Negative Constraint*: Directly interpolating user-supplied host strings into router CLI strings (`ping <host>`) without validation risks ZySH injection.
   - *Mandatory Alternative*: Validate `target` with strict IPv4 or hostname alphanumeric characters (`[a-zA-Z0-9.-]`), rejecting semicolons, quotes, newlines, and spaces before generating ZySH command strings.
3. **Forbidden Unauthenticated / Silent Failure on SSH Timeout**:
   - *Negative Constraint*: Silently hanging if the router is offline or authentication fails.
   - *Mandatory Alternative*: Check `_configured` and `ensureConnectedUnlocked()`, returning clear offline or degraded status immediately if SSH fails or times out.

---

## 1. Execution Boundaries & Strict Guardrails

1. **Virtual Tri-Modal Discipline**:
   - Plan presented in Virtual Plan Mode. Execution strictly blocked until user provides "Proceed".
2. **Anti-Sycophancy & Code Integrity**:
   - 100% complete, compilable, drop-in replacement code with complete error handling.
3. **Pre-Commit Compliance & Copyright**:
   - `python3 ../intelligence/scripts/check_compliance.py --policy selfso` must pass.
   - Strict personal project copyright: `Copyright (C) 2026, Charles Chiou`.
4. **C++ Quality Gates**:
   - Dual-suite `make test` must pass (100% assertions, 0 leaks).

---

## 2. Technical Approach & Architecture

### 2.1 Router Subcommand Interface in `NetMonShell`

When an operator enters commands at the `netmon>` prompt:

```text
netmon> router status
--- Zyxel Gateway Status ---
Status:       online
Driver:       ZyxelDriver (USG FLEX 200)
Model:        USG FLEX 200
Firmware:     V5.43(ABUI.0)
Build Date:   2026-07-25 02:30:50
Transport:    SSH-2.0 (libssh2)

netmon> router ping 1.1.1.1 3
--- Zyxel Router Ping Probe: 1.1.1.1 ---
Packets:      3 transmitted, 3 received, 0.0% loss
Round-Trip:   min = 11.82 ms, avg = 12.14 ms, max = 12.48 ms

netmon> router traceroute 1.1.1.1
--- Zyxel Router Traceroute: 1.1.1.1 ---
Hops:
  1  192.168.8.1 (0.61 ms)
  2  10.0.0.1 (2.14 ms)
  3  1.1.1.1 (11.51 ms)

netmon> router help
Usage: router status | router ping <host> [count] | router traceroute <host> | router set-password | router clear-password
```

### 2.2 Methods & Signatures

#### 1. `include/ZyxelDriver.hxx` & `src/ZyxelDriver.cxx`
```cpp
nlohmann::json ping(const std::string &target, int count = 4);
nlohmann::json traceroute(const std::string &target);
```
- Validate target (IPv4 or hostname: `[a-zA-Z0-9.-]`, length <= 128).
- Clamp `count` to range `[1, 10]`.
- Handle dry-run mode: record command in `_dryRunLog` and return synthetic success result.
- Handle offline / unconfigured state: return `{"status": "offline", "error": "..."}`.
- In live mode: execute `ZyxelSystemCmd::cmdPing(target, count)` via `_sshClient.executeCommand()`, parse with `ZyxelSystemCmd::parsePing()`, return structured JSON.
- For traceroute: execute `ZyxelSystemCmd::cmdTraceroute(target)`, parse with `ZyxelSystemCmd::parseTraceroute()`, return structured JSON.

#### 2. `include/NetMonShell.hxx` & `src/NetMonShell.cxx`
```cpp
void cmdRouterStatus();
void cmdRouterPing(const std::string &target, int count);
void cmdRouterTraceroute(const std::string &target);
```
- Update `printHelp()` with clear usage.
- Update `cmd == "router"` dispatch:
  - `subCmd == "status"` -> `cmdRouterStatus()`
  - `subCmd == "ping"` -> parse host & optional count -> `cmdRouterPing(host, count)`
  - `subCmd == "traceroute"` -> parse host -> `cmdRouterTraceroute(host)`
  - `subCmd == "set-password"` -> `cmdRouterSetPassword()`
  - `subCmd == "clear-password"` -> `cmdRouterClearPassword()`
  - Otherwise: print exact usage:
    `Usage: router status | router ping <host> [count] | router traceroute <host> | router set-password | router clear-password`

---

## 3. Staged Implementation Plan & Acceptance Criteria

### Envelope 1: Driver Diagnostic Methods & Shell Command Integration

- [x] Task 1.1: Add `ping` and `traceroute` methods to `ZyxelDriver` (`include/ZyxelDriver.hxx`, `src/ZyxelDriver.cxx`).
  - Target input validation, dry-run support, and `ZyxelSystemCmd` parser dispatch.
- [x] Task 1.2: Implement `cmdRouterStatus()`, `cmdRouterPing()`, `cmdRouterTraceroute()` in `NetMonShell` (`include/NetMonShell.hxx`, `src/NetMonShell.cxx`).
  - Restrict subcommands strictly to `status`, `ping`, `traceroute`, `set-password`, `clear-password`.
- [x] Task 1.3: Add CppUTest test cases to `test/TestZyxelDriver.cxx`.
  - Validate `ping` and `traceroute` nominal path, target validation reject, dry-run logging, and offline handling.
- [x] Task 1.4: Execute Full Regression Suite (`make test`) & Quality Gate.
  - Assert 100% assertions pass across both test binaries with zero memory leaks (129 tests, 861 checks, 0 failures, 0 leaks).
- [x] Task 1.5: Verify live on `rhino` (`ssh rhino`).
  - Rebuilt `netmon`, restarted in screen session, and verified `router status`, `router ping 1.1.1.1 2` in the live shell against physical USG FLEX 200.
- [x] Task 1.6: Pre-commit compliance scan (`python3 ../intelligence/scripts/check_compliance.py --policy selfso`).
  - All compliance checks passed cleanly!

**Hardstop (Passed):** Interactive router shell subcommands (`status`, `ping`, `traceroute`, `set-password`, `clear-password`) implemented, tested, verified on live physical hardware, and pre-commit compliance scanned. Hardstop satisfied.

---

### Envelope 2: Router Output Polish — Trailing Prompt Stripping, 60s Timeout & Ctrl+C Interruption

**Goal:** Eliminate trailing router prompt (`Router>`, `Router#`) from `router traceroute` output, extend execution timeout from 30s to 60s, and implement graceful `Ctrl+C` command cancellation so operators can interrupt long traces without terminating the `netmon` daemon.

Planned files:
- `include/ZyxelSshClient.hxx` — Add `stripTrailingPrompt` static method and cancellation hook `cancelActiveCommand()`;
- `src/ZyxelSshClient.cxx` — Implement `stripTrailingPrompt`, check cancellation in `drainUntilPrompt`, and inject `\x03\n` on cancel;
- `src/zyxel/ZyxelSystemCmd.cxx` — In `parseTraceroute`, clean `out.rawOutput` with `stripTrailingPrompt`;
- `src/ZyxelDriver.cxx` — Increase `traceroute` execution timeout from 30,000 ms to 60,000 ms and expose interrupt forwarding;
- `src/NetMonShell.cxx` — Update banner text to `(up to 60s)...`, hook scoped `SIGINT` handler during interactive execution, and handle cancellation;
- `src/Main.cxx` — Coordinate `SIGINT` so active shell commands receive interrupt first before process termination;
- `test/TestZyxelSshClient.cxx` — Add CppUTest case for `stripTrailingPrompt` and cancellation state;
- `test/TestZyxelSystemCmd.cxx` — Add CppUTest case asserting `parseTraceroute` output contains no trailing `Router>`.

- [x] Task 2.1: Implement `stripTrailingPrompt` and cancellation support in `ZyxelSshClient` (`include/ZyxelSshClient.hxx`, `src/ZyxelSshClient.cxx`).
  - Target trailing `\w+(?:\([A-Za-z0-9_.-]+\))?[>#]\s*$` and trim trailing newlines.
  - In `drainUntilPrompt`, check `_cancelled` flag; on cancel, write `\x03\n` to channel, drain prompt, and return `SshResult::ERR_INTERRUPTED`.
- [x] Task 2.2: Apply prompt stripping to `ZyxelSystemCmd::parseTraceroute` and extend timeout to 60s in `ZyxelDriver::traceroute` and `NetMonShell::cmdRouterTraceroute`.
- [x] Task 2.3: Implement scoped `SIGINT` interception in `NetMonShell` and `Main.cxx` to interrupt foreground commands cleanly.
- [x] Task 2.4: Add CppUTest test cases to `test/TestZyxelSshClient.cxx` and `test/TestZyxelSystemCmd.cxx`.
- [x] Task 2.5: Execute Full Regression Suite (`make test`) & Quality Gate.
  - Verify 100% assertions pass and 0 memory leaks across both test suites (132 tests, 870 checks, 0 failures, 0 leaks).
- [x] Task 2.6: Verify live on `rhino` (`ssh rhino`).
  - Deploy to screen session on `rhino`, verify `router traceroute 8.8.8.8` outputs clean hops ending without `Router>`, and test `Ctrl+C` interrupt.
- [x] Task 2.7: Pre-commit compliance scan (`python3 ../intelligence/scripts/check_compliance.py --policy selfso`).
  - All compliance checks passed cleanly!

**Hardstop (Passed):** Trailing prompt stripping, 60s timeout, non-blocking SSH mode, stale buffer drain, and interactive `Ctrl+C` command cancellation fully implemented, qualified via CppUTest (132 tests, 870 checks, 0 leaks), verified live on physical hardware in `screen` on `rhino`, and pre-commit compliance scanned. Hardstop satisfied.

---

## Appendix. Lifecycle Transition & Status Log

| Date | Previous State | New State | Lifecycle Directory | Notes / Rationale |
|---|---|---|---|---|
| 2026-10-01 | — | `Proposed` | `plan/` | Initial plan for interactive router commands |
| 2026-10-01 | `Proposed` | `Executing` | `plan/` | Approved by user; Envelope 1 executed & qualified |
| 2026-10-01 | `Executing` | `Proposed` | `plan/` | Envelope 2 proposed for prompt stripping & 60s timeout |
| 2026-10-01 | `Proposed` | `Executing` | `plan/` | User approved Envelope 2 with Ctrl+C interrupt ("do it") |
| 2026-10-01 | `Executing` | `Executing` | `plan/` | Envelope 2 complete & verified live; awaiting user acceptance |
| 2026-10-01 | `Executing` | `Done` | `plan/done/` | Accepted by user; committed to git |



