# Plan: Security Audit Trail, Human Approval, and Router Syslog

- **Date**: 2026-09-26
- **Target Platform / Scope**: `netmon` daemon, local operator CLI, loopback web admin, and router syslog ingest
- **Status**: Done
- **Lifecycle Location**: `plan/done/`
- **Artifacts Directory**: `plan/done/plan_security_audit_and_syslog.artifacts/`
- **Agent Mode**: Single-Agent Pair Programming
- **Core Objectives**:
    1. A local human can authenticate, recover a forgotten admin password, and approve or deny agent mutations.
    2. The agent socket cannot approve its own mutation, change policy, or forge an operator session.
    3. Every mutation decision is durable and queryable, and router syslog is labeled as time-adjacent evidence rather than proof.

---

## 0. Corrections & Negative Constraints (Do Not Reintroduce)

1. **Do not serve admin credentials on the LAN.** The existing dashboard listener may remain on its configured address. It serves monitoring only. Password login, sessions, policy changes, approvals, audit reads, and syslog reads belong exclusively to a second listener bound to `127.0.0.1`. This plan does not add TLS and does not move the dashboard to loopback.
2. **Do not treat socket separation as process isolation.** `aimon` has no CLI, host-shell, or web-token path. That stops a well-behaved agent from approving itself. It does not stop code execution inside `netmon`: the JSON-RPC parser, vault, and router SSH client share one process.
3. **Do not approve an underspecified ticket.** A pending action stores the canonical mutation payload. Approval replays that payload and no other arguments. A ticket that records only tool name, target IP, and reason cannot be executed.
4. **The queue has one source of truth.** Pending actions persist in SQLite and are reloaded after restart. A second in-memory queue is forbidden.
5. **Default policy is `require_approval`.** `live` and any policy change require an authenticated human. The agent socket cannot set policy. `disabled` and `dry_run` never touch the router.
6. **Break-glass has one path.** Only the host-shell command `netmon set-password`, on a controlling terminal, may replace the admin password without the current password. CLI password change requires the current password and an authenticated CLI session.
7. **Sessions are opaque and bound to a vault generation.** Tokens are 256-bit random values stored in daemon memory, with a 15-minute idle timeout. Each session records the vault credential generation current when it was issued. Password replacement increments that generation in the vault. Every validation takes the vault lock and reads the generation from disk. A cached generation is forbidden. Do not use a stateless HMAC of the password.
8. **SQLite is the audit authority.** The decision and its chain record commit in one database transaction through `audit_outbox`. The file is a rebuildable projection: startup appends missing sequence numbers, and a mismatched chain is rewritten from SQLite. Do not describe two independent writes, or mode `0600`, as tamper-proof.
9. **Approval is claimed before the router call.** A conditional update changes `pending` to `executing` and must affect one row before any router I/O. A second approval affects zero rows and stops. A crash in `executing` is recorded as interrupted and is not retried automatically. An operator must then reconcile it as `applied` or `retry` after inspecting the router. `applied` records success without another router call. `retry` returns the ticket to `pending`.
10. **UDP syslog is not proof.** Accept datagrams only from the configured router address. A message inside ten seconds of an agent action is marked time-adjacent. It does not change the audit decision and is not displayed as the cause of that action.
11. **Do not leave audit or syslog reads unauthenticated.** They expose operator decisions and router messages, and they are served only by the loopback admin listener.
12. **The admin listener defaults to `127.0.0.1:3886`.** That port is not the dashboard (`3884`), aimon (`3883`), gateway (`3885`), or meshmon (`16880`) port. Startup rejects a configured admin port in that set. The bind address is not configurable.
13. **CppUTest is the only qualification, and it is not in the tree yet.** `CMakeLists.txt` has no test executable and there is no `test/` source. Envelope 1 creates that integration. Do not assume a prior `make test` target exists. Do not add GoogleTest, Catch2, a shell-only suite, or a manual curl log as a substitute. Do not rename, comment out, or filter off a `TEST()` required below. Do not pass CppUTest `-m`, `-x`, or `-i`. The memory-leak detector stays enabled.
14. **`RecordingRouter` is the only test adapter.** It stands in for the external router and is linked only into `test_netmon_suite`. Vault, checkpoint, SQLite, files, locks, UDP, and HTTP stay real. Do not mock those internals, and do not compile `RecordingRouter` into the `netmon` binary.

---

## 1. Execution Boundaries & Strict Guardrails

During execution, the assistant operates strictly under these boundaries:

1. **Direct Question Answering (Virtual Ask Mode)**:
   - Answer all user questions, inquiries, and status checks directly in text using read-only inspection tools.
   - Strictly no unsolicited file edits or scratch file generation during exploratory Q&A.
2. **Mandatory User Authorization Gate (Virtual Plan Mode)**:
   - Never modify source code, configuration files, or documentation without an approved plan and explicit user instruction to proceed.
3. **Plan Artifacts Directory**:
   - Save all generated mockups, analysis logs, test evidence, diagrams, and supplementary artifacts under `plan/plan_security_audit_and_syslog.artifacts/`.
4. **Anti-Sycophancy & Code Completeness**:
   - Provide critical technical friction; challenge flawed premises or bug-prone designs.
   - Strictly no code stubs, placeholder comments (`// TODO`), or truncated blocks (`/* ... */`). All code edits must be 100% complete, fully drop-in compilable, and include complete error handling.
5. **Zero-Tolerance Guessing & Speculative Workarounds**:
   - Never guess commands, configuration values, file paths, credentials, or internal APIs.
   - Stop immediately upon encountering unexpected errors or discrepancies and report raw facts to the user.
6. **Ground Truth & Real Verification (Proper Testing Standard)**:
   - Invalidate proxy-only checks and tautological tests.
   - Validate changes through real functional transactions and end-to-end verification.
   - **All envelopes must be qualified with proper testing. If tests do not pass, implementation cannot advance to the next envelope under any circumstances.**
   - **Definition of Proper Testing (All Using CppUTest)**:
     1. **Repeatable**: Deterministic, hermetic, fully automated CLI execution (`make test`) producing identical results across runs with zero flakiness and clean fixture setup/teardown.
     2. **Coverage / Boundary / Fault**: Comprehensive test matrices exercising nominal paths, boundary limits, and fault injection.
     3. **Formalized Across All Testing Scopes in CppUTest**:
        - **Unit Testing**: `TEST_GROUP(<Component>)`.
        - **Integrated Testing**: `TEST_GROUP(Integration_<Subsystem>)` using real files, real sockets, and real SQLite databases under gitignored `test_data/`.
        - **Regression Testing**: `make test` before advancing past any envelope hardstop.
     4. **Strict Ground-Truth / Zero-Mock Rule**:
        - Mocking of the vault, checkpoint, database, files, locks, processes, UDP, or HTTP is strictly prohibited.
        - `test/RecordingRouter.cxx` is the sole adapter. It replaces only the external router inside `test_netmon_suite`. The physical router is not required. The `netmon` binary still links `ZyxelDriver`.
7. **Surgical Edits & Code Integrity**:
   - Use targeted line replacements.
   - Preserve all existing comments, docstrings, license headers, and surrounding code formatting in unmodified sections.
8. **Git Commit Prohibition & Pre-Commit Review**:
   - Never execute `git commit` in any repository without an explicit, direct command from the user.
   - Run required pre-commit compliance and lint checks before any commit is created.
9. **Project-Specific Boundaries & Operational Invariants**:
   - C++ sources stay `.cxx` / `.hxx`, 4-space BSD style, and keep the existing personal copyright header.
   - Do not log passwords, session tokens, or vault keys.
   - Do not widen the agent JSON-RPC interface with approve, deny, or policy-set methods.

---

## 2. Technical Approach & Architecture

### 2.1 Purpose and Non-Negotiable Acceptance Criteria

`SecurityCheckpoint` currently keeps a process-local audit vector. Agent mutations need a human gate, and the operator needs a durable record plus a view of what the router reported.

The threat model is deliberately narrow:

- The protected boundary is an over-eager, malfunctioning, or compromised AI
  client sending requests through the agent JSON-RPC interface.
- One local administrator operates the subsystem through the host terminal or
  a loopback browser.
- The audit trail is an operational history with accidental-corruption and
  simple-edit detection. It is not an independent forensic record.
- Arbitrary code execution inside `netmon`, compromise of the local account,
  and compromise of the host are explicitly out of scope.

Success means all of the following:

- A local operator can set, replace, and use an admin password.
- With policy `require_approval`, an agent mutation is stored and is not sent to the router until an authenticated human approves that exact stored payload.
- Denial, expiry, `dry_run`, and `disabled` produce no router call. Two concurrent approvals produce one router call.
- A host-shell password change increments the vault generation under the vault lock, and every session issued under the old generation fails on its next use even if the daemon never restarts.
- An interrupted ticket stays out of the router path until the operator reconciles it as `applied` or `retry`.
- After a crash, SQLite and the audit-file projection contain the same committed sequence.
- Syslog datagrams from any address other than the configured router are dropped.
- The dashboard remains available on its existing listener, and that listener has no admin routes. Admin routes are reachable only at `127.0.0.1:3886` unless `web.admin_port` selects another port outside the reserved set.

### 2.2 System Architecture Blueprint

```text
aimon                         local human
  JSON-RPC only                 host shell: set-password
  no approve/policy             increments vault generation
        |                       CLI and 127.0.0.1 admin listener
        v                              |
+-------+------------------------------+------+
| netmon process                              |
|                                             |
|  dashboard listener: monitoring only        |
|  admin listener: 127.0.0.1 only             |
|                                             |
|  session valid only at current generation   |
|                                             |
|  policy gate                                |
|    disabled / dry_run -> SQLite decision    |
|    require_approval -> pending ticket       |
|    approve: pending -> executing -> router  |
|    live -> router, still one transaction    |
|                                             |
|  one SQLite transaction:                    |
|    ticket result + audit_outbox row         |
|       |                                     |
|       +--> audit file projected from outbox |
|                                             |
|  UDP syslog from configured router only     |
|    time-adjacent label, separate table      |
+---------------------------------------------+
```

### 2.3 Detailed Specifications, Interfaces & Data Schemas

Policy modes are `disabled`, `dry_run`, `require_approval`, and `live`. The stored default is `require_approval`.

A pending ticket contains:

- numeric id, creation time, expiry time
- requester identity supplied by the agent transport
- tool name
- canonical JSON payload exactly as admitted by the checkpoint
- status: `pending`, `executing`, `approved`, `denied`, `expired`, `interrupted`

Approval first runs a conditional update equivalent to changing `pending` to `executing` only when the id matches, status is `pending`, and expiry is in the future. Zero changed rows means another caller won or the ticket is no longer executable, and no router call is made. One changed row authorizes exactly one router call with the stored payload. Success commits `approved` and the audit outbox row in one transaction. Router failure commits the ticket back to `pending` and an outbox row that says execution failed. A process restart that finds `executing` commits `interrupted` and does not call the router again. Denial is the same conditional update from `pending` to `denied`; it loses cleanly when the row is already `executing`.

`audit_outbox` contains a monotonic sequence, the previous SHA-256, the decision fields, and an exported flag. That insert uses the same transaction as the ticket update. A flusher appends unexported rows to the audit file in sequence order and then marks them exported. Startup appends any committed sequence missing from the file. If the file's hash at a sequence disagrees with SQLite, startup replaces the file from the outbox. SQLite remains authoritative.

Password verification uses a constant-time compare. The vault stores an integer credential generation with the password. Writers and readers share `vault.lock` beside the vault file. A writer takes an exclusive `fcntl` lock for the whole read-modify-write: write `vault.enc.tmp` with mode `0600`, `fsync` it, `rename` it over `vault.enc`, then `fsync` the directory before dropping the lock. A reader takes a shared lock, opens `vault.enc` by name, and reads the generation before dropping the lock. Session creation stores 32 random bytes, last-used time, and that freshly read generation. `AuthManager` does not cache vault contents. Login and session validation each take the shared lock and read `vault.enc`. Idle time over 15 minutes deletes the session. Host `set-password` and CLI password change increment the generation under the exclusive lock. They do not need the daemon's session table. A lock left by a dead process is released by the operating system when that process exits.

The existing dashboard listener keeps its configured bind and port `3884` and does not register admin routes. The admin listener binds `127.0.0.1` and `web.admin_port`, whose default is `3886`. Startup fails if that port is `3883`, `3884`, `3885`, or `16880`. Admin routes are login, logout, status, pending, approve, deny, reconcile, policy, audit, and syslog. All except login require a bearer session whose generation still matches the locked vault read.

An `interrupted` ticket accepts only reconcile. `reconcile <id> applied` conditionally changes `interrupted` to `approved`, performs no router call, and writes an audit outbox row stating the operator confirmed the effect. `reconcile <id> retry` conditionally changes `interrupted` to `pending`, performs no router call, and audits that the operator returned it to the queue. Any other status is rejected. A later approval of a retried ticket uses the normal `pending` to `executing` claim.

CLI commands are `auth login`, `auth set-password`, `policy`, `pending`, `approve <id>`, `deny <id>`, `reconcile <id> applied|retry`, `audit`, and `syslog`. `auth set-password` requires the current password. The host command `netmon set-password` does not. The admin listener exposes the same reconcile operation.

The syslog listener binds UDP `1514` by default, parses RFC 3164 and RFC 5424, and drops a datagram whose source address is not the configured router. Stored events may be flagged time-adjacent to an audit row within ten seconds. That flag is informational.

Existing protected-address checks in `SecurityCheckpoint` remain in front of both immediate execution and approval. An approval cannot bypass them.

### 2.3.1 Answers frozen for the implementer

1. **Vault contents.** `vault.enc` is AES-256-GCM. The 32-byte key lives only in `vault.key`, created once, mode `0600`. The directory is mode `0700`. The decrypted payload is JSON with `generation` as an integer, plus a PBKDF2-HMAC-SHA256 verifier: 16-byte salt, 210000 iterations, and a 32-byte hash. There is no plaintext password field. Verification compares the derived hash with `CRYPTO_memcmp`. A missing key or vault fails closed. Envelope 1 does not implement a hardware-token backend.
2. **Audit file.** The projection path is `Config::resolveHomePath("~/.config/netmon/audit.log")`, mode `0600`. `security.audit_file` may override it and is resolved through the same function. Tests pass their own temporary path and never write the operator's real config directory.
3. **Router dispatch.** Production code calls `ZyxelDriver::getInstance()` and its existing `RouterDriver` methods. The stored payload names one of those methods and its arguments. The test binary links `RecordingRouter` in place of that implementation. Do not add `setRouterDriver()`, a `shared_ptr` registration slot, or a daemon runtime switch between the two.
4. **Revocation timing.** A password change becomes visible on the next locked vault read. It does not require a push into the daemon.
5. **Plan state.** The user approved execution on 2026-09-26. The implementer does not move this file to `plan/done/` or mark it `Done` until every envelope gate has passed and the user accepts the result.

### 2.4 Threat Model, Safety & Failure Invariants

- The AI client is untrusted. The `netmon` process and its local administrator
  are trusted.
- An unauthenticated web or CLI caller cannot approve, deny, change policy, or read audit and syslog.
- A non-loopback HTTP caller cannot reach those routes at all.
- The agent RPC interface gains no method that approves, denies, or changes policy.
- Expired, denied, and interrupted tickets are not executable.
- At most one in-process approval can move a ticket from `pending` to `executing`.
- Router failure returns a ticket to `pending`. A crash after the router call but before that result is committed becomes `interrupted`, never a second automatic router call.
- A password change in another process revokes sessions on their next validation through a locked disk read of the vault generation.
- `interrupted` is not an executable status. Only `applied` or `retry` reconciliation moves it, and only `retry` can later reach the router.
- Spoofed syslog cannot create or alter an audit decision.
- Vault unlock failure fails closed: no password change, no session, no live mutation.
- There is one administrator identity; role-based access control and
  multi-party approval are out of scope.
- Same-host compromise of the `netmon` process or the account that owns the
  audit file is outside the protection claim.

---

## 3. Staged Implementation Plan & Acceptance Criteria

### 3.0 Mandatory Qualification Protocol & Strict Progression Gate Rule

> [!CAUTION]
> **Strict Non-Negotiable Gate Rule**:
> - **All envelopes must be qualified with proper testing.**
> - **Proper Testing Standard (All Using CppUTest)**:
>   1. **Repeatable**: `make test` returns exit code 0 with no leaks.
>   2. **Coverage / Boundary / Fault**: nominal, boundary, and fault cases below are assertions, not manual notes.
>   3. **Formalized in CppUTest Across Scopes**: unit and integrated groups as named per envelope, then full `make test`.
>   4. **Zero-Mock Discipline**: real files, SQLite, UDP, and HTTP on loopback. No mock vault, checkpoint, or database. `RecordingRouter` is allowed only as the external-router adapter in the test binary.
> - **If tests do not pass, implementation CANNOT advance to the next envelope.**
> - **Failure Action Protocol**: stop, report the raw failure, fix it inside the current envelope, and re-run before touching the next envelope's files.

### 3.0.1 Implementer lock

`CMakeLists.txt` does not define CppUTest today, and `test/` does not exist. Envelope 1 creates both before any feature test. It adds `pkg_check_modules(CPPUTEST REQUIRED cpputest)`, one executable named `test_netmon_suite`, and a custom target named `test` that runs that executable with `-v` and no other arguments. The command is:

```text
cmake -S . -B build
cmake --build build --target test
```

The run fails the envelope unless the process exit code is 0 and the output contains `OK (<N> tests,`, where `<N>` is the cumulative count stated by that envelope.

Every required test is a `TEST(Group, Name)` macro in the named file. A missing macro fails the envelope even when the executable exits 0. Tests use real temporary files, SQLite, `fork` for the second vault process, real UDP datagrams, and real HTTP listeners. The test binary links `test/RecordingRouter.cxx` to count router executions. The `netmon` binary links `ZyxelDriver` and does not contain `RecordingRouter`.

---

### Envelope 1: Human authentication and break-glass

**Goal:** A local operator can set and use an admin password; a LAN HTTP client cannot.

Planned files:
- `include/AuthManager.hxx` — password and opaque session interface;
- `src/AuthManager.cxx` — vault access, constant-time compare, session table;
- `src/Main.cxx` — host-shell `set-password` that does not start the daemon;
- `src/WebServer.cxx` — loopback-only login and session check;
- `test/TestAuthManager.cxx` — the eleven Envelope 1 tests named below;
- `test/RecordingRouter.cxx` — router-call counter linked only into `test_netmon_suite`;
- `CMakeLists.txt` — CppUTest target `test_netmon_suite` and target `test`.

- [x] Task 1.1: Store the password and credential generation in the vault. Take `vault.lock` around every generation read and the atomic `vault.enc.tmp` rename. Bind each session to the generation read under that lock.
  - **Target Files**: `include/AuthManager.hxx`, `src/AuthManager.cxx`, `src/Main.cxx`
  - **Verification**: A second process runs `set-password` against the same vault while the daemon is alive. The daemon's next validation, without restart, rejects the old session. The generation used for that decision was read from disk under the lock.
- [x] Task 1.2: Keep the dashboard listener unchanged and add the admin listener at `127.0.0.1:3886`.
  - **Target Files**: `src/WebServer.cxx`, `include/WebServer.hxx`, `src/Config.cxx`, `include/Config.hxx`
  - **Verification**: Dashboard requests still succeed on port `3884`. Admin login succeeds on `127.0.0.1:3886` and is absent from the dashboard listener. Configuring the admin port as `3883`, `3884`, `3885`, or `16880` prevents startup.
- [x] Task 1.3: Add these tests to `test/TestAuthManager.cxx` and run the suite.
  - **Target Files**: `test/TestAuthManager.cxx`, `CMakeLists.txt`
  - **Required tests**:
    - Unit: `TEST(AuthManager, RejectsEmptyPassword)`, `TEST(AuthManager, AcceptsCorrectPassword)`, `TEST(AuthManager, RejectsWrongPassword)`, `TEST(AuthManager, SessionTokenIs32Bytes)`, `TEST(AuthManager, IdleSessionExpires)`, `TEST(AuthManager, LoginReadsVaultFromDisk)`, `TEST(AuthManager, MissingVaultFailsClosed)`.
    - Fault: `TEST(AuthManager, SecondProcessGenerationRevokesLiveSession)`.
    - Integration: `TEST(Integration_LoopbackAuth, LoginSucceedsOn3886)`, `TEST(Integration_LoopbackAuth, DashboardListenerHasNoAdminRoute)`, `TEST(Integration_LoopbackAuth, ReservedAdminPortRefusesStartup)`.
  - **Verification**: `cmake --build build --target test` exits 0 and prints `OK (11 tests,`. Each macro is present in the source.

**Hardstop:** Do not start the approval queue until Envelope 1 tests pass. An admin route on the dashboard listener, or a session that survives a vault generation change, is a failed envelope.

---

### Envelope 2: Durable approval queue

**Goal:** Agent mutations wait as exact stored payloads until an authenticated human acts.

Planned files:
- `include/SecurityCheckpoint.hxx` — policy and queue operations;
- `src/SecurityCheckpoint.cxx` — gate, enqueue, approve, deny, expiry;
- `include/SnmpDatabase.hxx` — pending-action and audit schema;
- `src/SnmpDatabase.cxx` — transactional updates;
- `test/TestApprovalQueue.cxx` — queue qualification.

- [x] Task 2.1: Persist the canonical payload and enforce `require_approval` by default.
  - **Target Files**: `include/SecurityCheckpoint.hxx`, `src/SecurityCheckpoint.cxx`, `include/SnmpDatabase.hxx`, `src/SnmpDatabase.cxx`
  - **Verification**: Restarting the process reloads the same pending payload. Two overlapping approvals produce one transition to `executing` and one router call. A restarted `executing` ticket becomes `interrupted` and makes no router call. `reconcile applied` audits it without router I/O. `reconcile retry` returns it to `pending`, and only a later approval calls the router. The agent RPC surface has no approve, reconcile, or policy method.
- [x] Task 2.2: Add these tests to `test/TestApprovalQueue.cxx` and run the full suite, including Envelope 1.
  - **Target Files**: `test/TestApprovalQueue.cxx`, `test/RecordingRouter.cxx`, `CMakeLists.txt`
  - **Required tests**:
    - Unit: `TEST(ApprovalQueue, DefaultModeIsRequireApproval)`, `TEST(ApprovalQueue, DisabledDoesNotCallRouter)`, `TEST(ApprovalQueue, DryRunDoesNotCallRouter)`, `TEST(ApprovalQueue, EnqueueStoresCanonicalPayload)`, `TEST(ApprovalQueue, ApproveReplaysStoredPayload)`, `TEST(ApprovalQueue, DenyDoesNotCallRouter)`.
    - Boundary: `TEST(ApprovalQueue, ExpiredTicketCannotBeApproved)`, `TEST(ApprovalQueue, ProtectedAddressSurvivesApproval)`.
    - Fault: `TEST(ApprovalQueue, RouterFailureReturnsPending)`, `TEST(ApprovalQueue, CrashWhileExecutingBecomesInterrupted)`, `TEST(ApprovalQueue, ReconcileAppliedDoesNotCallRouter)`, `TEST(ApprovalQueue, ReconcileRetryThenApproveCallsRouterOnce)`, `TEST(ApprovalQueue, ConcurrentApproveAffectsOneRow)`.
    - Integration: `TEST(ApprovalQueue, RestartReloadsPayload)`, `TEST(Integration_ApprovalRestart, SecondInstanceApprovesStoredPayload)`, `TEST(ApprovalQueue, AgentInterfaceHasNoApproveMethod)`.
  - **Verification**: `cmake --build build --target test` exits 0 and prints `OK (27 tests,`. Envelope 1 tests still pass in the same run.

**Hardstop:** Do not add operator presentation or syslog until a restarted process can approve only the original stored payload.

---

### Envelope 3: Audit record and operator surfaces

**Goal:** Decisions are queryable and chained, and only an authenticated local operator can view or act on them.

Planned files:
- `src/SecurityCheckpoint.cxx` — commit the audit outbox with the decision, then project the file;
- `src/NetMonShell.cxx` — authenticated operator commands;
- `include/NetMonShell.hxx` — command declarations;
- `src/WebServer.cxx` — authenticated loopback routes;
- `web/index.html`, `web/style.css`, `web/app.js`, `include/WebAssets.hxx` — local admin UI;
- `test/TestAuditTrail.cxx` — chain and access qualification.

- [x] Task 3.1: Commit each decision and its `audit_outbox` row in one transaction, project the file from that outbox, and expose the operator controls on the admin listener.
  - **Target Files**: `src/SecurityCheckpoint.cxx`, `src/SnmpDatabase.cxx`, `src/NetMonShell.cxx`, `src/WebServer.cxx`, `web/index.html`, `web/app.js`, `web/style.css`, `include/WebAssets.hxx`
  - **Verification**: Killing the process after the SQLite commit but before the file append is repaired from the outbox on startup. A mismatched file is replaced from SQLite. Unauthenticated admin-listener reads fail. CLI password change requires the current password and increments the generation.
- [x] Task 3.2: Add these tests to `test/TestAuditTrail.cxx` and run the full suite, including Envelopes 1 and 2.
  - **Target Files**: `test/TestAuditTrail.cxx`, `CMakeLists.txt`
  - **Required tests**:
    - Unit: `TEST(AuditChain, ApproveWritesSqliteAndOutbox)`, `TEST(AuditChain, DenyWritesSqliteAndOutbox)`.
    - Boundary: `TEST(AuditChain, FirstRecordHasEmptyPreviousHash)`, `TEST(AuditChain, SecondRecordLinksToFirst)`.
    - Fault: `TEST(AuditChain, CrashBeforeProjectionIsRepaired)`, `TEST(AuditChain, MismatchedFileIsRebuiltFromSqlite)`.
    - Integration: `TEST(Integration_OperatorAccess, MissingBearerIsRejected)`, `TEST(Integration_OperatorAccess, CliPasswordChangeRequiresCurrentPassword)`, `TEST(Integration_OperatorAccess, CliPasswordChangeIncrementsGeneration)`.
  - **Verification**: `cmake --build build --target test` exits 0 and prints `OK (36 tests,`. Envelope 1 and 2 tests still pass in the same run.

**Hardstop:** Do not ingest syslog until the audit chain and authentication checks pass.

---

### Envelope 4: Router syslog ingest

**Goal:** Only the configured router can add syslog events, and those events do not become audit proof.

Planned files:
- `include/SyslogServer.hxx` — UDP listener and parser interface;
- `src/SyslogServer.cxx` — source filter, RFC 3164/5424 parse, storage;
- `src/SnmpDatabase.cxx` — `syslog_events` table;
- `src/WebServer.cxx`, `src/NetMonShell.cxx` — authenticated read surfaces;
- `test/TestSyslogIngest.cxx` — datagram qualification.

- [x] Task 4.1: Bind UDP `1514`, drop other sources, and store a time-adjacent flag without changing audit status.
  - **Target Files**: `include/SyslogServer.hxx`, `src/SyslogServer.cxx`, `src/SnmpDatabase.cxx`
  - **Verification**: A datagram from the configured router is stored. A datagram from another address is not. Approving or denying a ticket does not depend on a syslog row.
- [x] Task 4.2: Add these tests to `test/TestSyslogIngest.cxx` and run the full suite, including Envelopes 1 through 3.
  - **Target Files**: `test/TestSyslogIngest.cxx`, `CMakeLists.txt`
  - **Required tests**:
    - Unit: `TEST(SyslogParser, AcceptsRfc3164FromRouter)`, `TEST(SyslogParser, AcceptsRfc5424FromRouter)`.
    - Boundary: `TEST(SyslogParser, DropsEmptyDatagram)`, `TEST(SyslogParser, DropsOversizedDatagram)`.
    - Fault: `TEST(SyslogParser, DropsMalformedPriority)`, `TEST(Integration_SyslogFilter, DropsNonRouterSource)`, `TEST(Integration_SyslogFilter, TimeAdjacentDoesNotChangeAuditStatus)`.
    - Integration: `TEST(Integration_SyslogFilter, AuthenticatedLoopbackReadReturnsLabel)`.
  - **Verification**: `cmake --build build --target test` exits 0 and prints `OK (44 tests,`. Every test from Envelopes 1 through 3 still passes in that same run.

**Hardstop:** The plan is not complete while a non-router datagram can enter `syslog_events` or while syslog can change an approval result.

---

### 3.8 Mandatory Verification and Validation Gates Reference

#### 3.8.1 Authentication gate
- **CppUTest Groups**: Unit (`TEST_GROUP(AuthManager)`), Integrated (`TEST_GROUP(Integration_LoopbackAuth)`)
- **Nominal Coverage**: Password set, verify, session create, and generation mismatch.
- **Boundary Conditions**: Empty password and idle expiry.
- **Fault Injection**: Missing vault, locked generation increment by a live second process, admin request on the dashboard listener, and reserved admin port.
- **Regression Verification**: `make test` passes all accumulated groups.
- **Acceptance Threshold**: 100% assertions pass, 0 memory leaks, exit code 0.

#### 3.8.2 Approval gate
- **CppUTest Groups**: Unit (`TEST_GROUP(ApprovalQueue)`), Integrated (`TEST_GROUP(Integration_ApprovalRestart)`)
- **Nominal Coverage**: Enqueue, restart, approve stored payload, deny.
- **Boundary Conditions**: Expiry and protected-address rejection.
- **Fault Injection**: Router failure returns the ticket to pending. A crash while executing becomes interrupted. Reconcile `applied` does not call the router. Reconcile `retry` requires a new approval before router I/O. Concurrent approval affects one row.
- **Regression Verification**: `make test` passes all accumulated groups.
- **Acceptance Threshold**: 100% assertions pass, 0 memory leaks, exit code 0.

#### 3.8.3 Audit and operator gate
- **CppUTest Groups**: Unit (`TEST_GROUP(AuditChain)`), Integrated (`TEST_GROUP(Integration_OperatorAccess)`)
- **Nominal Coverage**: SQLite row and linked audit line for approve and deny.
- **Boundary Conditions**: First previous-hash is empty.
- **Fault Injection**: Crash before projection is repaired from the outbox. A mismatched file is rebuilt. Missing bearer token is rejected.
- **Regression Verification**: `make test` passes all accumulated groups.
- **Acceptance Threshold**: 100% assertions pass, 0 memory leaks, exit code 0.

#### 3.8.4 Syslog gate
- **CppUTest Groups**: Unit (`TEST_GROUP(SyslogParser)`), Integrated (`TEST_GROUP(Integration_SyslogFilter)`)
- **Nominal Coverage**: Router datagram stored and readable by an authenticated local caller.
- **Boundary Conditions**: Empty and oversized datagrams.
- **Fault Injection**: Non-router source dropped. Audit status unchanged.
- **Regression Verification**: `make test` passes all accumulated groups.
- **Acceptance Threshold**: 100% assertions pass, 0 memory leaks, exit code 0.

---

### 3.9 Completion Definition

The architecture is freeze-ready when this plan is approved. The implementation is complete only when:

1. All envelopes execute in order without crossing a hardstop.
2. Every gate in Section 3.8 passes with 100% verification.
3. `cmake --build build --target test` prints `OK (44 tests,` with exit code 0 and no memory leak. All 44 `TEST()` macros required by this plan are present.
4. The dashboard listener exposes no admin route. Admin HTTP is on `127.0.0.1:3886`. A live second process changing the vault password makes existing sessions fail on the next locked generation read.
5. Concurrent approval causes one router call. An interrupted execution is not replayed until the operator reconciles it as `retry` and approves it again. `applied` records the outcome without router I/O. The audit file matches the committed outbox after recovery.
6. An approved action replays its stored payload, and a spoofed syslog datagram neither enters storage nor changes that decision.
7. Implementation changes are reviewed and committed only on explicit user instruction.

---

## Appendix. Lifecycle Transition & Status Log

| Date | Previous State | New State | Lifecycle Directory | Notes / Rationale |
|---|---|---|---|---|
| 2026-09-26 | Unknown | `Proposed` | `plan/old/` | Reformatted to the single-agent execution template. |
| 2026-09-26 | `Proposed` | `Proposed` | `plan/` | Promoted from `plan/old/` after the CppUTest bootstrap and `RecordingRouter` boundary were frozen. Not approved for execution. |
| 2026-09-26 | `Proposed` | `Executing` | `plan/` | User approved execution. |
| 2026-09-26 | `Executing` | `Done` | `plan/done/` | Qualification passed: `cmake --build build --target test` exited 0 with `OK (44 tests, 44 ran, 332 checks, 0 ignored, 0 filtered out, 7983 ms)` and no memory leak. Not committed. |

### Checkpoints & Iteration Log
- 2026-09-26: Architectural pass. Admin traffic restricted to loopback HTTP, approval bound to a stored payload, audit claim narrowed to an append-only hash chain, and syslog demoted to filtered time-adjacent evidence.
- 2026-09-26: User clarified the threat model: untrusted agent requests,
  local-only single-administrator operation, and an operational audit trail.
  Compromise of `netmon` or the host is outside scope.
- 2026-09-26: Closed the remaining architecture gaps. Session revocation uses a
  vault credential generation, approval claims `executing` before router I/O,
  SQLite `audit_outbox` is the audit authority, and admin HTTP is a separate
  `127.0.0.1` listener. The existing dashboard listener stays monitoring-only.
- 2026-09-26: Closed the execution contracts. Vault generation reads and writes
  use `vault.lock` plus the existing atomic rename, with no cached generation.
  Interrupted tickets require operator reconcile `applied` or `retry`.
  The admin listener defaults to `127.0.0.1:3886`.
- 2026-09-26: Locked qualification to one CppUTest executable. Each envelope
  names its required `TEST()` macros and the cumulative `OK (N tests,`
  count: 11, 27, 36, then 44.
- 2026-09-26: Recorded that CppUTest is absent from the tree and must be
  created in Envelope 1. Permitted `RecordingRouter` only as the external
  router adapter in the test binary. Promoted this plan to `plan/` as
  `Proposed`.
- 2026-09-26: Froze the vault record, audit path, and router dispatch so
  Envelope 1 cannot invent a second design.
- 2026-09-26: Independent qualification rerun passed all 44 required
  `TEST()` macros, including the prior syslog leak and concurrent-approve
  crash. Dashboard listener has no admin route. Admin HTTP is the JSON API
  on `127.0.0.1:3886`. Operator actions are the host CLI and that API.
  Moved to `plan/done/` without a git commit.
