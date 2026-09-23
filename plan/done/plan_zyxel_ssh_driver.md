# Implementation Plan — Zyxel USG FLEX 200 SSH Driver & Pluggable Credential Vault Architecture

Implement a robust, thread-safe, and fault-tolerant SSH client driver for the Zyxel USG FLEX 200 gateway in `netmon` using `libssh2`, backed by a **pluggable `CredentialVault` abstraction**, an **ephemeral human-delegated Maintenance Token subsystem**, **CppUTest regression testing framework with strict test data isolation in `.gitignore`**, and **synchronized architectural and operator documentation**.

The implementation is structured into **5 sequential, test-driven phases**. Each phase includes comprehensive unit, boundary, and error-handling tests. Subsequent phases integrate the components and execute full regression suites before proceeding to the next gate.

---

## Analysis Issues, Pitfalls & Comprehensive Resolutions

The following table tracks each architectural analysis finding, pitfall, and hardened resolution:

| # | Analysis Finding & Pitfall | Risk & Impact | Hardened Resolution & Architecture | Status |
| :--- | :--- | :--- | :--- | :--- |
| **1** | **Multi-Threaded Concurrency on `libssh2` & Head-of-Line Blocking** | `WebServer`, `AimonGatewayClient`, and `NetMonShell` invoke driver concurrently. `libssh2` sessions are not thread-safe. A hanging diagnostic query stalls all callers and blocks automated firewall blocks. | Serialized via `std::mutex _sshMutex`. Strict per-command I/O timeouts (3–5s max). Socket-level timeouts configured via `SO_RCVTIMEO`/`SO_SNDTIMEO` and `libssh2_session_set_timeout()`. Read-only diagnostics do not starve high-priority transactions. | **Resolved** |
| **2** | **ZySH Sub-Mode Traps, Prompt Shift & Echo/Paging Fragility** | Commands like `policy-control rule-insert 1` enter sub-modes (`Router(config-policy-control)#`). Single `exit` leaves CLI in sub-mode. Output pagination (`--More--`) freezes driver. Output containing `#` causes early prompt match. PTY echoes typed commands. | 1. Regex prompt matcher: `[\r\n][\w.-]+(\([A-Za-z0-9_.-]+\))?[>#]\s*$`<br>2. Multi-level submode exit recovery: loop `exit` / `\x03` / `end` until root prompt (`#` or `>`) is confirmed.<br>3. Suppress paging with `terminal length 0` + auto-space/q on `--More--` detection.<br>4. Exact command echo prefix matching and stripping.<br>5. PTY buffer drain/flush prior to sending new commands. | **Resolved** |
| **3** | **Zero Plaintext, Envelope Security & Atomic File Operations** | Passing credentials via env vars leaks to `/proc/*/environ`. Writing directly to `vault.enc` risks corruption on crash. `std::string` heap residue evades memory zeroing. | 1. Zero environment variables: credentials exclusively in `CredentialVault`.<br>2. Permissions: Directory `~/.config/netmon` `0700`, keyfile `0600`.<br>3. Atomic file writes: write to `vault.enc.tmp` with `0600` then atomic POSIX `rename()`.<br>4. String sanitization: `explicit_bzero` on raw data pointers and zero-capacity shrink.<br>5. Explicit threat model: Software mode protects against config/env leaks; YubiHSM 2 provides hardware isolation. | **Resolved** |
| **4** | **Config Lock Contention, Syntax Errors & Flash Wear Mitigation** | Entering `configure terminal` fails if Web GUI holds lock (`% Configuration is locked`). ZySH address objects reject dots in names. Immediate `write` (`copy running-config startup-config`) on every rule wears flash and spikes router CPU for 5–15s. | 1. Inspect `configure terminal` output for `% Configuration is locked` and abort cleanly with `status: "locked"`.<br>2. Sanitize object names (dots to underscores: `NETMON_BLK_192_168_8_50`) and use ZySH syntax `address-object <name> host <ip>`.<br>3. Delete rules by name (`no policy-control <name>`), not shifting numeric index.<br>4. Apply rules to in-memory running-config immediately for zero-latency enforcement; debounce flash writes (`write`) by 30s of quiescence with forced flush on SIGTERM. | **Resolved** |
| **5** | **Session Table Volume & Router CPU Spikes** | Running `show session table` streams tens of thousands of lines over PTY, stalling the driver for 10–30s and causing CPU spikes on the USG FLEX 200. | Routine health checks, web polling, and telemetry strictly query `show session summary`. Full `show session table` is reserved for on-demand diagnostic requests with explicit limits. | **Resolved** |
| **6** | **Safeguard Duplication & Dynamic Invariant Protection** | Hardcoding blacklists inside `ZyxelDriver` duplicates logic from `SecurityCheckpoint`. Hardcoded IPs fail when deployed on different subnets. | 1. Single source of truth: `ZyxelDriver::blockIp()` internally calls `SecurityCheckpoint::getInstance().validateBlockRequest()` and logs audit events. This is the sole validation point.<br>2. Callers (`AimonGatewayClient`, `WebServer`, `NetMonShell`) do NOT redundantly re-validate; they call `ZyxelDriver::blockIp()` directly.<br>3. Dynamic invariant registration: `SecurityCheckpoint` dynamically adds `Config::getGatewayHost()`, local interface IP, and subnet broadcasts at startup. | **Resolved** |
| **7** | **Subsystem Architecture: Full Singleton (No `shared_ptr`)** | The existing `AimonGatewayClient` uses `shared_ptr<RouterDriver>` and `setRouterDriver()`, but every other subsystem in netmon uses a concrete singleton `::getInstance()` pattern. | **Full Singleton**: Implement `ZyxelDriver::getInstance()` returning `ZyxelDriver&`. Remove `std::shared_ptr<RouterDriver> _routerDriver` and `setRouterDriver()` from `AimonGatewayClient`. All callers access `ZyxelDriver::getInstance()` directly. `RouterDriver` base class retains virtual methods but with default "unconfigured" stubs (current behavior) so the singleton is always valid. | **Resolved** |
| **8** | **`libssh2` Session Teardown, Fault Recovery & Proactive Reconnection** | Socket timeouts or transport disconnects leave `libssh2` state machine desynchronized. Reactive-only reconnection causes 3–5s latency spikes on the first post-failure command. | 1. On any timeout or socket transport error, mark connection dirty, perform full socket/session teardown (`disconnect()`).<br>2. Background keepalive thread: sends `libssh2_keepalive_send()` every 15s to detect dead connections proactively.<br>3. Reconnection with exponential backoff (1s → 2s → 4s → ... → 30s cap) in a background reconnection loop.<br>4. `isConnected()` state exposed so callers can return `"router offline"` immediately rather than blocking on handshake. | **Resolved** |
| **9** | **HSM Backend Isolation & Network Resilience** | Software vault cannot unwrap HSM ciphertext. If YubiHSM connector is temporarily unreachable at boot, daemon hangs. | `vault.backend` ("software" vs "yubihsm") dictates backend at init. Software vault is an independent standalone backend. HSM vault implements lazy connection and exponential backoff retry. | **Resolved** |
| **10** | **`libssh2_init()` / `libssh2_exit()` Global Lifecycle** | `libssh2_init(0)` must be called exactly once before any session creation (process-global). If omitted, `libssh2_session_init()` returns NULL or crashes depending on crypto backend. | Called in `Main.cxx` before any subsystem starts. `libssh2_exit()` called during orderly shutdown after all subsystems are stopped. | **Resolved** |
| **11** | **Credential Rotation & Authentication Failure Detection** | Router password changes on Zyxel Web GUI cause `LIBSSH2_ERROR_AUTHENTICATION_FAILED` (-18) on every subsequent SSH command. Without detection, the driver silently retries forever. | 1. Detect `LIBSSH2_ERROR_AUTHENTICATION_FAILED` specifically during `connect()`.<br>2. On auth failure: mark vault credentials stale, emit operator-visible alert to `NetMonShell` and stderr log, enter degraded "credentials expired" mode, stop reconnection attempts.<br>3. `vault set router.password` triggers: disconnect active SSH → update vault → initiate fresh connection with new credentials. | **Resolved** |
| **12** | **`CredentialVault` Thread-Safety & Key Integrity** | Concurrent `storeSecret()` (from `NetMonShell`) and `retrieveSecret()` (from `ZyxelSshClient`) on different threads race on the in-memory secret map. Corrupted `vault.key` causes GCM auth failure with undefined behavior. | 1. `SoftwareCredentialVault` internally protected by `std::mutex _vaultMutex`.<br>2. On GCM authentication tag failure: `initialize()` returns `false`, logs diagnostic "vault key corrupted or vault.enc tampered", driver enters degraded "unconfigured" mode with operator-visible alerts.<br>3. `HsmCredentialVault` similarly mutex-protected. | **Resolved** |
| **13** | **Debounce Timer Concrete Design & Shutdown Safety** | "30s quiescence timer" has no specified implementation. Pending `write` commands are lost on SIGTERM. Rapid block/unblock cycles trigger pointless flash saves. | 1. `std::atomic<bool> _pendingWrite` flag + `std::chrono::steady_clock::time_point _lastRuleChange` tracked in `ZyxelDriver`.<br>2. Before each command dispatch, check if pending write is overdue (>30s) and issue inline `write`.<br>3. On SIGTERM/SIGINT shutdown: `ZyxelDriver` destructor or `stop()` method force-flushes pending `write` synchronously before SSH teardown.<br>4. `unblockIp` after `blockIp` of same IP: cancel pending write only if no other rules changed; otherwise let timer continue. | **Resolved** |
| **14** | **`router.user` Plaintext Config — Intentional Design** | `router.user` remains in plaintext `netmon.cfg` while `router.password` is in the vault. | **By design**: Router admin usernames (typically `"admin"`) are non-secret operational parameters. Documented explicitly. Only `router.password` (and future SSH private key passphrases) reside in `CredentialVault`. | **Resolved (by design)** |
| **15** | **IPv6 — Explicit v1 Scope Exclusion** | All IP validation, object naming (`NETMON_BLK_192_168_8_50`), and safeguards assume IPv4. Zyxel USG FLEX 200 supports IPv6 policy-control. | **Explicitly scoped out of v1**: The SSH driver targets IPv4-only `policy-control` rules and `address-object host <IPv4>` syntax. IPv6 support is deferred to a future iteration. Documented in plan. | **Resolved (scoped out)** |
| **16** | **Double-Validation Redundancy** | `AimonGatewayClient` performed redundant validation checks prior to forwarding to `ZyxelDriver`. | Removed from `AimonGatewayClient`; `ZyxelDriver::blockIp()` is the sole validation gate. | **Resolved** |
| **17** | **Vault Key Corruption Resilience** | GCM decryption errors in vault crashed or returned empty data. | Handled via diagnostic log and clean degraded mode fallback. | **Resolved** |
| **18** | **Hardware Token Connection Resilience** | YubiHSM connector network delays hung subsystem initialization. | Lazy connection + exponential backoff retry thread. | **Resolved** |
| **19** | **Vault Thread-Safety Guarantee** | In-memory maps in vault lacked synchronization across concurrent REST, MCP, and CLI calls. | Added internal `std::mutex` to both `SoftwareCredentialVault` and `HsmCredentialVault`. | **Resolved** |
| **20** | **Comprehensive 14-Point Test Matrix** | Prior plan lacked edge case tests for lock contention, submode recovery, and keepalive timing. | Expanded verification matrix with 10 automated and 4 live hardware tests. | **Resolved** |
| **21** | **Ephemeral Human-Delegated Maintenance Token & Reconnect Resilience** | AI agents need authority for router maintenance without granting perpetual ambient access or exposing SSH credentials. Sockets (SSE/TCP) drop during maintenance, wiping ephemeral session IDs. | 1. **Human-in-the-Loop Token Generation**: Authenticated operator generates 128-bit random token in Web UI (`netmon:3884`) with explicit TTL (default 1 hour, scope `router:maintenance`).<br>2. **Reconnect-Resilient State**: Token hash and expiry timestamp stored in `SecurityCheckpoint` (independent of transient socket IDs or SSE session IDs). Survives SSE drops, IDE reconnects, and socket blips.<br>3. **Dual-Path Authorization**: (A) One-time MCP tool `auth_enable_maintenance(token)` opens the maintenance window in `SecurityCheckpoint`; (B) Optional `token` parameter in `firewall_block_ip(ip, reason, token)` for stateless execution.<br>4. **Immutable Invariants & Audit**: Protected infrastructure invariants (`192.168.8.1`, `builder`, `rhino`) remain enforced even with a valid token. Audit log records operator identity, token ID, and action.<br>5. **Emergency Revocation**: Operator can revoke the token instantly from Web UI or CLI. | **Resolved** |
| **22** | **Documentation & Specification Synchronization** | Architecture documentation (`Design.md`) and user/operator guides (`README.md`) drift out of date when new subsystems (Credential Vault, Zyxel SSH Driver, Maintenance Token) are introduced. | Comprehensive documentation plan: Update `Design.md` with full architectural specifications, sequence diagrams, and security models; update `README.md` with CLI commands, Web UI token workflow, MCP tool definitions, and configuration reference. | **Resolved** |
| **23** | **CppUTest Regression Framework, Privacy Sanitization & Test Fixture Isolation** | Ad-hoc testing lacks standardized assertions, leak detection, and regression chaining. Hardcoding live secrets or private device data in test fixtures risks leaking sensitive credentials to git. | 1. **Standardize on CppUTest**: Integrate `CppUTest` for all unit, integration, and regression suites (`TEST_GROUP`, `TEST`, memory leak detection).<br>2. **Strict Test Data Isolation**: All test artifacts and fixtures write exclusively to `.gitignored` `test_data/` or sandboxed `/tmp/netmon_test_*`.<br>3. **Zero Private Data in Git**: No real passwords, physical MACs, or private paths in repository.<br>4. **`Testing.md` Document**: Authoritative guide explaining test execution and how to populate test inputs if `~/.config/netmon/` is absent. | **Resolved** |

---

## Architectural Workflow & Component Topology

```text
+---------------------------------------------------------------------------------------------------+
|                                     NetMon Application Layer                                      |
|                                                                                                   |
|  +---------------------+        +--------------------+        +-------------------------------+   |
|  |   NetMonShell CLI   |        | WebServer REST API |        |      AimonGatewayClient       |   |
|  | ('vault', 'token')  |        |  (Web Dashboard)   |        |       (AIMON MCP Agent)       |   |
|  +----------+----------+        +---------+----------+        +---------------+---------------+   |
|             |                             |                                   |                   |
|             |      1. Generate Token      |                                   |                   |
|             |      (Auth Operator)        |                                   |                   |
|             +------------+                |                                   |                   |
|                          |                v                                   |                   |
|                          |    +-----------------------------+                 |                   |
|                          +--->|   Token & Invariant Store   |<----------------+                   |
|                               |    (SecurityCheckpoint)     |   2. auth_enable_maintenance(token) |
|                               | - 128-bit SHA-256 Hash      |   3. firewall_block_ip(ip, reason)  |
|                               | - Time-to-Live Expiry Clock |                                     |
|                               | - Emergency Revocation      |                                     |
|                               | - Core Invariants (Gateway) |                                     |
|                               +--------------+--------------+                                     |
|                                              | (Validation: Invariants + Active Token / Window)   |
|                                              v                                                    |
|                               +------------------------------+                                    |
|                               |  ZyxelDriver::getInstance()  |                                    |
|                               |  - Mutex Serialization       |                                    |
|                               |  - Debounced Flash Write     |                                    |
|                               |  - Auth Failure Detection    |                                    |
|                               +--------------+---------------+                                    |
+----------------------------------------------|----------------------------------------------------+
                                               |
               +-------------------------------+-------------------------------+
               |                                                               |
               v                                                               v
+----------------------------------------------+       +--------------------------------------------+
|             SSH Transport Engine             |       |           Security & Vault Layer           |
|                                              |       |        (ZERO ENVIRONMENT VARIABLES)        |
|  +----------------------------------------+  |       |                                            |
|  |             ZyxelSshClient             |  |       |           CredentialVaultFactory           |
|  | - dumb PTY allocation                  |  |       |                     |                      |
|  | - PTY buffer flush/drain               |  |       |                     v                      |
|  | - Dynamic Regex Prompt Matching        |  |       |         <<CredentialVault>> (API)          |
|  | - Multi-Level Submode Recovery         |  |       |             +-------+-------+              |
|  | - Page Suppress (--More-- Auto)        |  |       |             |               |              |
|  | - Exact Echo Strip Match               |  |       |             v               v              |
|  | - Auto-Teardown on Socket Error        |  |       |       +-----------+   +-----------+        |
|  | - Background Keepalive (15s)           |  |       |       | Software  |   |  YubiHSM  |        |
|  | - Exponential Backoff Reconnect        |  |       |       | (mutex)   |   |  (mutex)  |        |
|  | - Auth Failure -> Stale Alert          |  |       |       +-----+-----+   +-----+-----+        |
|  +--------------------+-------------------+  |       |             |               |              |
+-----------------------|----------------------+       |             v               v              |
                        |                              |       +-----------+   +-----------+        |
                        | SSH-2.0 (Port 22)            |       |  OpenSSL  |   | libyubihsm|        |
                        v                              |       |AES-256-GCM|   | Hardware  |        |
+----------------------------------------------+       |       +-----+-----+   +-----+-----+        |
|              Zyxel USG FLEX 200              |       |             |               |              |
|                                              |       |             | (Atomic POSIX |              |
|  - System & Interface Diagnostics            |       |             |  Rename Write)|              |
|  - DNS Proxy & DHCP Pools                    |       |             v               v              |
|  - Policy-Control Firewall (IPv4)            |       |       +-----------+   +-----------+        |
|  - Active Session Summary                    |       |       | vault.key |   |USB Connect|        |
+----------------------------------------------+       |       |  (0600)   |   | Port 12345|        |
                                                       |       +-----+-----+   +-----+-----+        |
                                                       |             |               |              |
                                                       |             +-------+       |              |
                                                       |                     v       v              |
                                                       |       +---------------------------+        |
                                                       |       | ~/.config/netmon/ (0700)  |        |
                                                       |       | vault.enc (AES-256-GCM)   |        |
                                                       |       +---------------------------+        |
+---------------------------------------------------------------------------------------------------+
```

---

## Phased Implementation, Testing & Regression Plan (CppUTest Framework)

```text
┌───────────────────────────────────────────────────────────────────────────────────┐
│ PHASE 1: Pluggable Credential Vault Subsystem (Zero Env Vars, OpenSSL, YubiHSM)   │
│ └─ CppUTest Suite: test/TestCredentialVault.cxx (Encryption, Permissions, Mutex)  │
└────────────────────────────────────────┬──────────────────────────────────────────┘
                                         ▼ (Gate 1 Passed)
┌───────────────────────────────────────────────────────────────────────────────────┐
│ PHASE 2: Fault-Tolerant SSH Transport Engine (libssh2, PTY, Regex, Keepalive)     │
│ ├─ CppUTest Suite: test/TestZyxelSshClient.cxx (Prompt Matcher, Echo, Submode)    │
│ └─ CppUTest Regression: Phase 1 (Vault) + Phase 2 (SSH Transport)                 │
└────────────────────────────────────────┬──────────────────────────────────────────┘
                                         ▼ (Gate 2 Passed)
┌───────────────────────────────────────────────────────────────────────────────────┐
│ PHASE 3: Dynamic Invariants, Maintenance Token & Zyxel Driver Subsystem           │
│ ├─ CppUTest Suite: test/TestSecurityCheckpoint.cxx, test/TestZyxelDriver.cxx      │
│ └─ CppUTest Regression: Phase 1 (Vault) + Phase 2 (SSH) + Phase 3 (Driver/Tokens) │
└────────────────────────────────────────┬──────────────────────────────────────────┘
                                         ▼ (Gate 3 Passed)
┌───────────────────────────────────────────────────────────────────────────────────┐
│ PHASE 4: Subsystem Wiring, MCP Gateway, CLI Shell, Web Dashboard & Live Hardware  │
│ ├─ CppUTest Suite: test/TestMcpIntegration.cxx, test/TestWebAuth.cxx              │
│ ├─ Live USG FLEX 200 End-to-End Hardware Verification                             │
│ └─ Full System CppUTest Regression Suite: All unit & integration tests (make test)│
└────────────────────────────────────────┬──────────────────────────────────────────┘
                                         ▼ (Gate 4 Passed)
┌───────────────────────────────────────────────────────────────────────────────────┐
│ PHASE 5: Comprehensive Documentation & Testing Guide (Design.md, README, Testing) │
│ └─ Verification: Documentation integrity, link validity, formatting standard      │
└───────────────────────────────────────────────────────────────────────────────────┘
```

---

### Test Data Privacy & Directory Isolation Standard

To ensure compliance with the repository privacy rules:
1. **Zero Private Data in Git**: Test code and repositories must **never** check in real physical MAC addresses, live router passwords, private SSH keys, or personal home directory paths.
2. **`.gitignored` Test Data Directory**:
   - The root `.gitignore` is updated to exclude `test_data/` and `.test_data/`.
   - All test fixtures write temporary configuration files (`netmon.cfg`), encrypted vaults (`vault.enc`), key files (`vault.key`), and device registries (`devices.cfg`) strictly inside `test_data/` or a temporary directory `/tmp/netmon_test_XXXXXX`.
3. **Runtime Fixture Cleanup**:
   - Every CppUTest test group implements `TEST_TEARDOWN()` to clean up transient files and zero out memory buffers.
4. **`Testing.md` SOP**:
   - Clear instructions on running CppUTest suites.
   - Detailed explanation on how synthetic test fixtures are generated.
   - Instructions on populating custom live hardware test inputs if `~/.config/netmon/` is uninitialized.

---

### Phase 1: Pluggable Credential Vault Subsystem (Zero Environment Variables)

#### 1.1 Implementation Scope
- **[MODIFY]** [.gitignore](file:///home/samurai/work/netmon/.gitignore): Add `test_data/` and `.test_data/`.
- **[MODIFY]** [CMakeLists.txt](file:///home/samurai/work/netmon/CMakeLists.txt):
  - Add `pkg_check_modules(LIBSSH2 REQUIRED libssh2)`.
  - Check optional `libyubihsm` (`HAVE_YUBIHSM=1`).
  - Add `find_package(CppUTest)` or `pkg_check_modules(CPPUTEST REQUIRED cpputest)`.
  - Compile targets: `src/CredentialVault.cxx`, `src/SoftwareCredentialVault.cxx`, `src/HsmCredentialVault.cxx`.
  - Build CppUTest test runner: `test/test_netmon_suite`.
- **[MODIFY]** [Config.hxx](file:///home/samurai/work/netmon/include/Config.hxx) / [Config.cxx](file:///home/samurai/work/netmon/src/Config.cxx):
  - Remove `getenv("NETMON_ROUTER_USER")` and `getenv("NETMON_ROUTER_PASSWORD")`.
  - Remove plaintext `_routerPassword` and `getRouterPassword()`/`setRouterPassword()`.
  - Add non-sensitive vault fields: `vault.backend`, `vault.key_file`, `vault.enc_file`, `vault.hsm_url`, `vault.hsm_key_id`.
- **[NEW]** [CredentialVault.hxx](file:///home/samurai/work/netmon/include/CredentialVault.hxx) / [CredentialVault.cxx](file:///home/samurai/work/netmon/src/CredentialVault.cxx): Abstract base class and `CredentialVaultFactory::create(const Config &config)`.
- **[NEW]** [SoftwareCredentialVault.hxx](file:///home/samurai/work/netmon/include/SoftwareCredentialVault.hxx) / [SoftwareCredentialVault.cxx](file:///home/samurai/work/netmon/src/SoftwareCredentialVault.cxx):
  - Internal `std::mutex _vaultMutex` serialization.
  - Automatic `test_data/` directory creation with strict `0700` permissions.
  - 256-bit key storage in `test_data/vault.key` with strict `0600` permissions.
  - AES-256-GCM ciphertext in `test_data/vault.enc` written via atomic temporary file rename (`vault.enc.tmp` -> `vault.enc`).
  - GCM authentication tag verification and corruption detection.
  - Memory sanitization with `explicit_bzero` and string capacity reset.
- **[NEW]** [HsmCredentialVault.hxx](file:///home/samurai/work/netmon/include/HsmCredentialVault.hxx) / [HsmCredentialVault.cxx](file:///home/samurai/work/netmon/src/HsmCredentialVault.cxx):
  - Hardware-backed YubiHSM 2 implementation with lazy connector connection and retry thread.

#### 1.2 Phase 1 CppUTest Suite (`test/TestCredentialVault.cxx`)
- **`TEST_GROUP(CredentialVaultTest)`**:
  - `TEST_SETUP()`: Initializes sandboxed `test_data/` directory with `0700` permissions.
  - `TEST_TEARDOWN()`: Cleans up `test_data/` temporary files.
  - `TEST(CredentialVaultTest, PermissionsAndDirectoryCreation)`: Asserts directory created `0700`, keyfile created `0600`.
  - `TEST(CredentialVaultTest, RoundTripStorageAndRetrieval)`: Asserts exact string retrieval for standard secrets.
  - `TEST(CredentialVaultTest, BoundaryConditions)`: Tests empty secrets, 4 KB large payloads, UTF-8 strings, special characters, and newlines.
  - `TEST(CredentialVaultTest, AtomicWriteIntegrity)`: Verifies `vault.enc.tmp` is created `0600` and atomically renamed to `vault.enc`.
  - `TEST(CredentialVaultTest, CorruptionAndTamperingDetection)`: Corrupts ciphertext and key bytes; asserts `initialize()` returns `false` with diagnostic message and no crash.
  - `TEST(CredentialVaultTest, MultithreadedConcurrency)`: 10 worker threads concurrently storing/retrieving secrets; verifies thread safety under `_vaultMutex`.
  - `TEST(CredentialVaultTest, MemorySanitization)`: Verifies `explicit_bzero` clears secret buffers after use.
- **Phase 1 Exit Gate**: `make test-vault` runs CppUTest suite and passes 100% with zero memory leaks.

---

### Phase 2: Fault-Tolerant Zyxel SSH Transport Engine

#### 2.1 Implementation Scope
- **[MODIFY]** [Main.cxx](file:///home/samurai/work/netmon/src/Main.cxx): Add global `libssh2_init(0)` at startup and `libssh2_exit()` at shutdown.
- **[NEW]** [ZyxelSshClient.hxx](file:///home/samurai/work/netmon/include/ZyxelSshClient.hxx) / [ZyxelSshClient.cxx](file:///home/samurai/work/netmon/src/ZyxelSshClient.cxx):
  - Mutex serialization (`_sshMutex`).
  - Connection lifecycle: `connect()`, `disconnect()`, `isConnected()`. Socket timeout via `SO_RCVTIMEO`/`SO_SNDTIMEO` and `libssh2_session_set_timeout()`.
  - PTY allocation (`dumb`), PTY buffer flush before each command.
  - Regex prompt matcher: `[\r\n][\w.-]+(\([A-Za-z0-9_.-]+\))?[>#]\s*$`.
  - Multi-level submode recovery: loop `exit` / `\x03` / `end` until root `#` is restored.
  - Output paging suppression: `terminal length 0` + `--More--` auto-handling.
  - Command echo prefix stripping.
  - Background keepalive thread: 15s interval via `libssh2_keepalive_send()`.
  - Reconnection state machine with exponential backoff (1s -> 30s cap).
  - Authentication failure detection (`LIBSSH2_ERROR_AUTHENTICATION_FAILED` -> stale state, halt retries).
  - Secret retrieval exclusively from `CredentialVault`.

#### 2.2 Phase 2 CppUTest Suite (`test/TestZyxelSshClient.cxx`)
- **`TEST_GROUP(ZyxelSshClientTest)`**:
  - `TEST(ZyxelSshClientTest, RegexPromptMatcherVariations)`: Asserts prompt match on root (`Router#`, `Router>`), submodes (`Router(config-policy-control)#`, `USG(config-if)#`), trailing whitespace, and rejects '#' within output payload.
  - `TEST(ZyxelSshClientTest, EchoPrefixStripping)`: Verifies exact command echo removal on single and multi-line output with special characters.
  - `TEST(ZyxelSshClientTest, SubmodeEscapeRecoveryLoop)`: Simulates CLI trapped 3 levels deep in submodes; asserts recovery loop issues escape sequences until root prompt `#` is confirmed.
  - `TEST(ZyxelSshClientTest, AuthFailureStateMachine)`: Simulates `LIBSSH2_ERROR_AUTHENTICATION_FAILED`; verifies driver enters stale credentials mode and stops infinite reconnection loops.
  - `TEST(ZyxelSshClientTest, SocketTimeoutTeardown)`: Simulates transport hang; verifies timeout fires, channel is torn down, and `isConnected() == false`.
  - `TEST(ZyxelSshClientTest, MultithreadedChannelMutex)`: Concurrent simulated command dispatches; verifies serialized channel I/O.
- **Phase 2 CppUTest Regression**: Execute `make test` covering `CredentialVaultTest` + `ZyxelSshClientTest`.
- **Phase 2 Exit Gate**: All Phase 1 and Phase 2 CppUTest tests pass 100%.

---

### Phase 3: Dynamic Invariants, Maintenance Token Subsystem & Zyxel Driver

#### 3.1 Implementation Scope
- **[MODIFY]** [SecurityCheckpoint.hxx](file:///home/samurai/work/netmon/include/SecurityCheckpoint.hxx) / [SecurityCheckpoint.cxx](file:///home/samurai/work/netmon/src/SecurityCheckpoint.cxx):
  - Dynamic invariant registration (`registerDynamicInvariants`).
  - Invariant protection (`isIpProtected`).
  - Ephemeral human-delegated Maintenance Token subsystem:
    - `generateMaintenanceToken(ttlSeconds, creator, scope)`
    - `activateMaintenanceToken(token, outReason)`
    - `revokeMaintenanceToken()`
    - `isMaintenanceActive()`, `getMaintenanceStatus()`
  - Dual-path validation: `validateBlockRequest(ip, token, outReason)`.
  - Expanded audit logging (`logAudit`) with token ID and operator attribution.
- **[MODIFY]** [RouterDriver.hxx](file:///home/samurai/work/netmon/include/RouterDriver.hxx): Non-pure virtual interface with default unconfigured stubs and `token` parameter.
- **[MODIFY]** [ZyxelDriver.hxx](file:///home/samurai/work/netmon/include/ZyxelDriver.hxx) / [ZyxelDriver.cxx](file:///home/samurai/work/netmon/src/ZyxelDriver.cxx):
  - Singleton `ZyxelDriver::getInstance()`.
  - Diagnostics: `getStatus()`, `getDnsConfig()`, `getRules()`, `getSessions()` (`show session summary`).
  - Transactional firewall operations: `blockIp(ip, reason, token)` and `unblockIp(ip, token)`.
  - Sole validation gate: internal call to `SecurityCheckpoint::validateBlockRequest()`.
  - Sanitized object naming: `NETMON_BLK_<IP>` (IPv4 only).
  - Config lock detection (`% Configuration is locked`) with clean abort (`status: "locked"`).
  - Transaction rollback on failure (clean address-object and policy-control cleanup).
  - Debounced flash write: 30s quiescence timer with forced flush on SIGTERM/SIGINT.
  - `executeCli(cmd)` with output capture and error inspection.

#### 3.2 Phase 3 CppUTest Suite (`test/TestSecurityCheckpoint.cxx`, `test/TestZyxelDriver.cxx`)
- **`TEST_GROUP(SecurityCheckpointTest)`**:
  - `TEST(SecurityCheckpointTest, DynamicInvariantProtection)`: Asserts `192.168.8.1`, `rhino`, `builder`, `127.0.0.1`, and `255.255.255.255` cannot be blocked under any circumstance.
  - `TEST(SecurityCheckpointTest, MaintenanceTokenLifecycle)`: Generates 2-second token, activates it, verifies access, sleeps 3 seconds, verifies expiration.
  - `TEST(SecurityCheckpointTest, EmergencyTokenRevocation)`: Generates 1-hour token, activates it, calls `revokeMaintenanceToken()`, asserts immediate denial.
  - `TEST(SecurityCheckpointTest, DualPathAuthorization)`: Asserts Path A (stateless token parameter) and Path B (active session window) both authorize successfully.
  - `TEST(SecurityCheckpointTest, SocketDisconnectResilience)`: Verifies token validity persists in `SecurityCheckpoint` across client socket reconnect simulations.
- **`TEST_GROUP(ZyxelDriverTest)`**:
  - `TEST(ZyxelDriverTest, ConfigLockDetection)`: Simulates `% Configuration is locked`; verifies clean abort with `status: "locked"`.
  - `TEST(ZyxelDriverTest, TransactionRollbackOnFailure)`: Simulates rule insertion failure; verifies rollback cleans address objects and returns CLI to root prompt `#`.
  - `TEST(ZyxelDriverTest, DebouncedFlashWriteAndSigtermFlush)`: Issues rapid block calls; verifies 30s debounce and synchronous flash write on shutdown.
- **Phase 3 CppUTest Regression**: Execute `make test` covering `CredentialVaultTest`, `ZyxelSshClientTest`, `SecurityCheckpointTest`, and `ZyxelDriverTest`.
- **Phase 3 Exit Gate**: All Phase 1, Phase 2, and Phase 3 tests pass 100%.

---

### Phase 4: Subsystem Wiring, MCP Gateway, CLI Shell, Web Dashboard & Live Hardware

#### 4.1 Implementation Scope
- **[MODIFY]** [Main.cxx](file:///home/samurai/work/netmon/src/Main.cxx):
  - Subsystem initialization order.
  - Remove `shared_ptr<RouterDriver>` wiring; all subsystems use `ZyxelDriver::getInstance()`.
  - Register dynamic invariants in `SecurityCheckpoint`.
- **[MODIFY]** [AimonGatewayClient.hxx](file:///home/samurai/work/netmon/include/AimonGatewayClient.hxx) / [AimonGatewayClient.cxx](file:///home/samurai/work/netmon/src/AimonGatewayClient.cxx):
  - Register new MCP tool `auth_enable_maintenance(token)`.
  - Update `firewall_block_ip` and `firewall_unblock_ip` tool schemas with optional `"token"` property.
  - Implement tool handler `toolAuthEnableMaintenance(token)`.
  - Forward `token` argument from firewall tools to `ZyxelDriver::getInstance().blockIp(ip, reason, token)`.
- **[MODIFY]** [NetMonShell.hxx](file:///home/samurai/work/netmon/include/NetMonShell.hxx) / [NetMonShell.cxx](file:///home/samurai/work/netmon/src/NetMonShell.cxx):
  - Interactive CLI commands: `vault status`, `vault set <key>`, `router set-password`, `token gen [ttl]`, `token status`, `token revoke`, `router *`.
- **[MODIFY]** [WebServer.hxx](file:///home/samurai/work/netmon/include/WebServer.hxx) / [WebServer.cxx](file:///home/samurai/work/netmon/src/WebServer.cxx):
  - REST endpoints: `/api/router/*`, `/api/vault/status`, `/api/auth/token/generate`, `/api/auth/token/revoke`, `/api/auth/token/status`.
  - Web Dashboard UI: "AI Maintenance Authorization" card with 1-click clipboard copy, countdown timer, and emergency revocation button.

#### 4.2 Phase 4 CppUTest Integration & Live Gateway Hardware Tests
- **`TEST_GROUP(McpIntegrationTest)` & `TEST_GROUP(WebAuthTest)`**:
  - `TEST(McpIntegrationTest, ToolDispatchAndTokenVerification)`: Simulates `aimon` JSON-RPC `tools/call` for `auth_enable_maintenance`, `firewall_block_ip`, and `firewall_get_status`.
  - `TEST(WebAuthTest, RestEndpointLifecycle)`: Validates `/api/auth/token/generate`, `/api/auth/token/status`, and `/api/auth/token/revoke`.
- **Live Gateway Hardware Tests (USG FLEX 200)**:
  1. **Unattended Reboot Emulation**: Store password in vault; restart daemon; verify automatic connection and decryption without operator input.
  2. **Proactive Reconnection & Keepalive**: Reboot USG FLEX 200 gateway; verify `netmon` detects disconnection via keepalive and reconnects with exponential backoff.
  3. **End-to-End Delegated Maintenance Workflow**:
     - Generate token in Web UI (`:3884`).
     - Prompt AI agent with token -> AI calls `auth_enable_maintenance(token)` -> AI calls `firewall_block_ip(test_ip, reason)`.
     - Verify rule active on USG FLEX 200 with sanitized object name (`NETMON_BLK_<IP>`).
     - AI calls `firewall_unblock_ip(test_ip)` -> verify clean rule and object removal.
  4. **Credential Rotation**: Change router password in Zyxel Web GUI; verify `netmon` detects auth failure and enters stale mode; update via `vault set router.password`; verify immediate reconnection.
- **Full CppUTest Regression Suite**: Execute `make test-all` running all test groups across Phases 1–4.
- **Phase 4 Exit Gate**: Full CppUTest regression suite passes 100% and live hardware tests succeed.

---

### Phase 5: Comprehensive Documentation & Testing Guide

#### 5.1 Implementation Scope
- **[NEW]** [Testing.md](file:///home/samurai/work/netmon/Testing.md):
  - Comprehensive guide for running CppUTest unit and regression suites (`make test`, `make test-vault`, `make test-all`).
  - Test environment privacy and directory isolation standard (`test_data/` in `.gitignore`).
  - Step-by-step SOP for populating test inputs and fixtures if `~/.config/netmon/` is uninitialized on a developer machine.
  - Distinctions between synthetic mock fixtures and optional live gateway credentials.
- **[MODIFY]** [Design.md](file:///home/samurai/work/netmon/Design.md):
  - Update system architecture diagram, component topology, and data flow.
  - Add Section 3.6: **Zyxel USG FLEX 200 SSH Driver Architecture**.
  - Add Section 3.7: **Pluggable Credential Vault Subsystem**.
  - Add Section 3.8: **Ephemeral Human-Delegated Maintenance Token & Security Checkpoint**.
- **[MODIFY]** [README.md](file:///home/samurai/work/netmon/README.md):
  - Update feature highlights (Zyxel router integration, Credential Vault, Human-Delegated Maintenance Tokens).
  - Update Configuration Reference (`vault.*` options, removal of plaintext password env vars).
  - Document interactive CLI commands in `NetMonShell` (`vault`, `token`, `router`).
  - Document Web Dashboard maintenance flow (token generation, clipboard copy, live countdown, emergency revocation).
  - Document MCP toolset catalog (`auth_enable_maintenance`, updated `firewall_block_ip` / `firewall_unblock_ip` schemas).

#### 5.2 Phase 5 Verification
- Verify documentation adheres strictly to project guidelines (personal project of Charles Chiou, `samurai@selfso.com`, domain `selfso.com`, zero employer boilerplate, generic sanitized paths and placeholders).
- Verify all links and code symbol references resolve correctly.
- **Phase 5 Exit Gate**: Documentation complete, consistent, and validated.

---

## Scope Exclusions (v1)

1. **IPv6 Policy-Control**: All IP validation, address object naming (`NETMON_BLK_<IP>`), and firewall rule operations are strictly IPv4. IPv6 policy-control support is deferred to a future iteration.
2. **SSH Public Key Authentication**: v1 uses password-only authentication via `CredentialVault`. SSH key-based auth is deferred.
3. **Multi-Router Support**: v1 targets a single Zyxel USG FLEX 200 gateway. Multi-device driver dispatching is deferred.
