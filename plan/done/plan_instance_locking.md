# Plan: Single-Instance Process Locking Mechanism for NetMon

- **Date**: 2026-09-30
- **Target Platform / Scope**: `netmon` daemon, CLI commands (`status`, `set-password`, `router set-password`), CppUTest suite
- **Status**: Done
- **Lifecycle Location**: `plan/done/`
- **Artifacts Directory**: `plan/plan_instance_locking.artifacts/`
- **Agent Mode**: Single-Agent Pair Programming
- **Core Objectives**:
  1. Prevent concurrent duplicate daemon instances of `netmon` from running on the same host machine, avoiding port conflicts (`:3884`, `:3886`), raw socket collisions (`br0`), and SQLite lock contention.
  2. Maintain multi-host NFS isolation across physical machines (e.g., `rhino` vs `builder`) sharing `/home/samurai`, ensuring lockfiles are namespaced by local hostname (`~/.config/netmon/netmon.<hostname>.pid`).
  3. Guarantee immunity against stale lockfiles and PID recycling through POSIX `fcntl(F_SETLK, F_WRLCK)` advisory locks combined with `/proc/<pid>/comm` process validation.
  4. Ensure one-shot CLI commands (`status`, `set-password`, and new `router set-password`) execute without holding the daemon lock or being blocked by an already running daemon.

---

## 0. Corrections & Negative Constraints (Do Not Reintroduce)

1. **Do NOT Use Non-Namespaced Lock Files (`~/.config/netmon/netmon.pid`)**:
   - *Why Rejected*: The operator environment shares `/home/samurai` across machines (`builder`, `rhino`, `fox`) via NFS. A fixed lock file path like `~/.config/netmon/netmon.pid` or `netmon.lock` would cause a daemon running on `rhino` to falsely lock out `builder` or vice versa.
   - *Required Pattern*: Lock file paths must strictly embed the hostname: `~/.config/netmon/netmon.<hostname>.pid`.
2. **Do NOT Rely Solely on `kill(pid, 0)` for Stale Process Detection**:
   - *Why Rejected*: If the system restarts or long uptime elapses, an unrelated system process may be allocated the recycled PID. `kill(oldPid, 0)` returns 0 for any live process belonging to the user or system, causing false lockouts.
   - *Required Pattern*: Check `kill(oldPid, 0)` AND verify that `/proc/<pid>/comm` contains `netmon`. If the process name does not match, the PID was recycled and the lock file is stale.
3. **Do NOT Use Blocking Lock Calls (`F_SETLKW` or `LOCK_EX` without `LOCK_NB`)**:
   - *Why Rejected*: If a second instance is launched (e.g. by accident in another screen or script), it must never hang indefinitely waiting on the lock.
   - *Required Pattern*: Use non-blocking `fcntl(fd, F_SETLK, &fl)` and immediately fail with a descriptive diagnostic message showing the conflicting PID and lock file path.
4. **Do NOT Require Daemon Lock for Read-Only or One-Shot CLI Modes**:
   - *Why Rejected*: Commands like `./build/netmon status`, `./build/netmon set-password`, or `./build/netmon router set-password` must be executable while the main daemon is running in the background. Enforcing the instance lock on these commands would break administrative workflows.
   - *Required Pattern*: Enforce `InstanceLock::acquire()` solely during daemon execution modes (`daemon` and `run`).

---

## 1. Execution Boundaries & Strict Guardrails

During execution, the assistant operates strictly under these boundaries:

1. **Direct Question Answering (Virtual Ask Mode)**:
   - Answer all user questions, inquiries, and status checks directly in text using read-only inspection tools.
   - Strictly no unsolicited file edits or scratch file generation during exploratory Q&A.
2. **Mandatory User Authorization Gate (Virtual Plan Mode)**:
   - Never modify source code, configuration files, or documentation without an approved plan and explicit user instruction to proceed.
3. **Plan Artifacts Directory**:
   - Save all generated mockups, analysis logs, test evidence, diagrams, and supplementary artifacts under `plan/plan_instance_locking.artifacts/`.
4. **Anti-Sycophancy & Code Completeness**:
   - Provide critical technical friction; challenge flawed premises or bug-prone designs.
   - Strictly no code stubs, placeholder comments (`// TODO`), or truncated blocks (`/* ... */`). All code edits must be 100% complete, fully drop-in compilable, and include complete error handling.
5. **Zero-Tolerance Guessing & Speculative Workarounds**:
   - Never guess commands, configuration values, file paths, credentials, or internal APIs.
   - Stop immediately upon encountering unexpected errors or discrepancies and report raw facts to the user.
6. **Ground Truth & C/C++ Qualification**:
   - Invalidate proxy-only checks (process existence or compile success alone) and tautological tests.
   - A C or C++ behavior change starts with a CppUTest that fails on the old code, then passes under the repository's existing `make test`. Do not add `make check`, `make cross`, or raw `cmake` / `ctest`.
   - Cover the nominal path, the boundary (empty, minimum, maximum, truncated, oversized), and one fault (invalid input, timeout, or disconnect).
   - AddressSanitizer and UBSan (`-fsanitize=address,undefined`) when memory ownership or pointer manipulation is involved.
   - `mull-runner` once at plan close-out when C or C++ tests changed.
   - If `make test` does not pass, do not start the next envelope.
7. **Surgical Edits & Code Integrity**:
   - Use targeted line replacements (`replace_file_content` or `multi_replace_file_content`).
   - Preserve all existing comments, docstrings, license headers, and surrounding code formatting in unmodified sections.
8. **Git Commit Prohibition & Pre-Commit Review**:
   - Never execute `git commit` in any repository without an explicit, direct command from the user.
   - Run required pre-commit compliance and lint checks before any commit is created.
9. **Project-Specific Boundaries & Operational Invariants**:
   - Follow `cpp-style` standards: `.hxx`/`.cxx` files, PascalCase, BSD 4-space Emacs footer, member variables prefixed with `_`.
   - Never terminate or disturb the active `netmon` daemon on `rhino` (`PID 807190`).
   - Use `Config::resolveHomePath("~/.config/netmon")` for directory resolution.

---

## 2. Technical Approach & Architecture

### 2.1 Purpose and Non-Negotiable Acceptance Criteria
- **Problem**: Currently, executing `./build/netmon` while an existing `netmon` process is running on the same host results in address binding collisions (`WebServer` port 3884, `SyslogServer` UDP port 514, `SnmpDatabase` file locking conflicts). There is no pre-flight mechanism to detect a duplicate instance.
- **Criteria**:
  1. A second invocation of `netmon daemon` or `netmon run` on the same host must immediately exit with status code 1 and print an informative diagnostic showing the conflicting PID and lock file path.
  2. If the previous process crashed or died abruptly leaving a stale PID lock file, a new instance must automatically detect the dead process, safely acquire the lock, truncate the file, and proceed without manual operator intervention.
  3. If an unrelated process was assigned the old PID after a reboot, `/proc/<pid>/comm` checking must confirm the PID belongs to another binary and reclaim the lock safely.
  4. Instances running on distinct hosts sharing NFS home (`/home/samurai`) must acquire independent lock files (`netmon.<hostname>.pid`) without mutual lockout.
  5. One-shot commands (`status`, `set-password`, and `router set-password`) must operate without requiring or contending for the instance lock.

### 2.2 System Architecture Blueprint

```
                      +------------------------------------------+
                      |               netmon main()              |
                      +------------------------------------------+
                                           |
                   +-----------------------+-----------------------+
                   |                                               |
         [One-Shot CLI Command]                            [Daemon / Run Mode]
         (status, set-password,                                    |
          router set-password)                        +---------------------------+
                   |                                  |   InstanceLock::acquire() |
         +-------------------+                        +---------------------------+
         | Execute Action    |                                     |
         | (No lock held)    |                      +--------------+--------------+
         +-------------------+                      |                             |
                                              [File Locked /              [Lock Acquired]
                                               Active Process]                    |
                                                    |                     +---------------+
                                          +-------------------+           | Start Sniffer |
                                          | Print PID & Lock  |           | Start Web/DB  |
                                          | Exit with code 1  |           | Run Shell     |
                                          +-------------------+           +---------------+
                                                                                  |
                                                                          [Normal Shutdown]
                                                                                  |
                                                                          +---------------+
                                                                          | release()     |
                                                                          | Unlink file   |
                                                                          +---------------+
```

### 2.3 Detailed Specifications, Interfaces & Data Schemas

#### Header: `include/InstanceLock.hxx`
```cpp
/*
 * InstanceLock.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_INSTANCE_LOCK_HXX
#define NETMON_INSTANCE_LOCK_HXX

#include <string>
#include <sys/types.h>

class InstanceLock {
public:
    InstanceLock();
    ~InstanceLock();

    bool acquire(const std::string &lockDir = "");
    void release();

    const std::string &getHostname() const;
    const std::string &getLockFilePath() const;
    pid_t getExistingPid() const;
    bool isLocked() const;

private:
    std::string _hostname;
    std::string _lockFilePath;
    int _lockFd;
    pid_t _existingPid;
    bool _locked;
};

#endif /* NETMON_INSTANCE_LOCK_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
```

#### Lock File Format & Content
- Path: `~/.config/netmon/netmon.<hostname>.pid` (fallback to `/tmp/netmon.<hostname>.pid` if config directory uncreatable).
- Format:
  ```text
  <pid>\n
  <hostname>\n
  ```

#### State Machine & Acquisition Flow
1. **Hostname Resolution**: `gethostname()`, truncate at first dot (domain-strip).
2. **Directory Resolution**: Defaults to `Config::resolveHomePath("~/.config/netmon")`, creates via `mkdir(dir.c_str(), 0755)` if not existing, fallback to `/tmp`.
3. **Stale Lock Inspection**:
   - If lock file exists and can be read: parse `oldPid` and `recordedHost`.
   - If `recordedHost` matches `_hostname`: check `kill(oldPid, 0) == 0 || errno == EPERM`.
   - If alive, read `/proc/<pid>/comm`. If comm contains `netmon`, conflict confirmed: save `_existingPid = oldPid` and return `false`.
   - If comm does not contain `netmon` or `kill()` returns -1 with `ESRCH`, process is dead or recycled: proceed to overwrite.
4. **POSIX Lock Acquisition**:
   - `open(_lockFilePath.c_str(), O_RDWR | O_CREAT, 0644)`
   - `fcntl(_lockFd, F_SETLK, &fl)` with `fl.l_type = F_WRLCK`.
   - If `errno == EACCES || errno == EAGAIN`, another process actively holds the kernel descriptor lock: read PID from file, close fd, return `false`.
5. **Payload Update**:
   - `ftruncate(_lockFd, 0)`, write current `getpid()` and `_hostname`, `fsync(_lockFd)`.
   - Mark `_locked = true`, return `true`.
6. **Release**:
   - `fl.l_type = F_UNLCK`, `fcntl(_lockFd, F_SETLK, &fl)`.
   - `close(_lockFd)`, `unlink(_lockFilePath.c_str())`.

### 2.4 Threat Model, Safety & Failure Invariants
- **NFS Lock Partitioning**: NFS file locking can be unreliable across network clients. By scoping the lock file name strictly to `<hostname>`, lock acquisition is performed exclusively on local filesystem inodes by the kernel hosting the process.
- **Unexpected Process Crash / SIGKILL**: If `SIGKILL` or power failure kills `netmon`, the kernel automatically releases the `fcntl` record lock. The file remains on disk with the old PID. On next start, the stale detection logic checks `kill(oldPid, 0)` and `/proc/<pid>/comm`, recognizing the stale state and cleanly taking over the file.
- **PID Recycling**: After a long period, an unrelated daemon might receive the old PID. The `/proc/<pid>/comm` check ensures `netmon` is actually running before declaring a conflict.
- **Graceful Termination**: On `SIGINT`/`SIGTERM`, `signalHandler` requests shutdown, and `main()` invokes `instanceLock.release()` which removes the PID file from disk.

---

## 3. Staged Implementation Plan & Acceptance Criteria

### 3.0 Mandatory Qualification Protocol & Strict Progression Gate Rule

> [!CAUTION]
> **Strict Non-Negotiable Gate Rule**:
> - C or C++ envelopes follow Section 1 item 6. `make test` exits 0. The new test failed before the change.
> - Bypassing tests, commenting out assertions, or advancing on a partial pass is forbidden.
> - **Failure Action Protocol**:
>   1. If any test assertion fails, throws an unhandled exception, crashes, or times out: **STOP IMMEDIATELY**.
>   2. Report the raw failure facts and diagnostic output.
>   3. Diagnose and fix the root cause strictly within the current envelope's boundaries.
>   4. Re-execute the qualification and regression test suites until 100% pass rate is verified.
>   5. Log the verified pass output in the Appendix before touching any file belonging to the next envelope.

---

### Envelope 1: Core `InstanceLock` Component & CppUTest Unit Qualification Suite

**Goal:** Author `include/InstanceLock.hxx`, `src/InstanceLock.cxx`, add build target entries to `CMakeLists.txt`, and implement the full CppUTest qualification suite `test/TestInstanceLock.cxx` covering nominal, boundary, and fault injection cases.

Planned files:
- `include/InstanceLock.hxx` — Single-instance process lock class definition;
- `src/InstanceLock.cxx` — Posix `fcntl` and `/proc/<pid>/comm` implementation;
- `CMakeLists.txt` — Add `src/InstanceLock.cxx` and `test/TestInstanceLock.cxx` to build & test targets;
- `test/TestInstanceLock.cxx` — Dedicated CppUTest qualification test suite.

- [x] Task 1.1: Author `include/InstanceLock.hxx` conforming to `cpp-style` standards.
  - **Target Files**: `include/InstanceLock.hxx`
  - **Verification**: Header syntax and modeline verification.
- [x] Task 1.2: Implement `src/InstanceLock.cxx` with hostname extraction, directory fallback, `/proc/<pid>/comm` verification, and POSIX `fcntl` write locking.
  - **Target Files**: `src/InstanceLock.cxx`
  - **Verification**: Compiles cleanly with project flags.
- [x] Task 1.3: Update `CMakeLists.txt` to register `src/InstanceLock.cxx` in `SOURCES` and `TEST_SOURCES`, and `test/TestInstanceLock.cxx` in `TEST_SOURCES`.
  - **Target Files**: `CMakeLists.txt`
  - **Verification**: `make` builds the updated objects.
- [x] Task 1.4: Implement & Execute Envelope 1 CppUTest Unit & Integrated Qualification Suites (`test/TestInstanceLock.cxx`)
  - **Target Files**: `test/TestInstanceLock.cxx`
  - **Test Matrix (CppUTest)**:
    - *Nominal (AcquireAndRelease)*: First instance acquires lock in isolated test directory (`test_data/test_instance_lock`), verifies file created with PID and hostname. Second instance in same process fails. First instance releases, second instance successfully acquires.
    - *Boundary (CustomDirectory & UncreatableFallback)*: Custom directory path succeeds. Uncreatable directory (e.g. `/proc/invalid_dir`) safely falls back to `/tmp`.
    - *Fault Injection (StaleDeadPidOverwritten)*: Write dead PID (e.g. 999999) to lock file; `acquire()` recognizes stale state, safely acquires lock and overwrites.
    - *Fault Injection (RecycledPidDifferentCommOverwritten)*: Write active PID of an unrelated system binary (e.g. PID 1); `/proc/<pid>/comm` confirms comm does not contain `netmon`, detects PID recycling, safely overwrites.
    - *Multi-Host Isolation*: Verify lock filename is strictly formatted as `netmon.<hostname>.pid`.
  - **Verification**: `./build/test_netmon_suite -v -sg InstanceLockTest` exits code 0 with 100% assertions passing and 0 memory leaks.
- [x] Task 1.5: Execute Full CppUTest Regression Suite
  - **Target Files**: `Makefile`, `CMakeLists.txt`
  - **Verification**: `make test` passes all accumulated CppUTest suites with zero failures and zero regressions.

**Hardstop:** Implementation cannot advance to Envelope 2 until Envelope 1 CppUTest tests and regression suite pass with 100% verification.

---

### Envelope 2: Daemon Integration, One-Shot CLI Router Password Provisioning & Live Verification

**Goal:** Integrate `InstanceLock` into `src/Main.cxx` for daemon modes (`daemon`, `run`), provide out-of-band CLI router password command (`router set-password`), verify non-blocking execution for CLI tools, and validate live single-instance enforcement.

Planned files:
- `src/Main.cxx` — Integrate `InstanceLock`, add `router set-password` CLI command, guard daemon startup;
- `include/NetMonShell.hxx` / `src/NetMonShell.cxx` — (If needed for helper sharing or CLI command handling).

- [x] Task 2.1: Integrate `InstanceLock` into `src/Main.cxx` around daemon startup:
  - Guard `daemon` and `run` modes with `instanceLock.acquire()`.
  - Print descriptive diagnostic and exit with status 1 on failure.
  - Release lock on clean shutdown.
  - **Target Files**: `src/Main.cxx`
  - **Verification**: Inspection and compilation check.
- [x] Task 2.2: Add one-shot CLI command `router set-password` (and alias `router-set-password`) in `src/Main.cxx`:
  - Prompts for router password using `getpass()` twice.
  - Stores securely into `vault.enc` via `AuthManager::getInstance().setRouterPassword()`.
  - Executes without taking the daemon instance lock.
  - **Target Files**: `src/Main.cxx`
  - **Verification**: Run `./build/netmon --help` and verify command documentation.
- [x] Task 2.3: Execute Full CppUTest Regression Suite
  - **Target Files**: `Makefile`, `CMakeLists.txt`
  - **Verification**: `make test` passes all test suites.
- [x] Task 2.4: Real Functional End-to-End Verification:
  - Launch `./build/netmon run` in background;
  - Attempt to launch second instance `./build/netmon daemon`; verify exit code 1 with error message:
    `[netmon] Error: Another instance of netmon is already running on host '<host>' (PID <pid>)`
  - Terminate background instance; verify lock file is removed.
  - Verify `./build/netmon status` and `./build/netmon --version` execute cleanly without lock contention.
  - **Verification**: All commands produce expected ground-truth terminal outputs.

**Hardstop:** Implementation cannot be completed until real end-to-end verification confirms single-instance enforcement and zero regressions.

---

### 3.8 Mandatory Verification and Validation Gates Reference

#### 3.8.1 Gate 1: InstanceLock CppUTest Qualification Suite
- **CppUTest Groups**: `TEST_GROUP(InstanceLockTest)`
- **Nominal Coverage**: Hermetic lock acquisition, PID file content inspection, lock release, cleanup of file.
- **Boundary Conditions**: Custom directory, fallback to `/tmp` when directory permissions fail.
- **Fault Injection**: Dead PID detection, `/proc/<pid>/comm` PID recycling validation.
- **Regression Verification**: `make test` exits 0.
- **Acceptance Threshold**: 100% assertions pass, 0 memory leaks, exit code 0.

#### 3.8.2 Gate 2: Full System Regression & Daemon Enforcement
- **CppUTest Groups**: `test_netmon_suite`, `test_zyxel_driver_suite`
- **Nominal Coverage**: Full regression pass on all existing tests (`TestAuthManager`, `TestCredentialVault`, `TestApprovalQueue`, `TestAuditTrail`, `TestSyslogIngest`, `TestZyxelDriver`, `TestZyxelSshClient`).
- **Live Functional Verification**: Secondary daemon launch blocked on same host; CLI commands run unhindered.
- **Acceptance Threshold**: 100% assertions pass, exit code 0, clean lock removal.

---

### 3.9 Completion Definition

The architecture is freeze-ready when this plan is approved. The implementation is complete only when:
1. All envelopes execute in order without crossing a hardstop;
2. Every gate in Section 3.8 passes with 100% verification;
3. Complete CppUTest suites pass under `make test`;
4. Live functional demonstration verifies dual-instance prevention on the local host;
5. Changes are committed to git ONLY on explicit user instruction following the pre-commit compliance checklist.

---

## Appendix. Lifecycle Transition & Status Log

| Date | Previous State | New State | Lifecycle Directory | Notes / Rationale |
|---|---|---|---|---|
| 2026-09-30 | — | `Proposed` | `plan/` | Initial plan authored following audit of `embdevenv` InstanceLock |
| 2026-09-30 | `Proposed` | `Executing` | `plan/` | Approved by user; execution initiated |
| 2026-09-30 | `Executing` | `Done` | `plan/done/` | Completed, verified, deployed, and committed to git |

### Checkpoints & Iteration Log
- 2026-09-30 21:05: Plan drafted for porting `InstanceLock` to `netmon` with NFS hostname namespacing, `/proc/<pid>/comm` stale detection, and one-shot CLI ergonomics. Awaiting user authorization.
- 2026-09-30 21:13: User approved plan. Status transitioned to Executing. Starting Envelope 1.
- 2026-09-30 21:15: Envelope 1 complete: InstanceLock component, CMake target updates, and CppUTest suite passed 100%.
- 2026-09-30 21:18: Envelope 2 complete: Main.cxx daemon locking, router-set-password command, regression tests passed (57 netmon tests, 39 zyxel tests).
- 2026-09-30 21:22: Live end-to-end verification passed (dual-instance prevention, stale lock auto-reclaim, clean lock removal, CLI non-contention). Pre-commit compliance verified 100%.
