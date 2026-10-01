# Plan: Persistent Post-Compilation Capability Helper (setcap_netmon)

- **Date**: 2026-10-01
- **Target Platform / Scope**: `netmon` — Post-compilation binary capability management (`src/setcap_netmon.c`, `Makefile`, `.gitignore`)
- **Status**: Done
- **Lifecycle Location**: `plan/done/`
- **Artifacts Directory**: `plan/plan_setcap_permission_helper.artifacts/`
- **Agent Mode**: Single-Agent Pair Programming
- **Core Objectives**:
  1. Eliminate the `br0: You don't have permission to perform this capture on that device (socket: Operation not permitted)` error when unprivileged users run `netmon`.
  2. Implement a dedicated, lightweight C helper source file (`src/setcap_netmon.c`) tracked cleanly in Git as a standard unprivileged file (`0755`/`0644`, owned by the developer, zero root tracking in Git).
  3. Support a one-time operator setup (`make setcap_netmon && sudo chown root:root setcap_netmon && sudo chmod 4755 setcap_netmon`) that enables zero-sudo, passwordless capability application on `build/netmon` after every compilation.
  4. Enforce strict dual-condition execution gating: `make all` tests BOTH that `build/setcap_netmon` has the setuid bit (`-u`) AND is owned by root UID 0 (`stat -c '%u' = 0`). If either condition fails, `make` skips it completely with zero execution and zero noise.
  5. Enforce self-deletion & non-blocking `distclean`: Support `build/setcap_netmon --clean` so the helper uses its own root privileges (`EUID 0`) to cleanly unlink itself during `make distclean`, preventing unprivileged permission errors.
  6. Enforce strict lifecycle preservation: `build/setcap_netmon` is never touched or deleted during normal `make` or `make clean`; it is ONLY removed on `make distclean`.

---

## 0. Corrections & Negative Constraints (Do Not Reintroduce)

1. **Forbidden Shell Script Setuid (`chmod 4755 script.sh`)**:
   - *Disproven Hypothesis*: Setting the setuid bit (`chmod 4755`) on a shell script (`#!/bin/bash` or `#!/bin/sh`) allows it to run as root when executed by an unprivileged user.
   - *Mandatory Alternative*: The Linux kernel's `execve()` system call explicitly ignores `S_ISUID` on interpreted scripts. We must use a compiled ELF binary (`src/setcap_netmon.c` compiled to `build/setcap_netmon`), which the Linux kernel fully honors for setuid execution.
2. **Forbidden Incomplete Setuid Checks (Missing Root Ownership Verification)**:
   - *Disproven Hypothesis*: Checking only `[ -u build/setcap_netmon ]` is insufficient because an unprivileged user can set `chmod u+s` on their own file, which runs with their own UID, not root.
   - *Mandatory Alternative*: Gating requires a compound check: both `[ -u build/setcap_netmon ]` AND `[ "$$(stat -c '%u' build/setcap_netmon 2>/dev/null)" = "0" ]`. If it is not owned by UID 0, it is never executed.
3. **Forbidden Blind Deletion Failures in `distclean`**:
   - *Disproven Hypothesis*: Plain `rm -f build/setcap_netmon` can fail or prompt if the file is write-protected or owned by root in restricted directory environments.
   - *Mandatory Alternative*: `setcap_netmon` implements `--clean` mode: because the binary executes with `EUID 0`, it unlinks itself with root privileges. `distclean` calls `build/setcap_netmon --clean`, followed by `-rm -f build/setcap_netmon 2>/dev/null || true` as a non-blocking fallback.
4. **Forbidden Overwrite Attempts on Active Root Binary**:
   - *Disproven Hypothesis*: Running `gcc -o setcap_netmon` when `build/setcap_netmon` already exists as a root-owned file causes `Permission denied` compiler aborts.
   - *Mandatory Alternative*: `make setcap_netmon` checks if `build/setcap_netmon` is already root-owned and setuid. If so, it reports status and exits cleanly without trying to overwrite it.
5. **Forbidden Agent Sudo Invocations & Sudoers File Modifications**:
   - *Disproven Hypothesis*: Automating permission fixes by injecting sudoers rules or invoking `sudo` from build scripts or AI agent tools.
   - *Mandatory Alternative*: Zero `sudo` calls from the agent, and zero modifications to `/etc/sudoers.d/`. The user explicitly owns the one-time `chown root` / `chmod 4755` step on the helper binary outside the agent context.
6. **Forbidden Deletion of Setuid Binary During Normal Clean**:
   - *Disproven Hypothesis*: Placing the helper inside `build/` or deleting it in `make clean` forces the operator to repeatedly run `chown root` and `chmod 4755` on every rebuild or clean cycle.
   - *Mandatory Alternative*: `build/setcap_netmon` resides at the repository root and is explicitly preserved across `make` and `make clean`. It is removed ONLY when `make distclean` is executed.

---

## 1. Execution Boundaries & Strict Guardrails

1. **Virtual Tri-Modal Discipline**: Operates in Virtual Plan Mode. All code changes remain strictly paused until the user explicitly issues "Proceed".
2. **Strict Personal Project Copyright**: Every file must feature `Copyright (C) 2026, Charles Chiou` and BSD 4-space Emacs modelines.
3. **No Third-Party Names**: Strict zero-tolerance for third-party corporate entities.
4. **Physical Ground Truth & Real Verification**: Qualification requires compiling the helper, verifying compound `[ -u ] && [ stat = 0 ]` behavior, verifying `--clean` self-deletion under `distclean`, and verifying `make clean` preserves the file.

---

## 2. Architecture & Workflow Design

```
+--------------------------------------------------------------------------------+
| ONE-TIME OPERATOR SETUP (by user, once per clone)                             |
|                                                                                |
|   1. make setcap_netmon                                                        |
|   2. sudo chown root:root setcap_netmon && sudo chmod 4755 setcap_netmon       |
+--------------------------------------------------------------------------------+
                                       |
                                       v
+--------------------------------------------------------------------------------+
| NORMAL DEVELOPMENT LIFECYCLE (Zero sudo, zero prompts)                        |
|                                                                                |
|   make                                                                         |
|     ├── 1. Compile C++ sources -> build/netmon                                 |
|     └── 2. Test: [ -u build/setcap_netmon ] && [ stat -c %u == 0 ]                |
|              YES: Execute build/setcap_netmon build/netmon (EUID=0)                |
|                   Applies CAP_NET_RAW; target remains owned by user!           |
|              NO:  Skip silently. Zero execution, zero noise.                   |
|                                                                                |
|   make clean                                                                   |
|     └── Cleans build/*; PRESERVES build/setcap_netmon                             |
|                                                                                |
|   make distclean                                                               |
|     ├── 1. If setuid root: build/setcap_netmon --clean (self-unlinks as root)     |
|     ├── 2. Fallback: -rm -f build/setcap_netmon 2>/dev/null || true               |
|     └── 3. Removes build/                                                      |
+--------------------------------------------------------------------------------+
```

### Component Details
1. **`src/setcap_netmon.c`**:
   - Pure POSIX / standard C, zero external library dependencies.
   - Command line options:
     - `--clean`: When running as root (`geteuid() == 0`), resolves own binary path via `/proc/self/exe` or `argv[0]` and calls `unlink()` to cleanly remove itself.
     - `<target_path>`: Validates path ends with `netmon` and resides in build tree. Calls `/usr/sbin/setcap cap_net_raw=eip <target>` via `fork`/`execv` with EUID 0.
   - If invoked without root privileges:
     - Emits clear instruction: `[setcap_netmon] Warning: helper lacks root ownership or setuid bit. Run once: sudo chown root:root setcap_netmon && sudo chmod 4755 setcap_netmon`.
     - Returns code 1.

2. **`Makefile`**:
   - `all` target with compound check:
     ```makefile
     all: submodules
     	@mkdir -p $(BUILD_DIR)
     	@cd $(BUILD_DIR) && (test -f Makefile || cmake .. -DCMAKE_BUILD_TYPE=Release)
     	@$(MAKE) -C $(BUILD_DIR) -j$(NUM_PROCS)
     	@if [ -u build/setcap_netmon ] && [ "$$(stat -c '%u' build/setcap_netmon 2>/dev/null)" = "0" ]; then \
     		build/setcap_netmon $(BUILD_DIR)/netmon; \
     	fi
     ```
   - Explicit `setcap_netmon` target:
     ```makefile
     setcap_netmon: src/setcap_netmon.c
     	@if [ -u build/setcap_netmon ] && [ "$$(stat -c '%u' build/setcap_netmon 2>/dev/null)" = "0" ]; then \
     		echo "build/setcap_netmon is already active (root-owned, setuid). Nothing to compile."; \
     	else \
     		rm -f build/setcap_netmon 2>/dev/null || true; \
     		$(CC) -O2 src/setcap_netmon.c -o setcap_netmon; \
     		echo "Built build/setcap_netmon. To activate setuid capability, run once:"; \
     		echo "  sudo chown root:root setcap_netmon && sudo chmod 4755 setcap_netmon"; \
     	fi
     ```
   - `clean` target:
     ```makefile
     clean:
     	@if [ -d $(BUILD_DIR) ] && [ -f $(BUILD_DIR)/Makefile ]; then \
     		$(MAKE) -C $(BUILD_DIR) clean; \
     	fi
     	# NOTE: build/setcap_netmon is explicitly preserved across clean
     ```
   - `distclean` target:
     ```makefile
     distclean:
     	@if [ -u build/setcap_netmon ] && [ "$$(stat -c '%u' build/setcap_netmon 2>/dev/null)" = "0" ]; then \
     		build/setcap_netmon --clean 2>/dev/null || true; \
     	fi
     	@-rm -f build/setcap_netmon 2>/dev/null || true
     	rm -rf $(BUILD_DIR)
     ```

3. **`.gitignore`**:
   - Append `/setcap_netmon` to prevent the compiled local binary from ever being tracked in Git.

---

## 3. Step-by-Step Implementation Envelopes

### Envelope 1: Implement `src/setcap_netmon.c`
- [x] Create `src/setcap_netmon.c` with safe argument parsing, path verification, and root escalation.
- [x] Implement `--clean` self-deletion via `unlink()` on `/proc/self/exe` or `argv[0]`.
- [x] Implement `apply_capabilities(const char *target_path)` invoking `/usr/sbin/setcap cap_net_raw=eip <target>`.
- [x] Verify compilation with standard `gcc -O2 -Wall -Wextra src/setcap_netmon.c -o setcap_netmon`.

### Envelope 2: Integrate into `Makefile` and `.gitignore`
- [x] Update `.gitignore` with `/setcap_netmon`.
- [x] Update `Makefile`:
  - Add `setcap_netmon` target with already-active guard.
  - In `all`: strictly guard with `if [ -u build/setcap_netmon ] && [ "$$(stat -c '%u' build/setcap_netmon 2>/dev/null)" = "0" ]; then build/setcap_netmon $(BUILD_DIR)/netmon; fi`.
  - In `clean`: ensure `build/setcap_netmon` is preserved.
  - In `distclean`: call `build/setcap_netmon --clean` with `-rm -f` fallback.
- [x] Update `LanSniffer.cxx` preflight error hints to mention `make setcap_netmon`.

### Envelope 3: Verification & Lifecycle Qualification
- [x] Verify compound check: when binary lacks root ownership or setuid bit, `make all` does not invoke it.
- [x] Verify `make clean` does NOT remove `build/setcap_netmon`.
- [x] Verify `make distclean` removes `build/setcap_netmon` via `--clean`.
- [x] Run `make test` to ensure zero regression across test suites.
- [x] Run compliance scanner `check_compliance.py --policy selfso --staged`.

---

## 4. Verification & Evidence File
- **Evidence File**: `plan/plan_setcap_permission_helper.artifacts/gate-envelope-1.txt`
- Verification commands:
  ```bash
  make
  # When build/setcap_netmon is absent, unprivileged, or not root-owned, no invocation occurs
  make clean
  # Clean leaves build/setcap_netmon intact
  make distclean
  test ! -f build/setcap_netmon # MUST SUCCEED (Cleaned)
  ```

---

## Appendix. Lifecycle Transition & Status Log

| Date | Previous State | New State | Lifecycle Directory | Notes / Rationale |
|---|---|---|---|---|
| 2026-10-01 | — | `Proposed` | `plan/` | Initial plan drafted based on user constraints (clean in git, no sudo prompts, preserved across clean, removed only in distclean) |
| 2026-10-01 | `Proposed` | `Proposed` | `plan/` | Renamed helper and target to `setcap_netmon` per user directive |
| 2026-10-01 | `Proposed` | `Proposed` | `plan/` | Added compound root ownership verification (`stat -c %u = 0`) alongside `test -u` |
| 2026-10-01 | `Proposed` | `Proposed` | `plan/` | Added `--clean` self-deletion to helper and non-blocking fallback in `distclean` to prevent unprivileged root deletion failures |
| 2026-10-01 | `Proposed` | `Executing` | `plan/` | Approved by user; initiating implementation envelopes |
| 2026-10-01 | `Executing` | `Done` | `plan/done/` | Implementation qualified, helper activated, netmon deployed on rhino with active br0 packet capture |
