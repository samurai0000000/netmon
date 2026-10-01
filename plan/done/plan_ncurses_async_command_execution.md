# Plan: Ncurses Asynchronous Command Execution & Input Blocking Paradigm

- **Date**: 2026-10-01
- **Target Platform / Scope**: `netmon` Ncurses Console subsystem (`NcursesConsole.hxx`, `NcursesConsole.cxx`, `NetMonShell.hxx`, `NetMonShell.cxx`)
- **Status**: Done
- **Lifecycle Location**: `plan/done/plan_ncurses_async_command_execution.md`
- **Artifacts Directory**: `plan/done/plan_ncurses_async_command_execution.artifacts/`
- **Agent Mode**: Single-Agent Pair Programming
- **Core Objectives**:
  1. Break the synchronous CLI lock-step: transition command execution out of the main curses event loop into a dedicated asynchronous worker thread.
  2. Implement the ncurses input-blocking state machine: when a command is accepted, input is blocked for new commands, while the curses event loop continues running smoothly (100ms non-blocking polling).
  3. Ensure instantaneous cancellation: intercept `Ctrl+C` (ASCII 3 or SIGINT) in the active event loop to immediately signal `NetMonShell::cancelCurrentCommand()` and abort in-flight commands (SSH ping, traceroute, router status).
  4. Stream command outputs and maintain live UI updates (logs, header ticks, pane scrolling) continuously without terminal freezes.

---

## 0. Corrections & Negative Constraints (Do Not Reintroduce)

1. **No Synchronous Command Execution on the Curses Thread**:
   - *Previous Defect*: `processCommand(line)` called `NetMonShell::getInstance().executeCommand(line)` directly on the main curses thread while holding `_uiMutex`.
   - *Consequence*: During long-running network or SSH commands (e.g. `router traceroute`), `wgetch()` was never called; the entire event loop was suspended; keystrokes queued in the kernel tty buffer; Ctrl+C could not be processed; the screen was completely frozen.
   - *Mandatory Architecture*: The curses thread must *only* manage window rendering, event polling, and state transitions. Command execution must occur asynchronously in a background worker thread.

2. **No Unmanaged Typing During Execution (Input Blocking Paradigm)**:
   - *The Shift*: While a command executes, the console is NOT in lock-step text entry mode. It must block alphanumeric character input so the user cannot queue arbitrary garbage into the command buffer while a command is active.
   - *Allowed Interactivity During Execution*:
     - `Ctrl+C` (ASCII 3): Immediately aborts in-flight command.
     - `PgUp` / `PgDn` (and arrow keys if appropriate): Scrolls the middle command output history.
     - `KEY_RESIZE`: Dynamically recalculates layout without corrupting windows.
     - Alphanumeric / Enter keys: Dropped/ignored with input state visually indicated on the bottom line (e.g., `netmon [busy - Ctrl+C to abort]>`).

---

## 1. Execution Boundaries & Strict Guardrails

1. **Direct Question Answering (Virtual Ask Mode)**:
   - All status, inquiries, and technical details answered directly in text.
2. **Mandatory User Authorization Gate (Virtual Plan Mode)**:
   - Zero source modifications until this plan is explicitly approved by the user ("Proceed").
3. **Plan Artifacts Directory**:
   - Logs and evidence saved under `plan/plan_ncurses_async_command_execution.artifacts/`.
4. **Anti-Sycophancy & Code Completeness**:
   - No code stubs, no truncated comments (`// TODO`), fully drop-in compilable code.
5. **Zero-Tolerance Guessing**:
   - Verify all behaviors against live screen sessions on `rhino` and local unit test runners.
6. **Physical Ground Truth & C/C++ Qualification**:
   - CppUTest qualification suite in `test/TestNcursesConsole.cxx`.
   - Full regression via `make test`.
   - Live qualification in GNU screen on `rhino` verifying `router traceroute` non-blocking execution, real-time output streaming, and immediate Ctrl+C abort.
7. **Git Commit Prohibition**:
   - No `git commit` commands without explicit user directive.

---

## 2. Technical Approach & Architecture

### 2.1 Purpose and Non-Negotiable Acceptance Criteria
- When an operator presses `Enter` on a command:
  1. The command line is archived in history and echoed to the middle output window.
  2. The console state switches to `State::EXECUTING`.
  3. A background worker thread begins `executeCommand(cmd)`.
  4. The bottom input prompt updates to reflect the executing state (e.g. `netmon [executing: <cmd> | Ctrl+C to cancel]>`).
  5. The curses event loop (`run()`) continues cycling with `wtimeout(_inputWin, 100)`.
  6. Typing letters does NOT append to `_inputBuffer` (input blocked).
  7. Pressing `Ctrl+C` calls `cancelCurrentCommand()`, instantly terminating the SSH/network operation.
  8. Streaming output from `std::cout` / `std::cerr` flushes into the middle output pane in real time via `NcursesStreamBuf`.
  9. Upon worker completion, the thread is joined, state transitions back to `State::READY`, and prompt returns to `netmon> `.

### 2.2 State Machine Blueprint

```
                      +-------------------+
                      |   State::READY    | <--------------------+
                      | Prompt: netmon>   |                      |
                      +-------------------+                      |
                                |                                |
                     [Enter pressed with cmd]                    |
                                |                                |
                                v                                |
                     +----------------------+                    |
                     |  State::EXECUTING    |                    |
                     | Prompt: [busy/abort] |                    |
                     | Worker thread runs   |                    |
                     +----------------------+                    |
                       |         |         |                     |
     [Alphanumeric]    |     [Ctrl+C]      | [Worker completes / |
           v           |         v         |  command canceled]  |
      (Ignore/Drop)    |    (Signal        |                     |
                       |     Cancel)       +---------------------+
                       |         |
                       +---------+
```

### 2.3 Detailed Specifications

#### Data Structures (`NcursesConsole.hxx`)
```cpp
enum class ConsoleInputState {
    READY,
    EXECUTING
};

// Concurrency control for command execution
std::atomic<ConsoleInputState> _inputState{ConsoleInputState::READY};
std::unique_ptr<std::thread> _cmdWorker;
std::atomic<bool> _workerFinished{false};
std::string _activeCommandName;
```

#### Event Loop Handling (`NcursesConsole.cxx`)
1. **In `run()`**:
   - Check `_workerFinished`: if true, join `_cmdWorker`, reset `_workerFinished = false`, set `_inputState = ConsoleInputState::READY`, refresh middle panel and redraw input line.
   - `wgetch(_inputWin)` polls with 100ms timeout.
   - If `ch == ERR`: perform periodic header ticks and worker status checks.
   - If `ch == 3` (`Ctrl+C`):
     - If `_inputState == ConsoleInputState::EXECUTING`:
       - Trigger `NetMonShell::getInstance().cancelCurrentCommand()`.
       - Add visual indicator `^C [Aborting command...]` to middle panel.
     - Else:
       - Clear `_inputBuffer`, redraw input line.
   - If `ch == KEY_PPAGE || ch == KEY_NPAGE || ESC sequence for scroll`:
     - Allow middle panel scrolling regardless of `_inputState`.
   - If `ch == KEY_RESIZE`:
     - Handle window resize smoothly.
   - If alphanumeric / Enter:
     - If `_inputState == ConsoleInputState::EXECUTING`: **DROP / IGNORE**.
     - If `_inputState == ConsoleInputState::READY`: append to `_inputBuffer` and redraw input line.

---

## 3. Staged Implementation Plan & Acceptance Criteria

### Envelope 1: Asynchronous Execution Engine & Input-Blocking State Machine

**Goal:** Decouple command execution from the curses thread, implement input blocking during execution, and support real-time Ctrl+C cancellation.

Planned files:
- `include/NcursesConsole.hxx` — Add `ConsoleInputState`, worker thread management, and atomic completion flags.
- `src/NcursesConsole.cxx` — Implement async command dispatch in `processCommand()`, input filtering in `run()`, non-blocking prompt updates, and worker thread lifecycle management.
- `test/TestNcursesConsole.cxx` — Unit tests verifying input state transitions, worker execution synchronization, and input suppression while busy.

- [ ] Task 1.1: Add `ConsoleInputState` and worker lifecycle members to `NcursesConsole.hxx`.
- [ ] Task 1.2: Refactor `processCommand()` to spawn worker thread and update `_inputState = ConsoleInputState::EXECUTING`.
- [ ] Task 1.3: Update `run()` event loop to handle worker completion, ignore alphanumeric input during execution, and route `Ctrl+C` directly to `cancelCurrentCommand()`.
- [ ] Task 1.4: Update `redrawInputLine()` to show `netmon [busy: <cmd> (Ctrl+C to abort)]> ` when executing.
- [ ] Task 1.5: Update `TestNcursesConsole.cxx` and verify with `make test`.

**Hardstop:** All unit tests must pass before deployment to live system.

---

### Envelope 2: Live Deployment, Real-Time SSH Qualification & Ctrl+C Abort Testing

**Goal:** Deploy binary to `rhino`, execute live SSH commands (`router ping`, `router traceroute`), verify that typing is blocked, UI does not freeze, and Ctrl+C immediately cancels the command.

Planned files:
- (Deployment to `rhino` via rsync and screen restart)

- [ ] Task 2.1: Build release binary on `builder` with `make`.
- [ ] Task 2.2: Deploy binary to `rhino` (`192.168.8.30`), apply `setcap cap_net_raw=eip`.
- [ ] Task 2.3: Restart `netmon` cleanly inside the existing GNU screen session (`1425665.netmon`).
- [ ] Task 2.4: Execute `router traceroute 8.8.8.8`:
  - Verify prompt enters `[busy]` state and typing is blocked.
  - Verify middle panel and top server log panel remain completely active and scrollable.
  - Verify header uptime continues ticking.
- [ ] Task 2.5: Press `Ctrl+C` during an active traceroute:
  - Verify immediate cancellation on router and return to `netmon> ` prompt within < 1 second.
- [ ] Task 2.6: Verify `aimon` service health reporting `HEALTHY`.
