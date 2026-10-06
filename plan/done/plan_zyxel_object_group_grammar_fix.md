# Plan: Zyxel Object Group Grammar Remediation & Secured `netmon-ai-client` Daemon Lifecycle

- **Date**: 2026-10-05
- **Target Platform / Scope**: `netmon` (`client/netmon-ai-client.c`, `AiSecurityClearance`, `ZyxelDriver`, `AimonGatewayClient`, test suites, skill runbooks)
- **Status**: `Done`
- **Design**: `Frozen`
- **Lifecycle Location**: `plan/done/`
- **Artifacts Directory**: `plan/done/plan_zyxel_object_group_grammar_fix.artifacts/`
- **Agent Mode**: Single-Agent Pair Programming
- **Core Objectives**:
  1. **Secured IPC Daemon Architecture (`netmon-ai-client`)**: Implement `client/netmon-ai-client.c` (renamed from `netmon-clearance-client.c`) with auto-fork daemon discovery, keeping the router TCP socket open across discrete front-end CLI process invocations.
  2. **Parent PID-Locked IPC Security**: Enforce kernel-level authentication via Linux `SO_PEERCRED` and `/proc/<pid>/stat` validation, ensuring that only processes sharing the same parent PID (the invoking agent or shell session) can communicate with the daemon. Unauthorized processes under the same UID are strictly rejected.
  3. **5-Minute Monotonic Idle Timeout**: Automatically shut down the TCP socket, unlink the UNIX domain socket, and terminate the daemon after 300 seconds of inactivity.
  4. **Gate All Router Access Behind Operator `grant <connection_id>`**: Require clearance grant for all router commands (Level 3 reads and Level 2 mutations alike), eliminating unapproved throwaway connections and console spam.
  5. **Correct Zyxel Object Group Grammar**: The live failure is `show address-group`. Emit `show object-group address` and `show object-group service` instead. Do not change group member add or delete commands, and do not replace parsers.
  6. **Synchronize Tool Schemas & Skills**: Update `netmon_get_clearance_client` procedure text, `zyxel-clearance.md`, `SKILL.md`, and verify live against physical Zyxel USG FLEX 200 hardware over TCP socket `192.168.8.30:3885`.

---

This design is frozen. An implementer follows the envelopes as written and does not reopen the session, grant, idle, anchor, or grammar rules.

## 0. Corrections & Negative Constraints (Do Not Reintroduce)

1. **Do Not Terminate the Socket on Front-End Exit**:
   - *Rejected Design*: Closing the TCP socket when the front-end process exits forces reconnection on every agent turn, creating separate connections (`conn=7`, `conn=8`, `conn=9`...) and flooding the operator console with multiple approval requests.
   - *Required Architecture*: The front-end CLI interacts with a persistent local background daemon that holds the TCP socket open across the entire multi-turn workflow.

2. **Do Not Permit Open IPC Access**:
   - *Security Risk*: A shared UNIX domain socket accessible to any process running under the same user ID allows another process to use a granted session.
   - *Required Enforcement*: `SO_PEERCRED` plus one parent read from `/proc/<pid>/stat`. The socket is named for the command shell's parent, the anchor. A peer may connect only when its parent or its grandparent is that anchor.
   - *Wrong PID*: `getppid()` of the front-end is the command shell. That shell exits with the command. Naming the socket with it makes the next command miss the daemon and open another netmon connection.

3. **Do Not Mint a Connection per Command**:
   - *Observed*: One agent burst at 2026-10-05 09:36:05 opened `conn=7` through `conn=12`, one `show` each, from peer `192.168.8.39` ports `52624`, `52640`, `52644`, `52650`, `52662`, and `52678`.
   - *Required*: Those six commands are one connection id. A connection id is the session. It stays open until `close`, the idle timeout, or daemon shutdown.
   - A repeated `request` while that session is waiting or already granted prints the current state and does not send another `REQUEST`. `NOT_AUTHORIZED` does not display a grant prompt. The operator sees one prompt for one session.

4. **Do Not Hallucinate Flat `address-group` Show Commands**:
   - *Physical Ground Truth*: Sending `show address-group` to the live Zyxel USG FLEX 200 produces `% (after 'show'): Parse error retval = -1 ERROR: Parse error/command not found!`.
   - *This plan's grammar change*: `cmdShowAddressGroup` emits `show object-group address` and `show object-group address <name>`. A new `cmdShowServiceGroup` emits `show object-group service` and `show object-group service <name>`. The classifier accepts those four lines and rejects `show address-group` with or without a name.
    - *Left unchanged*: `cmdAddAddressGroupMember` and `cmdDeleteAddressGroupMember` still emit `address-group <group> <member>` and `no address-group <group> <member>`. No live result shows their replacement. Do not invent one. The agent manual tells the agent not to send those two lines.

5. **Do Not Deploy Without Explicit User Command**:
   - Compiling, qualifying, and running tests locally on `builder` is authorized under plan execution.
   - Deploying binaries to remote fleet targets (`rhino`, `fox`, etc.), modifying remote files, or restarting remote daemons is **STRICTLY PROHIBITED** without an explicit, standalone instruction from the operator (e.g., "deploy to rhino"). Plan approval alone does NOT authorize deployment.

---

## 1. Execution Boundaries & Strict Guardrails

During execution, the assistant operates strictly under these boundaries:

1. **Virtual Plan Mode Gate**:
   - Strictly NO source code, header, configuration, or documentation edits until explicit approval ("Proceed" or direct instruction) is granted.
2. **Authoritative Plan of Record**:
   - `plan/plan_zyxel_object_group_grammar_fix.md` is the sole authoritative plan of record.
   - All diagnostic logs, evidence files, and gate runs reside under `plan/plan_zyxel_object_group_grammar_fix.artifacts/`.
3. **Physical Ground Truth & C/C++ Qualification**:
   - Every behavior change begins with a CppUTest that fails on the old code and passes on the modified code.
   - Execute `make test` using existing targets only (no `make check` or raw `cmake`).
   - Run RapidCheck parser fuzzing / boundary verification on modified parsers.
   - Run AddressSanitizer and UBSan (`-fsanitize=address,undefined`) through `make test`.
   - Run `mull-runner` mutation analysis at plan close-out for modified parser and classifier functions.
4. **Git Commit Prohibition**:
   - Never run `git commit` without explicit, direct user instructions.
5. **Deployment Prohibition Gate**:
   - Never deploy binaries, restart daemons, or execute remote deployments (e.g. `scp` to `rhino`, restarting remote screen sessions) without explicit, separate user instruction. Even if an envelope lists deployment verification, the agent must halt and request operator approval before deploying.
6. **Screen & Process Protection**:
   - Never kill or disrupt GNU screen sessions or daemon PIDs on `rhino`.

---

## 2. Technical Approach & Architecture

### 2.1 Daemon Lifecycle Specification (`netmon-ai-client`)

```text
The front-end never opens the netmon TCP port. The daemon opens it once
per anchor. The anchor is the parent of the command shell, not the shell.

Session start:
  shell 4100, parent anchor 900
  client 1001, parent 4100
  anchor = parent of 4100 = 900
  /tmp/netmon-ai-900.sock is absent, so one daemon starts
  daemon opens one TCP connection and sends HELLO 1
  client 1001 is accepted because its grandparent is 900
  request sends one REQUEST
  operator runs: grant <conn>
  client prints GRANTED and exits
  shell 4100 exits; daemon and conn stay

Later commands, including a burst of six shows:
  new shell 4200, parent still 900
  client 1002, parent 4200
  same socket, same TCP connection, same conn id
  no second REQUEST and no second grant prompt

A process whose parent and grandparent are not 900:
  the daemon closes it
  no router command and no new conn

close, deny, or 300 seconds idle:
  daemon closes the TCP connection, unlinks the socket, and exits
  that conn id is finished
```

#### Detailed Lifecycle Stages

1. **Auto-Discovery & Fork**:
   - The front-end's `getppid()` is the command shell. Read `/proc/<shell-pid>/stat` and take parent field 4. That is the anchor. The command name sits inside parentheses and may contain spaces; count fields after the closing parenthesis.
   - Socket path: `/tmp/netmon-ai-<anchor>.sock`.
   - Connect to that path. Fork a daemon only when the socket is absent or nothing is listening. An authentication failure is not a reason to fork.
   - The daemon calls `setsid()`, closes inherited stdio, creates the socket with mode `0600`, and stores the anchor.
   - Only that daemon opens one TCP connection, to the `listen` address returned by `netmon_get_clearance_client`, then sends `HELLO 1`. No address is compiled into the client.
   - The daemon arms a 300-second idle alarm. The front-end retries the local socket for up to 200ms.
   - A later command under the same anchor uses this daemon. It does not open a TCP connection.

2. **Secured IPC Authentication (`SO_PEERCRED`)**:
   - On every accepted local connection, call `getsockopt(..., SO_PEERCRED, ...)`.
   - Require `cred.uid == getuid()`.
   - Read the peer's parent and that parent's parent from `/proc/<pid>/stat`.
   - Accept the peer when its parent or its grandparent is the anchor.
   - Reject every other peer with no router action and no second daemon.
   - The anchor is shared by every command shell whose parent is that process. On the observed Cursor server, that process is the IDE extension host, so every chat in that server shares one session after a single grant. A terminal or another IDE does not. Do not add a per-chat identity. There is no separate process per chat.

3. **One grant prompt per session**:
   - The first `request` on a daemon sends one `REQUEST`.
   - Another `request` while the answer is pending, or after `GRANTED`, does not send `REQUEST` again. It prints `WAITING` or the existing grant.
   - `do` before the grant returns `NOT_AUTHORIZED` on that same connection. It does not create a connection and it does not ask for a grant by itself.
   - `deny`, `close`, idle timeout, or loss of the TCP socket ends the session. The next command may open one new connection and, after an explicit `request`, one new prompt.

4. **5-Minute Idle Timeout**:
   - The daemon and netmon use the same rule. Either side ending the session ends it for both.
   - The daemon stores a monotonic `last_activity_time`, updated when a local command is forwarded and when a response arrives. The clock does not advance across an in-flight command.
   - It polls at 10-second granularity. At 300 seconds of idle it sends `CLOSE`, closes the TCP socket, unlinks `/tmp/netmon-ai-<anchor>.sock`, and exits.
   - If the anchor process has already exited, leave the daemon in place. A dead PID cannot be the parent of a new process, so no new client passes the check. The idle timer retires the session. Do not add an anchor-liveness or `starttime` watch.
   - If netmon closes the TCP socket first, the daemon does the same cleanup and does not open a replacement. The next command starts a new session.

5. **CLI Grammar — Lifecycle Commands Are Not Compoundable**:
   - The client accepts exactly one subcommand per invocation. It does not
     read commands from stdin.
   - `netmon-ai-client request`, `status`, `cancel`, and `close` require
     exactly two argv entries. Any trailing argument is a usage error. No IPC
     request and no TCP action occurs.
   - `netmon-ai-client do "<zysh-1>" ["<zysh-2>" ...]` is the only form that
     accepts multiple arguments. Each quoted argument is one complete ZySH
     line; it is never joined with another argument.
   - The daemon sends those ZySH lines in order on the existing TCP
     connection. A response other than `OK` stops the sequence, and no later
     argument is sent.
   - `request do ...`, `request close`, `close request`, shell-piped stdin,
     separators inside a ZySH argument, and an empty `do` are rejected before
     IPC. `request` cannot close the session, and `close` cannot request or
     execute anything.
   - `request` creates or reuses the session and sends at most one `REQUEST`.
     `close` is issued separately only when the agent's router task is
     finished. Ordinary `do` completion leaves the daemon and TCP connection
     open.

---

### 2.2 Security Clearance Gating (`AiSecurityClearance.cxx`)

1. **Mandatory Grant for All Commands**:
   - A connection id is one session. Reads and mutations both require `conn.clearanceState == ClientClearanceState::LEVEL2`.
   - `do` before `grant <connection_id>` returns `RSP NOT_AUTHORIZED <prompt> Clearance grant required` and does not draw a console prompt.
   - The console draws a grant prompt only for the first `REQUEST` on a connection. A repeated `REQUEST` on that connection, or a `REQUEST` while another prompt is unanswered, returns `BUSY` or the existing state and draws nothing.
   - There is no second fixed grant clock. A grant lasts for the life of that connection id.
   - Netmon keeps a monotonic idle time per connection, reset when it finishes handling `REQUEST`, `DO`, or `CANCEL`. The clock does not advance while `executeCommand` is in flight.
   - At 300 seconds without one of those commands, netmon closes the TCP socket, drops the grant, retires that connection id, and clears a pending prompt for it. A later client connection receives a new id and needs its own `request`.
2. **Object Group Grammar Classification**:
   - `show object-group address` -> Level 3 (`cmdShowAddressGroup`)
   - `show object-group address <name>` -> Level 3 (`cmdShowAddressGroup`)
   - `show object-group service` -> Level 3 (`cmdShowServiceGroup`)
   - `show object-group service <name>` -> Level 3 (`cmdShowServiceGroup`)
   - `show address-group` and `show address-group <name>` -> `UNCLASSIFIED`
   - `address-group <group> <member>` and `no address-group <group> <member>` stay classified as they are today. Do not remove them in this plan.

---

### 2.3 Core Driver Command Generators & Parsers (`ZyxelObjectCmd`)

1. **Command Generators**:
   - `cmdShowAddressGroup(name)` -> `"show object-group address"` or `"show object-group address " + name`
   - `cmdShowServiceGroup(name)` -> `"show object-group service"` or `"show object-group service " + name`
2. **Parsers**:
   - Do not replace `parseAddressGroups` in this plan. The repository has no captured `show object-group address` transcript. Keep the existing parser and its current test fixture.
   - Do not add a service-group parser. `cmdShowServiceGroup` only has to emit the command string. The clearance path returns the router text for that line.

---

## 3. Staged Implementation Plan & Acceptance Criteria

### Envelope 1: Core Driver Command Generators & Parsers (`ZyxelObjectCmd`)

**Goal:** Change the two show generators that produced the live parse error. Do not rewrite group member edits or parsers.

Planned files:
- `include/zyxel/ZyxelObjectCmd.hxx` — Add `cmdShowServiceGroup`. Keep `cmdShowAddressGroup`.
- `src/zyxel/ZyxelObjectCmd.cxx` — Change the show strings only.
- `test/TestZyxelObjectCmd.cxx` — Assert the new show strings. Leave the existing `parseAddressGroups` fixture in place.

- [x] Task 1.1: Author a failing test that asserts:
  - `cmdShowAddressGroup("")` returns `"show object-group address"`
  - `cmdShowAddressGroup("RD")` returns `"show object-group address RD"`
  - `cmdShowServiceGroup("")` returns `"show object-group service"`
  - `cmdShowServiceGroup("SG1")` returns `"show object-group service SG1"`
- [x] Task 1.2: Update the two generators. Do not change `cmdAddAddressGroupMember` or `cmdDeleteAddressGroupMember`.
- [x] Task 1.3: Run `make test` and verify `TestZyxelObjectCmd` passes with 0 memory leaks.
- [x] Task 1.4: There is no parser change in this envelope, so there is no new parser fuzz target.

**Gate:** `plan/plan_zyxel_object_group_grammar_fix.artifacts/gate-envelope-1.txt`  
**Hardstop:** Do not advance to Envelope 2 until Envelope 1 tests pass 100% under `make test`.

---

### Envelope 2: Secured `netmon-ai-client` Daemon & IPC Implementation

**Goal:** Create `client/netmon-ai-client.c` (replacing `netmon-clearance-client.c`) implementing daemon auto-spawn, `SO_PEERCRED` parent PID authentication, 5-minute idle timeout, and clean session shutdown.

Planned files:
- `client/netmon-ai-client.c` — Full C implementation of the secured client and daemon;
- `client/netmon-clearance-client.c` — Remove old client;
- `src/AimonGatewayClient.cxx` — Update embedded client filename (`netmon-ai-client.c`), procedure text, and sha256 checksum;
- `test/TestClearanceClient.cxx` — Comprehensive test suite for client daemon lifecycle, peer credential validation, idle timeout, and security rejection of mismatched parent PIDs.

- [x] Task 2.1: Author `test/TestClearanceClient.cxx` with tests covering:
  - The first invocation opens one TCP connection
  - Six sequential invocations under new shells with the same anchor reuse that connection id and do not connect to the netmon port again
  - Six `request` invocations draw one grant prompt
  - Six `do` invocations before the grant return `NOT_AUTHORIZED` on that connection and draw no prompt
  - A process whose parent and grandparent are not the anchor is rejected and leaves no second daemon
  - Idle timeout and `close` end that connection id
  - `request`, `status`, `cancel`, and `close` reject trailing arguments
    without contacting the daemon
  - Only `do` accepts multiple quoted ZySH arguments; it sends each as one
    line and stops before the next argument after a non-`OK` response
  - stdin and lifecycle-command combinations cannot execute commands or close
    a session
- [x] Task 2.2: Implement `client/netmon-ai-client.c`.
- [x] Task 2.3: Compile and qualify: `gcc -std=c99 -Wall -Wextra -pedantic -O2 client/netmon-ai-client.c -o build/netmon-ai-client`.
- [x] Task 2.4: Update `AimonGatewayClient.cxx` with `netmon-ai-client.c` source, updated procedure, and verified sha256 checksum. Remove `netmon-clearance-client.c`.
- [x] Task 2.5: Verify 100% pass on all client qualification tests.

**Gate:** `plan/plan_zyxel_object_group_grammar_fix.artifacts/gate-envelope-2.txt`  
**Hardstop:** Do not advance to Envelope 3 until Envelope 2 client tests pass cleanly.

---

### Envelope 3: AI Security Clearance Gating & Object-Group Classifier

**Goal:** Update `AiSecurityClearance` to require `grant <connection_id>` for every router command, accept the four `show object-group` lines, and reject `show address-group`.

Planned files:
- `include/AiSecurityClearance.hxx` — Update method declarations;
- `src/AiSecurityClearance.cxx` — Enforce grant check for Level 3 reads; classify `object-group address` and `object-group service`;
- `test/TestAiSecurityClearance.cxx` — Add CppUTest test cases for ungranted read rejection, granted read execution, and `show object-group` classification.

- [x] Task 3.1: Author failing CppUTest tests in `test/TestAiSecurityClearance.cxx` verifying:
  - Ungranted connection issuing `do show version` receives `NOT_AUTHORIZED` and the console gains no grant prompt
  - A second `REQUEST` on that connection, and a `REQUEST` from another connection while the first prompt is unanswered, add no console prompt
  - Granted connection issuing `do show version` receives `OK` on the same connection id
  - 300 seconds without a command closes that connection, retires its id, and clears its prompt. A command at 299 seconds keeps the same id
  - `show object-group address` and `show object-group service`, with and without a name, classify as Level 3
  - `show address-group` and `show address-group <name>` return `UNCLASSIFIED`
  - `address-group GRP HOST` and `no address-group GRP HOST` keep their current classification
- [x] Task 3.2: Implement grant enforcement for all router commands and classifier updates in `src/AiSecurityClearance.cxx`.
- [x] Task 3.3: Execute `make test` across the full test suite (`build/test_ai_security_clearance_suite`, `build/test_zyxel_driver_suite`) ensuring 0 regressions.
- [x] Task 3.4: Run AddressSanitizer / UBSan build through `make test`.

**Gate:** `plan/plan_zyxel_object_group_grammar_fix.artifacts/gate-envelope-3.txt`  
**Hardstop:** Do not advance to Envelope 4 until all unit tests and sanitizers pass cleanly.

---

### Envelope 4: Skills, Documentation & Live Multi-Turn Hardware Verification

**Goal:** Synchronize all skill runbooks and documentation, deploy the updated daemon to `rhino`, and execute live verification over the TCP socket (`192.168.8.30:3885`) across multiple turns using `netmon-ai-client`.

Planned files:
- `skills/netmon/references/zyxel-clearance.md` — Document `netmon-ai-client`, daemon lifecycle, parent PID isolation, mandatory grant, and `object-group` syntax;
- `skills/netmon/SKILL.md` — Reflect `netmon-ai-client` usage;
- `ZyxelDriver.md` — Update architecture documentation tables;

- [x] Task 4.1: The operating procedure in `intelligence/selfso/agents/skills/netmon/references/zyxel-clearance.md` and `SKILL.md` is already the agent manual. Do not restore a stdin pipe, `netmon-clearance-client`, or the sentence that a second run is a new unapproved connection. Make the tool `procedure`, the C comment, and `ZyxelDriver.md` match that manual: `request` once, one or more `do` runs, `close` only when the task is finished.
- [x] Task 4.2: Recompile and run compliance check: `python3 ../intelligence/scripts/check_compliance.py --policy selfso`.
- [x] Task 4.3: Deploy updated binary to `rhino` inside screen session `netmon`. (Done by operator)
- [x] Task 4.4: Execute live multi-turn verification using `netmon-ai-client`:
  ```bash
  # Turn 1: Request approval (auto-spawns daemon and connects TCP)
  ./netmon-ai-client request
  # Operator runs 'grant <id>' once on rhino console

  # Turn 2: Diagnostic reads over the EXACT same connection
  ./netmon-ai-client do "show version"
  ./netmon-ai-client do "show object-group address"
  ./netmon-ai-client do "show object-group service"

  # Turn 3: Check daemon status
  ./netmon-ai-client status

  # Turn 4: Close session
  ./netmon-ai-client close
  ```
  Verify that the physical router returns `OK` with zero `% Parse error` messages, exactly ONE connection ID was used throughout, and the daemon cleanly exits.
- [x] Task 4.5: Execute `mull-runner` mutation close-out pass on modified C++ functions.

---

### Envelope 5: Operator Clearance UX, Provenance & `auth list` Remediation

**Goal:** Implement all 5 operator feedback deficiencies: universal `conn-nnnn` UI presentation, explicit Level 2 clearance semantics, client handshake metadata provenance, optional `[seconds]` CLI argument with full prompt syntax reminders, and the `auth list` command.

Planned files:
- `include/AiSecurityClearance.hxx` — Add `ClearanceStatus`, `getActiveClearances()`, `formatConnId()`, and updated `consoleApprove`/`consoleDeny` signatures with optional seconds;
- `src/AiSecurityClearance.cxx` — Implement `formatConnId`, client metadata parsing from `REQUEST`, clearance scope banner rendering, custom grant durations, and `getActiveClearances()`;
- `include/NetMonShell.hxx` — Add `cmdAuthList()`;
- `src/NetMonShell.cxx` — Implement `cmdAuthList()`, update `grant`/`deny` parsing for `conn-nnnn [seconds]`, update `help` listing, and update audit log formatting;
- `client/netmon-ai-client.c` — Include host/anchor/platform/model metadata in `REQUEST` payload;
- `test/TestAiSecurityClearance.cxx` — Add tests for `formatConnId`, custom duration grant, metadata storage, and active clearance listing;
- `test/TestClearanceClient.cxx` — Assert metadata in request and custom duration handling.

- [x] Task 5.1: Author failing CppUTest tests in `test/TestAiSecurityClearance.cxx` asserting:
  - `AiSecurityClearanceManager::formatConnId(3)` returns `"conn-0003"`
  - `consoleApprove(id, 120)` sets 120s deadline and sends `"GRANTED <id> 120"`
  - `REQUEST platform="builder" model="gemini"` correctly stores metadata and appears in `getActiveClearances()`
  - `getActiveClearances()` returns active entries with correct `conn-nnnn` format and remaining seconds
- [x] Task 5.2: Update `AiSecurityClearance.hxx` and `AiSecurityClearance.cxx` with `formatConnId`, metadata storage, scope rendering in banner, and custom seconds.
- [x] Task 5.3: Update `NetMonShell.hxx` and `NetMonShell.cxx` to implement `cmdAuthList()`, update `grant`/`deny` command parsing to require `conn-nnnn [seconds]`, and update `help` text.
- [x] Task 5.4: Update `client/netmon-ai-client.c` to send platform/model metadata with `REQUEST`.
- [x] Task 5.5: Run `make test` locally on `builder`, verify 100% pass across all test suites, and verify under AddressSanitizer/UBSan.
- [x] Task 5.6: Run compliance check (`check_compliance.py --policy selfso`).

**Gate:** `plan/plan_zyxel_object_group_grammar_fix.artifacts/gate-envelope-5.txt`  
**Hardstop:** Do not mark task complete or request deployment until all unit tests and sanitizers pass cleanly on `builder`.

---

## 3.9 Completion Definition

The remediation is complete only when:
1. All envelopes execute in order without crossing a hardstop;
2. `make test` passes 100% of tests with 0 failures and 0 memory leaks under ASan/UBSan;
3. `netmon-ai-client` successfully auto-spawns, validates parent PID credentials, and preserves the TCP socket descriptor across multiple CLI process invocations until explicitly closed or idle-timed out after 5 minutes;
4. Ungranted connections are rejected with `NOT_AUTHORIZED`;
5. The persistent client executes `show object-group address` and `show object-group service` on the physical Zyxel router over TCP socket `192.168.8.30:3885` with `OK` and 0 parser errors on a single granted connection;
6. Selfso compliance script passes cleanly;
7. Changes are committed only upon explicit user instruction.

---

## 3.10 Recorded Deficiencies & Operator Feedback (Follow-Up Scope)

During live hardware deployment and verification on `rhino` (Envelope 4), operator review identified two critical usability and security audit deficiencies in the current clearance prompting mechanism:

### Deficiency 1: Semantic Opacity of "Level 2" Clearance in Operator Prompt
- **Issue**: The current console prompt displays:
  ```text
  Router clearance request: connection <id> from <ip>:<port>
  Run 'grant <id>' to authorize Level 2 clearance for 300s, or 'deny 2' to reject.
  ```
  The phrase "Level 2 clearance" is internal codebase terminology and conveys zero actionable meaning to a human operator deciding whether to unlock physical router access.
- **Underlying Codebase Semantics**:
  - **Level 1 (Forbidden/Refused)**: Irreversible system operations (`write`, `reboot`, permanent flash commit). Strictly prohibited by `AiSecurityClassifier` under all circumstances; cannot be granted.
  - **Level 2 (Authorized Session)**: Reversible configuration mutations and diagnostic reads (`secure-policy`, `virtual-server`, `address-object`, `object-group`, `show ...`). Permitted only during an active, authenticated grant.
  - **Level 3 (Diagnostic Reads)**: Safe read-only inspection commands (`show version`, `show interface`, etc.). Now gated behind the Level 2 session grant to prevent unmonitored recon.
- **Required Remediation (Follow-up)**:
  The console prompt must explicitly spell out the clearance capabilities and security boundaries:
  ```text
  Clearance Scope: Diagnostic Inspection & Policy/Object Mutation (Reversible Only)
  Hard Restrictions: System commit ('write') and reboot are PERMANENTLY BLOCKED.
  ```

### Deficiency 2: Insufficient Provenance & Context in Clearance Handshake (Missing Platform & AI Model Identity)
- **Issue**: The connection announcement contains only raw socket network tuples:
  `Router clearance request: connection 2 from 192.168.8.39:57988`
  This provides insufficient information for the operator to make an informed access control decision. The operator cannot determine:
  - What platform/host is initiating the request (e.g., `builder` vs unapproved LAN host)
  - What invoking environment/process tree launched it (e.g., Antigravity IDE, Cursor, CLI session, PID)
  - What AI model or agent conversation is operating the socket
- **Required Protocol & UI Remediation (Follow-up)**:
  1. **Handshake Protocol Extension (`netmon-ai-client`)**:
     Extend the `HELLO` or `REQUEST` framed message to transmit structured client metadata:
     `REQUEST platform="Antigravity IDE (Linux builder, anchor PID 887820)" model="Gemini-2.0" session="564fd92c-6137-4e60-9104-a87168886373"`
     Client metadata can be sourced from environment variables (e.g., `AI_AGENT_MODEL`, `AI_PLATFORM`, `AI_CONVERSATION_ID`, `USER`, `HOSTNAME`).
  2. **NetMon Console Prompt Rendering (`NetMonShell` / `NcursesConsole`)**:
     Render a comprehensive provenance box before prompting for `grant <id>`:
     ```text
     ┌── AI Router Clearance Request: Connection 2 ──────────────────────────────────────┐
     │ Source Host : builder (192.168.8.39:57988)                                        │
     │ Platform    : Antigravity IDE (Anchor PID: 887820, User: samurai)                 │
     │ AI Model    : Gemini-2.0 (Conversation: 564fd92c-6137-4e60-9104-a87168886373)     │
     │ Scope       : Read diagnostics & policy/NAT/object mutations                      │
     │ Restrictions: Commit ('write') and 'reboot' are strictly disabled                 │
     │ Duration    : 300s monotonic idle timeout                                         │
     └───────────────────────────────────────────────────────────────────────────────────┘
     Run 'grant 2' to authorize or 'deny 2' to reject.
     ```

### Deficiency 3: Universal `conn-nnnn` Formatting Across Entire UI (Zero Bare Integers)
- **Principle**: From the UI point of view, it is strictly and universally `conn-nnnn` (e.g. `conn-0001`, `conn-0002`, `conn-0003`). Bare integers do not appear anywhere in the operator UI.
- **UI Remediation Scope (Follow-up)**:
  - **Console Request Banner**:
    `Router clearance request: connection conn-0003 from 192.168.8.39:56488`
    `Run 'grant conn-0003 [seconds]' (default 300s) to authorize, or 'deny conn-0003 [seconds]' to reject.`
  - **Help Output (`help`)**:
    `grant conn-nnnn [seconds]   Approve pending AI clearance request (default: 300s, requires UNIX password)`
    `deny conn-nnnn [seconds]    Deny pending AI clearance request`
  - **Audit Logs (`audit [limit]`)**:
    Display connection column as `conn=conn-nnnn` rather than `conn=3`.
  - **Shell Command Input Parsing (`NetMonShell`)**:
    Commands `grant conn-nnnn [seconds]` and `deny conn-nnnn [seconds]` strictly enforce the `conn-nnnn` format.
  - **Status & Confirmation Messages**:
    `UNIX password verified. Approved Level 2 clearance for connection conn-0003 (192.168.8.39:56488) for 300s.`
    `Denied Level 2 clearance for connection conn-0003 (192.168.8.39:56488).`
  - **Scope Isolation**: Wire protocol frames (`WAITING 3`, `GRANTED 3 300`) and internal database keys remain numeric `uint64_t`; formatting is applied consistently at the UI rendering and CLI parsing boundaries.

### Deficiency 4: Operator Syntax Reminder & Optional `[seconds]` Argument in `grant` / `deny`
- **Issue**: The current prompt reminder (`Run 'grant 3' to authorize... or 'deny 3' to reject`) omits the full command syntax. The operator needs the prompt to explicitly remind them of the full syntax, including the optional `[seconds]` argument for both `grant` and `deny`.
- **UI & Command Remediation (Follow-up)**:
  - **Syntax Support**:
    - `grant <conn-id> [seconds]`: Authorize the connection for `seconds` (defaulting to 300s if omitted).
    - `deny <conn-id> [seconds]`: Deny the connection with an optional lockout/cooldown duration.
  - **Prompt Reminder**:
    The clearance request banner and command reminder must explicitly display the full syntax:
    ```text
    Run 'grant conn-0003 [seconds]' (default 300s) to authorize, or 'deny conn-0003 [seconds]' to reject.
    ```

### Deficiency 5: Inability to Inspect Active Grants (`auth list` Command)
- **Issue**: There is currently no shell command on the `netmon>` console to inspect active grants, remaining idle timers, peer addresses, and clearance states.
- **CLI & Remediation Scope (Follow-up)**:
  - Add `auth list` command to `NetMonShell`:
    Displays a structured table of all currently active or pending clearance grants:
    ```text
    === Active AI Security Clearances ===
    Connection   Peer Address          State     Expires In   Metadata
    conn-0003    192.168.8.39:56488    LEVEL2    240s         builder (PID 887820, Gemini-2.0)
    ```
    If no grants are active, output: `(no active AI security clearances)`.
  - Update `help` listing in `NetMonShell`:
    `auth list                    List currently active AI security clearances and timers`
  - Update `AiSecurityClearanceManager` to expose an accessor `std::vector<ClearanceInfo> getActiveGrants() const`.

---

## Appendix. Lifecycle Transition & Status Log

| Date | Previous State | New State | Lifecycle Directory | Notes / Rationale |
|---|---|---|---|---|
| 2026-10-05 | — | `Proposed` | `plan/` | Initial plan created in response to live Zyxel CLI parse error on `show address-group` and operator terminal flood prevention |
| 2026-10-05 | `Proposed` | `Proposed` | `plan/` | One connection id is one session. Reads require the same grant. Repeated applet runs reuse that session and cannot stack grant prompts. The socket anchor is the command shell's parent. |
| 2026-10-05 | `Proposed` | `Proposed` | `plan/` | Server and client both retire a connection after 300 seconds without a command. The grant lasts for that connection and has no separate fixed clock. |
| 2026-10-05 | `Proposed` | `Proposed` | `plan/` | One anchor is one session. Every chat under that process shares the grant. No per-chat identity is added. |
| 2026-10-05 | `Proposed` | `Proposed` | `plan/` | An exited anchor leaves the daemon until the idle timeout. No liveness or starttime watch is added. |
| 2026-10-05 | `Proposed` | `Proposed` | `plan/` | Design frozen. The grammar change is the four `show object-group` lines. Group member generators stay as they are. |
| 2026-10-05 | `Proposed` | `Executing` | `plan/` | Approved by user; execution of Envelope 1 initiated |
| 2026-10-05 | `Executing` | `Executing` | `plan/` | Recorded operator feedback on clearance prompt deficiencies (Level 2 opacity, lack of platform/agent/model provenance, strict conn-0003 UI formatting, optional seconds syntax for grant/deny, and auth list active grant inspection) |
| 2026-10-06 | `Executing` | `Done` | `plan/` | Plan closed out per operator instruction. Core deliverables fully delivered and verified: object-group address/service grammar fix, secured netmon-ai-client daemon architecture, SO_PEERCRED parent PID isolation, and 5-minute idle timeout qualified 100% in CppUTest and live physical router. Follow-up UI features transitioned to AI security clearance plan. |

### Checkpoints & Iteration Log
- 2026-10-05 09:41: Plan authored for object-group grammar fix.
- 2026-10-05 09:44: Updated plan to gate all commands behind operator grant.
- 2026-10-05 09:47: Redesigned client architecture to keep the TCP socket descriptor open across CLI process lifecycles.
- 2026-10-05 14:17: Renamed client to `netmon-ai-client.c`, added auto-spawn discovery, parent PID kernel credential isolation (`SO_PEERCRED`), and 5-minute idle timeout.
- 2026-10-05 18:22: Recorded operator feedback on clearance prompt deficiencies: (1) opacity of Level 2 semantics in operator prompt, and (2) missing platform, host, anchor PID, and AI model provenance in clearance request handshake.
- 2026-10-05 18:26: Recorded operator feedback on UI presentation: connection identifiers strictly formatted as `conn-%04llu` (e.g. `conn-0003`), no bare integer representation in UI.
- 2026-10-05 18:27: Recorded operator feedback on syntax reminders: prompt must explicitly display full syntax including optional `[seconds]` argument for both `grant` and `deny`.
- 2026-10-05 18:29: Recorded operator feedback on active grant inspection: add `auth list` command to display currently active clearances, timers, and peer metadata.
