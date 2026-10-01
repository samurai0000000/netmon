# Plan: NetMon Ncurses Split-Pane Console (80x24 + Word Wrap)

- **Date**: 2026-10-01
- **Target Platform / Scope**: `netmon` — Interactive Terminal Console Subsystem (`include/NcursesConsole.hxx`, `src/NcursesConsole.cxx`, `src/Main.cxx`, `src/NetMonShell.cxx`, `CMakeLists.txt`, `test/TestNcursesConsole.cxx`)
- **Status**: Done
- **Lifecycle Location**: `plan/done/`
- **Artifacts Directory**: `plan/plan_ncurses_console.artifacts/`
- **Agent Mode**: Single-Agent Pair Programming
- **Core Objectives**:
  1. Port and adapt the proven split-pane ncurses architecture from `aimon` (`NcursesConsole.hxx`/`cxx`) into `netmon` to replace raw line-oriented stdio during interactive GNU screen sessions.
  2. Implement an isolated upper server log window (5 rows) with `std::streambuf` redirection of `std::cout`/`std::cerr`, ensuring asynchronous background sniffer, ARP, SNMP, and syslog logs never interleave or corrupt the operator's active CLI commands.
  3. Enforce strict **80x24 standard VT100 terminal geometry** (1 header row, 5 server log rows, 1 middle separator, 15 command history rows, 1 bottom separator, 1 prompt row = exactly 24 rows; width = 80 columns).
  4. Implement strict **word-boundary line wrapping** (`wrapText(text, maxCols)`) for all command outputs and logs, preventing horizontal text clipping and eliminating ncurses multi-row layout corruption.
  5. Provide non-interactive fallback when `stdin` is not a TTY or when launched in background mode (`netmon run`).

---

## 0. Corrections & Negative Constraints (Do Not Reintroduce)

1. **Forbidden Raw Terminal I/O Interleaving in Interactive Mode**:
   - *Disproven Hypothesis*: Allowing background threads (e.g. `LanSniffer`, `SyslogServer`, `SnmpAggregator`) to print directly to `std::cout` while an operator is typing commands in GNU screen causes garbled text, split prompts, and broken input buffers.
   - *Mandatory Alternative*: Redirect `std::cout` and `std::cerr` through a custom `std::streambuf` (`NcursesStreamBuf`) feeding an isolated, scrolling upper log window (`_logWin`). Interactive commands and responses write strictly to the middle panel (`_cmdWin`).
2. **Forbidden Unwrapped Long String Printing in Ncurses**:
   - *Disproven Hypothesis*: Calling `mvwprintw()` with strings longer than the window width causes ncurses to wrap characters into the next row without advancing the rendering loop's row index, overwriting subsequent lines and desynchronizing scroll calculation.
   - *Mandatory Alternative*: Every incoming line is pre-processed by a word-boundary text wrapper (`wrapText`). Lines exceeding `_termCols - 1` are split into continuation lines indented with 2 spaces. Each wrapped segment occupies exactly 1 screen row.
3. **Forbidden Hard-Coded Window Dimensions**:
   - *Disproven Hypothesis*: Hardcoding 80x24 positions without dynamic recalculation causes buffer overflow or black screens if the terminal window is resized (e.g. terminal maximized or screen resized).
   - *Mandatory Alternative*: Intercept `KEY_RESIZE` and window dimension changes via `handleResize()`. Dynamically recalculate `_cmdHeight = _termRows - 9`. Enforce minimum bounds (rows >= 12, cols >= 40) with graceful scaling.
4. **Forbidden Blocking Input Loop**:
   - *Disproven Hypothesis*: Blocking indefinitely on `getch()` halts header telemetry updates and prevents clean signal handling during shutdown.
   - *Mandatory Alternative*: Configure non-blocking input with timeout (`wtimeout(_inputWin, 100)`). A periodic 100ms tick updates the top header bar and checks for shutdown requests.

---

## 1. Execution Boundaries & Strict Guardrails

During execution, the assistant operates strictly under these boundaries:

1. **Virtual Tri-Modal Discipline**: Operates in Virtual Plan Mode. All code changes remain strictly paused until the user explicitly issues "Proceed".
2. **Strict Personal Project Copyright**: Every file must feature `Copyright (C) 2026, Charles Chiou` and BSD 4-space Emacs modelines.
3. **No Third-Party Names**: Strict zero-tolerance for third-party corporate entities.
4. **Physical Ground Truth & Real Verification**: Qualification requires compiling with `make`, executing `build/netmon status`, verifying ncurses initialization, validating 80x24 layout rendering, and testing interactive command dispatch.
5. **Preservation of Non-Interactive Fallback**: Piped commands (e.g. `echo status | ./build/netmon`) or non-TTY execution must execute normally via standard stdio without invoking ncurses.
6. **Git Commit Prohibition**: Never run `git commit` without an explicit command from the user.

---

## 2. Technical Approach & Architecture

### 2.1 Purpose and Non-Negotiable Acceptance Criteria
- **Purpose**: When `netmon` runs as a daemon under GNU screen (`screen -S netmon ./build/netmon`), background telemetry logs (sniffer packets, ARP discoveries, SNMP polling traps, syslog lines) continuously flood `stdout`. This clobbers operator CLI input. Adopting `aimon`'s ncurses console splits the terminal into dedicated panes, isolating logs from interactive commands while guaranteeing clean 80x24 rendering.
- **Non-Negotiable Acceptance Criteria**:
  1. Standard 80x24 VT100 terminal geometry strictly respected (exactly 24 rows, 80 columns).
  2. Background daemon logs stream exclusively to the top 5-row scrollback pane.
  3. Interactive shell commands and execution responses render in the middle 15-row pane.
  4. Command outputs wider than 80 columns wrap on word boundaries without horizontal clipping or row overwriting.
  5. Interactive prompt (`netmon> `) with cursor and buffer editing on row 23.
  6. Non-TTY environments (CI, pipes, scripts) bypass ncurses automatically.

### 2.2 System Architecture Blueprint

```text
+-------------------------------------------------------------------------------+
|                             NcursesConsole Subsystem                          |
+-------------------------------------------------------------------------------+
       |                                                         |
 [Background Threads]                                      [Operator Input]
 (LanSniffer, Snmp, Syslog)                                (Keyboard in Screen)
       |                                                         |
       v                                                         v
 std::cout / std::cerr                                    _inputWin (Row 23)
       |                                                         |
       v (redirected streambuf)                                  v
 NcursesStreamBuf                                          processCommand()
       |                                                         |
       v                                                         v
 logServer(text)                                           NetMonShell::execute()
       |                                                         |
       v (wprintw + scrollok)                                    v
  _logWin (Rows 1-5)                                      logOutput(response)
                                                                 |
                                                                 v
                                                           wrapText(80 cols)
                                                                 |
                                                                 v
                                                          _cmdHistory deque
                                                                 |
                                                                 v
                                                          _cmdWin (Rows 7-21)
```

### 2.3 Detailed Specifications, Interfaces & 80x24 Layout

#### The 80x24 Terminal Layout Rendering

```text
Col:      0         1         2         3         4         5         6         7
Row:      0123456789012345678901234567890123456789012345678901234567890123456789
00   +-------------------------------------------------------------------------------+ [Header Bar]
00   | NetMon v1.0.5 | PCAP: [ACTIVE: br0] | Zyxel: [CONNECTED] | Devs: 143 | aimon: [OK] |
01   +-------------------------------------------------------------------------------+ [Server Logs]
01   |[10-01 08:32:19] [INFO] LanSniffer: Live streaming capture active on br0       |
02   |[10-01 08:32:19] [INFO] WebServer: Dashboard listening on http://0.0.0.0:3884  |
03   |[10-01 08:32:19] [INFO] SyslogServer: Listening on UDP port 1514               |
04   |[10-01 08:32:20] [INFO] AimonGatewayClient: Connected to aimon at 192.168.8.39 |
05   |[10-01 08:32:20] [INFO] SnmpAggregator: Polling 1 WAN interface and 2 switches |
06   +-------------------------------------------------------------------------------+ [Mid Divider]
06   |--- Command Outputs [Up/Down/PgUp/PgDn to scroll] -----------------------------|
07   +-------------------------------------------------------------------------------+ [Command Pane]
07   |netmon> status                                                                 |
08   |--- NetMon Daemon Status ---                                                   |
09   |Interface:        br0                                                          |
10   |Capture Engine:   PCAP live raw packet capture (ACTIVE)                        |
11   |Gateway Client:   CONNECTED (192.168.8.39:3885)                                |
12   |Registered Devs:  143 online                                                  |
13   |Uptime:           48 minutes                                                   |
14   |                                                                               |
15   |netmon> router ping 8.8.8.8 count 3                                            |
16   |Pinging 8.8.8.8 (3 packets) via Zyxel gateway (Ctrl+C to cancel)...           |
17   |--- Zyxel Router Ping Probe: 8.8.8.8 ---                                       |
18   |Packets:      3 transmitted, 3 received, 0.0% packet loss                      |
19   |Round-Trip:   min=10.2ms, avg=11.4ms, max=12.8ms                               |
20   |                                                                               |
21   |netmon>                                                                        |
22   +-------------------------------------------------------------------------------+ [Bot Divider]
22   |-------------------------------------------------------------------------------|
23   +-------------------------------------------------------------------------------+ [Prompt Line]
23   |netmon> router status_                                                         |
     +-------------------------------------------------------------------------------+
```

#### Row Allocation Budget in Standard 80x24:
| Row(s) | Window Pointer | Height | Purpose & Visual Semantics |
|---|---|---|---|
| **0** | `_headerWin` | 1 | High-contrast telemetry bar (White on Blue, bold): version, PCAP state, router status, gateway status, online device count |
| **1 – 5** | `_logWin` | 5 | Scrolling daemon logs (redirected `std::cout`/`std::cerr`) with `scrollok(..., TRUE)` |
| **6** | `_midSepWin` | 1 | Divider line (`ACS_HLINE`) with title & dynamic scroll indicator badge (`[^ N lines scrolled]`) |
| **7 – 21** | `_cmdWin` | 15 | Interactive CLI commands, router diagnostic outputs, and scrollback buffer (up to 1,000 lines) |
| **22** | `_bottomSepWin`| 1 | Divider line (`ACS_HLINE`) separating output area from input line |
| **23** | `_inputWin` | 1 | Interactive prompt (`netmon> `) with cursor position, editing keys, and command history recall |
| **Total** | | **24** | **100% compliant with standard 80x24 VT100 / POSIX terminal geometry** |

#### Word-Boundary Line Wrapping Algorithm (`wrapText`)
```cpp
// Algorithmic invariant: No line pushed to _cmdHistory may exceed maxCols.
// If a line is longer than maxCols:
// 1. Find the last whitespace character at or before index (maxCols - 1).
// 2. If whitespace found, split at whitespace.
// 3. If no whitespace found (unbroken token), hard-break at (maxCols - 1).
// 4. Continuation lines are prefixed with 2 spaces ("  ") and recursively wrapped.
// 5. Output maintains 1-to-1 correspondence between history entries and screen rows.
```

#### Color Pairs Configuration:
- `PAIR_HEADER` (1): White on Blue (bold)
- `PAIR_PROMPT` (2): Green on default (bold)
- `PAIR_INFO` (3): Cyan on default
- `PAIR_WARN` (4): Yellow on default
- `PAIR_ERROR` (5): Red on default
- `PAIR_TEXT` (6): White on default
- `PAIR_MUTED` (7): White/Grey on default

### 2.4 Threat Model, Safety & Failure Invariants
- **SIGINT / SIGTERM Safety**: Console registers signal handler restoration. `endwin()` is guaranteed to execute before process termination to prevent leaving the operator's terminal in raw/noecho state.
- **Terminal Resize (`KEY_RESIZE`)**: Re-measures `getmaxyx(stdscr, rows, cols)`. If terminal is larger than 80x24, `_cmdHeight` expands dynamically (`_termRows - 9`). If smaller than minimum bounds (12x40), renders gracefully without buffer overruns.
- **Daemon Redirection Deadlock Prevention**: `NcursesStreamBuf` operations use a dedicated `_bufferMutex` and dispatch to `_logWin` under `_uiMutex`. Lock ordering is strictly unidirectional to prevent AB-BA deadlocks between background logging threads and UI redraws.

---

## 3. Staged Implementation Plan & Acceptance Criteria

### 3.0 Mandatory Qualification Protocol & Strict Progression Gate Rule

> [!CAUTION]
> **Strict Non-Negotiable Gate Rule**:
> - C++ changes follow Section 1 item 4. `make test` exits 0.
> - Bypassing tests or advancing without verification is forbidden.
> - Verification commands:
>   `make`
>   `./build/test_netmon_suite`
>   `python3 ../intelligence/scripts/check_compliance.py --policy selfso --staged`

---

### Envelope 1: Curses Build Configuration & Header Architecture

**Goal:** Integrate Curses dependency into `CMakeLists.txt` and define complete `NcursesConsole.hxx` and `NcursesStreamBuf` classes.

Planned files:
- `CMakeLists.txt` — Add `find_package(Curses REQUIRED)`, include dirs, and library linkage;
- `include/NcursesConsole.hxx` — Declare `NcursesStreamBuf` and `NcursesConsole` with full lifecycle, window pointers, and wrapping APIs.

- [x] Task 1.1: Update `CMakeLists.txt` with Curses dependency
  - **Target Files**: `CMakeLists.txt`
  - **Verification**: `cmake -B build` completes successfully finding Curses and tinfo.
- [x] Task 1.2: Create `include/NcursesConsole.hxx`
  - **Target Files**: `include/NcursesConsole.hxx`
  - **Verification**: Header parses cleanly without syntax errors, includes BSD modeline and Charles Chiou copyright.

**Hardstop:** CMake configuration succeeds and header passes static inspection.

---

### Envelope 2: `NcursesConsole.cxx` Implementation (80x24 Geometry, StreamBuf Redirection, Word-Wrap)

**Goal:** Implement `NcursesConsole.cxx` with strict 80x24 window geometry, `wrapText()` word-boundary wrapping, `std::streambuf` redirection, and scrolling history.

Planned files:
- `src/NcursesConsole.cxx` — Full ncurses console implementation.

- [x] Task 2.1: Implement window setup, destruction, and resize handling
  - **Target Files**: `src/NcursesConsole.cxx`
  - **Verification**: Proper row budgeting (1 header, 5 logs, 1 mid sep, 15 cmd, 1 bot sep, 1 input = 24 rows).
- [x] Task 2.2: Implement `NcursesConsole::wrapText()`
  - **Target Files**: `src/NcursesConsole.cxx`
  - **Verification**: Word-boundary line breaking correctly formats long strings into <= 80-col segments.
- [x] Task 2.3: Implement `NcursesStreamBuf` redirection and `logServer()` / `logOutput()`
  - **Target Files**: `src/NcursesConsole.cxx`
  - **Verification**: Log lines route to `_logWin`, command responses route to `_cmdWin`.

**Hardstop:** `make` compiles `src/NcursesConsole.cxx` without errors or warnings.

---

### Envelope 3: Interactive CLI Shell Integration & Daemon Lifecycle

**Goal:** Connect `NcursesConsole` to `NetMonShell` and `Main.cxx` with non-interactive TTY fallback and clean shutdown.

Planned files:
- `src/NetMonShell.cxx` — Route output through `NcursesConsole` when active;
- `src/Main.cxx` — Initialize `NcursesConsole` in interactive daemon mode, restore terminal on exit.

- [x] Task 3.1: Wire `NetMonShell` command execution into `NcursesConsole::logOutput()`
  - **Target Files**: `src/NetMonShell.cxx`
  - **Verification**: Shell commands produce formatted output in the middle panel.
- [x] Task 3.2: Update `Main.cxx` for interactive TTY detection and ncurses startup
  - **Target Files**: `src/Main.cxx`
  - **Verification**: Running `./build/netmon` in a terminal launches ncurses console; running `./build/netmon status` or piped stdin runs in headless line mode.

**Hardstop:** `./build/netmon --help` and `./build/netmon status` work normally in headless mode.

---

### Envelope 4: Unit Qualification (`TestNcursesConsole.cxx`) & Full Regression

**Goal:** Author hermetic CppUTest tests validating word-boundary line wrapping, stream buffer capturing, and window dimension calculation.

Planned files:
- `test/TestNcursesConsole.cxx` — CppUTest suite for `NcursesConsole` utilities;
- `CMakeLists.txt` — Register `test/TestNcursesConsole.cxx` in `test_netmon_suite`.

- [x] Task 4.1: Author `test/TestNcursesConsole.cxx`
  - **Target Files**: `test/TestNcursesConsole.cxx`
  - **Verification**: Tests cover nominal wrapping, oversized unbroken tokens, whitespace splitting, indentation, and multiline text.
- [x] Task 4.2: Execute `make test`
  - **Target Files**: `test/TestNcursesConsole.cxx`
  - **Verification**: All tests pass 100% with 0 failures and 0 memory leaks.
- [x] Task 4.3: Pre-commit compliance review
  - **Target Files**: Workspace
  - **Verification**: `python3 ../intelligence/scripts/check_compliance.py --policy selfso --staged` reports 0 violations.

**Hardstop:** All tests pass and compliance check is clean.

---

### 3.8 Mandatory Verification and Validation Gates Reference

#### 3.8.1 Word-Boundary Line Wrapping Gate (`TEST_GROUP(NcursesWordWrap)`)
- **Nominal Coverage**: Short lines (< 80 cols) remain unmodified. Normal English sentences wrap at space boundaries.
- **Boundary Conditions**: Exact 79/80-column strings; single unbroken strings > 80 columns (URLs/hashes) hard-break at column limit without infinite recursion.
- **Acceptance Threshold**: 100% assertions pass, 0 memory leaks.

#### 3.8.2 StreamBuf Redirection Gate (`TEST_GROUP(NcursesStreamBuf)`)
- **Nominal Coverage**: Writing to `std::cout` and `std::cerr` intercepts characters and flushes lines into destination buffer on `\n` or `std::endl`.
- **Fault Injection**: Mid-line flushes, empty flushes, carriage return stripping (`\r\n`).
- **Acceptance Threshold**: 100% assertions pass, 0 memory leaks.

---

### 3.9 Completion Definition

The implementation is complete only when:
1. All envelopes execute in order without crossing a hardstop;
2. `make` builds cleanly with zero compiler warnings;
3. `make test` executes all CppUTest suites with 100% pass rate;
4. The 80x24 ncurses split-pane UI renders cleanly without horizontal clipping or row desynchronization;
5. Non-interactive fallback works seamlessly for piped commands and background runs;
6. `check_compliance.py --policy selfso --staged` returns 0 violations.

---

## Appendix. Lifecycle Transition & Status Log

| Date | Previous State | New State | Lifecycle Directory | Notes / Rationale |
|---|---|---|---|---|
| 2026-10-01 | — | `Proposed` | `plan/` | Initial plan drafted following aimon ncurses playbook, enforcing strict 80x24 geometry and word-boundary line wrapping |
| 2026-10-01 | `Proposed` | `Executing` | `plan/` | Approved by user for execution |
| 2026-10-01 | `Executing` | `Done` | `plan/done/` | All 4 envelopes implemented and verified; 100% test pass and zero compliance violations |

### Checkpoints & Iteration Log
- 2026-10-01 08:40: Plan formulated with 80x24 VT100 ASCII rendering and word-boundary wrapping specification.
- 2026-10-01 08:48: Envelopes 1-4 completed. All unit tests passed, headless fallback verified, compliance clean.

