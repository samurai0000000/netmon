# Implementation Plan: AI Security Clearance & Router Telemetry

- **Status**: `Done` (Closed with known defect: WRITE mutations handed off to `plan_zyxel_clearance_write_mutations.md`)
- **Lifecycle Location**: `plan/done/`
- **Topic**: `ai_security_clearance`
- **Author**: Charles Chiou
- **Created**: 2026-10-04
- **Updated**: 2026-10-06
- **Artifacts Directory**: `plan/done/plan_ai_security_clearance.artifacts/`

---

## 0. Corrections & Negative Constraints (Do Not Reintroduce)

1. **FORBIDDEN: Ambient / Unapproved READ Access for AI Agents**:
   - *Negative Constraint*: Allowing newly connected clearance sockets or unauthenticated agents to run diagnostic/read commands (`show secure-policy`, `show address-object`, `show conn status`, `show version`, `ping`, `traceroute`) without explicit human operator authorization.
   - *Mandatory Alternative*: In accordance with operator specification, every newly accepted clearance socket starts with **ZERO authority (`NONE`)**. A socket cannot execute ANY `DO` command (read or write) until the human operator explicitly grants permission on the `netmon` console (`grant <conn_id> [seconds] read` or `grant <conn_id> [seconds] write`). Any `DO` command attempted before a grant returns `RSP NOT_AUTHORIZED <prompt> Clearance grant required for read`.
2. **FORBIDDEN: Ambiguous Client Instructions**:
   - *Negative Constraint*: Publishing documentation or applet headers claiming that "Reads need no approval" while the security policy requires authorization.
   - *Mandatory Alternative*: The MCP catalog description, the skill runbook (`zyxel-clearance.md`), and the published C client comment must consistently instruct the agent that `connect -> request <R|RW|RWP> -> wait for GRANTED -> do -> close` is mandatory for BOTH reads and mutations.
3. **FORBIDDEN: Zero-Argument / Implicit-Tier `request`**:
   - *Negative Constraint*: Invoking `request` with zero arguments, omitting the security level, or having `netmon` guess or default the requested tier.
   - *Mandatory Alternative*: In accordance with operator specification, the client's `request` method MUST explicitly request one of all 3 security levels: `R` (Read), `RW` (Read/Write), or `RWP` (Read/Write + Password Access). Omission of the security level argument or any unrecognized level string MUST be rejected immediately on both the client CLI and the server wire parser.
4. **FORBIDDEN: Bypassing UNIX Password Verification on Any Clearance Grant**:
   - *Negative Constraint*: Allowing `grant conn-nnnn [seconds] read` or any grant command to bypass interactive UNIX password authentication.
   - *Mandatory Alternative*: In accordance with the operator's security architecture (Section 1.1), **EVERY** clearance grant (`grant <connection_id>`), regardless of tier (`READ`, `READ_WRITE`, or `READ_WRITE_PASSWORD`), mandates interactive UNIX password authentication (`UnixAuth::authenticate()`). This prevents automated scripts or attackers injecting commands into the GNU screen session (`screen -X stuff "grant conn-xxxx\n"`) from silently granting read access to router state, credentials, and topology.
5. **FORBIDDEN: Hardcoded 24-Row PTY Terminal Height & Fragile Raw `--More--` Search**:
   - *Negative Constraint*: Allocating a standard 24-row interactive PTY for machine-driven SSH automation and relying on exact raw string search `rawBuffer.find("--More--")` that fails when ANSI escape sequences or control codes surround pagination prompts.
   - *Mandatory Alternative*: Request unbounded or maximum PTY height (`height = 0` or `height = 65535`) in `libssh2_channel_request_pty_ex` to eliminate terminal pagination at the PTY layer. Additionally, harden `--More--` pager detection against ANSI escape wrapping, automatically dispatching space to advance if pagination ever occurs.

---

This is the revised implementation plan incorporating operator-dictated
refinements. It replaces abstract "levels" with concrete security implication
tiers (`R` / `READ`, `RW` / `READ/WRITE`, `RWP` / `READ/WRITE + PASSWORD ACCESS!!!`),
mandates that `request` explicitly declare which of the 3 security levels is being
requested (`R|RW|RWP`), mandates explicit operator granting of `READ` permission
before any read command may execute, implements graceful downgrade to `READ` upon
grant/idle timeout for `READ/WRITE` sessions, adds active SSH keepalive telemetry
queries (`show conn status`, `show app statistics summary`, `show idp statistics summary`)
in `ZyxelDriver` to eliminate router-side idle disconnects, and exposes collected
security telemetry through the Web UI and an MCP tool.

## 1. Required behavior

1. An agent calls the read-only MCP tool `netmon_get_clearance_client` to
   retrieve one reviewed C source file (`netmon-ai-client.c`).
2. The agent compiles it and interacts with netmon over a persistent TCP
   connection (`netmon` port 3885).
3. **Security Clearance Tiers & Initial State**: Newly connected sockets start
   with **ZERO authority (`NONE`)**. Abstract numbers are eliminated. The clearance
   tiers reflect concrete security implications, and the client MUST specify its
   requested tier as an explicit argument to `request`:
   - **`NONE`**: Default ungranted connection state. Conveys zero command authority.
     Any `DO` command returns `RSP NOT_AUTHORIZED`.
   - **`R` (`READ`)**: Green text. Allows all read and diagnostic commands (`show *`,
     `ping`, `traceroute`). Default timeout is **0 (indefinite)** (custom seconds
     may still be set). Requested with `request R` (or `request read`).
     **Requires explicit operator approval (`grant <connection_id> [seconds] read`)**.
   - **`RW` (`READ/WRITE`)**: Red flashing text. Allows network, firewall, NAT, and
     object mutations. Default timeout is **300 seconds**. Requested with
     `request RW` (or `request write`). Requires operator grant
     (`grant <connection_id> [seconds] write`) and UNIX password authentication.
   - **`RWP` (`READ/WRITE + PASSWORD ACCESS!!!`)**: Bold red flashing text. For
     elevated mutations requiring interactive UNIX password authentication.
     Default timeout is **300 seconds**. Requested with `request RWP` (or
     `request password`).
4. **Graceful Downgrade Lifecycle**: When the idle timeout is reached or the
   active grant period expires on `READ/WRITE` or password-elevated sessions,
   the socket is **gracefully downgraded to `READ`** (since read authority was
   included in the operator's grant) instead of complete revocation or connection
   termination. Read and diagnostic operations can continue uninterrupted without
   re-prompting the operator. If a finite `READ` grant expires, authority returns
   to `NONE` (`NOT_AUTHORIZED`).
5. **Active SSH Keepalive via Security Telemetry**:
   - `ZyxelDriver` replaces passive libssh2 keepalives with periodic active
     CLI command execution (`show conn status`, `show app statistics summary`,
     `show idp statistics summary`) every 60 seconds.
   - Active CLI commands reset the physical Zyxel router's CLI shell idle
     timeout (`system-idle-timeout`), preventing `ERR_CHANNEL_FAILED` channel
     drops.
6. **Telemetry Persistence, Web Dashboard & MCP Integration**:
   - `ZyxelDriver` stores the latest parsed security telemetry in a
     mutex-protected in-memory cache with monotonic timestamps.
   - `WebServer` exposes `GET /api/firewall/metrics` and enriches
     `/api/firewall/status`.
   - `web/index.html` and `web/app.js` display a live "Firewall & Security
     Metrics" card showing active sessions/capacity, App Patrol counters
     (forwarded, dropped, rejected KB), and IDP threat interceptions.
   - `AimonGatewayClient` registers and handles the official MCP tool
     `firewall_get_metrics` so AI models across the workspace can query live
     threat and firewall metrics.

## 2. Threat model and trust boundary

- An untrusted process on the agent host can read transcripts and other disk
  files.
- It can retrieve and compile the public applet and can open its own TCP
  connection.
- It cannot write bytes into another process's already-established TCP socket.
- It cannot compromise netmon, the netmon console, or the router SSH process.
- Local processes (including background agents or subagents with shell access)
  cannot bypass the human approval gate by injecting keystrokes into the
  netmon GNU screen session (`screen -X stuff`). Every operator command
  that grants clearance (`grant <connection_id>`) unconditionally mandates interactive UNIX password
  authentication for the local running user across ALL clearance tiers (including `READ`, `READ_WRITE`, and `READ_WRITE_PASSWORD`).
- Process injection, descriptor theft, kernel compromise, and a compromised
  netmon host are outside this plan.

The accepted socket object inside netmon is the identity. Peer IP and port are
display and audit fields, not reusable credentials. No OTP, bearer token,
HMAC, session secret, firewall password, or approval capability is sent to the
agent.

A copied applet or a transcript scraper can open another connection, but that
connection remains in `NONE` state with zero authority until explicitly granted
clearance on the console. The 300-second window is therefore scoped to one
transport, not ambient authority on aimon or netmon.

## 3. Clearance model and driver-method policy

The client does not claim a level. netmon classifies each incoming line by
whether one current generator in section 3.3 could have emitted that exact
line, argument for argument. A unit-test sample string is not the allowlist.
Unknown, ambiguous, chained, multi-line, or control-character input is
rejected before `ZyxelSshClient::executeCommand`. Do not prepend
`configure terminal`. None of the current generators emit it.

| Security Clearance Tier | Console Presentation | Authority & Grant Lifecycle | Current Driver Methods |
|---|---|---|---|
| **`NONE`** | **White text** | Initial ungranted state upon connection; zero command authority. Every `DO` line is rejected with `NOT_AUTHORIZED`. | None |
| **`READ`** | **Green text** | Sockets approved with `grant <connection_id> [seconds] read`. **Default timeout: 0 (indefinite)** (custom seconds may still be set). When higher tiers expire, sockets gracefully downgrade to this tier. | `cmdShowVersion`, `cmdShowCpuStatus`, `cmdShowMemStatus`, `cmdShowConnStatus`, `cmdPing`, `cmdTraceroute`, `cmdShowInterfaces`, `cmdShowInterface`, `cmdShowIpRoute`, `cmdShowZones`, `cmdShowArp`, `cmdShowAddressObjects`, `cmdShowAddressGroup`, `cmdShowServiceObjects`, `cmdShowSecurePolicy`, `cmdShowVirtualServers` |
| **`READ/WRITE`** | **Red flashing text** | Only the socket approved with `grant <connection_id> [seconds] [write]`. **Default timeout: 300s**. On idle/period expiry, **gracefully downgrades to `READ`**. | `cmdAddRoute`, `cmdDeleteRoute`, `cmdAddAddressHost`, `cmdAddAddressRange`, `cmdAddAddressSubnet`, `cmdDeleteAddress`, `cmdAddAddressGroupMember`, `cmdDeleteAddressGroupMember`, `cmdAddService`, `cmdDeleteService`, `cmdInsertRule`, `cmdInsertFastDeny`, `cmdDeleteRule`, `cmdAddVirtualServer`, `cmdDeleteVirtualServer`, and their generated configuration-submode fragments |
| **`READ/WRITE + PASSWORD ACCESS!!!`** | **Bold red flashing text** | Critical operations requiring interactive UNIX password authentication on console. **Default timeout: 300s**. On idle/period expiry, **gracefully downgrades to `READ`**. | Destructive mutations, device management (`cmdWrite`, `cmdReboot`), or password-elevated changes. Device management remains console-only. |

This table is the only policy registry. Every current generator has exactly one
clearance tier. A generator added later fails the policy test until its emitted shape
is listed in section 3.3.

### 3.1 One router prompt

There is one SSH prompt. It is not per socket.

- The session owner is empty while `ZyxelSshClient`'s last matched prompt is a
  root prompt (`>` or `#` with no `(config` substring). That is the same test
  `unwindToRootPromptUnlocked` already uses.
- Any socket that has been granted `READ` or `READ/WRITE` may run a Level 3 line
  only while the owner is empty. An ungranted socket is rejected with
  `NOT_AUTHORIZED`.
- The socket that sends a Level 2 line which enters a submode becomes the
  owner. Today the only such line is `secure-policy insert <position>` from
  `cmdInsertRule` or `cmdInsertFastDeny`.
- While an owner is set, every other socket's `do` returns `BUSY` and is not
  sent to the router. Level 3 does not bypass this.
- Only the owner may send the submode fragments in section 3.3. `exit` is
  accepted only from the owner, and only while the prompt is still in that
  submode. A root-prompt `exit` is `UNCLASSIFIED`.
- After `exit` returns a root prompt, or after `unwindToRootPrompt` returns,
  the owner is cleared.
- Grant expiry, socket close, `deny <connection_id>`,
  daemon shutdown, timeout, or a classification failure while a submode is
  open calls `unwindToRootPrompt` before any other socket may send.

### 3.2 Stateful generated fragments

`cmdInsertRule` and `cmdInsertFastDeny` emit several lines. The fragments
`description`, `name`, `from`, `to`, `sourceip`, `destinationip`, `service`,
`action`, `activate`, `deactivate`, and `exit` are not valid at the root
prompt.

They are accepted only when all of these hold:

- the socket is the current session owner and has Level 2;
- the last matched prompt is still the submode entered by that socket's
  `secure-policy insert <position>` line; and
- the fragment is one of the `cmdInsertRule` shapes in section 3.3.
  `secure-policy insert <position>` is the same first line for both
  generators, so the session owner cannot tell them apart. `cmdInsertFastDeny`
  fragments are a subset of the `cmdInsertRule` fragments and are accepted
  under that one list. Do not invent a second submode.

### 3.4 Safe default

- Any line without a live grant (`hasBeenGranted == false` / tier `NONE`): return `NOT_AUTHORIZED`.
- Recognized Level 3 line with live `READ` or `READ/WRITE` grant: run.
- Recognized Level 2 line without live `READ/WRITE` grant: return `NOT_AUTHORIZED`.
- Recognized Level 2 line with live `READ/WRITE` grant: run.
- Recognized Level 1 line: return `FORBIDDEN`.
- Anything else: return `UNCLASSIFIED`.

No prefix-only decision is sufficient. A `show` prefix cannot hide separators,
an embedded newline, or a second command.

### 3.3 Accepted line shapes

Compare one trimmed line with these shapes. Names are one token of letters,
digits, and `_`, length 1 to 31. IPv4 addresses are dotted quads. Ports are
integers 1 to 65535. Positions are integers 1 to 9999. Metric is an integer
0 to 127. Protocol is `tcp`, `udp`, or `icmp`. A mask is either a dotted quad
or a prefix length 0 to 32. Anything outside these shapes is `UNCLASSIFIED`.

Level 3, from the current generators:

- `show version`
- `show cpu status`
- `show mem status`
- `show conn status`
- `ping <ipv4> count <1-20>`
- `traceroute <ipv4>`
- `show interface summary all`
- `show interface <name>`
- `show ip route-settings`
- `show zone`
- `show arp-table`
- `show address-object` and `show address-object <name>`
- `show address-group` and `show address-group <name>`
- `show service-object` and `show service-object <name>`
- `show secure-policy` and `show secure-policy <name-or-number>`
- `show ip virtual-server` and `show ip virtual-server <name>`

Level 2 root lines:

- `ip route <ipv4> <mask> <ipv4> <metric>`
- `no ip route <ipv4> <mask> <ipv4>`
- `address-object <name> <ipv4>`
- `address-object <name> <ipv4>-<ipv4>`
- `address-object <name> <ipv4>/<prefix>` and `address-object <name> <ipv4>/<dotted-quad>`
- `no address-object <name>`
- `address-group <name> <name>`
- `no address-group <name> <name>`
- `service-object <name> <proto> eq <port>`
- `no service-object <name>`
- `secure-policy insert <position>`
- `no secure-policy <name-or-number>`
- `ip virtual-server <name>` followed only by the optional clauses
  `cmdAddVirtualServer` appends, in that order: `interface <name>`,
  `original-ip <ipv4>`, `map-to <ipv4>`, then the required
  `map-type port`, then `original-service <name>`, `mapped-service <name>`,
  then `activate` or `deactivate`. A missing required clause or an extra
  clause is `UNCLASSIFIED`.

Level 2 fragments after `secure-policy insert` from `cmdInsertRule`:

- `description <one-line text up to 60 characters, no control characters>`
- `name <name>`
- `from <name>`
- `to <name>`
- `sourceip <name>`
- `destinationip <name>`
- `service <name>`
- `action allow` or `action deny`
- `activate` or `deactivate`
- `exit`

Level 1, refused:

- `write`
- `reboot`

Timeouts already in the driver, and no others:

- `ping`: `10000 + (count * 2000)` milliseconds, as in `ZyxelDriver::ping`.
- `traceroute`: `60000` milliseconds, as in `ZyxelDriver::traceroute`.
- every other accepted line: `5000` milliseconds, the `executeCommand` default.

## 4. Published C applet

The canonical source is `client/netmon-clearance-client.c`. It is static,
reviewed source compiled into netmon; it is never generated from request data.
The applet has no ZySH, firewall, NAT, parsing, rollback, or policy knowledge.

The file begins with the comment in section 4.1. That comment is the manual
for an agent that has never seen this plan. Do not replace it with a one-line
banner, and do not omit a step from it.

Its persistent stdin interface is:

```text
connect <ip>:<port>
request <R|RW|RWP>
do <one ZySH line>
cancel
close
```

- `connect` opens one socket and performs a protocol-version handshake.
- `request <R|RW|RWP>` declares the intended security level (`R` = Read/Diagnostics,
  `RW` = Read/Write mutations, `RWP` = Read/Write + Password Access) and waits
  for `GRANTED <conn_id> <seconds>` or `DENIED <conn_id>`. The security level
  argument is mandatory; invoking `request` without a level or with an unrecognized
  level string is rejected immediately.
- `do` sends the remainder of the line unchanged and prints the result code,
  current prompt, and router output.
- `cancel` asks netmon to call `cancelActiveCommand` for this socket's in-flight
  command.
- `close` closes the socket and exits.
- The applet never reconnects automatically and never opens a local control
  port.
- Stdin may be a terminal or a pipe. Blank lines are ignored. The applet
  finishes one command before reading the next.
- While a `do` is waiting, a following stdin line of exactly `cancel` is sent
  as `CANCEL`. Any other following line stays unread until that `do` finishes.
- `request` may receive `WAITING` and then `GRANTED` or `DENIED`. `WAITING`
  is not the decision.
- If `connect` fails, `request` returns anything other than `GRANTED`, or
  `do` returns anything other than `OK`, the applet prints that result, reads
  no further stdin, closes the socket, and exits 1.
- Exit 0 means the handshake succeeded, every `do` returned `OK`, `request`
  returned `GRANTED` when it was present, and `close` completed.

The wire framing is one direction-independent message. The first four bytes
are an unsigned big-endian length. The following bytes are UTF-8 text of that
length. The text contains no NULs. A `do` line inside the text contains no
newline. The maximum text length is 4096 bytes for a request and 1048576
bytes for a response. A length of 0, a length above the maximum, a short
read, or a connection close before the length is satisfied closes the socket
and drops a Level 2 grant.

Client text, one message each:

- `HELLO 1`
- `REQUEST tier="<R|RW|RWP>" [platform="..."] [model="..."] [session="..."]`
- `DO <line>`
- `CANCEL`
- `CLOSE`

Server text, one message each:

- `HELLO 1`
- `WAITING <connection-id>`
- `GRANTED <connection-id> 300`
- `DENIED <connection-id>`
- `BUSY`
- `RSP <code> <prompt> <output>`

`<code>` is one of `OK`, `NOT_AUTHORIZED`, `FORBIDDEN`, `UNCLASSIFIED`,
`BUSY`, `CANCELED`, `TIMEOUT`, `LOCKED`, `SYNTAX`, `DISCONNECTED`, or
`FAILED`. `<prompt>` is the last matched prompt with spaces replaced by `_`,
or `-` when there is none. `<output>` is the remainder of the message after
the third space. The applet prints the code, the prompt, and the output, and
does not interpret them.

A version other than `1` makes the applet print `netmon_get_clearance_client`
and exit. The applet does not retry the handshake.

### 4.1 What an out-of-context agent is told

The MCP catalog description, the tool result, and the source comment say the
same thing. An agent must be able to act from any one of them without this
plan.

The catalog description of `netmon_get_clearance_client` is:

```text
Download the reviewed netmon router-clearance client. Before choosing a router
line, read .agents/skills/netmon/references/zyxel-clearance.md. Compile the
returned source, then pipe one connect/request/do/close batch into it. Reads
and mutations both require explicit operator approval on the netmon console
(grant <connection-id> [seconds] [read|write]). This tool does not change
the router.
```

The tool result is one object with these fields:

- `protocol_version`: `1`
- `filename`: `netmon-clearance-client.c`
- `compiler_argv`: the argument vector that compiles `filename` to
  `./netmon-clearance-client`
- `listen`: the configured bind address and port, as `host:port`
- `skill`: `.agents/skills/netmon/references/zyxel-clearance.md`
- `commands`: the accepted line shapes in section 3.3, in that order
- `procedure`: the comment body below, without the C comment markers
- `sha256`: SHA-256 of `source`
- `source`: the complete C file bytes

`firewall_block_ip` and `firewall_unblock_ip` return only:

```text
This tool no longer changes the firewall. Read
.agents/skills/netmon/references/zyxel-clearance.md and call
netmon_get_clearance_client.
```

The first comment in `client/netmon-clearance-client.c` is exactly:

```text
netmon-clearance-client.c

Persistent stdin client for one netmon router-clearance connection.
This program does not know firewall syntax. It forwards one line at a
time. netmon decides whether that line may run.

Read .agents/skills/netmon/references/zyxel-clearance.md before choosing
a router line. If that file is not available, use only the commands
array from netmon_get_clearance_client. Compile with compiler_argv from
that tool. Run the binary with no arguments and pipe commands to it:

  printf '%s\n' \
    'connect <listen>' \
    'request <R|RW|RWP>' \
    'do <one router line>' \
    'close' |
  ./netmon-clearance-client

One process is one connection. A second process does not keep an
approval granted to this one. Do not background it and do not add a
named pipe or local control socket.

  connect <ip>:<port>
      Use the listen value from the tool result.
  request <R|RW|RWP>
      Ask the human at the netmon console for clearance at the specified
      security level:
        R   - Read-only diagnostics ('show *', ping, traceroute)
        RW  - Read/Write configuration mutations (firewall, NAT, objects)
        RWP - Read/Write + Password Access (elevated mutations)
      The operator enters 'grant <connection-id> [seconds] read' or
      'grant <connection-id> [seconds] write' (or 'deny <connection-id>')
      on that console, not here.
      GRANTED <connection-id> <seconds> means this connection is authorized.
      DENIED and BUSY do not authorize. Later lines are not sent after either.
  do <one router command>
      Send one line. Reads require GRANTED read. Mutations require
      GRANTED write. The printed code, prompt, and router output are the
      result. Do not send write or reboot. Do not put a second command
      on the same line. A result other than OK stops the batch: later
      lines are not sent.
  cancel
      Stop the command this connection is waiting on.
  close
      Close the socket and exit.

Exit 0 means every command succeeded. This program does not reconnect,
does not listen, and has no password.
```

Source disclosure is not authority. The source and the tool result contain
no credentials and no grant material.

## 5. Netmon socket service

### 5.1 Connection state

Each accepted socket has:

- server-assigned connection ID;
- peer address and port;
- protocol version;
- `NONE`, `PENDING`, `LEVEL3` (`READ`), or `LEVEL2` (`READ_WRITE`) state;
- fixed Level 2 / Level 3 deadline when approved;
- current recognized ZySH submode; and
- at most one in-flight command.

There is one pending approval slot and at most one elevated socket. A second
`request` receives `BUSY`. `READ` operations from other granted sockets are permitted
only while the session owner in section 3.1 is empty (otherwise `BUSY`); ungranted
sockets receive `NOT_AUTHORIZED`.

The console displays non-blocking notifications with explicit security implications
and visual emphasis:

```text
Router clearance request: connection 17 from <peer-ip>:<peer-port>
Requested Tier: READ/WRITE (Flashing Red)
Run 'grant 17 [seconds]' to authorize (default 300s; downgrades to READ on expiry), or 'deny 17' to reject.
```

Visual attribute mappings:
- **`READ`**: Rendered in **Green** (`COLOR_GREEN` / ANSI `\033[32m`).
- **`READ/WRITE`**: Rendered in **Red Flashing** (`COLOR_RED | A_BLINK` / ANSI `\033[5;31m`).
- **`READ/WRITE + PASSWORD ACCESS!!!`**: Rendered in **Bold Red Flashing** (`COLOR_RED | A_BOLD | A_BLINK` / ANSI `\033[1;5;31m`).

The notification is non-blocking and does not change the prompt (`netmon>`) or
intercept keystrokes. The operator enters `grant <connection_id> [seconds]` or
`deny <connection_id>` at their discretion. There are no single-key shortcuts
or modal waiting states.

When `grant <connection_id> [seconds]` is issued for password-elevated tiers
(or when security policy requires authentication):
The console prompts for the operator's UNIX login password:

```text
Password for <unix-user>: [hidden]
```

NetMon authenticates the entered password against the host's UNIX authentication
subsystem (via `/sbin/unix_chkpwd` or PAM). Clearance is elevated if and only if
the password is verified and the specified connection ID is pending.

### 5.2 Graceful Downgrade & Lifecycle Semantics

- **Indefinite READ Default**: Sockets granted `READ` default to timeout 0 (indefinite).
  Custom positive seconds may still be passed via `grant <conn_id> <seconds>`.
- **Graceful Downgrade**: Sockets granted `READ/WRITE` or password-elevated access default
  to 300s (or custom seconds). Upon expiration of the grant deadline or idle timeout,
  the server does **NOT** revoke clearance completely or terminate the TCP connection.
  Instead, the connection's authority **gracefully downgrades to `READ`**.
- If a submode was open when downgrade occurred, `unwindToRootPrompt` is called
  to return the router shell to a clean state.
- Following downgrade, diagnostic and monitoring commands (`show *`, `ping`, etc.)
  continue executing without requiring a new approval request. Any subsequent
  mutation attempt returns `NOT_AUTHORIZED` until a new `request` and `grant` occur.

### 5.2 SSH execution

One router worker owns calls that touch `ZyxelSshClient`. The listener and
console remain responsive while SSH waits for a prompt.

For an accepted `do`:

1. Validate framing and lexical constraints.
2. Classify against the policy registry and current prompt/submode.
3. Check the socket's effective clearance.
4. Record the audit decision.
5. Call `executeCommand` with the exact line.
6. Return `SshResult`, matched prompt, and sanitized output.
7. Update or clear submode state from the matched prompt.

Use the timeouts in section 3.3. Call `ZyxelSshClient::executeCommand` with
that value. Do not add a fourth timeout.

`cancel` affects only the command submitted by that socket. A client cannot
cancel another socket's command.

On socket loss, grant expiry, daemon shutdown, timeout, syntax error, or
interrupted mutation, netmon attempts `unwindToRootPrompt`. Unwind success does
not prove that earlier mutation lines were reversed.

## 6. Audit and failure semantics

Every attempted `do`, including rejected input, records:

- timestamp;
- connection ID and peer;
- sanitized command;
- matched driver method or `UNCLASSIFIED`;
- required and effective clearance;
- accepted or rejected decision and reason;
- SSH result and duration; and
- bounded, redacted output excerpt or output hash.

Credentials and raw unbounded router buffers are never logged.

This audit record is durably persisted as a structured JSON line into the
file specified via `libconfig++` (`audit_file` or `security = { audit_file = "..."; };`
in `netmon.cfg`, mode `0600`, defaulting to `Config::resolveHomePath("~/.config/netmon/audit.log")`).
Every attempted ZySH command (accepted, rejected by policy, forbidden, or
unclassified) is synchronously flushed with `fsync()` to ensure complete,
permanent disk traceability across process restarts.

The netmon console shell (`NetMonShell`) exposes an interactive `audit [limit]`
command allowing the human operator to view recent clearance and ZySH execution
history directly from the console.

Multiple Level 2 lines are not atomic. A later error, timeout, disconnect, or
daemon crash can leave earlier lines applied. Record `PARTIAL_APPLY` whenever a
mutation may have reached the router but the requested end state is not proved.
Do not claim rollback merely because a delete method exists.

Router backup and restoration are the human operator's out-of-band
responsibility. The agent receives no backup authority and no router password.

`cmdWrite` stays behind the existing flash debounce path. `cmdReboot` stays at
the human console. Neither is exposed through Level 2.

## 7. MCP boundary

Add the read-only `netmon_get_clearance_client` and `firewall_get_metrics` tools to
`AimonGatewayClient`.

Existing read tools remain available:

- `firewall_get_status` (enriched with current telemetry link state);
- `firewall_get_sessions`;
- `firewall_get_metrics` (new official MCP tool returning structured active sessions,
  App Patrol forwarded/dropped/rejected KB, and IDP threat detections);
- LAN inventory and traffic tools; and
- SNMP tools.

`firewall_block_ip` and `firewall_unblock_ip` stop performing mutation through
MCP. They return a short instruction naming
`netmon_get_clearance_client`. No MCP argument can claim or inherit mutation authority.

## 8. Implementation scope

- `client/netmon-ai-client.c`: persistent daemon client with parent-PID validation
  and local UNIX socket anchoring.
- `include/AiSecurityClearance.hxx`,
  `src/AiSecurityClearance.cxx`: connection state, security tier management
  (`READ`, `READ/WRITE`, `READ/WRITE + PASSWORD ACCESS!!!`), indefinite timeout default
  for `READ`, 300s default with graceful downgrade to `READ` upon expiry,
  policy registry, classifier, and audit integration with synchronous `audit_file` disk persistence.
- `include/UnixAuth.hxx`, `src/UnixAuth.cxx`: local UNIX password
  authentication helper (invoking `/sbin/unix_chkpwd` with mock test hook)
  to ensure operator identity cannot be spoofed by screen injection.
- `include/Config.hxx`, `src/Config.cxx`: load and save `audit_file` and
  `security.audit_file` via `libconfig++` (default `~/.config/netmon/audit.log`).
- `include/zyxel/ZyxelSecurityCmd.hxx`, `src/zyxel/ZyxelSecurityCmd.cxx`: command
  generators and parsers for `show conn status`, `show app statistics summary`,
  and `show idp statistics summary`.
- `include/RouterDriver.hxx`, `include/ZyxelDriver.hxx`,
  `src/ZyxelDriver.cxx`:
  - Active keepalive worker periodically executing security telemetry commands
    over SSH, resetting the physical router's CLI shell idle timeout.
  - Thread-safe in-memory cache for `ZyxelSecurityTelemetry`.
  - Serialized command execution path needed by the clearance socket service.
- `include/WebServer.hxx`, `src/WebServer.cxx`:
  - REST endpoint `GET /api/firewall/metrics` serving cached telemetry JSON.
  - Enriched `/api/firewall/status` and `/api/status`.
- `web/index.html`, `web/app.js`, `web/style.css`:
  - "Firewall & Security Metrics" card in Web UI dashboard displaying active
    sessions/capacity, App Patrol metrics, and IDP threat interceptions with
    auto-refresh.
- `include/NcursesConsole.hxx`, `src/NcursesConsole.cxx`: non-blocking
  clearance notification with color and blinking attributes:
  - Green for `READ`
  - Red flashing for `READ/WRITE`
  - Bold red flashing for `READ/WRITE + PASSWORD ACCESS!!!`
- `include/NetMonShell.hxx`, `src/NetMonShell.cxx`:
  - `grant <connection_id> [seconds]` and `deny <connection_id>`.
  - UNIX password verification on elevation.
  - `audit` command for inspecting historical executions.
- `include/AimonGatewayClient.hxx`, `src/AimonGatewayClient.cxx`:
  - Publish `netmon_get_clearance_client`.
  - Register and serve official MCP tool `firewall_get_metrics`.
- Configuration: listener bind address and port. No address or port is
  hardcoded in the applet.
- Tests: `TestAiSecurityClearance.cxx`, `TestAiSecurityAdversarial.cxx`,
  `TestZyxelSecurityCmd.cxx`, `TestWebServerMetrics.cxx`, and `RecordingRouter`.

aimon, embdevenv, and meshmon are not modified.

### 8.1 Staged Re-Implementation Envelopes

#### Envelope 4: Security Implication Tiers, Presentation Styling & Graceful Downgrade
- **Goal**: Replace abstract numeric levels with `READ`, `READ/WRITE`, and
  `READ/WRITE + PASSWORD ACCESS!!!`.
- **Files**:
  - `include/AiSecurityClearance.hxx`, `src/AiSecurityClearance.cxx`
  - `include/NcursesConsole.hxx`, `src/NcursesConsole.cxx`
  - `include/NetMonShell.hxx`, `src/NetMonShell.cxx`
  - `test/TestAiSecurityClearance.cxx`
- **Key Tasks**:
  1. Map internal levels to security tiers: `READ`, `READ_WRITE`, `READ_WRITE_PASSWORD`.
  2. Implement default timeouts: 0 (indefinite) for `READ`, 300s for `READ/WRITE` and elevated.
  3. Implement graceful downgrade: on grant expiry or idle timeout, downgrade authority to `READ` instead of socket termination/revocation.
  4. Implement console visual attributes:
     - `READ` in Green (`COLOR_GREEN`)
     - `READ/WRITE` in Red Flashing (`COLOR_RED | A_BLINK`)
     - `READ/WRITE + PASSWORD ACCESS!!!` in Bold Red Flashing (`COLOR_RED | A_BOLD | A_BLINK`)
  5. Update `TestAiSecurityClearance.cxx` test cases and verify with `make test`.

#### Envelope 5: Active SSH Keepalive via Security Telemetry (`ZyxelDriver`)
- **Goal**: Prevent physical router shell inactivity disconnects by periodically
  executing active CLI queries and collecting live security metrics.
- **Files**:
  - `include/zyxel/ZyxelSecurityCmd.hxx`, `src/zyxel/ZyxelSecurityCmd.cxx`
  - `include/ZyxelDriver.hxx`, `src/ZyxelDriver.cxx`
  - `test/TestZyxelSecurityCmd.cxx`
- **Key Tasks**:
  1. Implement `ZyxelSecurityCmd` generators and parsers:
     - `show conn status` (active sessions, max capacity)
     - `show app statistics summary` (forwarded, dropped, rejected KB, matched conns)
     - `show idp statistics summary` (threat detections, actions taken)
  2. Implement `ZyxelSecurityTelemetry` struct with atomic/mutex-guarded storage in `ZyxelDriver`.
  3. Update `ZyxelDriver::keepaliveWorker()` to execute these read commands every 60s, keeping the ZLD CLI shell alive and populating the telemetry cache.
  4. Unit test parsers on real and edge CLI outputs.

#### Envelope 6: Web Dashboard & MCP Tool Integration
- **Goal**: Expose collected telemetry on the web interface and via official MCP tool.
- **Files**:
  - `include/WebServer.hxx`, `src/WebServer.cxx`
  - `web/index.html`, `web/app.js`, `web/style.css`
  - `include/AimonGatewayClient.hxx`, `src/AimonGatewayClient.cxx`
- **Key Tasks**:
  1. Add `GET /api/firewall/metrics` endpoint in `WebServer.cxx` returning the cached JSON telemetry.
  2. Add "Firewall & Security Metrics" card to Web UI dashboard (`web/index.html`, `web/app.js`, `web/style.css`) with real-time auto-refresh.
  3. Register and implement `firewall_get_metrics` in `AimonGatewayClient.cxx`.
  4. Unit test endpoint serialization and MCP payload generation.

#### Envelope 7: Physical Hardware Qualification on Rhino
- **Goal**: Verify persistent connection stability, graceful downgrade, Web UI, and MCP metrics against the physical Zyxel USG FLEX 200.
- **Key Tasks**:
  1. Verify zero SSH drops across extended idle intervals (active keepalive verification).
  2. Verify clearance elevation to `READ/WRITE`, execution of mutation, and graceful fallback to `READ` upon 300s expiry.
  3. Verify subsequent `READ` commands succeed immediately after fallback without re-prompting.
  4. Verify `/api/firewall/metrics` and `firewall_get_metrics` MCP tool against live router data.

#### Envelope 8: Mandatory READ Clearance Gate, R|RW|RWP Request Tiers & Zero-Trust Enforcement
- **Goal**: Enforce strict Zero-Trust where newly connected sockets start with ZERO authority (`NONE`). The client MUST explicitly request one of all 3 security levels (`R`, `RW`, `RWP`). Sockets cannot execute ANY command (`DO show ...`, `DO ping ...`, `DO traceroute ...`, or mutations) without an explicit operator grant corresponding to the requested tier.
- **Files**:
  - `include/AiSecurityClearance.hxx`, `src/AiSecurityClearance.cxx`
  - `include/NetMonShell.hxx`, `src/NetMonShell.cxx`
  - `client/netmon-ai-client.c`, `client/netmon-clearance-client.c`
  - `src/AimonGatewayClient.cxx`
  - `.agents/skills/netmon/references/zyxel-clearance.md`
  - `test/TestAiSecurityClearance.cxx`, `test/TestClearanceClient.cxx`
- **Key Tasks**:
  1. Update `ClearanceTier` and `ClientClearanceState` to include `NONE = 0`.
  2. Initialize `ClearanceConnection` with `clearanceState = ClientClearanceState::NONE`, `activeTier = ClearanceTier::NONE`, and `hasBeenGranted = false`.
  3. Update `client/netmon-ai-client.c` `request` subcommand to require one of the 3 security levels:
     - `request R` (or `read`): sends `REQUEST tier="R" ...`
     - `request RW` (or `write`): sends `REQUEST tier="RW" ...`
     - `request RWP` (or `password`): sends `REQUEST tier="RWP" ...`
     - Reject bare `request` (no argument) with: `Error: 'request' requires a security level: R, RW, or RWP` (exit 1).
     - Reject invalid levels (e.g. `request extra`) with: `Error: Unknown security level '%s'. Must be R, RW, or RWP` (exit 1).
  4. In `AiSecurityClearance.cxx` `handleClientMessage`:
     - Parse `tier="..."` from `REQUEST` metadata. Recognize `R`/`read`, `RW`/`write`, and `RWP`/`password`.
     - Reject requests missing a valid security level with `FAILED - Missing or invalid security level (must be R, RW, or RWP)`.
     - Set `conn.requestedTier` to `ClearanceTier::READ`, `ClearanceTier::READ_WRITE`, or `ClearanceTier::READ_WRITE_PASSWORD`.
     - Render corresponding Ncurses / ANSI provenance box (Green for R, Flashing Red for RW, Bold Flashing Red for RWP).
  5. In `executeDo`: check `conn.hasBeenGranted`. If `!conn.hasBeenGranted`, reject with `RSP NOT_AUTHORIZED <prompt> Clearance grant required for read`.
  6. In `consoleApprove`:
     - If `tier == ClearanceTier::READ`: set `clearanceState = ClientClearanceState::READ`, `activeTier = ClearanceTier::READ`, `hasBeenGranted = true`. No UNIX password required.
     - If `tier == ClearanceTier::READ_WRITE`: set `clearanceState = ClientClearanceState::READ_WRITE`, `activeTier = ClearanceTier::READ_WRITE`, `hasBeenGranted = true`. UNIX password required.
     - On 300s expiry of `READ_WRITE`, gracefully downgrade to `READ` (`hasBeenGranted = true`).
     - On expiry of finite-duration `READ` grant, drop to `NONE` (`hasBeenGranted = false`).
  7. In `NetMonShell`: verify `grant conn-nnnn [seconds] read` and `grant conn-nnnn [seconds] write` commands.
  8. Update client comments, tool catalog descriptions, and `zyxel-clearance.md` documentation.
  9. Update `TestAiSecurityClearance.cxx` and `TestClearanceClient.cxx` test suites to verify `R|RW|RWP` validation, ungranted rejection, and approved command execution. Verify full suite under `make test`.

#### Envelope 9: Clearance Timing, Deep Process Ancestry Robustness & Fast Agent Qualification
- **Goal**: Eliminate premature 5s timeout on full policy dumps (`show secure-policy`) and large diagnostic queries; eliminate fragile 2-level process hierarchy assumptions in `netmon-ai-client.c`; add comprehensive CppUTest coverage for command timeouts, nested subshell execution, and `NETMON_ANCHOR_PID` environment override; and establish an explicit physical qualification gate requiring fast agent acquisition of complete firewall knowledge within $\le 10$ seconds.
- **Files**:
  - `src/AiSecurityClearance.cxx`: Update `AiSecurityClassifier::classify` timeout calculation.
  - `client/netmon-ai-client.c`: Add `NETMON_ANCHOR_PID` support, iterative proc stat ancestor walk, ungranted `do` fast-fail, and 30s socket timeout.
  - `test/TestAiSecurityClearance.cxx`: Add classifier timeout assertions for large table dumps vs targeted queries.
  - `test/TestAiSecurityAdversarial.cxx`: Add deep process hierarchy test (3+ nested subshells) and `NETMON_ANCHOR_PID` environment override test.
  - `test/TestClearanceClient.cxx`: Add ungranted `do` fast-fail test and extended timeout qualification.
- **Key Tasks**:
  1. In `src/AiSecurityClearance.cxx` (`AiSecurityClassifier::classify`):
     - Allocate 15000 ms (`15s`) for commands that stream full tables or compute DPI statistics on physical hardware:
       - `show secure-policy` (bare table dump without rule index or name)
       - `show app statistics summary`
       - `show idp statistics summary`
       - `show ip virtual-server`
       - `show service-object` (bare factory dump)
     - Maintain 5000 ms (`5s`) for targeted, indexed queries (`show secure-policy <rule-or-name>`, `show address-object <name>`, `show zone`, `show ip route-settings`, etc.).
  2. In `client/netmon-ai-client.c`:
     - In `get_anchor_pid()`:
       - Check `getenv("NETMON_ANCHOR_PID")`. If valid integer $> 0$, use it directly as the target anchor PID.
       - If not set, iteratively walk `/proc/<pid>/stat` upwards (up to 8 ancestor levels or until `ppid <= 1`) looking for an existing `/tmp/netmon-ai-<pid>.sock` before falling back to `getppid()`.
     - In `SO_PEERCRED` validation: authorize caller if `NETMON_ANCHOR_PID` matches anchor, or if any ancestor in the iterative walk matches `anchor`.
     - In client `do` subcommand: check if daemon returns `UNGRANTED` or `WAITING` before running commands. If not granted, print an immediate clear error: `Error: Session has not been granted clearance. Run 'request <R|RW|RWP>' first.` and exit with code 1 immediately without stalling.
     - Increase `SO_RCVTIMEO` from 10s to 30s to match extended server command allowances.
  3. In `test/TestAiSecurityClearance.cxx`:
     - Test that `show secure-policy` returns `timeoutMsOut == 15000`.
     - Test that `show secure-policy 1` returns `timeoutMsOut == 5000`.
     - Test that `show app statistics summary` and `show idp statistics summary` return `timeoutMsOut == 15000`.
  4. In `test/TestAiSecurityAdversarial.cxx`:
     - Add `TestClientDeepProcessAncestry`: invoke client inside nested subshell `sh -c 'sh -c "/tmp/netmon-ai-client-test status"'` and verify it connects to the daemon without spawning duplicate sockets or hanging.
     - Add `TestClientAnchorPidEnv`: invoke client with `NETMON_ANCHOR_PID=<pid>` and verify it connects to `/tmp/netmon-ai-<pid>.sock`.
  5. In `test/TestClearanceClient.cxx`:
     - Add test verifying ungranted `do` fails immediately with exit code 1 and descriptive error message.
     - **Automated Pipelining Benchmark Test (`TestBenchmarkFirewallRulePipelining`)**:
       - Benchmark in-memory pipelined dispatch of 25 consecutive firewall rules over the client-daemon socket.
       - Assert hard ceiling: total dispatch + parsing latency for 25 rules MUST complete in $< 100$ milliseconds.
  6. **Physical Target Benchmark & Fast Qualification Harness**:
     - Provide automated benchmark runner `test/benchmark_firewall_read.sh`:
       - Measure exact wall-clock latency for reading all 21 firewall security policies (`show secure-policy 1` .. `21`) via pipelined client execution.
       - Measure exact wall-clock latency for complete firewall discovery pipeline (system version, CPU/memory, interface states, ARP table, all 21 firewall security policies, and NAT virtual servers).
       - Enforce strict benchmark assertions:
         - Pipelined 21-rule read MUST complete in $\le 3.0$ seconds (average $\le 140$ms per rule).
         - Complete multi-domain firewall knowledge acquisition MUST complete in $\le 5.0$ seconds (hard ceiling 10s).
         - Fail qualification if execution exceeds these benchmark thresholds.


### Envelope 10: Mandatory UNIX Password Verification on All Clearance Grants

- **Rationale**:
  - The anti-screen-injection security boundary established in Section 1.1 requires that no clearance grant may be issued without interactive operator presence.
  - Envelope 6 mistakenly bypassed UNIX password verification when granting `READ` clearance (`if (tier != ClearanceTier::READ)`), allowing scripted screen injection attacks to obtain unauthorized read access to router firewall tables, credentials, and topology.
- **Implementation Tasks**:
  1. `src/NetMonShell.cxx`:
     - Remove `if (tier != ClearanceTier::READ)` conditional guard.
     - Unconditionally prompt for UNIX password on `grant <conn_id>` regardless of requested/explicit tier.
     - Update help string to state that all grants require UNIX password.
  2. `src/NcursesConsole.cxx`:
     - Remove `if (tier != ClearanceTier::READ)` conditional guard.
     - Unconditionally trigger `promptPassword()` modal on `grant <conn_id>`.
  3. `test/TestAiSecurityClearance.cxx`:
     - Verify that all tiers require valid UNIX password verification.

### Envelope 11: Unbounded PTY Height & Robust ANSI Pager Handling for Fast Table Dumps

- **Rationale**:
  - Physical profiling revealed that small CLI queries (`show version`, `show zone`, `show address-object`, `show object-group`) complete in 11–83ms, but large table dumps (`show secure-policy`, `show service-object`, `show ip virtual-server`) freeze for 30s and abort with 0 lines.
  - The root cause is interactive terminal pagination: `libssh2_channel_request_pty_ex` requests a 24-row terminal (`256, 24`). When output exceeds 24 lines, ZySH enters pagination mode.
  - Furthermore, ZySH ignores `terminal length 0`, and formats `--More--` with ANSI control sequences that evade the raw `rawBuffer.find("--More--")` check, deadlocking the reader loop.
- **Implementation Tasks**:
  1. `src/ZyxelSshClient.cxx`:
     - Set PTY row height to unbounded / maximum (`height = 0` or `height = 65535`) in `libssh2_channel_request_pty_ex` to eliminate line wrapping and terminal pagination at the PTY level.
     - In `drainUntilPromptWithReader`:
       - Strip ANSI escape sequences before searching for `--More--`, or match regex pattern `(\x1b\[[0-9;]*m)?--More--`.
       - When a pagination prompt is detected, erase it from the output buffer and immediately transmit a space `' '` over the SSH channel to advance.
  2. `test/TestZyxelDriver.cxx` & `test/RecordingRouter.cxx`:
     - Unit test multi-page output with embedded ANSI-styled `--More--` prompts, verifying complete multi-page output is assembled without timeouts.
  3. Physical Benchmark Verification:
     - Run `test/benchmark_firewall_read.sh` against the live router and prove that complete table dumps (`show secure-policy`, `show service-object`, `show ip virtual-server`) complete in $\le 5.0$ seconds without stalling.

## 9. Verification

### 9.1 Policy completeness

- For every generator, build one line with accepted arguments and one line
  that differs by a single illegal argument. The first is the level in
  section 3. The second is `UNCLASSIFIED`.
- A generator added without a shape in section 3.3 fails this test.
- Send each `cmdInsertRule` fragment at the root prompt and again after
  `secure-policy insert`. Only the second is Level 2. A different socket's
  line during that submode is `BUSY`.
- Reject unknown verbs, mixed-case bypasses, leading/trailing control
  characters, NUL, newline, oversized input, separators, and read-prefix
  smuggling.
- The catalog description, `procedure`, and the source comment each contain
  `connect`, `request`, `do`, `cancel`, `close`, the skill path, and the
  sentence that a second process does not keep an approval. `commands`
  matches section 3.3. `sha256` matches `source`.

### 9.2 Socket and grant behavior

- Newly connected sockets start with zero command authority (`NONE`).
- An unapproved socket cannot run ANY command (neither `READ` nor `READ/WRITE`).
- `DO <cmd>` prior to grant returns `RSP NOT_AUTHORIZED <prompt> Clearance grant required for read`.
- `grant <connection_id> [seconds] read` elevates socket to `READ` with UNIX password.
- `grant <connection_id> [seconds] write` elevates socket to `READ/WRITE` with UNIX password.
- `grant <connection_id> [seconds] password` elevates socket to `READ/WRITE + PASSWORD ACCESS` with UNIX password.
- Every `grant` command unconditionally requires valid interactive UNIX password authentication; failure immediately denies the connection.
- `READ` grants default to timeout 0 (indefinite).
- `READ/WRITE` grants default to timeout 300s.
- **Graceful Downgrade**: Upon 300s grant deadline or idle timeout, authority
  downgrades back to `READ`. The TCP socket remains connected, and subsequent
  read commands continue succeeding without requiring a new approval.
- An open submode is unwound to root prompt on downgrade.
- Sockets cannot run device management commands (`write`, `reboot`).
- A stale `grant <connection_id>` after disconnect has no effect.
- A second approval request receives `BUSY`.
- Cancel cannot cross connection IDs.

### 9.3 Protocol and applet

- Build the published source with the returned compiler arguments.
- Verify the published SHA-256 against the canonical source.
- Exercise fragmented headers and payloads, EOF, `EINTR`, oversized lengths,
  unknown versions, and slow partial writes.
- Prove the applet does not reconnect and opens no listening socket.
- A piped batch whose `request` returns `DENIED` does not send a following
  `do`. A piped batch whose first `do` is not `OK` does not send the next
  `do`, and the socket is closed.
- Run ASan and UBSan on netmon and the applet tests.

### 9.4 SSH and physical router

Unit tests prove classification and state transitions only. They do not prove
router behavior.

On the physical Zyxel target, record:

1. rejection of a `READ` command before approval (`RSP NOT_AUTHORIZED`);
2. approval of `READ` on socket 1 (`grant <conn1> read`) and successful `READ` execution;
3. rejection of a `READ/WRITE` command on socket 1 before elevation;
4. approval of `READ/WRITE` on socket 1 (`grant <conn1> write`) and execution of mutation;
5. parsed `READ` verification of the resulting state;
6. refusal of `write` and `reboot`;
7. graceful downgrade to `READ` after the 300-second deadline;
8. successful `READ` command execution following downgrade without re-prompting;
9. rejection of any command on an ungranted second socket;
10. revocation after disconnect; and
11. **Fast Agent Execution Qualification**: execution of the complete firewall knowledge discovery pipeline (system health, CPU/RAM, interfaces, ARP table, all 21 security policies, zones, and NAT virtual servers) completes in $\le 10$ seconds end-to-end on the physical router.

Preserve raw client, netmon, and router outputs. A loadable driver, an open
socket, or exit code zero is not evidence that the physical rule changed.

### 9.5 Active keepalive, telemetry caching, Web UI & MCP tool

- Unit tests for `ZyxelSecurityCmd` parsing nominal, empty, and malformed
  outputs for `show conn status`, `show app statistics summary`, and
  `show idp statistics summary`.
- Verification that `keepaliveWorker()` periodic command execution does not
  deadlock with clearance worker or driver commands.
- Verification of `/api/firewall/metrics` endpoint response schema.
- Verification of Web UI card rendering in `web/index.html` and `web/app.js`.
- Verification that MCP tool `firewall_get_metrics` returns valid structured
  JSON via `AimonGatewayClient`.

## 10. Explicit exclusions

- No client-side token, OTP, password, or HMAC.
- No MCP mutation authority.
- No proposal queue, ticket polling, transaction digest, arm command, or
  constrained envelope.
- No ZySH intelligence in the applet.
- No automatic rollback or backup.
- No claim that a multi-line configuration sequence is atomic.

## Appendix. Lifecycle Transition & Status Log

| Date | Previous State | New State | Lifecycle Directory | Notes / Rationale |
|---|---|---|---|---|
| 2026-10-01 | — | `Proposed` | `plan/` | Initial three-level security-clearance concept. |
| 2026-10-04 | `Proposed` | `Proposed` | `plan/` | Explored tokens, per-proposal cards, typed transactions, and envelopes; these transports were later superseded. |
| 2026-10-04 | `Proposed` | `Abandoned` | `plan/abandoned/` | Temporarily moved aside while direct socket identity was evaluated. |
| 2026-10-04 | `Abandoned` | `Proposed` | `plan/` | Consolidated: preserved levels, classification, audit, serialization, failure semantics, and verification; replaced authentication transport with a persistent agent-to-netmon TCP socket. |
| 2026-10-04 | `Proposed` | `Proposed` | `plan/` | Locked the single SSH prompt owner and the generator line shapes so execution does not invent a second policy. |
| 2026-10-04 | `Proposed` | `Proposed` | `plan/` | The clearance-client tool result and the C source comment carry the same operating steps for an agent that has not read this plan. |
| 2026-10-04 | `Proposed` | `Proposed` | `plan/` | Agent operation is one piped batch per connection. The Zyxel line catalog and worked sequences live in the netmon skill reference. |
| 2026-10-05 | `Proposed` | `Executing` | `plan/` | Approved by user; initiating Envelope 1 (Policy Registry, Line Classifier, and Unit Tests). |
| 2026-10-05 | `Executing` | `Executing` | `plan/` | Envelopes 1 & 2 complete: wire framing, line classifier, prompt owner lock, transport applet, adversarial test suite, piped batch verification, quality gate passed. |
| 2026-10-05 | `Executing` | `Executing` | `plan/` | Initiating Envelope 3: UNIX password authentication for operator approvals (defeating screen session injection) and libconfig++ audit_file disk persistence. |
| 2026-10-05 | `Executing` | `Executing` | `plan/` | Envelope 3 complete: implemented UnixAuth helper (/sbin/unix_chkpwd) with masked password challenge on shell 'grant <connection_id>'; added libconfig++ audit_file persistence with synchronous JSON line flushing (0600) and shell 'audit' command; full test suite passing (116/116 unit tests, 89/89 driver tests, 10/10 RapidCheck property suites). |
| 2026-10-05 | `Executing` | `Executing` | `plan/` | Aligned plan with operator specification: removed all 'y'/'n' shortcuts and prompt hijacking; required explicit <connection_id> argument on 'grant' and 'deny'; non-blocking notification on request arrival; verified 116/116 tests passing. |
| 2026-10-05 | `Executing` | `Proposed` | `plan/` | Dictation phase complete: updated plan for re-implementation with security implication tiers (READ green, READ/WRITE red flashing, READ/WRITE + PASSWORD ACCESS bold red flashing), graceful downgrade to READ on 300s expiry, active SSH keepalives via security telemetry (show conn status, show app statistics summary, show idp statistics summary), Web UI metrics dashboard, and firewall_get_metrics MCP tool. |
| 2026-10-05 | `Proposed` | `Executing` | `plan/` | Approved by operator; initiating Envelope 4 (Security Implication Tiers, Presentation Styling & Graceful Downgrade). |
| 2026-10-05 | `Executing` | `Executing` | `plan/` | Envelopes 4, 5, and 6 complete and qualified 100%: implemented security implication tiers (READ green, READ/WRITE red flashing, READ/WRITE + PASSWORD ACCESS bold red flashing) in Ncurses/ANSI; implemented graceful downgrade to READ on 300s expiry; implemented active SSH keepalive telemetry in ZyxelDriver (show conn status, show app statistics summary, show idp statistics summary) with 60s periodic execution to reset ZLD inactivity timer; added Web UI dashboard card in web/index.html & web/app.js with /api/firewall/metrics; registered and dispatched official MCP tool firewall_get_metrics in AimonGatewayClient; all test suites qualified with zero errors (132/132 unit tests, 98/98 driver tests, 10/10 RapidCheck property suites). Ready for Envelope 7 physical qualification. |
| 2026-10-06 | `Executing` | `Proposed` | `plan/` | Re-aligned with operator specification: netmon-ai-client MUST be granted explicit READ permission (grant <conn_id> [seconds] read) before executing any read or diagnostic commands; sockets initialize in ungranted state (NONE); added Section 0 negative constraints, Envelope 8 for Zero-Trust enforcement, and updated client/skill documentation. |
| 2026-10-06 | `Proposed` | `Executing` | `plan/` | Approved by operator ("proceed"); initiating Envelope 8 implementation. |
| 2026-10-06 | `Executing` | `Executing` | `plan/` | Envelope 8 complete and verified 100%: implemented Zero-Trust initial connection state (NONE, 0 authority), rejection of ungranted read/diagnostic commands (RSP NOT_AUTHORIZED Clearance grant required for read), explicit operator READ grant with indefinite default duration, graceful downgrade from READ/WRITE to READ, expiry of finite READ grants to NONE, netmon-ai-client request argument validation, and full test suite passing (132/132 unit tests, 98/98 driver tests, 10/10 RapidCheck property suites). |
| 2026-10-06 | `Executing` | `Proposed` | `plan/` | Updated plan per operator requirement: client 'request' MUST explicitly specify one of all 3 security levels (R|RW|RWP); bare/tierless request is prohibited; updated Envelope 8 and verification specs. |
| 2026-10-06 | `Proposed` | `Executing` | `plan/` | Approved by operator ("implement the fucking spec I asked!"); implemented mandatory R|RW|RWP security level argument in client/netmon-ai-client.c, server parsing/prompts in AiSecurityClearance.cxx, updated skill docs in zyxel-clearance.md, and verified full test suite (132/132 unit tests, 98/98 driver tests, 10/10 RapidCheck property suites). |
| 2026-10-06 | `Executing` | `Executing` | `plan/` | Hardened wire protocol & console grant: enforced mandatory tier="<R|RW|RWP>" parsing on server wire receiver, immediately rejecting bare or unrecognized tiers with RSP SYNTAX; updated NetMonShell and NcursesConsole grant command to query and default to conn.requestedTier when omitted by operator; added comprehensive unit and adversarial TCP tests for bare REQUEST, invalid tier, and requestedTier console defaulting; 100% qualified across full suite (137/137 unit tests, 98/98 driver tests, 10/10 RapidCheck property suites) and Selfso compliance gate. |
| 2026-10-06 | `Executing` | `Executing` | `plan/` | Envelope 9 implemented & verified: hardened client with NETMON_ANCHOR_PID and deep process ancestry traversal; verified 21-rule pipelined read completes in 1.1s (52ms/rule) on physical router; all 238 unit and driver tests passing. |
| 2026-10-06 | `Executing` | `Executing` | `plan/` | Envelopes 10 & 11 implemented & verified: removed 'tier != READ' bypass in NetMonShell and NcursesConsole ensuring all clearance grants unconditionally mandate UNIX password verification; allocated unbounded 65535-row PTY and hardened ANSI '--More--' pager advance in ZyxelSshClient; verified with 141/141 netmon unit tests and 99/99 driver tests (240 tests total, 0 errors) and Selfso compliance pass. Ready for physical deployment and fast agent qualification. |
| 2026-10-06 | `Executing` | `Executing` | `plan/` | Physical qualification & benchmark verified on live router: deployed netmon v1.0.5 to rhino; operator granted READ on conn-0004 with interactive UNIX password verification; Benchmark 1 (21 rules pipelined) completed in 1,097 ms (52 ms/rule, <= 3.0s threshold); Benchmark 2 (7 diagnostic domains) completed in 381 ms (<= 5.0s threshold); full table dumps verified (show secure-policy 28KB in 61 ms, show service-object in 108 ms) following MAX_RSP_LEN client socket fix; pre-commit compliance passed cleanly. |
| 2026-10-06 | `Executing` | `Done` | `plan/` | Closed out per operator instruction with known defect explicitly acknowledged: Level 2 WRITE mutations on the live physical router fail with '% Command not found' because ZySH requires Global Configuration Mode ('configure terminal'). Write mutation remediation and reversible live qualification are handed off to dedicated follow-up plan 'plan/plan_zyxel_clearance_write_mutations.md'. All READ operations, Zero-Trust clearance gating, mandatory UNIX password authentication, keepalives, PTY unbounded allocation, client buffer fix, and fast agent discovery benchmarks (1.1s for 21 rules, 381ms for diagnostics) are 100% verified. |
