# Plan: Zyxel CLI Parser Hardening & Robustness Overhaul

- **Date**: 2026-10-01
- **Target Platform / Scope**: `netmon` — Gateway telemetry parsers (`src/zyxel/`, `include/zyxel/`, `test/`)
- **Status**: `Done`
- **Lifecycle Location**: `plan/done/`
- **Artifacts Directory**: `plan/plan_zyxel_parser_robustness.artifacts/`
- **Agent Mode**: Single-Agent Pair Programming
- **Core Objectives**:
  1. Replace fragile whitespace token splitting with fixed-width column-boundary geometry for all CLI tables (`show secure-policy`, `show ip route-settings`, `show address-object`, `show ip virtual-server`).
  2. Implement a typed, structured AST for `traceroute` (`ZyxelHop`, `ZyxelHopProbe`, ECMP multi-gateway tracking, latency, and ICMP error flags).
  3. Harden `parsePing` against 100% packet loss, DNS resolution failures, and host unreachable states without reporting false parser errors.
  4. Replace brittle colon/substring arithmetic in metric parsers (`parseCpuStatus`, `parseMemStatus`, `parseConnStatus`) with robust token extraction.
  5. Introduce generative property-based testing via RapidCheck (`/usr/include/rapidcheck.h`, `librapidcheck.a`) verifying zero crashes, memory leaks, or unhandled exceptions across arbitrary truncated, noisy, or corrupted inputs.

---

## 0. Corrections & Negative Constraints (Do Not Reintroduce)

1. **Forbidden Whitespace Token Splitting on Fixed-Width Tables**:
   - *Negative Constraint*: Splitting table rows on arbitrary whitespace (`stream >> token`) fails whenever rule names, descriptions, or firmware strings contain spaces (e.g. `"Default LAN Rule"`, `"V5.43(ABUI.0) Patch 1"`), silently corrupting adjacent column fields.
   - *Mandatory Alternative*: Compute exact `[startCol, endCol]` character spans from header divider lines (`====  ========  ======`), extracting cells strictly by positional geometry.
2. **Forbidden Blind MOTD / Banner Divider Snapping**:
   - *Negative Constraint*: Searching for the first occurrence of `====` or `----` misidentifies greeting banners or legal notices as table headers.
   - *Mandatory Alternative*: Require that the divider line is immediately preceded by recognizable column header keywords (e.g., `"Route"`, `"Rule"`, `"Name"`, `"Interface"`, `"Index"`).
3. **Forbidden False Parse Errors on Diagnostic Drops**:
   - *Negative Constraint*: Treating unreachable targets or 100% dropped ping probes as parse failures (`[ZYXEL_PARSE_ERROR]`).
   - *Mandatory Alternative*: Classify unreachable hosts and complete drops as valid diagnostic outcomes with `packetLossPercent = 100.0` and `packetsReceived = 0`.
4. **Forbidden Static Regex Memory Leaks in CppUTest**:
   - *Negative Constraint*: Instantiating `static const std::regex` inside functions called during tests triggers CppUTest leak warnings due to libstdc++ static heap allocations.
   - *Mandatory Alternative*: Warm up all static regexes before test execution in `TestZyxelMain.cxx` / `TestMain.cxx`, or use zero-allocation character scanning where possible.

---

## 1. Execution Boundaries & Strict Guardrails

1. **Virtual Tri-Modal Discipline**:
   - Plan presented in Virtual Plan Mode. Execution is strictly blocked until explicit user approval ("Proceed" or direct instruction).
2. **Anti-Sycophancy & Code Integrity**:
   - 100% complete, fully compilable, drop-in replacement code with complete error handling. Strictly zero code stubs or truncated blocks.
3. **Pre-Commit Compliance & Copyright**:
   - `python3 ../intelligence/scripts/check_compliance.py --policy selfso` must pass cleanly.
   - Strict personal project copyright: `Copyright (C) 2026, Charles Chiou`.
   - Zero third-party corporate or employer names.
4. **C++ Quality Gates**:
   - `make test` must exit 0 across both test binaries (`test_zyxel_driver_suite`, `test_netmon_suite`) with 0 failures and 0 memory leaks.
   - RapidCheck property testing integrated for parser fuzzing per `.agents/rules/gemini-quality-gates.md`.

---

## 2. Technical Approach & Architecture

### 2.1 Fixed-Width Column Lexer (`ZyxelScanner`)

ZySH CLI outputs format tables with headers and divider lines:
```text
Index  Rule Name             From    To      Source           Destination      Service  Action  Status
=====  ====================  ======  ======  ===============  ===============  =======  ======  ======
1      Default LAN Rule      LAN     WAN     any              any              any      permit  yes
2      Drop Inbound          WAN     LAN     any              any              any      deny    yes
```

We compute column bounding intervals from the divider line:
```cpp
struct TableColumn {
    std::string name;
    size_t      startCol = 0;
    size_t      endCol = std::string::npos;
};
```
1. `parseTableColumns(headerLine, dividerLine)`: Identifies each `===` span and maps the text directly above it as the column name.
2. `extractCell(row, column)`: Safely extracts and trims `row.substr(startCol, endCol - startCol)`, properly handling rows shorter than the full column width.

### 2.2 Structured Traceroute AST (`ZyxelHop` & `ZyxelHopProbe`)

In `include/zyxel/ZyxelTypes.hxx`:
```cpp
struct ZyxelHopProbe {
    std::string ip;
    double      rttMs = 0.0;
    bool        timeout = false;
    std::string icmpFlag; // "!H", "!N", "!P", "!X", etc.
};

struct ZyxelHop {
    int                        hopIndex = 0;
    std::vector<ZyxelHopProbe> probes;
    std::string                primaryIp;
    bool                       isCompleteTimeout = false;
};
```
Parser state machine handles:
- Standard 3-probe responses: `1  192.168.8.1  0.61 ms  0.59 ms  0.58 ms`
- Partial drops: `4  * * 220.128.10.125  14.515 ms`
- Full timeouts: `1  * * *`
- ECMP multi-gateway hops: `3  220.128.9.214  49.303 ms 220.128.9.114  49.133 ms  48.933 ms`
- ICMP unreachability flags: `!H`, `!N`, `!P`, `!X`

### 2.3 Diagnostic Fallback Engine (`parsePing`)

Handles:
- Complete timeout / 100% loss (`packets_transmitted > 0, packets_received = 0, packet_loss = 100.0%`).
- Host unknown / unresolvable: reports structured failure (`status: "unresolved"` or 100% loss) instead of parse exception.
- Destination Unreachable: parses ICMP error and sets packet loss to 100.0%.

---

## 3. Staged Implementation Plan & Acceptance Criteria

### Envelope 1: Column-Boundary Scanner & Metric Extraction Hardening

**Goal:** Implement robust fixed-width column parsing in `ZyxelScanner` and eliminate brittle substring arithmetic in `parseCpuStatus`, `parseMemStatus`, and `parseConnStatus`.

Planned files:
- `include/zyxel/ZyxelScanner.hxx` — Add `TableColumn`, `parseTableColumns()`, `extractCell()`, `extractPercentage()`;
- `src/zyxel/ZyxelScanner.cxx` — Implement column geometry detection and robust numeric token scanners;
- `src/zyxel/ZyxelSystemCmd.cxx` — Refactor `parseCpuStatus`, `parseMemStatus`, `parseConnStatus`, and `parseVersion`;
- `test/TestZyxelScanner.cxx` — New dedicated test fixture for column scanning and numeric token extraction;
- `test/TestZyxelSystemCmd.cxx` — Add regression cases for missing colons, multi-token descriptions, and edge-case formats.

- [x] Task 1.1: Add `TableColumn` and scanner utilities to `ZyxelScanner` (`include/zyxel/ZyxelScanner.hxx`, `src/zyxel/ZyxelScanner.cxx`).
- [x] Task 1.2: Refactor `parseCpuStatus`, `parseMemStatus`, and `parseConnStatus` in `ZyxelSystemCmd.cxx` using robust token extraction.
- [x] Task 1.3: Update `TestZyxelMain.cxx` to ensure all new scanner routines are warmed up before leak detection.
- [x] Task 1.4: Execute `make test` across both test suites (assert 100% pass, 0 leaks).

**Hardstop:** Envelope 1 CppUTest tests must pass before advancing to Envelope 2.

---

### Envelope 2: Structured Diagnostic AST & Robust Table Parsers

**Goal:** Implement full typed AST parsing for `traceroute`, robust drop/unreach handling for `ping`, and upgrade all table parsers to column-boundary scanning.

Planned files:
- `include/zyxel/ZyxelTypes.hxx` — Define `ZyxelHopProbe`, `ZyxelHop`, and update `ZyxelDiagnosticResult`;
- `src/zyxel/ZyxelSystemCmd.cxx` — Rewrite `parseTraceroute` (per-hop probe AST) and `parsePing` (unreachable/drop handling);
- `src/zyxel/ZyxelNetworkCmd.cxx` — Upgrade `parseIpRoutes`, `parseInterfaces`, `parseArpTable` to column-boundary scanning;
- `src/zyxel/ZyxelFirewallCmd.cxx` — Upgrade `parseSecurePolicy` to column-boundary scanning (handling space-separated rule names);
- `src/zyxel/ZyxelNatCmd.cxx` — Upgrade `parseVirtualServer` to column-boundary scanning;
- `test/TestZyxelSystemCmd.cxx`, `test/TestZyxelNetworkCmd.cxx`, `test/TestZyxelFirewallCmd.cxx` — Add comprehensive fixtures for complex tables and ECMP traces.

- [x] Task 2.1: Update `ZyxelTypes.hxx` with `ZyxelHop` and `ZyxelHopProbe` definitions.
- [x] Task 2.2: Rewrite `parseTraceroute` with tokenized per-hop parser and `parsePing` with unreach/drop classifiers.
- [x] Task 2.3: Upgrade `ZyxelNetworkCmd.cxx`, `ZyxelFirewallCmd.cxx`, and `ZyxelNatCmd.cxx` to use `parseTableColumns`.
- [x] Task 2.4: Execute Full Regression Suite (`make test`). Assert 100% pass, 0 leaks.

**Hardstop:** Envelope 2 CppUTest tests must pass before advancing to Envelope 3.

---

### Envelope 3: Generative Property Testing (RapidCheck Fuzzing) & Live Qualification

**Goal:** Implement property-based fuzz tests using system RapidCheck library and qualify hardened parsers on physical hardware.

Planned files:
- `CMakeLists.txt` — Link `rapidcheck` for fuzzing tests;
- `test/fuzz/TestZyxelParsersFuzz.cxx` — RapidCheck property tests covering:
  * Arbitrary string generation (random lengths, unicode, control characters);
  * Truncated transcripts at every character offset;
  * Space-padded and multi-column mutated tables;
- `test/RecordingRouter.cxx` — Align test mocks if necessary.

- [x] Task 3.1: Integrate RapidCheck into CMake build for parser fuzzing.
- [x] Task 3.2: Implement RapidCheck properties in `test/fuzz/TestZyxelParsersFuzz.cxx` verifying zero crashes, 0 memory leaks, and deterministic failure returns on arbitrary inputs.
- [x] Task 3.3: Execute Full Regression Suite (`make test`).
- [x] Task 3.4: Verify live on `rhino` (`ssh rhino`) with `router status`, `router ping`, and `router traceroute`.
- [x] Task 3.5: Pre-commit compliance scan (`python3 ../intelligence/scripts/check_compliance.py --policy selfso`).

**Hardstop:** RapidCheck properties must pass 100 iterations per check, full regression suite must pass with 0 memory leaks, and compliance check must pass.

### Envelope 4: Transport Layer Zero-Byte Read & Non-Blocking Stream Hardening

**Goal:** Eliminate premature prompt exit caused by `libssh2_channel_read` returning 0 in non-blocking mode, and establish automated unit testing for the SSH transport pump.

Planned files:
- `include/ZyxelSshClient.hxx` — Declare `drainUntilPromptWithReader` and `drainUntilPromptForTesting`;
- `src/ZyxelSshClient.cxx` — Refactor `drainUntilPrompt` to treat `n == 0 && !eof` as `EAGAIN` rather than EOF;
- `test/TestZyxelSshClient.cxx` — Add automated unit tests covering zero-byte reads, EAGAIN backoff, chunked packet arrival, timeout, true EOF detection, and Ctrl+C cancellation;
- Physical verification on `rhino` (`192.168.8.30`) with `router ping 8.8.8.8` and `router traceroute 8.8.8.8`.

- [x] Task 4.1: Refactor `drainUntilPrompt` in `src/ZyxelSshClient.cxx` and declare testing hook in `include/ZyxelSshClient.hxx`.
- [x] Task 4.2: Add unit tests in `test/TestZyxelSshClient.cxx` exercising zero-byte reads, chunking, and EOF semantics.
- [x] Task 4.3: Execute `make test` across both test suites (assert 100% pass, 0 leaks).
- [x] Task 4.4: Deploy to `rhino` and verify `router ping 8.8.8.8` and `router traceroute 8.8.8.8` live.
- [x] Task 4.5: Run Selfso pre-commit compliance check.

**Hardstop:** Unit tests must pass offline, `make test` must exit 0 with 0 memory leaks, and live router commands must produce valid hops and packet statistics.

---

## Appendix. Lifecycle Transition & Status Log

| Date | Previous State | New State | Lifecycle Directory | Notes / Rationale |
|---|---|---|---|---|
| 2026-10-01 | — | `Proposed` | `plan/` | Initial proposal for parser robustness overhaul |
| 2026-10-01 | `Proposed` | `Executing` | `plan/` | Approved by user; Envelope 1 execution initiated |
| 2026-10-01 | `Executing` | `Executing` | `plan/` | Envelopes 1, 2, and 3 fully completed and verified |
| 2026-10-01 | `Executing` | `Executing` | `plan/` | Envelope 4 completed: 0-byte read handled, test suite updated, live verified on rhino |
| 2026-10-01 | `Executing` | `Executing` | `plan/` | 15-minute 3-round live soak test completed: 21 commands, 6 injected Ctrl+C cancellations, 0 failures, 0 desyncs |
| 2026-10-01 | `Executing` | `Done` | `plan/done/` | Accepted by user; committed to git |


