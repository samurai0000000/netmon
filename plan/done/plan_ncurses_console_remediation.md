# Plan: NetMon Ncurses Console Freeze & Layout Remediation

- **Date**: 2026-10-01
- **Target Platform / Scope**: `netmon` — Terminal Console Subsystem (`include/NcursesConsole.hxx`, `src/NcursesConsole.cxx`, `test/TestNcursesConsole.cxx`)
- **Status**: Done
- **Lifecycle Location**: `plan/done/`
- **Artifacts Directory**: `plan/plan_ncurses_console_remediation.artifacts/`
- **Agent Mode**: Single-Agent Pair Programming
- **Core Objectives**:
  1. Eliminate all terminal UI freezes, keystroke stalls, and input lags in GNU screen sessions over SSH.
  2. Anchor physical hardware cursor strictly at the input prompt (`netmon> `) so background header updates never steal the cursor or leave the prompt looking hung.
  3. Map Up/Down arrow keys to bash/readline command history recall, reserving PgUp/PgDn for output scrolling.
  4. Fix terminal geometry handling for screens with height < 24 rows (e.g. 139x19) to prevent bottom-row auto-scrolling and eliminate the 5-row blank gap.
  5. Add persistent in-memory ring buffer for daemon logs (`_serverLogHistory`) with an explicit pane header.

---

## 0. Corrections & Negative Constraints (Do Not Reintroduce)

1. **Forbidden Unanchored Hardware Cursor After Header Updates**:
   - *Disproven Hypothesis*: Calling `wrefresh(_headerWin)` in `updateHeader()` every 2 seconds without immediately touching/refreshing `_inputWin` leaves the terminal hardware cursor on Row 0 (Col 139). To the operator, the prompt cursor vanishes and typing appears frozen.
   - *Mandatory Alternative*: Any update to `_headerWin`, `_logWin`, or `_cmdWin` must conclude by explicitly restoring the hardware cursor to `_inputWin` at `(0, promptLen + inputOffset)` and refreshing `_inputWin`.
2. **Forbidden Up/Down Arrow Output Hijacking**:
   - *Disproven Hypothesis*: Mapping Up/Down arrow keys directly to output scrolling violates universal CLI user expectations (bash, zsh, aimon, python). When operators press Up Arrow, they expect command history recall; scrolling the output pane instead leaves the prompt empty and makes the terminal appear unresponsive.
   - *Mandatory Alternative*: Up/Down arrows (`KEY_UP`, `KEY_DOWN`, `\033[A`, `\033[B`) must cycle command history (`_commandHistoryRing`). PgUp/PgDn (`KEY_PPAGE`, `KEY_NPAGE`, `\033[5~`, `\033[6~`) handle output scrolling.
3. **Forbidden Bottom-Row Terminal Hardware Auto-Scroll**:
   - *Disproven Hypothesis*: Writing strings up to `_termCols` on the last terminal row (`_termRows - 1`) triggers hardware line-feed scrolling in VT100/xterm emulators, shifting Row 0 off the screen and desynchronizing curses window coordinates.
   - *Mandatory Alternative*: Enforce `scrollok(_inputWin, FALSE)`, `idlok(_inputWin, FALSE)`, and cap input line rendering at `_termCols - 2` characters.
4. **Forbidden Default ESCDELAY (1000ms)**:
   - *Disproven Hypothesis*: Default ncurses `ESCDELAY` is 1000ms. If an escape sequence is received over SSH, curses pauses for up to 1 second before resolving, causing perceived UI freezes and lag.
   - *Mandatory Alternative*: Explicitly set `set_escdelay(25)` during `init()`.
5. **Forbidden Discard of Daemon Log History on Redraw**:
   - *Disproven Hypothesis*: Erasing `_logWin` on repaint without a history buffer leaves Rows 1–5 as dead, blank space when no new logs have arrived.
   - *Mandatory Alternative*: Store incoming daemon logs in a 100-line circular buffer `_serverLogHistory` and redraw them on window setup/resize.

---

## 1. Execution Boundaries & Strict Guardrails

During execution, the assistant operates strictly under these boundaries:

1. **Virtual Tri-Modal Discipline**: Pauses in Virtual Plan Mode. Awaits explicit user instruction ("Proceed") before modifying code.
2. **Strict Personal Project Copyright**: Every modified file features `Copyright (C) 2026, Charles Chiou`.
3. **No Third-Party Names**: Zero tolerance for corporate entities.
4. **Physical Ground Truth & Real Verification**: Qualification requires building with `make`, running unit tests with `make test`, and verifying live keystrokes, history navigation, and cursor placement directly in GNU screen on `rhino`.
5. **Screen Invariant**: Never terminate or kill the `netmon` screen session on `rhino`.
6. **Git Commit Prohibition**: Never run `git commit` without explicit instruction.

---

## 2. Technical Approach & Architecture

### 2.1 Dynamic Terminal Geometry (<24 Rows Adaptation)

On the operator's current terminal (139 columns by 19 rows):
- **Row 0**: Header Bar (Version, PCAP, Zyxel, aimon status).
- **Row 1**: Log Pane Divider (`--- Daemon Logs & Syslog Events ---`).
- **Rows 2–3** (2 rows): Dedicated server log scrollback ring buffer.
- **Row 4**: Middle Divider (`--- Command Outputs [PgUp/PgDn to scroll] ---`).
- **Rows 5–16** (12 rows): Interactive command outputs panel (expanded from 10 to 12 rows!).
- **Row 17**: Bottom Divider (`--------------------------------------------`).
- **Row 18**: Input Prompt (`netmon> `) with locked hardware cursor.

If total rows >= 24 (standard VT100):
- Server log pane expands to 5 rows, Command outputs panel expands to 15 rows.

### 2.2 Cursor Anchoring Architecture

```text
[updateHeader() / logServer() / renderMiddlePanel()]
                 |
                 v
   Touch & redraw relevant window
                 |
                 v
    redrawInputLine()
                 |
                 +--> mvwprintw(_inputWin, 0, 0, "netmon> ")
                 +--> wprintw(_inputWin, "%s", visibleInput)
                 +--> wmove(_inputWin, 0, 8 + cursorCol)  <-- Hardware cursor locked!
                 +--> wrefresh(_inputWin)
```

---

## 3. Staged Implementation Plan & Acceptance Criteria

### Stage 1: Unit Test Qualification for History & Cursor Handling
- File: `test/TestNcursesConsole.cxx`
- Add test cases covering:
  - Up/Down arrow history recall and indexing.
  - Server log ring buffer storage and redraw.
  - Geometry calculation for short terminals (19 rows).

### Stage 2: Console Core Implementation
- Files: `include/NcursesConsole.hxx`, `src/NcursesConsole.cxx`
- Set `set_escdelay(25)` and `cbreak()`.
- Add `_serverLogHistory` deque (100 lines) and render log pane with title.
- Map Up/Down arrows to `_commandHistoryRing` recall.
- Map PgUp/PgDn to `_scrollOffset` middle panel scrolling.
- Anchor cursor to `_inputWin` in `updateHeader()` and all log callbacks.
- Protect last-row rendering with `scrollok(FALSE)` and width capping.

### Stage 3: Compilation & Unit Test Verification
- Run `make` on `builder`.
- Run `make test` on `builder`.

### Stage 4: Live Deployment & Verification on Rhino
- Redeploy to `rhino` via clean restart (`quit\n` inside screen).
- Verify interactive typing, Up/Down history recall, PgUp/PgDn scroll, and cursor blinking at `netmon> `.
