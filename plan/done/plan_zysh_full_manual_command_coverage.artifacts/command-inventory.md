# ZySH Complete Manual Command Inventory (Envelope 0)

- **Manual**: *Zyxel ZyWALL ZLD Series CLI Reference Guide* (Firmware 5.38 / USG FLEX Series, 666 pages)
- **Date**: 2026-10-06
- **Total Chapters Covered**: 73 (Chapters 1 through 73, complete)
- **Total Commands Cataloged**: 3211
- **Status**: Authoritative Reference Baseline for Envelopes 1 through 10

## 1. Disposition Legend & Verification Rules

| Disposition | Meaning | Live Execution Envelope | Test Safety Verification Gate |
|---|---|---|---|
| `read` | Read-only interrogation (`show ...`, `dir`, ping/diagnostic status) | Live firewall SSH session | Real tabular parsing, non-destructive, $\le 1500$ ms timing check |
| `tier1` | Dry-fire / invalid syntax check | Live firewall SSH session | Assert firmware parser rejects command (`% Parse error`) with zero router state mutation |
| `tier2` | Bounded reversible mutation on dummy RFC 5737 objects (`NETMON_QA_*`) | Live firewall SSH session | Sweeper rollback in `setup()` & `teardown()`, settling time 100 ms, zero NVRAM writes |
| `redline` | Destructive / persistent / destabilizing operation (reboot, write, interfaces) | **OFFLINE TESTS ONLY** | Excluded from live router; hermetic testing in `test/TestZyxel*Cmd.cxx` |
| `out-of-scope` | Conceptual syntax / keystroke conventions without router command | N/A | Documented reference; no live test |

## 2. Master Chapter Allocation Matrix (Chapters 1 to 73)

| Ch # | Chapter Title | Pages | Envelope | Owning Class | Commands | Read | Tier 1 | Tier 2 | Redline | Out-of-Scope | Scope / Boundary Notes |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | Command Line Interface | 26-41 | Envelope 1 | `ZyxelSystemCmd` | 1 | 0 | 0 | 0 | 0 | 1 | Conceptual/introductory CLI syntax conventions, keystroke tables, regex conventions; no operational router commands. |
| 2 | User and Privilege Modes | 42-44 | Envelope 1 | `ZyxelSystemCmd` | 34 | 7 | 24 | 0 | 3 | 0 | Standard Envelope 1 operational commands. |
| 3 | Object Reference | 45-46 | Envelope 1 | `ZyxelSystemCmd` | 26 | 26 | 0 | 0 | 0 | 0 | Standard Envelope 1 operational commands. |
| 4 | Status | 47-52 | Envelope 1 | `ZyxelSystemCmd` | 38 | 32 | 6 | 0 | 0 | 0 | Standard Envelope 1 operational commands. |
| 5 | Registration | 53-55 | Envelope 1 | `ZyxelSystemCmd` | 10 | 6 | 4 | 0 | 0 | 0 | Standard Envelope 1 operational commands. |
| 6 | AP Management | 56-66 | Envelope 10 | `ZyxelWlanCmd` | 52 | 18 | 33 | 0 | 1 | 0 | Standard Envelope 10 operational commands. |
| 7 | Built-in AP ................................................................................................................... 6 7 | 67-68 | Envelope 10 | `ZyxelWlanCmd` | 11 | 0 | 11 | 0 | 0 | 0 | Standard Envelope 10 operational commands. |
| 8 | AP Group | 69-75 | Envelope 10 | `ZyxelWlanCmd` | 40 | 6 | 34 | 0 | 0 | 0 | Standard Envelope 10 operational commands. |
| 9 | Wireless LAN Profiles | 76-96 | Envelope 10 | `ZyxelWlanCmd` | 127 | 10 | 114 | 3 | 0 | 0 | Standard Envelope 10 operational commands. |
| 10 | Rogue AP | 97-100 | Envelope 10 | `ZyxelWlanCmd` | 21 | 6 | 15 | 0 | 0 | 0 | Standard Envelope 10 operational commands. |
| 11 | Wireless Health | 101-105 | Envelope 10 | `ZyxelWlanCmd` | 20 | 3 | 17 | 0 | 0 | 0 | Standard Envelope 10 operational commands. |
| 12 | Wireless Frame Capture | 106-108 | Envelope 10 | `ZyxelWlanCmd` | 12 | 3 | 9 | 0 | 0 | 0 | Standard Envelope 10 operational commands. |
| 13 | Dynamic Channel Selection | 109-109 | Envelope 10 | `ZyxelWlanCmd` | 1 | 0 | 1 | 0 | 0 | 0 | Standard Envelope 10 operational commands. |
| 14 | Auto-Healing | 110-111 | Envelope 10 | `ZyxelWlanCmd` | 17 | 1 | 16 | 0 | 0 | 0 | Standard Envelope 10 operational commands. |
| 15 | LEDs .......................................................................................................................... 1 12 | 112-113 | Envelope 1 | `ZyxelSystemCmd` | 7 | 2 | 5 | 0 | 0 | 0 | Standard Envelope 1 operational commands. |
| 16 | Interfaces................................................................................................................... 11 4 | 114-161 | Envelope 2 | `ZyxelNetworkCmd` | 308 | 63 | 241 | 0 | 4 | 0 | Standard Envelope 2 operational commands. |
| 17 | Trunks....................................................................................................................... 16 2 | 162-165 | Envelope 2 | `ZyxelNetworkCmd` | 14 | 3 | 11 | 0 | 0 | 0 | Assigned strictly to Envelope 2 (Interfaces/L2/Trunks). Trunk load balancing, algorithms, interface groupings. |
| 18 | Route ........................................................................................................................ 16 6 | 166-175 | Envelope 3 | `ZyxelNetworkCmd` | 76 | 19 | 53 | 4 | 0 | 0 | Standard Envelope 3 operational commands. |
| 19 | Routing Protocol | 176-182 | Envelope 3 | `ZyxelNetworkCmd` | 62 | 6 | 56 | 0 | 0 | 0 | Standard Envelope 3 operational commands. |
| 20 | Zones........................................................................................................................ 18 3 | 183-185 | Envelope 2 | `ZyxelNetworkCmd` | 11 | 6 | 5 | 0 | 0 | 0 | Standard Envelope 2 operational commands. |
| 21 | DDNS | 186-188 | Envelope 5 | `ZyxelNatCmd` | 20 | 2 | 16 | 1 | 1 | 0 | Standard Envelope 5 operational commands. |
| 22 | Virtual Servers | 189-199 | Envelope 5 | `ZyxelNatCmd` | 36 | 5 | 20 | 11 | 0 | 0 | Standard Envelope 5 operational commands. |
| 23 | HTTP Redirect | 200-201 | Envelope 5 | `ZyxelNatCmd` | 6 | 1 | 5 | 0 | 0 | 0 | Standard Envelope 5 operational commands. |
| 24 | Redirect Service | 202-205 | Envelope 5 | `ZyxelNatCmd` | 15 | 1 | 14 | 0 | 0 | 0 | Standard Envelope 5 operational commands. |
| 25 | ALG...........................................................................................................................2 06 | 206-208 | Envelope 5 | `ZyxelNatCmd` | 1 | 0 | 1 | 0 | 0 | 0 | Standard Envelope 5 operational commands. |
| 26 | UPnP.........................................................................................................................20 9 | 209-211 | Envelope 5 | `ZyxelNatCmd` | 5 | 0 | 5 | 0 | 0 | 0 | Standard Envelope 5 operational commands. |
| 27 | IP/MAC Binding | 212-213 | Envelope 2 | `ZyxelNetworkCmd` | 7 | 5 | 2 | 0 | 0 | 0 | Assigned to Envelope 2. IP/MAC binding activation and status queries (omitted from official PDF alphabetical index). |
| 28 | Layer 2 Isolation | 214-216 | Envelope 2 | `ZyxelNetworkCmd` | 18 | 4 | 14 | 0 | 0 | 0 | Standard Envelope 2 operational commands. |
| 29 | Secure Policy | 217-238 | Envelope 6 | `ZyxelFirewallCmd` | 187 | 48 | 136 | 3 | 0 | 0 | Standard Envelope 6 operational commands. |
| 30 | Cloud CNM | 239-250 | Envelope 9 | `ZyxelAuthCmd` | 51 | 7 | 44 | 0 | 0 | 0 | Standard Envelope 9 operational commands. |
| 31 | Web Authentication | 251-261 | Envelope 9 | `ZyxelAuthCmd` | 71 | 19 | 51 | 1 | 0 | 0 | Standard Envelope 9 operational commands. |
| 32 | Hotspot | 262-276 | Envelope 9 | `ZyxelAuthCmd` | 128 | 35 | 92 | 0 | 1 | 0 | Standard Envelope 9 operational commands. |
| 33 | IPSec VPN | 277-294 | Envelope 8 | `ZyxelVpnCmd` | 186 | 22 | 161 | 0 | 3 | 0 | Standard Envelope 8 operational commands. |
| 34 | SSL VPN | 295-298 | Envelope 8 | `ZyxelVpnCmd` | 22 | 6 | 16 | 0 | 0 | 0 | Standard Envelope 8 operational commands. |
| 35 | L2TP VPN | 299-306 | Envelope 8 | `ZyxelVpnCmd` | 29 | 4 | 23 | 1 | 1 | 0 | Standard Envelope 8 operational commands. |
| 36 | Bandwidth Management | 307-312 | Envelope 7 | `ZyxelSecurityCmd` | 41 | 5 | 34 | 2 | 0 | 0 | Standard Envelope 7 operational commands. |
| 37 | Application Patrol | 313-315 | Envelope 7 | `ZyxelSecurityCmd` | 31 | 16 | 14 | 0 | 1 | 0 | Standard Envelope 7 operational commands. |
| 38 | Anti-Virus | 316-323 | Envelope 7 | `ZyxelSecurityCmd` | 56 | 20 | 35 | 0 | 1 | 0 | Standard Envelope 7 operational commands. |
| 39 | RTLS .........................................................................................................................32 4 | 324-325 | Envelope 7 | `ZyxelSecurityCmd` | 5 | 2 | 3 | 0 | 0 | 0 | Standard Envelope 7 operational commands. |
| 40 | Reputation Filter | 326-344 | Envelope 7 | `ZyxelSecurityCmd` | 143 | 51 | 92 | 0 | 0 | 0 | Standard Envelope 7 operational commands. |
| 41 | Sandboxing | 345-347 | Envelope 7 | `ZyxelSecurityCmd` | 22 | 8 | 14 | 0 | 0 | 0 | Standard Envelope 7 operational commands. |
| 42 | IDP Commands | 348-364 | Envelope 7 | `ZyxelSecurityCmd` | 71 | 25 | 45 | 0 | 1 | 0 | Standard Envelope 7 operational commands. |
| 43 | Content Filtering | 365-398 | Envelope 7 | `ZyxelSecurityCmd` | 100 | 21 | 79 | 0 | 0 | 0 | Standard Envelope 7 operational commands. |
| 44 | Anti-Spam | 399-410 | Envelope 7 | `ZyxelSecurityCmd` | 96 | 37 | 59 | 0 | 0 | 0 | Standard Envelope 7 operational commands. |
| 45 | Collaborative Detection & Response | 411-417 | Envelope 7 | `ZyxelSecurityCmd` | 34 | 7 | 26 | 0 | 1 | 0 | Assigned to Envelope 7 (UTM). Collaborative Detection & Response (CDR) containment rules and signatures. |
| 46 | SSL Inspection | 418-425 | Envelope 7 | `ZyxelSecurityCmd` | 42 | 12 | 30 | 0 | 0 | 0 | Assigned to Envelope 7 (UTM). SSL Inspection profiles, certificate inspection, and exclusions. |
| 47 | IP Exception | 426-427 | Envelope 2 | `ZyxelNetworkCmd` | 6 | 2 | 4 | 0 | 0 | 0 | Standard Envelope 2 operational commands. |
| 48 | Device HA | 428-437 | Envelope 6 | `ZyxelFirewallCmd` | 61 | 27 | 32 | 0 | 2 | 0 | Standard Envelope 6 operational commands. |
| 49 | Device Insight | 438-442 | Envelope 2 | `ZyxelNetworkCmd` | 19 | 7 | 12 | 0 | 0 | 0 | Standard Envelope 2 operational commands. |
| 50 | User/Group | 443-454 | Envelope 9 | `ZyxelAuthCmd` | 54 | 11 | 40 | 0 | 3 | 0 | Standard Envelope 9 operational commands. |
| 51 | Application Object | 455-457 | Envelope 4 | `ZyxelObjectCmd` | 13 | 2 | 11 | 0 | 0 | 0 | Standard Envelope 4 operational commands. |
| 52 | Addresses | 458-466 | Envelope 4 | `ZyxelObjectCmd` | 34 | 14 | 10 | 10 | 0 | 0 | Standard Envelope 4 operational commands. |
| 53 | Services | 467-469 | Envelope 4 | `ZyxelObjectCmd` | 12 | 2 | 4 | 6 | 0 | 0 | Standard Envelope 4 operational commands. |
| 54 | Schedules | 470-471 | Envelope 4 | `ZyxelObjectCmd` | 3 | 1 | 0 | 2 | 0 | 0 | Standard Envelope 4 operational commands. |
| 55 | AAA Server | 472-478 | Envelope 9 | `ZyxelAuthCmd` | 69 | 6 | 63 | 0 | 0 | 0 | Standard Envelope 9 operational commands. |
| 56 | Authentication Objects | 479-489 | Envelope 9 | `ZyxelAuthCmd` | 38 | 6 | 31 | 0 | 1 | 0 | Standard Envelope 9 operational commands. |
| 57 | Authentication Server | 490-491 | Envelope 9 | `ZyxelAuthCmd` | 13 | 3 | 10 | 0 | 0 | 0 | Standard Envelope 9 operational commands. |
| 58 | Certificates | 492-497 | Envelope 9 | `ZyxelAuthCmd` | 17 | 6 | 11 | 0 | 0 | 0 | Standard Envelope 9 operational commands. |
| 59 | ISP Accounts | 498-500 | Envelope 4 | `ZyxelObjectCmd` | 20 | 2 | 18 | 0 | 0 | 0 | Standard Envelope 4 operational commands. |
| 60 | SSL Application | 501-503 | Envelope 4 | `ZyxelObjectCmd` | 10 | 1 | 9 | 0 | 0 | 0 | Standard Envelope 4 operational commands. |
| 61 | DHCPv6 Objects | 504-506 | Envelope 4 | `ZyxelObjectCmd` | 13 | 5 | 8 | 0 | 0 | 0 | Standard Envelope 4 operational commands. |
| 62 | Dynamic Guest Accounts | 507-509 | Envelope 9 | `ZyxelAuthCmd` | 29 | 3 | 26 | 0 | 0 | 0 | Standard Envelope 9 operational commands. |
| 63 | System | 510-525 | Envelope 1 | `ZyxelSystemCmd` | 126 | 33 | 86 | 5 | 2 | 0 | Standard Envelope 1 operational commands. |
| 64 | System Remote Management | 526-538 | Envelope 1 | `ZyxelSystemCmd` | 62 | 8 | 51 | 0 | 3 | 0 | Standard Envelope 1 operational commands. |
| 65 | File Manager | 539-564 | Envelope 1 | `ZyxelSystemCmd` | 61 | 11 | 46 | 1 | 3 | 0 | Standard Envelope 1 operational commands. |
| 66 | Logs.......................................................................................................................... 5 65 | 565-571 | Envelope 1 | `ZyxelSystemCmd` | 59 | 11 | 44 | 3 | 1 | 0 | Standard Envelope 1 operational commands. |
| 67 | Reports and Reboot | 572-577 | Envelope 1 | `ZyxelSystemCmd` | 41 | 10 | 30 | 1 | 0 | 0 | Standard Envelope 1 operational commands. |
| 68 | Diagnostics and Remote Assistance | 578-580 | Envelope 1 | `ZyxelSystemCmd` | 20 | 7 | 11 | 1 | 1 | 0 | Standard Envelope 1 operational commands. |
| 69 | Session Timeout | 581-581 | Envelope 1 | `ZyxelSystemCmd` | 3 | 1 | 2 | 0 | 0 | 0 | Standard Envelope 1 operational commands. |
| 70 | Packet Flow Explore | 582-585 | Envelope 1 | `ZyxelSystemCmd` | 12 | 12 | 0 | 0 | 0 | 0 | Standard Envelope 1 operational commands. |
| 71 | Maintenance Tools | 586-596 | Envelope 1 | `ZyxelSystemCmd` | 62 | 13 | 34 | 5 | 10 | 0 | Standard Envelope 1 operational commands. |
| 72 | Miscellaneous | 597-603 | Envelope 1 | `ZyxelSystemCmd` | 33 | 9 | 21 | 0 | 3 | 0 | Standard Envelope 1 operational commands. |
| 73 | Managed AP Commands | 604-607 | Envelope 10 | `ZyxelWlanCmd` | 10 | 3 | 5 | 2 | 0 | 0 | Standard Envelope 10 operational commands. |

## 3. Summary Statistics by Envelope

| Envelope | Owning C++ Class | Chapters Count | Total Commands | Read | Tier 1 | Tier 2 | Redline | Out-of-Scope |
|---|---|---|---|---|---|---|---|---|
| Envelope 1 | `ZyxelSystemCmd` | 16 | 595 | 188 | 364 | 16 | 26 | 1 |
| Envelope 2 | `ZyxelNetworkCmd` | 7 | 383 | 90 | 289 | 0 | 4 | 0 |
| Envelope 3 | `ZyxelNetworkCmd` | 2 | 138 | 25 | 109 | 4 | 0 | 0 |
| Envelope 4 | `ZyxelObjectCmd` | 7 | 105 | 27 | 60 | 18 | 0 | 0 |
| Envelope 5 | `ZyxelNatCmd` | 6 | 83 | 9 | 61 | 12 | 1 | 0 |
| Envelope 6 | `ZyxelFirewallCmd` | 2 | 248 | 75 | 168 | 3 | 2 | 0 |
| Envelope 7 | `ZyxelSecurityCmd` | 11 | 641 | 204 | 431 | 2 | 4 | 0 |
| Envelope 8 | `ZyxelVpnCmd` | 3 | 237 | 32 | 200 | 1 | 4 | 0 |
| Envelope 9 | `ZyxelAuthCmd` | 9 | 470 | 96 | 368 | 1 | 5 | 0 |
| Envelope 10 | `ZyxelWlanCmd` | 10 | 311 | 50 | 255 | 5 | 1 | 0 |

## 4. Complete Command Inventory

| Chapter | Command | Envelope | Disposition | Owning Method |
|---|---|---|---|---|
| Ch 1 (Command Line Interfa) | `CLI Syntax & Keystroke Conventions (Help ?, Tab, Prompt levels)` | Envelope 1 | `out-of-scope` | `ZyxelSystemCmd::cmdCliSyntaxKeystrokeConventions()` |
| Ch 2 (User and Privilege M) | `apply` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdApply()` |
| Ch 2 (User and Privilege M) | `atse` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAtse()` |
| Ch 2 (User and Privilege M) | `clear` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdClear()` |
| Ch 2 (User and Privilege M) | `configure` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdConfigure()` |
| Ch 2 (User and Privilege M) | `copy` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCopy()` |
| Ch 2 (User and Privilege M) | `debug (*)` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDebug()` |
| Ch 2 (User and Privilege M) | `delete` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDelete()` |
| Ch 2 (User and Privilege M) | `details` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDetails()` |
| Ch 2 (User and Privilege M) | `diag` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDiag()` |
| Ch 2 (User and Privilege M) | `diag-info` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDiagInfo()` |
| Ch 2 (User and Privilege M) | `dir` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdDir()` |
| Ch 2 (User and Privilege M) | `disable` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDisable()` |
| Ch 2 (User and Privilege M) | `enable` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdEnable()` |
| Ch 2 (User and Privilege M) | `exit` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdExit()` |
| Ch 2 (User and Privilege M) | `interface` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdInterface()` |
| Ch 2 (User and Privilege M) | `no packet-trace` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNoPacketTrace()` |
| Ch 2 (User and Privilege M) | `nslookup` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdNslookup()` |
| Ch 2 (User and Privilege M) | `packet-trace` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdPacketTrace()` |
| Ch 2 (User and Privilege M) | `ping` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdPing()` |
| Ch 2 (User and Privilege M) | `ping6` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdPing6()` |
| Ch 2 (User and Privilege M) | `psm` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdPsm()` |
| Ch 2 (User and Privilege M) | `reboot` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdReboot()` |
| Ch 2 (User and Privilege M) | `release` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRelease()` |
| Ch 2 (User and Privilege M) | `rename` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRename()` |
| Ch 2 (User and Privilege M) | `renew` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRenew()` |
| Ch 2 (User and Privilege M) | `run` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRun()` |
| Ch 2 (User and Privilege M) | `set session-key {ah <256..4095> auth_key \| esp <256..4095> [cipher enc_key] authenticator auth_key} 285 setenv` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSetSessionKey285()` |
| Ch 2 (User and Privilege M) | `show` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdShow()` |
| Ch 2 (User and Privilege M) | `shutdown` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdShutdown()` |
| Ch 2 (User and Privilege M) | `telnet` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdTelnet()` |
| Ch 2 (User and Privilege M) | `test aaa` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdTestAaa()` |
| Ch 2 (User and Privilege M) | `traceroute` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdTraceroute()` |
| Ch 2 (User and Privilege M) | `traceroute6` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdTraceroute6()` |
| Ch 2 (User and Privilege M) | `write` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdWrite()` |
| Ch 3 (Object Reference) | `show reference object aaa authentication [default \| auth_method]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectAaa()` |
| Ch 3 (Object Reference) | `show reference object account pppoe [object_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectAccount()` |
| Ch 3 (Object Reference) | `show reference object account pptp [object_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectAccount()` |
| Ch 3 (Object Reference) | `show reference object address [object_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectAddress()` |
| Ch 3 (Object Reference) | `show reference object address6 [object_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectAddress6()` |
| Ch 3 (Object Reference) | `show reference object app-patrol [profile-name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectApp()` |
| Ch 3 (Object Reference) | `show reference object ca category {local\|remote} [cert_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectCa()` |
| Ch 3 (Object Reference) | `show reference object crypto map [crypto_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectCrypto()` |
| Ch 3 (Object Reference) | `show reference object dhcp6-lease-object [object_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectDhcp6()` |
| Ch 3 (Object Reference) | `show reference object dhcp6-request-object [object_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectDhcp6()` |
| Ch 3 (Object Reference) | `show reference object interface [interface_name \| virtual_interface_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectInterface()` |
| Ch 3 (Object Reference) | `show reference object isakmp policy [isakmp_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectIsakmp()` |
| Ch 3 (Object Reference) | `show reference object schedule [object_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectSchedule()` |
| Ch 3 (Object Reference) | `show reference object service [object_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectService()` |
| Ch 3 (Object Reference) | `show reference object sslvpn application [object_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectSslvpn()` |
| Ch 3 (Object Reference) | `show reference object sslvpn policy [object_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectSslvpn()` |
| Ch 3 (Object Reference) | `show reference object username [username]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectUsername()` |
| Ch 3 (Object Reference) | `show reference object zone [object_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectZone()` |
| Ch 3 (Object Reference) | `show reference object-group aaa ad [group_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectGroup()` |
| Ch 3 (Object Reference) | `show reference object-group aaa ldap [group_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectGroup()` |
| Ch 3 (Object Reference) | `show reference object-group aaa radius [group_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectGroup()` |
| Ch 3 (Object Reference) | `show reference object-group address [object_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectGroup()` |
| Ch 3 (Object Reference) | `show reference object-group address6 [object_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectGroup()` |
| Ch 3 (Object Reference) | `show reference object-group interface [object_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectGroup()` |
| Ch 3 (Object Reference) | `show reference object-group service [object_name]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectGroup()` |
| Ch 3 (Object Reference) | `show reference object-group username [username]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReferenceObjectGroup()` |
| Ch 4 (Status) | `[no] cpu-temperature-monitor activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCpuTemperatureMonitorActivate()` |
| Ch 4 (Status) | `content-filter dashboard statistics flush` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdContentFilterDashboardStatistics()` |
| Ch 4 (Status) | `cpu-temperature-monitor period minutes` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCpuTemperatureMonitorPeriod()` |
| Ch 4 (Status) | `cpu-temperature-monitor unit {celsius\| fahrenheit}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCpuTemperatureMonitorUnit()` |
| Ch 4 (Status) | `show anti-botnet dashboard statistics summary` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowAntiBotnetDashboard()` |
| Ch 4 (Status) | `show anti-spam dashboard statistics summary` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowAntiSpamDashboard()` |
| Ch 4 (Status) | `show anti-virus statistics summary` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowAntiVirusStatistics()` |
| Ch 4 (Status) | `show ap-info top number {sta \| usage} timer` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowApInfoTop()` |
| Ch 4 (Status) | `show ap-info total {sta \| usage} {24G \| 5G \| 6G\| all} timer` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowApInfoTotal()` |
| Ch 4 (Status) | `show ap-info {mac_address \| all} {sta \| usage} {24G \| 5G \| 6G\| all} timer` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowApInfoTimer()` |
| Ch 4 (Status) | `show boot status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowBootStatus()` |
| Ch 4 (Status) | `show comport status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowComportStatus()` |
| Ch 4 (Status) | `show content-filter dashboard statistics summary` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowContentFilterDashboard()` |
| Ch 4 (Status) | `show cpu all` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowCpuAll()` |
| Ch 4 (Status) | `show cpu status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowCpuStatus()` |
| Ch 4 (Status) | `show cpu-temperature-monitor status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowCpuTemperatureMonitor()` |
| Ch 4 (Status) | `show disk` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowDisk()` |
| Ch 4 (Status) | `show extension-slot` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowExtensionSlot()` |
| Ch 4 (Status) | `show idp dashboard statistics summary` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowIdpDashboardStatistics()` |
| Ch 4 (Status) | `show ip-reputation dashboard statistics summary` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowIpReputationDashboard()` |
| Ch 4 (Status) | `show led status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowLedStatus()` |
| Ch 4 (Status) | `show mac` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowMac()` |
| Ch 4 (Status) | `show mem status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowMemStatus()` |
| Ch 4 (Status) | `show ram-size` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowRamSize()` |
| Ch 4 (Status) | `show sandbox dashboard statistics summary` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSandboxDashboardStatistics()` |
| Ch 4 (Status) | `show security-service status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSecurityServiceStatus()` |
| Ch 4 (Status) | `show serial-number` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSerialNumber()` |
| Ch 4 (Status) | `show socket listen` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSocketListen()` |
| Ch 4 (Status) | `show socket open` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSocketOpen()` |
| Ch 4 (Status) | `show sta-info top number usage timer` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowStaInfoTop()` |
| Ch 4 (Status) | `show sta-info total usage timer` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowStaInfoTotal()` |
| Ch 4 (Status) | `show sta-info {mac_address \| all} usage timer` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowStaInfoUsage()` |
| Ch 4 (Status) | `show system protection signature update status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSystemProtectionSignature()` |
| Ch 4 (Status) | `show system protection signatures version` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSystemProtectionSignatures()` |
| Ch 4 (Status) | `show system uptime` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSystemUptime()` |
| Ch 4 (Status) | `show version` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowVersion()` |
| Ch 4 (Status) | `system protection signature update signature` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSystemProtectionSignatureUpdate()` |
| Ch 4 (Status) | `threat-website dashboard statistics flush` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdThreatWebsiteDashboardStatistics()` |
| Ch 5 (Registration) | `[no] security-service update-server activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSecurityServiceUpdateServer()` |
| Ch 5 (Registration) | `security-service update-server server-url <url>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSecurityServiceUpdateServer()` |
| Ch 5 (Registration) | `service-register _setremind {after-10-days \| after-180-days \| after-30-days \| every-time \| nev- er}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdServiceRegisterSetremind()` |
| Ch 5 (Registration) | `service-register checkexpire` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdServiceRegisterCheckexpire()` |
| Ch 5 (Registration) | `show device-register status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowDeviceRegisterStatus()` |
| Ch 5 (Registration) | `show security-service update-server` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSecurityServiceUpdate()` |
| Ch 5 (Registration) | `show service-register content-filter-engine` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowServiceRegisterContent()` |
| Ch 5 (Registration) | `show service-register status content-filter {commtouch}` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowServiceRegisterStatus()` |
| Ch 5 (Registration) | `show service-register status sslvpn-status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowServiceRegisterStatus()` |
| Ch 5 (Registration) | `show service-register status {all \| application-security \| as \| av \| cdr \| concurrent-device- upgrade \| content-filter \| firmware-upgrade \| geo-ip \| idp \| malware-blocker \| ctdb \| managed-ap-service \| pkg \| reputation-filter \| sandbox \| secu-reporter \| secure-wifi \| sslvpn \| sslvpn-status \| web-security \| zymesh\| network-premium}` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowServiceRegisterStatus()` |
| Ch 6 (AP Management) | `[no] capwap activate` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapActivate()` |
| Ch 6 (AP Management) | `[no] load-balancing <group1 \| group2> group_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLoadBalancingGroupName()` |
| Ch 6 (AP Management) | `[no] override-full-power activate` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdOverrideFullPowerActivate()` |
| Ch 6 (AP Management) | `[no] vlan_interface` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdVlanInterface()` |
| Ch 6 (AP Management) | `ap internal-auth no shared-secret` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdApInternalAuthNo()` |
| Ch 6 (AP Management) | `appear at the same level. ap internal-auth shared-secret key` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdAppearAtTheSame()` |
| Ch 6 (AP Management) | `capwap ap <mac address> [no] airtime-fairness activate` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApAirtimeFairness()` |
| Ch 6 (AP Management) | `capwap ap ac-ip {primary_ac_ip\|primary_ac_dns} {secondary_ac_ip\|secondary_ac_dns} 605 capwap ap ac-ip auto` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApAcIp()` |
| Ch 6 (AP Management) | `capwap ap ac-ip {primary_ac_ip} {secondary_ac_ip}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApAcIp()` |
| Ch 6 (AP Management) | `capwap ap add ap_mac [ap_model]` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApAddAp()` |
| Ch 6 (AP Management) | `capwap ap ap_mac` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApApMac()` |
| Ch 6 (AP Management) | `capwap ap ap_mac` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApApMac()` |
| Ch 6 (AP Management) | `capwap ap factory default ap_mac` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApFactoryDefault()` |
| Ch 6 (AP Management) | `capwap ap fallback disable` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApFallbackDisable()` |
| Ch 6 (AP Management) | `capwap ap fallback enable` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApFallbackEnable()` |
| Ch 6 (AP Management) | `capwap ap fallback interval <30..86400>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApFallbackInterval()` |
| Ch 6 (AP Management) | `capwap ap idle timeout {25–100}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApIdleTimeout()` |
| Ch 6 (AP Management) | `capwap ap kick {all \| ap_mac}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApKick()` |
| Ch 6 (AP Management) | `capwap ap led-off ap_mac` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApLedOff()` |
| Ch 6 (AP Management) | `capwap ap led-on ap_mac` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApLedOn()` |
| Ch 6 (AP Management) | `capwap ap reboot ap_mac` | Envelope 10 | `redline` | `ZyxelWlanCmd::cmdCapwapApRebootAp()` |
| Ch 6 (AP Management) | `capwap manual-add {enable \| disable}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapManualAdd()` |
| Ch 6 (AP Management) | `capwap station kick sta_mac` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapStationKickSta()` |
| Ch 6 (AP Management) | `country-code country_code` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCountryCodeCountryCode()` |
| Ch 6 (AP Management) | `lan-provision ap ap_mac` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLanProvisionApAp()` |
| Ch 6 (AP Management) | `lan-provision lan_port {activate \| inactivate} pvid <1..4094>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLanProvisionLanPort()` |
| Ch 6 (AP Management) | `lan-provision vlan_interface {activate \| inactivate} vid <1..4094> join lan_port {tag \| untag} [lan_port {tag \| un- tag}] [lan_port {tag \| untag}]` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLanProvisionVlanInterface()` |
| Ch 6 (AP Management) | `lan_port {activate \| inactivate} pvid <1..4094> .......................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLanPortPvid()` |
| Ch 6 (AP Management) | `no rap slot_name ssid-profile <1..6>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdNoRapSlotName()` |
| Ch 6 (AP Management) | `rap slot_name output-power wlan_power` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRapSlotNameOutput()` |
| Ch 6 (AP Management) | `rap slot_name ssid-profile <1..6> ssid_profile_name [tunlif interface] vid vlan_id` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRapSlotNameSsid()` |
| Ch 6 (AP Management) | `show capwap ap ac-ip` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapApAc()` |
| Ch 6 (AP Management) | `show capwap ap all statistics` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapApAll()` |
| Ch 6 (AP Management) | `show capwap ap ap_mac slot_name detail` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapApAp()` |
| Ch 6 (AP Management) | `show capwap ap fallback` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapApFallback()` |
| Ch 6 (AP Management) | `show capwap ap fallback interval` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapApFallback()` |
| Ch 6 (AP Management) | `show capwap ap idle timeout` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapApIdle()` |
| Ch 6 (AP Management) | `show capwap ap wait-list` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapApWait()` |
| Ch 6 (AP Management) | `show capwap ap {all \| ap_mac}` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapAp()` |
| Ch 6 (AP Management) | `show capwap ap {all \| ap_mac}` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapAp()` |
| Ch 6 (AP Management) | `show capwap ap {all \| ap_mac} config` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapApConfig()` |
| Ch 6 (AP Management) | `show capwap ap {all \| ap_mac} config status` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapApConfig()` |
| Ch 6 (AP Management) | `show capwap manual-add` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapManualAdd()` |
| Ch 6 (AP Management) | `show capwap station all` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapStationAll()` |
| Ch 6 (AP Management) | `show country-code list` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCountryCodeList()` |
| Ch 6 (AP Management) | `show default country-code` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowDefaultCountryCode()` |
| Ch 6 (AP Management) | `show lan-provision ap ap_mac interface {lan_port \| vlan_interface \| all\| ethernet \| uplink \| vlan}` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowLanProvisionAp()` |
| Ch 6 (AP Management) | `show sa monitor [ap-description desc] rap` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowSaMonitorAp()` |
| Ch 6 (AP Management) | `show vpn-policy-pool` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowVpnPolicyPool()` |
| Ch 6 (AP Management) | `status` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdStatus()` |
| Ch 6 (AP Management) | `vlan_interface {activate \| inactivate} vid <1..4094> join lan_port {tag \| untag} [lan_port {tag \| untag}] [lan_port {tag \| untag}]` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdVlanInterfaceVidJoin()` |
| Ch 6 (AP Management) | `vpn-policy-pool start start_ip end end_ip` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdVpnPolicyPoolStart()` |
| Ch 7 (Built-in AP ........) | `[no] ap-mode detection activate` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdApModeDetectionActivate()` |
| Ch 7 (Built-in AP ........) | `[no] slot_name ap-profile radio_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSlotNameApProfile()` |
| Ch 7 (Built-in AP ........) | `[no] slot_name monitor-profile monitor_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSlotNameMonitorProfile()` |
| Ch 7 (Built-in AP ........) | `[no] slot_name output-power wlan_power` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSlotNameOutputPower()` |
| Ch 7 (Built-in AP ........) | `[no] slot_name ssid-profile <1..8> ssid_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSlotNameSsidProfile()` |
| Ch 7 (Built-in AP ........) | `[no] slot_name zymesh-profile zymesh_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSlotNameZymeshProfile()` |
| Ch 7 (Built-in AP ........) | `ap-group-profile ap-group-profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdApGroupProfileAp()` |
| Ch 7 (Built-in AP ........) | `capwap ap local-ap` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApLocalAp()` |
| Ch 7 (Built-in AP ........) | `exit` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdExit()` |
| Ch 7 (Built-in AP ........) | `location location` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLocationLocation()` |
| Ch 7 (Built-in AP ........) | `sysname system_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSysnameSystemName()` |
| Ch 8 (AP Group) | `[no] ap-group-profile ap_group_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdApGroupProfileAp()` |
| Ch 8 (AP Group) | `[no] force vlan` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdForceVlan()` |
| Ch 8 (AP Group) | `[no] lan-provision model {nwa5301-nj \| wac6502d-e \| wac6502d-s \| wac6503d-s \|` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLanProvisionModelNwa5301()` |
| Ch 8 (AP Group) | `[no] lan-provision model {nwa5301-nj \| wac6502d-e \| wac6502d-s \| wac6503d-s \|` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLanProvisionModelNwa5301()` |
| Ch 8 (AP Group) | `[no] lan-provision model {nwa5301-nj \| wac6502d-e \| wac6502d-s \| wac6503d-s \|` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLanProvisionModelNwa5301()` |
| Ch 8 (AP Group) | `[no] lan-provision model {nwa5301-nj \| wac6502d-e \| wac6502d-s \| wac6503d-s \|` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLanProvisionModelNwa5301()` |
| Ch 8 (AP Group) | `[no] load-balancing [slot1 \| slot2] activate` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLoadBalancingSlot1Slot2()` |
| Ch 8 (AP Group) | `[no] load-balancing [slot1 \| slot2] kickout` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLoadBalancingSlot1Slot2()` |
| Ch 8 (AP Group) | `[no] slot_name ap-profile radio_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSlotNameApProfile()` |
| Ch 8 (AP Group) | `[no] slot_name monitor-profile monitor_profile_nameliInterval` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSlotNameMonitorProfile()` |
| Ch 8 (AP Group) | `[no] slot_name output-power wlan_power` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSlotNameOutputPower()` |
| Ch 8 (AP Group) | `[no] slot_name repeater-ap radio_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSlotNameRepeaterAp()` |
| Ch 8 (AP Group) | `[no] slot_name root-ap radio_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSlotNameRootAp()` |
| Ch 8 (AP Group) | `[no] slot_name ssid-profile <1..8> ssid_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSlotNameSsidProfile()` |
| Ch 8 (AP Group) | `[no] slot_name zymesh-profile zymesh_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSlotNameZymeshProfile()` |
| Ch 8 (AP Group) | `ap-group first-priority ap_group_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdApGroupFirstPriority()` |
| Ch 8 (AP Group) | `ap-group flush wtp-setting ap_group_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdApGroupFlushWtp()` |
| Ch 8 (AP Group) | `ap-group-member ap_group_profile_name [no] member mac_address` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdApGroupMemberAp()` |
| Ch 8 (AP Group) | `ap-group-member ap_group_wlan_name[no] member local-ap` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdApGroupMemberAp()` |
| Ch 8 (AP Group) | `ap-group-profile rename ap_group_profile_name1 ap_group_profile_name2` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdApGroupProfileRename()` |
| Ch 8 (AP Group) | `description description` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDescriptionDescription()` |
| Ch 8 (AP Group) | `exit` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdExit()` |
| Ch 8 (AP Group) | `load-balancing [slot1 \| slot2] alpha <1..255>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLoadBalancingSlot1Slot2()` |
| Ch 8 (AP Group) | `load-balancing [slot1 \| slot2] beta <1..255>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLoadBalancingSlot1Slot2()` |
| Ch 8 (AP Group) | `load-balancing [slot1 \| slot2] kickInterval <1..255> ..........................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLoadBalancingSlot1Slot2()` |
| Ch 8 (AP Group) | `load-balancing [slot1 \| slot2] liInterval <1..255>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLoadBalancingSlot1Slot2()` |
| Ch 8 (AP Group) | `load-balancing [slot1 \| slot2] max sta <1..127>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLoadBalancingSlot1Slot2()` |
| Ch 8 (AP Group) | `load-balancing [slot1 \| slot2] sigma <51..100>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLoadBalancingSlot1Slot2()` |
| Ch 8 (AP Group) | `load-balancing [slot1 \| slot2] timeout <1..255>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLoadBalancingSlot1Slot2()` |
| Ch 8 (AP Group) | `load-balancing [slot1 \| slot2] traffic level {high \| low \| medium}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLoadBalancingSlot1Slot2()` |
| Ch 8 (AP Group) | `load-balancing mode [slot1 \| slot2] {station \| traffic \| smart-classroom} .....................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLoadBalancingModeSlot1()` |
| Ch 8 (AP Group) | `show ap-group first-priority` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowApGroupFirst()` |
| Ch 8 (AP Group) | `show ap-group-profile ap_group_profile_name lan-provision interface {all \| vlan \| ethernet \| ap_lan_port \| vlan_interface} model {nwa5301-nj \| wac6502d-e \| wac6502d-s \| wac6503d-s \| wac6553d-e}` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowApGroupProfile()` |
| Ch 8 (AP Group) | `show ap-group-profile ap_group_profile_name lan-provision model` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowApGroupProfile()` |
| Ch 8 (AP Group) | `show ap-group-profile ap_group_profile_name load-balancing config` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowApGroupProfile()` |
| Ch 8 (AP Group) | `show ap-group-profile rule_count` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowApGroupProfile()` |
| Ch 8 (AP Group) | `show ap-group-profile {all \| ap_group_profile_name}` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowApGroupProfile()` |
| Ch 8 (AP Group) | `vlan <1..4094> {tag \| untag} ..................................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdVlan()` |
| Ch 8 (AP Group) | `wac6553d-e} ap_lan_port activate pvid <1..4094>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWac6553dEApLan()` |
| Ch 8 (AP Group) | `wac6553d-e} ap_lan_port inactivate pvid <1..4094>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWac6553dEApLan()` |
| Ch 9 (Wireless LAN Profile) | `2g-channel wireless_channel_2g` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmd2gChannelWirelessChannel()` |
| Ch 9 (Wireless LAN Profile) | `2g-multicast-speed wlan_2g_support_speed` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmd2gMulticastSpeedWlan()` |
| Ch 9 (Wireless LAN Profile) | `2g-wlan-rate-control rate_2g` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmd2gWlanRateControl()` |
| Ch 9 (Wireless LAN Profile) | `5g-basic-speed speed` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmd5gBasicSpeedSpeed()` |
| Ch 9 (Wireless LAN Profile) | `5g-channel wireless_channel_5g` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmd5gChannelWirelessChannel()` |
| Ch 9 (Wireless LAN Profile) | `5g-multicast-speed wlan_5g_basic_speed` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmd5gMulticastSpeedWlan()` |
| Ch 9 (Wireless LAN Profile) | `5g-wlan-rate-control rate_5g` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmd5gWlanRateControl()` |
| Ch 9 (Wireless LAN Profile) | `6g-channel wireless_channel_6g` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmd6gChannelWirelessChannel()` |
| Ch 9 (Wireless LAN Profile) | `6g-multicast-speed wlan_6g_basic_speed` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmd6gMulticastSpeedWlan()` |
| Ch 9 (Wireless LAN Profile) | `6g-wlan-rate-control rate_6g` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmd6gWlanRateControl()` |
| Ch 9 (Wireless LAN Profile) | `[no] 2g-scan-channel wireless_channel_2g` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmd2gScanChannelWireless()` |
| Ch 9 (Wireless LAN Profile) | `[no] 5g-scan-channel wireless_channel_5g` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmd5gScanChannelWireless()` |
| Ch 9 (Wireless LAN Profile) | `[no] activate` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdActivate()` |
| Ch 9 (Wireless LAN Profile) | `[no] activate` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdActivate()` |
| Ch 9 (Wireless LAN Profile) | `[no] ampdu` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdAmpdu()` |
| Ch 9 (Wireless LAN Profile) | `[no] amsdu` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdAmsdu()` |
| Ch 9 (Wireless LAN Profile) | `[no] block-ack` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdBlockAck()` |
| Ch 9 (Wireless LAN Profile) | `[no] broadcast` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdBroadcast()` |
| Ch 9 (Wireless LAN Profile) | `[no] broadcast` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdBroadcast()` |
| Ch 9 (Wireless LAN Profile) | `[no] ctsrts <0..2347>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCtsrts()` |
| Ch 9 (Wireless LAN Profile) | `[no] dcs activate` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDcsActivate()` |
| Ch 9 (Wireless LAN Profile) | `[no] disable-bss-color` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDisableBssColor()` |
| Ch 9 (Wireless LAN Profile) | `[no] disable-dfs-switch` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDisableDfsSwitch()` |
| Ch 9 (Wireless LAN Profile) | `[no] dot11n-disable-coexistence` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDot11nDisableCoexistence()` |
| Ch 9 (Wireless LAN Profile) | `[no] force-mu-mimo` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdForceMuMimo()` |
| Ch 9 (Wireless LAN Profile) | `[no] frag <256..2346>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdFrag()` |
| Ch 9 (Wireless LAN Profile) | `[no] htprotect` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdHtprotect()` |
| Ch 9 (Wireless LAN Profile) | `[no] ignore-country-ie` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdIgnoreCountryIe()` |
| Ch 9 (Wireless LAN Profile) | `[no] multicast ................................................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdMulticast()` |
| Ch 9 (Wireless LAN Profile) | `[no] multicast ................................................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdMulticast()` |
| Ch 9 (Wireless LAN Profile) | `[no] multicast-to-unicast` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdMulticastToUnicast()` |
| Ch 9 (Wireless LAN Profile) | `[no] nol-channel-block` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdNolChannelBlock()` |
| Ch 9 (Wireless LAN Profile) | `[no] reject-legacy-station` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRejectLegacyStation()` |
| Ch 9 (Wireless LAN Profile) | `[no] rssi-retry` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRssiRetry()` |
| Ch 9 (Wireless LAN Profile) | `[no] rssi-thres ...............................................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRssiThres()` |
| Ch 9 (Wireless LAN Profile) | `[no] ssid-profile wlan_interface_index ssid_profile` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSsidProfileWlanInterface()` |
| Ch 9 (Wireless LAN Profile) | `[no] suppress-retry-rts` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSuppressRetryRts()` |
| Ch 9 (Wireless LAN Profile) | `[no] transition-mode` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdTransitionMode()` |
| Ch 9 (Wireless LAN Profile) | `[no] wlan-macfilter-profile macfilter_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWlanMacfilterProfileMacfilter()` |
| Ch 9 (Wireless LAN Profile) | `[no] wlan-monitor-profile monitor_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWlanMonitorProfileMonitor()` |
| Ch 9 (Wireless LAN Profile) | `[no] wlan-radio-profile radio_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWlanRadioProfileRadio()` |
| Ch 9 (Wireless LAN Profile) | `[no] wlan-security-profile security_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWlanSecurityProfileSecurity()` |
| Ch 9 (Wireless LAN Profile) | `[no] wlan-ssid-profile ssid_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWlanSsidProfileSsid()` |
| Ch 9 (Wireless LAN Profile) | `[no] zero-wait-dfs ............................................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdZeroWaitDfs()` |
| Ch 9 (Wireless LAN Profile) | `[no] zymesh-profile zymesh_profile_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdZymeshProfileZymeshProfile()` |
| Ch 9 (Wireless LAN Profile) | `_two-factor-auth-send email email user username verification-code verification_code 445 tx-mask chain_mask` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdTwoFactorAuthSend()` |
| Ch 9 (Wireless LAN Profile) | `band {2.4G \|5G\| 6G} band-mode` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdBandBandMode()` |
| Ch 9 (Wireless LAN Profile) | `beacon-interval <40..1000>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdBeaconInterval()` |
| Ch 9 (Wireless LAN Profile) | `broadcast pps <1~10000>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdBroadcastPps()` |
| Ch 9 (Wireless LAN Profile) | `broadcast pps <1~10000>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdBroadcastPps()` |
| Ch 9 (Wireless LAN Profile) | `bss-color <0~63>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdBssColor()` |
| Ch 9 (Wireless LAN Profile) | `ch-width wlan_htcw` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdChWidthWlanHtcw()` |
| Ch 9 (Wireless LAN Profile) | `country-code country_code` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCountryCodeCountryCode()` |
| Ch 9 (Wireless LAN Profile) | `country-code country_code` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCountryCodeCountryCode()` |
| Ch 9 (Wireless LAN Profile) | `dcs 2g-selected-channel 2.4g_channels` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDcs2gSelectedChannel()` |
| Ch 9 (Wireless LAN Profile) | `dcs 5g-selected-channel 5g_channels` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDcs5gSelectedChannel()` |
| Ch 9 (Wireless LAN Profile) | `dcs 6g-selected-channel 6g_channels` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDcs6gSelectedChannel()` |
| Ch 9 (Wireless LAN Profile) | `dcs channel-deployment {3-channel\|4-channel}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDcsChannelDeployment()` |
| Ch 9 (Wireless LAN Profile) | `dcs client-aware {enable\|disable} .............................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDcsClientAware()` |
| Ch 9 (Wireless LAN Profile) | `dcs dcs-2g-method {auto\|manual}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDcsDcs2gMethod()` |
| Ch 9 (Wireless LAN Profile) | `dcs dcs-5g-method {auto\|manual}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDcsDcs5gMethod()` |
| Ch 9 (Wireless LAN Profile) | `dcs dcs-6g-method {auto\|manual}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDcsDcs6gMethod()` |
| Ch 9 (Wireless LAN Profile) | `dcs dfs-aware {enable\|disable}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDcsDfsAware()` |
| Ch 9 (Wireless LAN Profile) | `dcs dfs-aware {enable\|disable}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDcsDfsAware()` |
| Ch 9 (Wireless LAN Profile) | `dcs mode {interval\|schedule} ..................................................................................................` | Envelope 10 | `tier2` | `ZyxelWlanCmd::cmdDcsMode()` |
| Ch 9 (Wireless LAN Profile) | `dcs schedule <hh:mm> {mon\|tue\|wed\|thu\|fri\|sat\|sun} ............................................................................` | Envelope 10 | `tier2` | `ZyxelWlanCmd::cmdDcsSchedule()` |
| Ch 9 (Wireless LAN Profile) | `dcs sensitivity-level {high\|medium \|low}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDcsSensitivityLevel()` |
| Ch 9 (Wireless LAN Profile) | `dcs time-interval interval` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDcsTimeIntervalInterval()` |
| Ch 9 (Wireless LAN Profile) | `description description` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDescriptionDescription()` |
| Ch 9 (Wireless LAN Profile) | `dot11-preamble {long\|short}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDot11Preamble()` |
| Ch 9 (Wireless LAN Profile) | `dtim-period <1..255> ..........................................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDtimPeriod()` |
| Ch 9 (Wireless LAN Profile) | `exit` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdExit()` |
| Ch 9 (Wireless LAN Profile) | `exit` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdExit()` |
| Ch 9 (Wireless LAN Profile) | `exit` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdExit()` |
| Ch 9 (Wireless LAN Profile) | `exit` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdExit()` |
| Ch 9 (Wireless LAN Profile) | `exit` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdExit()` |
| Ch 9 (Wireless LAN Profile) | `exit` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdExit()` |
| Ch 9 (Wireless LAN Profile) | `exit` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdExit()` |
| Ch 9 (Wireless LAN Profile) | `exit` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdExit()` |
| Ch 9 (Wireless LAN Profile) | `guard-interval wlan_htgi` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdGuardIntervalWlanHtgi()` |
| Ch 9 (Wireless LAN Profile) | `limit-ampdu < 100..65535> .................................................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLimitAmpdu()` |
| Ch 9 (Wireless LAN Profile) | `limit-amsdu <2290..4096>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdLimitAmsdu()` |
| Ch 9 (Wireless LAN Profile) | `max-sw-retries <0..10>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdMaxSwRetries()` |
| Ch 9 (Wireless LAN Profile) | `multicast pps <1~10000> .......................................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdMulticastPps()` |
| Ch 9 (Wireless LAN Profile) | `multicast pps <1~10000> .......................................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdMulticastPps()` |
| Ch 9 (Wireless LAN Profile) | `no server-auth <1..2> .........................................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdNoServerAuth()` |
| Ch 9 (Wireless LAN Profile) | `no storm-control ethernet ap mac_address` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdNoStormControlEthernet()` |
| Ch 9 (Wireless LAN Profile) | `no storm-control wireless ap mac_address` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdNoStormControlWireless()` |
| Ch 9 (Wireless LAN Profile) | `output-power wlan_power` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdOutputPowerWlanPower()` |
| Ch 9 (Wireless LAN Profile) | `pn-check-thres <0..100> .......................................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdPnCheckThres()` |
| Ch 9 (Wireless LAN Profile) | `psk psk` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdPskPsk()` |
| Ch 9 (Wireless LAN Profile) | `role wlan_role` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRoleWlanRole()` |
| Ch 9 (Wireless LAN Profile) | `rssi-dbm <-20~-76> ............................................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRssiDbm()` |
| Ch 9 (Wireless LAN Profile) | `rssi-interval (1..86400> ......................................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRssiInterval186400()` |
| Ch 9 (Wireless LAN Profile) | `rssi-kickout <-20~-105> .......................................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRssiKickout()` |
| Ch 9 (Wireless LAN Profile) | `rssi-optype <0-3> .............................................................................................................` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRssiOptype()` |
| Ch 9 (Wireless LAN Profile) | `rssi-privilegetime` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRssiPrivilegetime()` |
| Ch 9 (Wireless LAN Profile) | `rssi-retrycount <1~100>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRssiRetrycount()` |
| Ch 9 (Wireless LAN Profile) | `rssi-verifytime` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRssiVerifytime()` |
| Ch 9 (Wireless LAN Profile) | `rx-mask chain_mask` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRxMaskChainMask()` |
| Ch 9 (Wireless LAN Profile) | `scan-dwell <100..1000>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdScanDwell()` |
| Ch 9 (Wireless LAN Profile) | `scan-method scan_method` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdScanMethodScanMethod()` |
| Ch 9 (Wireless LAN Profile) | `schedule schedule_object` | Envelope 10 | `tier2` | `ZyxelWlanCmd::cmdScheduleScheduleObject()` |
| Ch 9 (Wireless LAN Profile) | `show storm-control ethernet ap mac_address` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowStormControlEthernet()` |
| Ch 9 (Wireless LAN Profile) | `show wlan-macfilter-profile {all \| macfilter_profile_name}` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowWlanMacfilterProfile()` |
| Ch 9 (Wireless LAN Profile) | `show wlan-monitor-profile {all \| monitor_profile_name}` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowWlanMonitorProfile()` |
| Ch 9 (Wireless LAN Profile) | `show wlan-radio-profile {all \| radio_profile_name}` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowWlanRadioProfile()` |
| Ch 9 (Wireless LAN Profile) | `show wlan-security-profile {all \| security_profile_name}` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowWlanSecurityProfile()` |
| Ch 9 (Wireless LAN Profile) | `show wlan-ssid-profile {all \| ssid_profile_name}` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowWlanSsidProfile()` |
| Ch 9 (Wireless LAN Profile) | `show zymesh ap info` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowZymeshApInfo()` |
| Ch 9 (Wireless LAN Profile) | `show zymesh link info {repeater-ap \| root-ap}` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowZymeshLinkInfo()` |
| Ch 9 (Wireless LAN Profile) | `show zymesh provision-group` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowZymeshProvisionGroup()` |
| Ch 9 (Wireless LAN Profile) | `show zymesh-profile {all \| zymesh_profile_name}` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowZymeshProfile()` |
| Ch 9 (Wireless LAN Profile) | `ssid ssid` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSsidSsid()` |
| Ch 9 (Wireless LAN Profile) | `storm-control ethernet ap mac_address` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdStormControlEthernetAp()` |
| Ch 9 (Wireless LAN Profile) | `storm-control wireless ap mac_address` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdStormControlWirelessAp()` |
| Ch 9 (Wireless LAN Profile) | `subframe-ampdu <2..64>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSubframeAmpdu()` |
| Ch 9 (Wireless LAN Profile) | `subframe-ampdu <2..64>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSubframeAmpdu()` |
| Ch 9 (Wireless LAN Profile) | `wlan-macfilter-profile rename macfilter_profile_name1 macfilter_profile_name2` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWlanMacfilterProfileRename()` |
| Ch 9 (Wireless LAN Profile) | `wlan-monitor-profile rename monitor_profile_name1 monitor_profile_name2` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWlanMonitorProfileRename()` |
| Ch 9 (Wireless LAN Profile) | `wlan-radio-profile rename radio_profile_name1 radio_profile_name2` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWlanRadioProfileRename()` |
| Ch 9 (Wireless LAN Profile) | `wlan-security-profile rename security_profile_name1 security_profile_name2` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWlanSecurityProfileRename()` |
| Ch 9 (Wireless LAN Profile) | `wlan-ssid-profile rename ssid_profile_name1 ssid_profile_name2` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWlanSsidProfileRename()` |
| Ch 9 (Wireless LAN Profile) | `zymesh provision-group ac_mac` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdZymeshProvisionGroupAc()` |
| Ch 9 (Wireless LAN Profile) | `zymesh-profile rename zymesh_profile_name1 zymesh_profile_name2` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdZymeshProfileRenameZymesh()` |
| Ch 9 (Wireless LAN Profile) | `{bg \| bgn \| a \| ac \| an \| bgnax \| anacax\| ax}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdGeneric()` |
| Ch 9 (Wireless LAN Profile) | `\| uint32 <0..4294967295> \| ip ipv4 [ipv4 [ipv4]] \| fqdn fqdn [fqdn [fqdn]] \| text text \| hex hex \| vivc enter- prise_id hex_s [enterprise_id hex_s] \| vivs enterprise_id hex_s [enterprise_id hex_s] 129 2g-basic-speed speed` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdUint32IpIpv4Ipv4()` |
| Ch 10 (Rogue AP) | `[no] activate` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdActivate()` |
| Ch 10 (Rogue AP) | `[no] activate` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdActivate()` |
| Ch 10 (Rogue AP) | `[no] contain ap_mac` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdContainApMac()` |
| Ch 10 (Rogue AP) | `ap_mac` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdApMac()` |
| Ch 10 (Rogue AP) | `ap_mac` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdApMac()` |
| Ch 10 (Rogue AP) | `description2` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDescription2()` |
| Ch 10 (Rogue AP) | `exit` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdExit()` |
| Ch 10 (Rogue AP) | `exit` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdExit()` |
| Ch 10 (Rogue AP) | `friendly-ap ap_mac description2` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdFriendlyApApMac()` |
| Ch 10 (Rogue AP) | `monitoring flush` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdMonitoringFlush()` |
| Ch 10 (Rogue AP) | `no friendly-ap ap_mac` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdNoFriendlyApAp()` |
| Ch 10 (Rogue AP) | `no rogue-ap ap_mac` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdNoRogueApAp()` |
| Ch 10 (Rogue AP) | `rogue-ap ap_mac description2` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRogueApApMac()` |
| Ch 10 (Rogue AP) | `rogue-ap containment` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRogueApContainment()` |
| Ch 10 (Rogue AP) | `rogue-ap detection` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRogueApDetection()` |
| Ch 10 (Rogue AP) | `show rogue-ap containment config` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowRogueApContainment()` |
| Ch 10 (Rogue AP) | `show rogue-ap containment list` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowRogueApContainment()` |
| Ch 10 (Rogue AP) | `show rogue-ap detection info` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowRogueApDetection()` |
| Ch 10 (Rogue AP) | `show rogue-ap detection list {rogue \| friendly\| all}` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowRogueApDetection()` |
| Ch 10 (Rogue AP) | `show rogue-ap detection monitoring` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowRogueApDetection()` |
| Ch 10 (Rogue AP) | `show rogue-ap detection status` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowRogueApDetection()` |
| Ch 11 (Wireless Health) | `Router# show wireless-health-action` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRouterShowWirelessHealth()` |
| Ch 11 (Wireless Health) | `Router(config)# exit` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRouterConfigExit()` |
| Ch 11 (Wireless Health) | `Router(config)# wireless-health-action aggressiveness` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRouterConfigWirelessHealth()` |
| Ch 11 (Wireless Health) | `Router(config)# wireless-health-action aggressiveness low` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRouterConfigWirelessHealth()` |
| Ch 11 (Wireless Health) | `Router> configure terminal` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRouterConfigureTerminal()` |
| Ch 11 (Wireless Health) | `[no] wireless-health {activate \| radio \| sta}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWirelessHealth()` |
| Ch 11 (Wireless Health) | `[no] wlan-radio-profile radio profile name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWlanRadioProfileRadio()` |
| Ch 11 (Wireless Health) | `aggressiveness: low` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdAggressivenessLow()` |
| Ch 11 (Wireless Health) | `high low standard` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdHighLowStandard()` |
| Ch 11 (Wireless Health) | `radio-24g: none` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRadio24gNone()` |
| Ch 11 (Wireless Health) | `radio-5g: none` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRadio5gNone()` |
| Ch 11 (Wireless Health) | `show ap-info top 10 alert {2.4G \| 5G \| 6G\| all}` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowApInfoTop()` |
| Ch 11 (Wireless Health) | `show sta-info top 10 alert {2.4G \| 5G \| 6G\| all}` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowStaInfoTop()` |
| Ch 11 (Wireless Health) | `show wireless-health-action` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowWirelessHealthAction()` |
| Ch 11 (Wireless Health) | `station: none` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdStationNone()` |
| Ch 11 (Wireless Health) | `wireless-health radio action {dcs_now \| downgrade_cw \| none}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWirelessHealthRadioAction()` |
| Ch 11 (Wireless Health) | `wireless-health radio {action \| act-lock-time <1...1440> \| recovery-threshold <10...1000>\| act- threshold <10...1000>\| data-collect-interval <0...120>}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWirelessHealthRadio()` |
| Ch 11 (Wireless Health) | `wireless-health sta action {kick_sta \| none}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWirelessHealthStaAction()` |
| Ch 11 (Wireless Health) | `wireless-health sta {action \| act-lock-time <1...1440>\| act-threshold <10...1000>\| data-col- lect-interval <0...120>}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWirelessHealthSta()` |
| Ch 11 (Wireless Health) | `wireless-health-action aggressiveness {high \| standard \| low}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdWirelessHealthActionAggressiveness()` |
| Ch 12 (Wireless Frame Captu) | `[no] frame-capture activate` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdFrameCaptureActivate()` |
| Ch 12 (Wireless Frame Captu) | `exit` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdExit()` |
| Ch 12 (Wireless Frame Captu) | `file-prefix file_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdFilePrefixFileName()` |
| Ch 12 (Wireless Frame Captu) | `file_name` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdFileName()` |
| Ch 12 (Wireless Frame Captu) | `files-size mon_dir_size` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdFilesSizeMonDir()` |
| Ch 12 (Wireless Frame Captu) | `frame-capture configure` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdFrameCaptureConfigure()` |
| Ch 12 (Wireless Frame Captu) | `ip_address` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdIpAddress()` |
| Ch 12 (Wireless Frame Captu) | `mon_dir_size` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdMonDirSize()` |
| Ch 12 (Wireless Frame Captu) | `show capwap ap all lite2` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapApAll()` |
| Ch 12 (Wireless Frame Captu) | `show frame-capture config` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowFrameCaptureConfig()` |
| Ch 12 (Wireless Frame Captu) | `show frame-capture status` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowFrameCaptureStatus()` |
| Ch 12 (Wireless Frame Captu) | `src-ip {add\|del} {ipv4_address \| local}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdSrcIp()` |
| Ch 13 (Dynamic Channel Sele) | `dcs now {ap_mac \| profile_name}` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdDcsNow()` |
| Ch 14 (Auto-Healing) | `Router(config)#` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRouterConfig()` |
| Ch 14 (Auto-Healing) | `Router(config)# auto-healing activate` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRouterConfigAutoHealing()` |
| Ch 14 (Auto-Healing) | `Router(config)# auto-healing power-threshold -70` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRouterConfigAutoHealing()` |
| Ch 14 (Auto-Healing) | `Router(config)# show auto-healing config` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdRouterConfigShowAuto()` |
| Ch 14 (Auto-Healing) | `[no] auto-healing activate` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdAutoHealingActivate()` |
| Ch 14 (Auto-Healing) | `auto-healing activate: yes` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdAutoHealingActivateYes()` |
| Ch 14 (Auto-Healing) | `auto-healing healing threshold: -85 dBm` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdAutoHealingHealingThreshold()` |
| Ch 14 (Auto-Healing) | `auto-healing healing-interval interval` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdAutoHealingHealingInterval()` |
| Ch 14 (Auto-Healing) | `auto-healing healing-threshold` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdAutoHealingHealingThreshold()` |
| Ch 14 (Auto-Healing) | `auto-healing interval: 10` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdAutoHealingInterval10()` |
| Ch 14 (Auto-Healing) | `auto-healing margin` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdAutoHealingMargin()` |
| Ch 14 (Auto-Healing) | `auto-healing margin: 0` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdAutoHealingMargin0()` |
| Ch 14 (Auto-Healing) | `auto-healing power threshold: -70 dBm` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdAutoHealingPowerThreshold()` |
| Ch 14 (Auto-Healing) | `auto-healing power-threshold <-50~-80>` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdAutoHealingPowerThreshold()` |
| Ch 14 (Auto-Healing) | `auto-healing update` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdAutoHealingUpdate()` |
| Ch 14 (Auto-Healing) | `interval` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdInterval()` |
| Ch 14 (Auto-Healing) | `show auto-healing config` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowAutoHealingConfig()` |
| Ch 15 (LEDs ...............) | `led_locator ap_mac_address blink-timer <1..60>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLedLocatorApMac()` |
| Ch 15 (LEDs ...............) | `led_locator ap_mac_address off` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLedLocatorApMac()` |
| Ch 15 (LEDs ...............) | `led_locator ap_mac_address on` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLedLocatorApMac()` |
| Ch 15 (LEDs ...............) | `led_suppress ap_mac_address disable` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLedSuppressApMac()` |
| Ch 15 (LEDs ...............) | `led_suppress ap_mac_address enable` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLedSuppressApMac()` |
| Ch 15 (LEDs ...............) | `show led_locator ap_mac_address status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowLedLocatorAp()` |
| Ch 15 (LEDs ...............) | `show led_suppress ap_mac_address status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowLedSuppressAp()` |
| Ch 16 (Interfaces..........) | `(no) bootfile-name <filename>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdBootfileName()` |
| Ch 16 (Interfaces..........) | `(no) bootp-server <w.x.y.z>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdBootpServer()` |
| Ch 16 (Interfaces..........) | `[no] account profile_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdAccountProfileName()` |
| Ch 16 (Interfaces..........) | `[no] band {auto\|wcdma\|gsm\|lte} ................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdBand()` |
| Ch 16 (Interfaces..........) | `[no] bind interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdBindInterfaceName()` |
| Ch 16 (Interfaces..........) | `[no] budget active` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdBudgetActive()` |
| Ch 16 (Interfaces..........) | `[no] budget data active {download-upload\|download\|upload} <1..100000>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdBudgetDataActive()` |
| Ch 16 (Interfaces..........) | `[no] budget time active <1..672> ..............................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdBudgetTimeActive()` |
| Ch 16 (Interfaces..........) | `[no] cf-profile <profile name> {[no log]\|[log by-profile]} {activate \| deactivate} 223 [no] client-identifier mac_address` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdCfProfile223Client()` |
| Ch 16 (Interfaces..........) | `[no] client-name host_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdClientNameHostName()` |
| Ch 16 (Interfaces..........) | `[no] connectivity {nail-up \| dial-on-demand}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdConnectivity()` |
| Ch 16 (Interfaces..........) | `[no] connectivity-check continuous-log activate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdConnectivityCheckContinuousLog()` |
| Ch 16 (Interfaces..........) | `[no] corefile copy usb-storage ................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdCorefileCopyUsbStorage()` |
| Ch 16 (Interfaces..........) | `[no] default-router ip` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDefaultRouterIp()` |
| Ch 16 (Interfaces..........) | `[no] description description` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDescriptionDescription()` |
| Ch 16 (Interfaces..........) | `[no] description description` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDescriptionDescription()` |
| Ch 16 (Interfaces..........) | `[no] diag-info copy usb-storage` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDiagInfoCopyUsb()` |
| Ch 16 (Interfaces..........) | `[no] domain-name domain_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDomainNameDomainName()` |
| Ch 16 (Interfaces..........) | `[no] downstream <0..1048576>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDownstream()` |
| Ch 16 (Interfaces..........) | `[no] downstream <0..1048576>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDownstream()` |
| Ch 16 (Interfaces..........) | `[no] downstream <0..1048576>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDownstream()` |
| Ch 16 (Interfaces..........) | `[no] duplex <full \| half>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDuplex()` |
| Ch 16 (Interfaces..........) | `[no] first-dns-server {ip \| interface_name {1st-dns \| 2nd-dns \| 3rd-dns} \| ZyWALL} 129 [no] first-wins-server ip` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdFirstDnsServerZywall()` |
| Ch 16 (Interfaces..........) | `[no] hardware-address mac_address` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdHardwareAddressMacAddress()` |
| Ch 16 (Interfaces..........) | `[no] host ip` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdHostIp()` |
| Ch 16 (Interfaces..........) | `[no] idp-profile <profile name> {[no log]\|[log by-profile]} {activate \| deactivate} 223 [no] igmp activate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIdpProfile223Igmp()` |
| Ch 16 (Interfaces..........) | `[no] igmp direction {upstream \| downstream}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIgmpDirection()` |
| Ch 16 (Interfaces..........) | `[no] interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `[no] interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `[no] interface tunnel_iface` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceTunnelIface()` |
| Ch 16 (Interfaces..........) | `[no] ip address dhcp` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpAddressDhcp()` |
| Ch 16 (Interfaces..........) | `[no] ip address ip subnet_mask` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpAddressIpSubnet()` |
| Ch 16 (Interfaces..........) | `[no] ip address ip subnet_mask` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpAddressIpSubnet()` |
| Ch 16 (Interfaces..........) | `[no] ip dhcp pool profile_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpDhcpPoolProfile()` |
| Ch 16 (Interfaces..........) | `[no] ip dhcp-pool profile_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpDhcpPoolProfile()` |
| Ch 16 (Interfaces..........) | `[no] ip gateway ip` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpGatewayIp()` |
| Ch 16 (Interfaces..........) | `[no] ip helper-address ip` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpHelperAddressIp()` |
| Ch 16 (Interfaces..........) | `[no] ip ospf authentication [message-digest \| same-as-area] ...................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspfAuthenticationMessage()` |
| Ch 16 (Interfaces..........) | `[no] ip ospf authentication-key password` | Envelope 2 | `redline` | `ZyxelNetworkCmd::cmdIpOspfAuthenticationKey()` |
| Ch 16 (Interfaces..........) | `[no] ip ospf cost <1..65535>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspfCost()` |
| Ch 16 (Interfaces..........) | `[no] ip ospf dead-interval <1..65535>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspfDeadInterval()` |
| Ch 16 (Interfaces..........) | `[no] ip ospf hello-interval <1..65535>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspfHelloInterval()` |
| Ch 16 (Interfaces..........) | `[no] ip ospf message-digest-key <1..255> {md5 dr_authkey_16 \| encrypted-md5 encrypted_str}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspfMessageDigest()` |
| Ch 16 (Interfaces..........) | `[no] ip ospf priority <0..255>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspfPriority()` |
| Ch 16 (Interfaces..........) | `[no] ip ospf priority priority` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspfPriorityPriority()` |
| Ch 16 (Interfaces..........) | `[no] ip ospf retransmit-interval <1..65535>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspfRetransmitInterval()` |
| Ch 16 (Interfaces..........) | `[no] ip ospf {authentication-key key8 \| encrypted-authentication-key encrypted_str}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspf()` |
| Ch 16 (Interfaces..........) | `[no] ip proxy-arp activate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpProxyArpActivate()` |
| Ch 16 (Interfaces..........) | `[no] ip proxy-arp {ipv4 \| ipv4_range \| ipv4_cidr}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpProxyArp()` |
| Ch 16 (Interfaces..........) | `[no] ip rip v2-broadcast` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpRipV2Broadcast()` |
| Ch 16 (Interfaces..........) | `[no] ip rip {send \| receive} version <1..2>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpRipVersion()` |
| Ch 16 (Interfaces..........) | `[no] ip rip {send \| receive} version <1..2> [1.2] .............................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpRipVersion1()` |
| Ch 16 (Interfaces..........) | `[no] ip v2-broadcast` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpV2Broadcast()` |
| Ch 16 (Interfaces..........) | `[no] ipv6 address dhcp6_profile dhcp6_suffix_128` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpv6AddressDhcp6Profile()` |
| Ch 16 (Interfaces..........) | `[no] ipv6 dhcp6 address-request ...............................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpv6Dhcp6AddressRequest()` |
| Ch 16 (Interfaces..........) | `[no] ipv6 dhcp6 rapid-commit ..................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpv6Dhcp6RapidCommit()` |
| Ch 16 (Interfaces..........) | `[no] ipv6 dhcp6-request-object dhcp6_profile` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpv6Dhcp6RequestObject()` |
| Ch 16 (Interfaces..........) | `[no] ipv6 enable` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpv6Enable()` |
| Ch 16 (Interfaces..........) | `[no] ipv6 metric <0..15> ......................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpv6Metric()` |
| Ch 16 (Interfaces..........) | `[no] ipv6 nd ra accept ........................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpv6NdRaAccept()` |
| Ch 16 (Interfaces..........) | `[no] join interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdJoinInterfaceName()` |
| Ch 16 (Interfaces..........) | `[no] lease {<0..365> [<0..23> [<0..59>]] \| infinite}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdLease()` |
| Ch 16 (Interfaces..........) | `[no] local-address <ip>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdLocalAddress()` |
| Ch 16 (Interfaces..........) | `[no] local-address ip` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdLocalAddressIp()` |
| Ch 16 (Interfaces..........) | `[no] logging usb-storage` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdLoggingUsbStorage()` |
| Ch 16 (Interfaces..........) | `[no] metric <0..15> ...........................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdMetric()` |
| Ch 16 (Interfaces..........) | `[no] metric <0..15> ...........................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdMetric()` |
| Ch 16 (Interfaces..........) | `[no] mss <536..1452>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdMss()` |
| Ch 16 (Interfaces..........) | `[no] mss <536..1460> ..........................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdMss()` |
| Ch 16 (Interfaces..........) | `[no] mtu <576..1480> ..................................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdMtu()` |
| Ch 16 (Interfaces..........) | `[no] mtu <576..1500> ..................................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdMtu()` |
| Ch 16 (Interfaces..........) | `[no] negotiation auto .........................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNegotiationAuto()` |
| Ch 16 (Interfaces..........) | `[no] network interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNetworkInterfaceName()` |
| Ch 16 (Interfaces..........) | `[no] network interface_name area ip` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNetworkInterfaceNameArea()` |
| Ch 16 (Interfaces..........) | `[no] network-selection {auto\|home}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNetworkSelection()` |
| Ch 16 (Interfaces..........) | `[no] outonly-interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdOutonlyInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `[no] passive-interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdPassiveInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `[no] passive-interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdPassiveInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `[no] pin <pin code>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdPin()` |
| Ch 16 (Interfaces..........) | `[no] ping-check activate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdPingCheckActivate()` |
| Ch 16 (Interfaces..........) | `[no] ping-check activate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdPingCheckActivate()` |
| Ch 16 (Interfaces..........) | `[no] port interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdPortInterfaceName()` |
| Ch 16 (Interfaces..........) | `[no] priority-code <0..7. .......` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdPriorityCode07()` |
| Ch 16 (Interfaces..........) | `[no] remote-address <ip> ..............................................................................................................................1 4 4 [no] remote-address ip` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdRemoteAddress14()` |
| Ch 16 (Interfaces..........) | `[no] second-dns-server {ip \| interface_name {1st-dns \| 2nd-dns \| 3rd-dns} \| ZyWALL} 129 [no] second-wins-server ip` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdSecondDnsServerZywall()` |
| Ch 16 (Interfaces..........) | `[no] shutdown` | Envelope 2 | `redline` | `ZyxelNetworkCmd::cmdShutdown()` |
| Ch 16 (Interfaces..........) | `[no] shutdown` | Envelope 2 | `redline` | `ZyxelNetworkCmd::cmdShutdown()` |
| Ch 16 (Interfaces..........) | `[no] shutdown` | Envelope 2 | `redline` | `ZyxelNetworkCmd::cmdShutdown()` |
| Ch 16 (Interfaces..........) | `[no] slave interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdSlaveInterfaceName()` |
| Ch 16 (Interfaces..........) | `[no] speed <100,10>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdSpeed()` |
| Ch 16 (Interfaces..........) | `[no] starting-address ip -size <1..65535>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdStartingAddressIpSize()` |
| Ch 16 (Interfaces..........) | `[no] upstream <0..1048576>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdUpstream()` |
| Ch 16 (Interfaces..........) | `[no] upstream <0..1048576>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdUpstream()` |
| Ch 16 (Interfaces..........) | `[no] usb-storage activate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdUsbStorageActivate()` |
| Ch 16 (Interfaces..........) | `[no] usb-storage log_rotate_activate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdUsbStorageLogRotate()` |
| Ch 16 (Interfaces..........) | `[no] usb-storage update-firmware enable .......................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdUsbStorageUpdateFirmware()` |
| Ch 16 (Interfaces..........) | `[no] vlan-id <1..4094>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdVlanId()` |
| Ch 16 (Interfaces..........) | `[no]igmp activate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIgmpActivate()` |
| Ch 16 (Interfaces..........) | `account profile_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdAccountProfileName()` |
| Ch 16 (Interfaces..........) | `address ipv6_addr_prefix` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdAddressIpv6AddrPrefix()` |
| Ch 16 (Interfaces..........) | `address ipv6_addr_prefix` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdAddressIpv6AddrPrefix()` |
| Ch 16 (Interfaces..........) | `arp {arp-interval <1..1000> \| arp-ip-target <W.X.Y.Z>}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdArp()` |
| Ch 16 (Interfaces..........) | `binding interface interface_name crypto-map map_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdBindingInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `budget current-connection {keep\|drop}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdBudgetCurrentConnection()` |
| Ch 16 (Interfaces..........) | `budget new-connection {allow\|disallow} ........................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdBudgetNewConnection()` |
| Ch 16 (Interfaces..........) | `budget percentage {ptime\|pdata} <0..99>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdBudgetPercentage()` |
| Ch 16 (Interfaces..........) | `budget reset-counters` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdBudgetResetCounters()` |
| Ch 16 (Interfaces..........) | `budget reset-day <0..31>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdBudgetResetDay()` |
| Ch 16 (Interfaces..........) | `budget {log-percentage\|log-percentage-alert} [recursive <1..65535>]` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdBudgetRecursive()` |
| Ch 16 (Interfaces..........) | `budget {log\|log-alert}[recursive <1..65535>]` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdBudgetRecursive()` |
| Ch 16 (Interfaces..........) | `clear ip dhcp binding {ip \| *}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdClearIpDhcpBinding()` |
| Ch 16 (Interfaces..........) | `connectivity {nail-up \| dial-on-demand}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdConnectivity()` |
| Ch 16 (Interfaces..........) | `dhcp-option <1..254> option_name {boolean <0..1>\| uint8 <0..255> \| uint16 <0..65535>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDhcpOptionOptionName()` |
| Ch 16 (Interfaces..........) | `dhcp6 .........................................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDhcp6()` |
| Ch 16 (Interfaces..........) | `dhcp6 address-request` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDhcp6AddressRequest()` |
| Ch 16 (Interfaces..........) | `dhcp6 address-request` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDhcp6AddressRequest()` |
| Ch 16 (Interfaces..........) | `dhcp6 duid { duid \| mac }` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDhcp6Duid()` |
| Ch 16 (Interfaces..........) | `dhcp6 rapid-commit` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDhcp6RapidCommit()` |
| Ch 16 (Interfaces..........) | `dhcp6 rapid-commit` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDhcp6RapidCommit()` |
| Ch 16 (Interfaces..........) | `dhcp6 refresh-time { <600..4294967294> \| infinity }` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDhcp6RefreshTime()` |
| Ch 16 (Interfaces..........) | `dhcp6 { server \| client \| relay upper { config_interface \| ipv6_addr } }` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDhcp6()` |
| Ch 16 (Interfaces..........) | `dhcp6-lease-object dhcp6_profile` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDhcp6LeaseObjectDhcp6()` |
| Ch 16 (Interfaces..........) | `dhcp6-lease-object dhcp6_profile` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDhcp6LeaseObjectDhcp6()` |
| Ch 16 (Interfaces..........) | `dhcp6-request-object dhcp6_profile` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDhcp6RequestObjectDhcp6()` |
| Ch 16 (Interfaces..........) | `dhcp6-request-object dhcp6_profile` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDhcp6RequestObjectDhcp6()` |
| Ch 16 (Interfaces..........) | `downdelay <0..1000>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDowndelay()` |
| Ch 16 (Interfaces..........) | `enable` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdEnable()` |
| Ch 16 (Interfaces..........) | `enable` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdEnable()` |
| Ch 16 (Interfaces..........) | `exit` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdExit()` |
| Ch 16 (Interfaces..........) | `exit` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdExit()` |
| Ch 16 (Interfaces..........) | `exit` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdExit()` |
| Ch 16 (Interfaces..........) | `gateway` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdGateway()` |
| Ch 16 (Interfaces..........) | `igmp direction` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIgmpDirection()` |
| Ch 16 (Interfaces..........) | `igmp version <1..3>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIgmpVersion()` |
| Ch 16 (Interfaces..........) | `igmp {activate \| direction {downstream \| upstream} \| version <1..3>}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIgmpVersion()` |
| Ch 16 (Interfaces..........) | `interface cellular budget-auto-save <5..1440>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceCellularBudgetAuto()` |
| Ch 16 (Interfaces..........) | `interface dial interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceDialInterfaceName()` |
| Ch 16 (Interfaces..........) | `interface disconnect interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceDisconnectInterfaceName()` |
| Ch 16 (Interfaces..........) | `interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `interface interface_name ipv6` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceNameIpv6()` |
| Ch 16 (Interfaces..........) | `interface interface_name no ipv6` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceNameNo()` |
| Ch 16 (Interfaces..........) | `interface reset {interface_name\|virtual_interface_name\|all}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceReset()` |
| Ch 16 (Interfaces..........) | `interface send statistics interval <15..3600>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceSendStatisticsInterval()` |
| Ch 16 (Interfaces..........) | `interface-name {ppp_interface \| ethernet_interface} user_defined_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceNameUserDefined()` |
| Ch 16 (Interfaces..........) | `interface-rename old_user_defined_name new_user_defined_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceRenameOldUser()` |
| Ch 16 (Interfaces..........) | `ip address dhcp option-60 <text> ..............................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpAddressDhcpOption()` |
| Ch 16 (Interfaces..........) | `ip address ipv4 ipv4` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpAddressIpv4Ipv4()` |
| Ch 16 (Interfaces..........) | `ip dhcp pool rename profile_name profile_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpDhcpPoolRename()` |
| Ch 16 (Interfaces..........) | `ip dhcp static _import_static_file import file name interface interface name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpDhcpStaticImport()` |
| Ch 16 (Interfaces..........) | `ip gateway ip metric <0..15>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpGatewayIpMetric()` |
| Ch 16 (Interfaces..........) | `ip ospf authentication` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspfAuthentication()` |
| Ch 16 (Interfaces..........) | `ip ospf authentication message-digest` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspfAuthenticationMessage()` |
| Ch 16 (Interfaces..........) | `ip ospf authentication same-as-area` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspfAuthenticationSame()` |
| Ch 16 (Interfaces..........) | `ip ospf cost <1..65535>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspfCost()` |
| Ch 16 (Interfaces..........) | `ip ospf dead-interval <1..65535>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspfDeadInterval()` |
| Ch 16 (Interfaces..........) | `ip ospf hello-interval <1..65535> .............................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspfHelloInterval()` |
| Ch 16 (Interfaces..........) | `ip ospf message-digest-key <1..255> md5 password` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspfMessageDigest()` |
| Ch 16 (Interfaces..........) | `ip ospf retransmit-interval <1..65535>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpOspfRetransmitInterval()` |
| Ch 16 (Interfaces..........) | `ipv6 6to4 [prefix ipv6_addr_prefix \| destination-prefix ipv4_cidr \| relay ipv4]` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpv66to4PrefixIpv6()` |
| Ch 16 (Interfaces..........) | `ipv6 address dhcp6_profile dhcp6_suffix_128` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpv6AddressDhcp6Profile()` |
| Ch 16 (Interfaces..........) | `ipv6 address dhcp6_profile dhcp6_suffix_128` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpv6AddressDhcp6Profile()` |
| Ch 16 (Interfaces..........) | `ipv6 address ipv6_addr_prefix` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpv6AddressIpv6Addr()` |
| Ch 16 (Interfaces..........) | `ipv6 dhcp6 [client]` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpv6Dhcp6Client()` |
| Ch 16 (Interfaces..........) | `ipv6 dhcp6 duid { duid \| mac }` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpv6Dhcp6Duid()` |
| Ch 16 (Interfaces..........) | `lacp-rate {fast \| slow}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdLacpRate()` |
| Ch 16 (Interfaces..........) | `link-monitoring {arp \| mii \| none} ............................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdLinkMonitoring()` |
| Ch 16 (Interfaces..........) | `logging usb-storage category category disable` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdLoggingUsbStorageCategory()` |
| Ch 16 (Interfaces..........) | `logging usb-storage category category level <all\|normal>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdLoggingUsbStorageCategory()` |
| Ch 16 (Interfaces..........) | `logging usb-storage flushThreshold <1..100>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdLoggingUsbStorageFlushthreshold()` |
| Ch 16 (Interfaces..........) | `mac mac` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdMacMac()` |
| Ch 16 (Interfaces..........) | `miimon <1..1000>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdMiimon()` |
| Ch 16 (Interfaces..........) | `mode {802_3ad \| active-backup \| balance-alb \| mode 802_3ad}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdMode()` |
| Ch 16 (Interfaces..........) | `mtu <576..1492>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdMtu()` |
| Ch 16 (Interfaces..........) | `mtu <576..1492>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdMtu()` |
| Ch 16 (Interfaces..........) | `nd ra accept` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaAccept()` |
| Ch 16 (Interfaces..........) | `nd ra accept` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaAccept()` |
| Ch 16 (Interfaces..........) | `nd ra advertise` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaAdvertise()` |
| Ch 16 (Interfaces..........) | `nd ra advertise` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaAdvertise()` |
| Ch 16 (Interfaces..........) | `nd ra default-lifetime ........................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaDefaultLifetime()` |
| Ch 16 (Interfaces..........) | `nd ra default-lifetime <4..9000>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaDefaultLifetime()` |
| Ch 16 (Interfaces..........) | `nd ra hop-limit` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaHopLimit()` |
| Ch 16 (Interfaces..........) | `nd ra hop-limit <0..255>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaHopLimit()` |
| Ch 16 (Interfaces..........) | `nd ra managed-config-flag .....................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaManagedConfig()` |
| Ch 16 (Interfaces..........) | `nd ra managed-config-flag .....................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaManagedConfig()` |
| Ch 16 (Interfaces..........) | `nd ra max-rtr-interval ........................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaMaxRtr()` |
| Ch 16 (Interfaces..........) | `nd ra max-rtr-interval <4..1800>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaMaxRtr()` |
| Ch 16 (Interfaces..........) | `nd ra min-rtr-interval ........................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaMinRtr()` |
| Ch 16 (Interfaces..........) | `nd ra min-rtr-interval <3..1350>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaMinRtr()` |
| Ch 16 (Interfaces..........) | `nd ra mtu` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaMtu()` |
| Ch 16 (Interfaces..........) | `nd ra mtu <1280..1500> \| <0>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaMtu()` |
| Ch 16 (Interfaces..........) | `nd ra other-config-flag` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaOtherConfig()` |
| Ch 16 (Interfaces..........) | `nd ra other-config-flag` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaOtherConfig()` |
| Ch 16 (Interfaces..........) | `nd ra prefix-advertisement DHCP6_PROFILE DHCP6_SUFFIX_64` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaPrefixAdvertisement()` |
| Ch 16 (Interfaces..........) | `nd ra prefix-advertisement dhcp6_profile dhcp6_suffix_64` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaPrefixAdvertisement()` |
| Ch 16 (Interfaces..........) | `nd ra prefix-advertisement ipv6_addr_prefix [ auto { on \| off} ] [ link{ on \| off } ] [ preferred-time { <0..4294967294> \| infinity }] [valid-time{ <0..4294967294> \| infinity }]` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaPrefixAdvertisement()` |
| Ch 16 (Interfaces..........) | `nd ra reachable-time ..........................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaReachableTime()` |
| Ch 16 (Interfaces..........) | `nd ra reachable-time <0..3600000> .............................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaReachableTime()` |
| Ch 16 (Interfaces..........) | `nd ra retrans-timer` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaRetransTimer()` |
| Ch 16 (Interfaces..........) | `nd ra retrans-timer <0..4294967295>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaRetransTimer()` |
| Ch 16 (Interfaces..........) | `nd ra router-preference {low \| medium \| high }` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNdRaRouterPreference()` |
| Ch 16 (Interfaces..........) | `network IP/<1..32> .......................................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNetworkIp()` |
| Ch 16 (Interfaces..........) | `network ip mask` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNetworkIpMask()` |
| Ch 16 (Interfaces..........) | `no budget log` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNoBudgetLog()` |
| Ch 16 (Interfaces..........) | `no budget log-percentage` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNoBudgetLogPercentage()` |
| Ch 16 (Interfaces..........) | `no dhcp-option <1..254>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNoDhcpOption()` |
| Ch 16 (Interfaces..........) | `no ip ospf authentication` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNoIpOspfAuthentication()` |
| Ch 16 (Interfaces..........) | `no ip ospf message-digest-key` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNoIpOspfMessage()` |
| Ch 16 (Interfaces..........) | `no mac` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNoMac()` |
| Ch 16 (Interfaces..........) | `no network` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNoNetwork()` |
| Ch 16 (Interfaces..........) | `no udp-decoder {bad-udp-l4-size \| udp-land \| udp-smurf} log ....................................................................2 3 5 no use-defined-mac` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNoUdpDecoderLog()` |
| Ch 16 (Interfaces..........) | `ping-check` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdPingCheck()` |
| Ch 16 (Interfaces..........) | `ping-check {FQDN \| IPv4 \| default-gateway} [period <5..3600>] [timeout <1..10>] [fail-tolerance <1..10>] [method {icmp \| tcp}] [port <1..65535>] [probe-condition {any \| all}]` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdPingCheckPeriodTimeout()` |
| Ch 16 (Interfaces..........) | `ping-check {domain_name \| ip}` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdPingCheck()` |
| Ch 16 (Interfaces..........) | `ping-check {domain_name \| ip} fail-tolerance <1..10>` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdPingCheckFailTolerance()` |
| Ch 16 (Interfaces..........) | `ping-check {domain_name \| ip} method {icmp \| tcp}` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdPingCheckMethod()` |
| Ch 16 (Interfaces..........) | `ping-check {domain_name \| ip} period <5..30>` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdPingCheckPeriod()` |
| Ch 16 (Interfaces..........) | `ping-check {domain_name \| ip} port <1..65535>` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdPingCheckPort()` |
| Ch 16 (Interfaces..........) | `ping-check {domain_name \| ip} timeout <1..10>` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdPingCheckTimeout()` |
| Ch 16 (Interfaces..........) | `port status Port<1..x>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdPortStatusPort()` |
| Ch 16 (Interfaces..........) | `release dhcp interface-name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdReleaseDhcpInterfaceName()` |
| Ch 16 (Interfaces..........) | `renew dhcp interface-name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdRenewDhcpInterfaceName()` |
| Ch 16 (Interfaces..........) | `router ospf` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdRouterOspf()` |
| Ch 16 (Interfaces..........) | `router rip` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdRouterRip()` |
| Ch 16 (Interfaces..........) | `show` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdShow()` |
| Ch 16 (Interfaces..........) | `show bridge available member` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowBridgeAvailableMember()` |
| Ch 16 (Interfaces..........) | `show connectivity-check continuous-log status` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowConnectivityCheckContinuous()` |
| Ch 16 (Interfaces..........) | `show corefile copy usb-storage` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowCorefileCopyUsb()` |
| Ch 16 (Interfaces..........) | `show diag-info copy usb-storage` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowDiagInfoCopy()` |
| Ch 16 (Interfaces..........) | `show interface cellular [corresponding-slot\|device-status\|support-device]` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceCellularCorresponding()` |
| Ch 16 (Interfaces..........) | `show interface cellular budget-auto-save` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceCellularBudget()` |
| Ch 16 (Interfaces..........) | `show interface cellular corresponding-slot` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceCellularCorresponding()` |
| Ch 16 (Interfaces..........) | `show interface cellular device-status` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceCellularDevice()` |
| Ch 16 (Interfaces..........) | `show interface cellular status` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceCellularStatus()` |
| Ch 16 (Interfaces..........) | `show interface cellular support-device` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceCellularSupport()` |
| Ch 16 (Interfaces..........) | `show interface interface_name [budget]` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `show interface interface_name device profile` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `show interface interface_name device status` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `show interface interface_name proxy-arp address` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `show interface interface_name proxy-arp status` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceInterfaceName()` |
| Ch 16 (Interfaces..........) | `show interface lag` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceLag()` |
| Ch 16 (Interfaces..........) | `show interface lagx` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceLagx()` |
| Ch 16 (Interfaces..........) | `show interface ppp system-default` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfacePppSystem()` |
| Ch 16 (Interfaces..........) | `show interface ppp user-define` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfacePppUser()` |
| Ch 16 (Interfaces..........) | `show interface send statistics interval` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceSendStatistics()` |
| Ch 16 (Interfaces..........) | `show interface summary all` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceSummaryAll()` |
| Ch 16 (Interfaces..........) | `show interface summary all status` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceSummaryAll()` |
| Ch 16 (Interfaces..........) | `show interface tunnel status` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceTunnelStatus()` |
| Ch 16 (Interfaces..........) | `show interface tunnel_iface` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceTunnelIface()` |
| Ch 16 (Interfaces..........) | `show interface vti` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceVti()` |
| Ch 16 (Interfaces..........) | `show interface vtix` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceVtix()` |
| Ch 16 (Interfaces..........) | `show interface {ethernet \| vlan \| bridge \| ppp} status` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceStatus()` |
| Ch 16 (Interfaces..........) | `show interface {interface_name \| ethernet \| vlan \| bridge \| ppp \| virtual ethernet \| virtual vlan \| virtual bridge \| all}` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterface()` |
| Ch 16 (Interfaces..........) | `show interface-name` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceName()` |
| Ch 16 (Interfaces..........) | `show ip dhcp binding [ip]` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowIpDhcpBinding()` |
| Ch 16 (Interfaces..........) | `show ip dhcp dhcp-options` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowIpDhcpDhcp()` |
| Ch 16 (Interfaces..........) | `show ip dhcp list interface {all \| interface name} keyword keyword` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowIpDhcpList()` |
| Ch 16 (Interfaces..........) | `show ip dhcp pool [profile_name]` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowIpDhcpPool()` |
| Ch 16 (Interfaces..........) | `show ip dhcp pool profile_name dhcp-options` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowIpDhcpPool()` |
| Ch 16 (Interfaces..........) | `show ip dhcp static interface {all \| interface name}` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowIpDhcpStatic()` |
| Ch 16 (Interfaces..........) | `show ipv6 interface {interface_name \| all}` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowIpv6Interface()` |
| Ch 16 (Interfaces..........) | `show ipv6 nd ra status config_interface` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowIpv6NdRa()` |
| Ch 16 (Interfaces..........) | `show ipv6 static address interface` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowIpv6StaticAddress()` |
| Ch 16 (Interfaces..........) | `show lag available slaves` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowLagAvailableSlaves()` |
| Ch 16 (Interfaces..........) | `show logging status usb-storage` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowLoggingStatusUsb()` |
| Ch 16 (Interfaces..........) | `show ping-check [interface_name \| status]` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowPingCheckInterface()` |
| Ch 16 (Interfaces..........) | `show port setting` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowPortSetting()` |
| Ch 16 (Interfaces..........) | `show port statistic portx interval <5..3600>` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowPortStatisticPortx()` |
| Ch 16 (Interfaces..........) | `show port status` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowPortStatus()` |
| Ch 16 (Interfaces..........) | `show port type physical` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowPortTypePhysical()` |
| Ch 16 (Interfaces..........) | `show port vlan-id` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowPortVlanId()` |
| Ch 16 (Interfaces..........) | `show port-grouping` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowPortGrouping()` |
| Ch 16 (Interfaces..........) | `show rip {global \| interface {all \| interface_name}}` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowRip()` |
| Ch 16 (Interfaces..........) | `show usb-storage` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowUsbStorage()` |
| Ch 16 (Interfaces..........) | `show usb-storage space` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowUsbStorageSpace()` |
| Ch 16 (Interfaces..........) | `show usb-storage space ftp` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowUsbStorageSpace()` |
| Ch 16 (Interfaces..........) | `show usb-storage space tmp` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowUsbStorageSpace()` |
| Ch 16 (Interfaces..........) | `show usb-storage space usb` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowUsbStorageSpace()` |
| Ch 16 (Interfaces..........) | `show usb-storage update-firmware status` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowUsbStorageUpdate()` |
| Ch 16 (Interfaces..........) | `show vpn-interface-restriction status` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowVpnInterfaceRestriction()` |
| Ch 16 (Interfaces..........) | `traffic-prioritize {tcp-ack\|content-filter\|dns\|ipsec-vpn\|ssl-vpn} deactivate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdTrafficPrioritizeDeactivate()` |
| Ch 16 (Interfaces..........) | `traffic-prioritize {tcp-ack\|content-filter\|dns} bandwidth <0..1048576> priority <1..7> [maximize- bandwidth-usage];` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdTrafficPrioritizeBandwidthPriority()` |
| Ch 16 (Interfaces..........) | `traffic-prioritize {tcp-ack\|content-filter\|dns} bandwidth <0..1048576>];` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdTrafficPrioritizeBandwidth()` |
| Ch 16 (Interfaces..........) | `traffic-prioritize {tcp-ack\|content-filter\|dns} priority-code <0..7> deactivate . 148 traffic-prioritize {tcp-ack\|content-filter\|dns} priority-code <0..7> deactivate . 155 traffic-prioritize {tcp-ack\|content-filter\|dns\|ipsec-vpn\|ssl-vpn} bandwidth <0..1048576> prior- ity <1..7> [maximize-bandwidth-usage];` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdTrafficPrioritizePriorityCode()` |
| Ch 16 (Interfaces..........) | `tunnel destination ipv4` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdTunnelDestinationIpv4()` |
| Ch 16 (Interfaces..........) | `tunnel mode [ipv6ip [manual \| 6to4]]] .........................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdTunnelModeIpv6ipManual()` |
| Ch 16 (Interfaces..........) | `tunnel mode ip gre` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdTunnelModeIpGre()` |
| Ch 16 (Interfaces..........) | `tunnel source [ipv4\|tunnel_bind_interface\|_any]` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdTunnelSourceIpv4Tunnel()` |
| Ch 16 (Interfaces..........) | `type {external \| general \| internal}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdType()` |
| Ch 16 (Interfaces..........) | `type {internal \| external \| general}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdType()` |
| Ch 16 (Interfaces..........) | `updelay <0..1000>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdUpdelay()` |
| Ch 16 (Interfaces..........) | `usb-storage mount .............................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdUsbStorageMount()` |
| Ch 16 (Interfaces..........) | `usb-storage umount ............................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdUsbStorageUmount()` |
| Ch 16 (Interfaces..........) | `usb-storage warn <10..99> percentage` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdUsbStorageWarnPercentage()` |
| Ch 16 (Interfaces..........) | `usb-storage warn <100..9999> megabyte` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdUsbStorageWarnMegabyte()` |
| Ch 16 (Interfaces..........) | `usb-storage warn number <percentage\|megabyte>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdUsbStorageWarnNumber()` |
| Ch 16 (Interfaces..........) | `use-defined-mac` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdUseDefinedMac()` |
| Ch 16 (Interfaces..........) | `vpn-configuration-provision rule { delete conf_index \| move conf_index to conf_index } 286 vpn-interface-restriction activate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdVpnConfigurationProvisionRule()` |
| Ch 16 (Interfaces..........) | `vpn-interface-restriction deactivate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdVpnInterfaceRestrictionDeactivate()` |
| Ch 16 (Interfaces..........) | `xmit-hash-policy {layer2 \| layer2_3}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdXmitHashPolicy()` |
| Ch 17 (Trunks..............) | `[no] interface {num\|interface-name}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterface()` |
| Ch 17 (Trunks..............) | `[no] interface-group group-name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceGroupGroupName()` |
| Ch 17 (Trunks..............) | `[no] system default-snat` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdSystemDefaultSnat()` |
| Ch 17 (Trunks..............) | `algorithm {wrr\|llf\|spill-over}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdAlgorithm()` |
| Ch 17 (Trunks..............) | `exit` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdExit()` |
| Ch 17 (Trunks..............) | `flush` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdFlush()` |
| Ch 17 (Trunks..............) | `interface {num\|append\|insert num} interface-name [weight <1..10>\|limit <1..2097152>\|passive]` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceNameWeight()` |
| Ch 17 (Trunks..............) | `loadbalancing-index <inbound\|outbound\|total>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdLoadbalancingIndex()` |
| Ch 17 (Trunks..............) | `mode {normal\|trunk}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdMode()` |
| Ch 17 (Trunks..............) | `move <1..8> to <1..8>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdMoveTo()` |
| Ch 17 (Trunks..............) | `show interface-group {system-default\|user-define\|group-name}` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowInterfaceGroup()` |
| Ch 17 (Trunks..............) | `show system default-interface-group` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowSystemDefaultInterface()` |
| Ch 17 (Trunks..............) | `show system default-snat` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowSystemDefaultSnat()` |
| Ch 17 (Trunks..............) | `system default-interface-group group-name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdSystemDefaultInterfaceGroup()` |
| Ch 18 (Route ..............) | `[no] auto-destination` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAutoDestination()` |
| Ch 18 (Route ..............) | `[no] auto-disable .............................................................................................................` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAutoDisable()` |
| Ch 18 (Route ..............) | `[no] bwm activate` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdBwmActivate()` |
| Ch 18 (Route ..............) | `[no] deactivate` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdDeactivate()` |
| Ch 18 (Route ..............) | `[no] deactivate` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdDeactivate()` |
| Ch 18 (Route ..............) | `[no] description description` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdDescriptionDescription()` |
| Ch 18 (Route ..............) | `[no] description description` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdDescriptionDescription()` |
| Ch 18 (Route ..............) | `[no] destination {address6_object\|any}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdDestination()` |
| Ch 18 (Route ..............) | `[no] destination {address_object\|any}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdDestination()` |
| Ch 18 (Route ..............) | `[no] dscp class {default \| dscp_class}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdDscpClass()` |
| Ch 18 (Route ..............) | `[no] dscp class {default \| dscp_class}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdDscpClass()` |
| Ch 18 (Route ..............) | `[no] dscp {any \| <0..63>}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdDscp()` |
| Ch 18 (Route ..............) | `[no] dscp {any \| <0..63>}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdDscp()` |
| Ch 18 (Route ..............) | `[no] interface interface_name` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceName()` |
| Ch 18 (Route ..............) | `[no] interface interface_name` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceName()` |
| Ch 18 (Route ..............) | `[no] ip route control-virtual-server-rules activate` | Envelope 3 | `tier2` | `ZyxelNetworkCmd::cmdIpRouteControlVirtual()` |
| Ch 18 (Route ..............) | `[no] ip route {w.x.y.z} {w.x.y.z} {interface\|w.x.y.z} <0..127>` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdIpRoute()` |
| Ch 18 (Route ..............) | `[no] next-hop {auto\|gateway address object \|interface interface_name \|trunk trunk_name\|tunnel tunnel_name}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNextHop()` |
| Ch 18 (Route ..............) | `[no] next-hop {auto\|gateway address_object \|interface interface_name \|trunk trunk_name\|tunnel tunnel_name}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNextHop()` |
| Ch 18 (Route ..............) | `[no] policy controll-ipsec-dynamic-rules activate` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdPolicyControllIpsecDynamic()` |
| Ch 18 (Route ..............) | `[no] policy controll-virtual-server-rules activate` | Envelope 3 | `tier2` | `ZyxelNetworkCmd::cmdPolicyControllVirtualServer()` |
| Ch 18 (Route ..............) | `[no] policy override-direct-route activate` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdPolicyOverrideDirectRoute()` |
| Ch 18 (Route ..............) | `[no] policy6 override-direct-route activate` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdPolicy6OverrideDirectRoute()` |
| Ch 18 (Route ..............) | `[no] schedule schedule_object` | Envelope 3 | `tier2` | `ZyxelNetworkCmd::cmdScheduleScheduleObject()` |
| Ch 18 (Route ..............) | `[no] schedule schedule_object` | Envelope 3 | `tier2` | `ZyxelNetworkCmd::cmdScheduleScheduleObject()` |
| Ch 18 (Route ..............) | `[no] service {service_name\|any}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdService()` |
| Ch 18 (Route ..............) | `[no] service {service_name\|any}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdService()` |
| Ch 18 (Route ..............) | `[no] snat {outgoing-interface\| {address_object}}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdSnat()` |
| Ch 18 (Route ..............) | `[no] source {address6_object\|any}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdSource()` |
| Ch 18 (Route ..............) | `[no] source {address_object\|any}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdSource()` |
| Ch 18 (Route ..............) | `[no] srcport {profile_name\|any}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdSrcport()` |
| Ch 18 (Route ..............) | `[no] srcport {profile_name\|any}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdSrcport()` |
| Ch 18 (Route ..............) | `[no] sslvpn tunnel_name` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdSslvpnTunnelName()` |
| Ch 18 (Route ..............) | `[no] tunnel tunnel_name` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdTunnelTunnelName()` |
| Ch 18 (Route ..............) | `[no] tunnel tunnel_name` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdTunnelTunnelName()` |
| Ch 18 (Route ..............) | `[no] user user_name` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdUserUserName()` |
| Ch 18 (Route ..............) | `[no] user user_name` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdUserUserName()` |
| Ch 18 (Route ..............) | `conn-check {FQDN \| addr \| activate}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdConnCheck()` |
| Ch 18 (Route ..............) | `dscp-marking <0..63>` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdDscpMarking()` |
| Ch 18 (Route ..............) | `dscp-marking <0..63>` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdDscpMarking()` |
| Ch 18 (Route ..............) | `dscp-marking class {default \| dscp_class}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdDscpMarkingClass()` |
| Ch 18 (Route ..............) | `dscp-marking class {default \| dscp_class}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdDscpMarkingClass()` |
| Ch 18 (Route ..............) | `exit` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdExit()` |
| Ch 18 (Route ..............) | `exit` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdExit()` |
| Ch 18 (Route ..............) | `ip route replace {w.x.y.z} {w.x.y.z} {interface\|w.x.y.z} <0..127> with {w.x.y.z} {w.x.y.z} {in- terface\|w.x.y.z} <0..127>` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdIpRouteReplaceWith()` |
| Ch 18 (Route ..............) | `ip6 route destv6/prefix { ipv6_global_address \| ipv6_link_local \| interface} [<0..127>] 174 ip6 route destv6/prefix { ipv6_link_local interface} [<0..127>]` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdIp6RouteDestv6Prefix()` |
| Ch 18 (Route ..............) | `ip6 route replace destv6/prefix { gatewayv6 \| interface} [<0..127>] with destv6/prefix { gate- wayv6 \| interface} [<0..127>]` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdIp6RouteReplaceDestv6()` |
| Ch 18 (Route ..............) | `no dscp-marking` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNoDscpMarking()` |
| Ch 18 (Route ..............) | `no dscp-marking` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNoDscpMarking()` |
| Ch 18 (Route ..............) | `no ip6 route destv6/prefix { gatewayv6 \| interface} [<0..127>]` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNoIp6RouteDestv6()` |
| Ch 18 (Route ..............) | `policy default-route` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdPolicyDefaultRoute()` |
| Ch 18 (Route ..............) | `policy delete policy_number` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdPolicyDeletePolicyNumber()` |
| Ch 18 (Route ..............) | `policy flush` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdPolicyFlush()` |
| Ch 18 (Route ..............) | `policy list table` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdPolicyListTable()` |
| Ch 18 (Route ..............) | `policy move policy_number to policy_number` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdPolicyMovePolicyNumber()` |
| Ch 18 (Route ..............) | `policy {policy_number \| append \| insert policy_number}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdPolicy()` |
| Ch 18 (Route ..............) | `policy6 {policy_number \| append \| insert policy_number}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdPolicy6()` |
| Ch 18 (Route ..............) | `show bwm activation` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowBwmActivation()` |
| Ch 18 (Route ..............) | `show bwm-usage < [policy-route policy_number] \| [interface interface_name]` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowBwmUsagePolicy()` |
| Ch 18 (Route ..............) | `show ip route control-virtual-server-rules` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowIpRouteControl()` |
| Ch 18 (Route ..............) | `show ip route-settings` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowIpRouteSettings()` |
| Ch 18 (Route ..............) | `show policy-route [policy_number]` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowPolicyRoutePolicy()` |
| Ch 18 (Route ..............) | `show policy-route begin <1..200> end <1..200>` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowPolicyRouteBegin()` |
| Ch 18 (Route ..............) | `show policy-route conn-check` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowPolicyRouteConn()` |
| Ch 18 (Route ..............) | `show policy-route conn-check [policy_number]` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowPolicyRouteConn()` |
| Ch 18 (Route ..............) | `show policy-route conn-check status [policy_number]` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowPolicyRouteConn()` |
| Ch 18 (Route ..............) | `show policy-route controll-ipsec-dynamic-rules` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowPolicyRouteControll()` |
| Ch 18 (Route ..............) | `show policy-route controll-virtual-server-rules` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowPolicyRouteControll()` |
| Ch 18 (Route ..............) | `show policy-route override-direct-route` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowPolicyRouteOverride()` |
| Ch 18 (Route ..............) | `show policy-route rule_count` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowPolicyRouteRule()` |
| Ch 18 (Route ..............) | `show policy-route underlayer-rules` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowPolicyRouteUnderlayer()` |
| Ch 18 (Route ..............) | `show policy-route6 [policy_number]` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowPolicyRoute6Policy()` |
| Ch 18 (Route ..............) | `show policy-route6 begin <1..200> end <1..200>` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowPolicyRoute6Begin()` |
| Ch 18 (Route ..............) | `show policy-route6 controll-ipsec-dynamic-rules` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowPolicyRoute6Controll()` |
| Ch 18 (Route ..............) | `show policy-route6 override-direct-route` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowPolicyRoute6Override()` |
| Ch 18 (Route ..............) | `show policy-route6 rule_count` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowPolicyRoute6Rule()` |
| Ch 19 (Routing Protocol) | `[no] area IP [{stub \| nssa}] ..................................................................................................` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAreaIp()` |
| Ch 19 (Routing Protocol) | `[no] area IP authentication` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAreaIpAuthentication()` |
| Ch 19 (Routing Protocol) | `[no] area IP authentication authentication-key authkey` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAreaIpAuthenticationAuthentication()` |
| Ch 19 (Routing Protocol) | `[no] area IP authentication message-digest` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAreaIpAuthenticationMessage()` |
| Ch 19 (Routing Protocol) | `[no] area IP authentication message-digest-key <1..255> md5 authkey` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAreaIpAuthenticationMessage()` |
| Ch 19 (Routing Protocol) | `[no] area IP virtual-link IP ..................................................................................................` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAreaIpVirtualLink()` |
| Ch 19 (Routing Protocol) | `[no] area IP virtual-link IP authentication` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAreaIpVirtualLink()` |
| Ch 19 (Routing Protocol) | `[no] area IP virtual-link IP authentication authentication-key authkey` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAreaIpVirtualLink()` |
| Ch 19 (Routing Protocol) | `[no] area IP virtual-link IP authentication message-digest ....................................................................` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAreaIpVirtualLink()` |
| Ch 19 (Routing Protocol) | `[no] area IP virtual-link IP authentication message-digest-key <1..255> md5 authkey` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAreaIpVirtualLink()` |
| Ch 19 (Routing Protocol) | `[no] area IP virtual-link IP authentication same-as-area` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAreaIpVirtualLink()` |
| Ch 19 (Routing Protocol) | `[no] area IP virtual-link IP authentication-key authkey` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAreaIpVirtualLink()` |
| Ch 19 (Routing Protocol) | `[no] area IP virtual-link IP encrypted-authentication-key <ciphertext>` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAreaIpVirtualLink()` |
| Ch 19 (Routing Protocol) | `[no] as-number <1..4294967295>` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAsNumber()` |
| Ch 19 (Routing Protocol) | `[no] authentication mode {md5 \| text}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAuthenticationMode()` |
| Ch 19 (Routing Protocol) | `[no] authentication string authkey` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAuthenticationStringAuthkey()` |
| Ch 19 (Routing Protocol) | `[no] maximum-paths <1..255> ...................................................................................................` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdMaximumPaths()` |
| Ch 19 (Routing Protocol) | `[no] neighbor ipv4` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNeighborIpv4()` |
| Ch 19 (Routing Protocol) | `[no] neighbor ipv4 connect-retry ................................................................................................................` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNeighborIpv4ConnectRetry()` |
| Ch 19 (Routing Protocol) | `[no] neighbor ipv4 default-originate` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNeighborIpv4DefaultOriginate()` |
| Ch 19 (Routing Protocol) | `[no] neighbor ipv4 description description` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNeighborIpv4DescriptionDescription()` |
| Ch 19 (Routing Protocol) | `[no] neighbor ipv4 ebgp-multihop hops <1..255>` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNeighborIpv4EbgpMultihop()` |
| Ch 19 (Routing Protocol) | `[no] neighbor ipv4 maximum-prefix < 1..4294967295 >` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNeighborIpv4MaximumPrefix()` |
| Ch 19 (Routing Protocol) | `[no] neighbor ipv4 password password` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNeighborIpv4PasswordPassword()` |
| Ch 19 (Routing Protocol) | `[no] neighbor ipv4 remote-as <1..4294967295>` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNeighborIpv4RemoteAs()` |
| Ch 19 (Routing Protocol) | `[no] neighbor ipv4 timers < 0..65535> < 0..65535>` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNeighborIpv4Timers()` |
| Ch 19 (Routing Protocol) | `[no] neighbor ipv4 ttl-security [hops] <1..254> .....` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNeighborIpv4TtlSecurity()` |
| Ch 19 (Routing Protocol) | `[no] neighbor ipv4 update-source [ipv4\|interface_name]` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNeighborIpv4UpdateSource()` |
| Ch 19 (Routing Protocol) | `[no] neighbor ipv4 weight <1..65535>` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNeighborIpv4Weight()` |
| Ch 19 (Routing Protocol) | `[no] network interface area IP` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNetworkInterfaceAreaIp()` |
| Ch 19 (Routing Protocol) | `[no] network interface_name` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNetworkInterfaceName()` |
| Ch 19 (Routing Protocol) | `[no] network ipv4_cidr` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNetworkIpv4Cidr()` |
| Ch 19 (Routing Protocol) | `[no] outonly-interface interface_name` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdOutonlyInterfaceInterfaceName()` |
| Ch 19 (Routing Protocol) | `[no] passive-interface interface_name` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdPassiveInterfaceInterfaceName()` |
| Ch 19 (Routing Protocol) | `[no] passive-interface interface_name` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdPassiveInterfaceInterfaceName()` |
| Ch 19 (Routing Protocol) | `[no] redistribute connected` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdRedistributeConnected()` |
| Ch 19 (Routing Protocol) | `[no] redistribute {static \| ospf}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdRedistribute()` |
| Ch 19 (Routing Protocol) | `[no] redistribute {static \| rip}` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdRedistribute()` |
| Ch 19 (Routing Protocol) | `[no] redistribute {static \| rip} metric-type <1..2> metric <0..16777214>` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdRedistributeMetricTypeMetric()` |
| Ch 19 (Routing Protocol) | `[no] router-id IP .............................................................................................................` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdRouterIdIp()` |
| Ch 19 (Routing Protocol) | `[no] router-id router-id` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdRouterIdRouterId()` |
| Ch 19 (Routing Protocol) | `[no] version <1..2>` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdVersion()` |
| Ch 19 (Routing Protocol) | `area IP virtual-link IP message-digest-key <1..255> encrypted-authentication-key` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAreaIpVirtualLink()` |
| Ch 19 (Routing Protocol) | `area IP virtual-link IP message-digest-key <1..255> md5 authkey` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAreaIpVirtualLink()` |
| Ch 19 (Routing Protocol) | `authentication key <1..255> key-string authkey` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdAuthenticationKeyKeyString()` |
| Ch 19 (Routing Protocol) | `encrypted-string ciphertext` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdEncryptedStringCiphertext()` |
| Ch 19 (Routing Protocol) | `exit` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdExit()` |
| Ch 19 (Routing Protocol) | `no area IP virtual-link IP message-digest-key <1..255>` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNoAreaIpVirtual()` |
| Ch 19 (Routing Protocol) | `no authentication key .........................................................................................................` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdNoAuthenticationKey()` |
| Ch 19 (Routing Protocol) | `pwd-expiry link-to-device custom {myrouter \| <FQDN> \| <IPv6 Address> \| <W.X.Y.X>} 445 quit` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdPwdExpiryLinkTo()` |
| Ch 19 (Routing Protocol) | `redistribute {static \| ospf} metric <0..16>` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdRedistributeMetric()` |
| Ch 19 (Routing Protocol) | `router bgp` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdRouterBgp()` |
| Ch 19 (Routing Protocol) | `router ospf` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdRouterOspf()` |
| Ch 19 (Routing Protocol) | `router ospf` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdRouterOspf()` |
| Ch 19 (Routing Protocol) | `router ospf` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdRouterOspf()` |
| Ch 19 (Routing Protocol) | `router rip` | Envelope 3 | `tier1` | `ZyxelNetworkCmd::cmdRouterRip()` |
| Ch 19 (Routing Protocol) | `show bgp [global \|neighbor] ...................................................................................................` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowBgpGlobalNeighbor()` |
| Ch 19 (Routing Protocol) | `show bgp [summary \| route \| mem]` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowBgpSummaryRoute()` |
| Ch 19 (Routing Protocol) | `show ip bgp neighbor ipv4 [advertised-routes \| prefix-counts \| routes]` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowIpBgpNeighbor()` |
| Ch 19 (Routing Protocol) | `show ip route [kernel \| connected \| static \| ospf \| rip \| bgp]` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowIpRouteKernel()` |
| Ch 19 (Routing Protocol) | `show ip route bgp` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowIpRouteBgp()` |
| Ch 19 (Routing Protocol) | `show ospf area IP virtual-link` | Envelope 3 | `read` | `ZyxelNetworkCmd::cmdShowOspfAreaIp()` |
| Ch 20 (Zones...............) | `[no] crypto profile_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdCryptoProfileName()` |
| Ch 20 (Zones...............) | `[no] interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceName()` |
| Ch 20 (Zones...............) | `[no] sslvpn profile_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdSslvpnProfileName()` |
| Ch 20 (Zones...............) | `[no] zone profile_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdZoneProfileName()` |
| Ch 20 (Zones...............) | `show zone [profile_name]` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowZoneProfileName()` |
| Ch 20 (Zones...............) | `show zone binding-iface` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowZoneBindingIface()` |
| Ch 20 (Zones...............) | `show zone default-binding` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowZoneDefaultBinding()` |
| Ch 20 (Zones...............) | `show zone none-binding` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowZoneNoneBinding()` |
| Ch 20 (Zones...............) | `show zone system-default` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowZoneSystemDefault()` |
| Ch 20 (Zones...............) | `show zone user-define` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowZoneUserDefine()` |
| Ch 20 (Zones...............) | `zone profile_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdZoneProfileName()` |
| Ch 21 (DDNS) | `[no] additional-ddns-options` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdAdditionalDdnsOptions()` |
| Ch 21 (DDNS) | `[no] av-profile <profile name>{[no log]\|[log by-profile]} {activate \| deactivate} 223 [no] backmx` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdAvProfile223Backmx()` |
| Ch 21 (DDNS) | `[no] backup-custom ip` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdBackupCustomIp()` |
| Ch 21 (DDNS) | `[no] backup-iface interface_name` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdBackupIfaceInterfaceName()` |
| Ch 21 (DDNS) | `[no] custom ip` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdCustomIp()` |
| Ch 21 (DDNS) | `[no] ddns-server {FQDN DNS}` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdDdnsServer()` |
| Ch 21 (DDNS) | `[no] ha-iface interface_name` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdHaIfaceInterfaceName()` |
| Ch 21 (DDNS) | `[no] host hostname` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdHostHostname()` |
| Ch 21 (DDNS) | `[no] ip ddns profile profile_name` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdIpDdnsProfileProfile()` |
| Ch 21 (DDNS) | `[no] ip-select {iface \| auto \| custom}` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdIpSelect()` |
| Ch 21 (DDNS) | `[no] ip-select-backup {iface \| auto \| custom}` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdIpSelectBackup()` |
| Ch 21 (DDNS) | `[no] mx {ip \| domain_name}` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdMx()` |
| Ch 21 (DDNS) | `[no] service-type {dyndns \| dyndns_static \| dyndns_custom \| dynu-basic \| dynu-premium \| no-ip \| peanut-hull \| 3322-dyn \| 3322-static \| Selfhost \| User custom}` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdServiceType()` |
| Ch 21 (DDNS) | `[no] url {URL TEXT} ...........................................................................................................` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdUrl()` |
| Ch 21 (DDNS) | `[no] username username password password` | Envelope 5 | `redline` | `ZyxelNatCmd::cmdUsernameUsernamePasswordPassword()` |
| Ch 21 (DDNS) | `[no] wan-iface interface_name` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdWanIfaceInterfaceName()` |
| Ch 21 (DDNS) | `[no] wildcard` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdWildcard()` |
| Ch 21 (DDNS) | `[no]address6-object object_name interface-gateway interface {slaac \| static} {addr_index} 460 [no]https activate` | Envelope 5 | `tier2` | `ZyxelNatCmd::cmdAddress6ObjectObjectName()` |
| Ch 21 (DDNS) | `show ddns [profile_name]` | Envelope 5 | `read` | `ZyxelNatCmd::cmdShowDdnsProfileName()` |
| Ch 21 (DDNS) | `show ddns-status` | Envelope 5 | `read` | `ZyxelNatCmd::cmdShowDdnsStatus()` |
| Ch 22 (Virtual Servers) | `Command` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdCommand()` |
| Ch 22 (Virtual Servers) | `[no] activate` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdActivate()` |
| Ch 22 (Virtual Servers) | `[no] hash-auto ................................................................................................................` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdHashAuto()` |
| Ch 22 (Virtual Servers) | `[no] health-check activate ....................................................................................................` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdHealthCheckActivate()` |
| Ch 22 (Virtual Servers) | `[no] https enable-sni .........................................................................................................` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdHttpsEnableSni()` |
| Ch 22 (Virtual Servers) | `check-period 1-86400` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdCheckPeriod186400()` |
| Ch 22 (Virtual Servers) | `connect-timeout 1-300` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdConnectTimeout1300()` |
| Ch 22 (Virtual Servers) | `dns query fqdn` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdDnsQueryFqdn()` |
| Ch 22 (Virtual Servers) | `health-check type {http \| https \| tcp \| smtp \| dns \| ping}` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdHealthCheckType()` |
| Ch 22 (Virtual Servers) | `host sni` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdHostSni()` |
| Ch 22 (Virtual Servers) | `http path url` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdHttpPathUrl()` |
| Ch 22 (Virtual Servers) | `ip virtual-server delete profile_name` | Envelope 5 | `tier2` | `ZyxelNatCmd::cmdIpVirtualServerDelete()` |
| Ch 22 (Virtual Servers) | `ip virtual-server flush` | Envelope 5 | `tier2` | `ZyxelNatCmd::cmdIpVirtualServerFlush()` |
| Ch 22 (Virtual Servers) | `ip virtual-server load-balancer name` | Envelope 5 | `tier2` | `ZyxelNatCmd::cmdIpVirtualServerLoad()` |
| Ch 22 (Virtual Servers) | `ip virtual-server load-balancer rename old_name new_name` | Envelope 5 | `tier2` | `ZyxelNatCmd::cmdIpVirtualServerLoad()` |
| Ch 22 (Virtual Servers) | `ip virtual-server profile_name interface interface_name original-ip {any \| IP \| address_object} map-to {address_object \| ip} map-type original-service service_object mapped-service service_object [nat-loopback [nat-1-1-map] [deactivate] \| nat-1-1-map [deactivate] \| de- activate]` | Envelope 5 | `tier2` | `ZyxelNatCmd::cmdIpVirtualServerProfile()` |
| Ch 22 (Virtual Servers) | `ip virtual-server profile_name interface interface_name original-ip {any \| IP \| address_object} map-to {address_object \| ip} map-type port protocol {any \| tcp \| udp} original-port <1..65535> mapped-port <1..65535> [nat-loopback [nat-1-1-map] [deactivate] \| nat-1-1-map [deactivate] \| deactivate]` | Envelope 5 | `tier2` | `ZyxelNatCmd::cmdIpVirtualServerProfile()` |
| Ch 22 (Virtual Servers) | `ip virtual-server profile_name interface interface_name original-ip {any \| IP \| address_object} map-to {address_object \| ip} map-type ports protocol {any \| tcp \| udp} original-port- begin <1..65535> original-port-end <1..65535> mapped-port-begin <1..65535> [nat-loopback [nat-1-1-map] [deactivate] \| nat-1-1-map [deactivate] \| deactivate]` | Envelope 5 | `tier2` | `ZyxelNatCmd::cmdIpVirtualServerProfile()` |
| Ch 22 (Virtual Servers) | `ip virtual-server profile_name interface interface_name source-ip {any \| IPv4 \| address-object} original-ip {any \| ip \| address_object} map-to {address_object \| ip} map-type any [nat- loopback [nat-1-1-map] [deactivate] \| nat-1-1-map [deactivate] \| deactivate] 190 ip virtual-server rename profile_name profile_name` | Envelope 5 | `tier2` | `ZyxelNatCmd::cmdIpVirtualServerProfile()` |
| Ch 22 (Virtual Servers) | `ip virtual-server {activate \| deactivate} profile_name` | Envelope 5 | `tier2` | `ZyxelNatCmd::cmdIpVirtualServerProfile()` |
| Ch 22 (Virtual Servers) | `load-balance-algorithm {rr\|wrr\|lc\|sh}` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdLoadBalanceAlgorithm()` |
| Ch 22 (Virtual Servers) | `no ip virtual-server load-balancer name` | Envelope 5 | `tier2` | `ZyxelNatCmd::cmdNoIpVirtualServer()` |
| Ch 22 (Virtual Servers) | `no ip virtual-server profile_name` | Envelope 5 | `tier2` | `ZyxelNatCmd::cmdNoIpVirtualServer()` |
| Ch 22 (Virtual Servers) | `no real-server address` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdNoRealServerAddress()` |
| Ch 22 (Virtual Servers) | `persistence granularity netmask` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdPersistenceGranularityNetmask()` |
| Ch 22 (Virtual Servers) | `persistence timeout 1-86400` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdPersistenceTimeout186400()` |
| Ch 22 (Virtual Servers) | `real-server address mapped-port port weight weight [hash hash]` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdRealServerAddressMapped()` |
| Ch 22 (Virtual Servers) | `respmsg url-filter block-page background-color {<rgb(0,0,255)> \| <color name> \| <#00FF00>} respmsg url-filter block-page banner-color {<rgb(0,0,255)> \| <color name> \| <#00FF00>} 522 respmsg url-filter block-page banner-message-color {<rgb(0,0,255)> \| <color name> \| <#00FF00>} respmsg url-filter block-page message-color {<rgb(0,0,255)> \| <color name> \| <#00FF00>} 522 retry 1-99` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdRespmsgUrlFilterBlock()` |
| Ch 22 (Virtual Servers) | `show ip virtual-server [profile_name]` | Envelope 5 | `read` | `ZyxelNatCmd::cmdShowIpVirtualServer()` |
| Ch 22 (Virtual Servers) | `show ip virtual-server load-balance statistics rate name` | Envelope 5 | `read` | `ZyxelNatCmd::cmdShowIpVirtualServer()` |
| Ch 22 (Virtual Servers) | `show ip virtual-server load-balancer name` | Envelope 5 | `read` | `ZyxelNatCmd::cmdShowIpVirtualServer()` |
| Ch 22 (Virtual Servers) | `show ip virtual-server load-balancer name real-server` | Envelope 5 | `read` | `ZyxelNatCmd::cmdShowIpVirtualServer()` |
| Ch 22 (Virtual Servers) | `show ip virtual-server load-balancer statistics name` | Envelope 5 | `read` | `ZyxelNatCmd::cmdShowIpVirtualServer()` |
| Ch 22 (Virtual Servers) | `smtp helo-name name` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdSmtpHeloNameName()` |
| Ch 22 (Virtual Servers) | `status-code {int\|range} .............................................................................................................................` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdStatusCode()` |
| Ch 22 (Virtual Servers) | `virtual-service interface interface_name external-ip {address\|object} external-port port protocol {tcp\|udp}] 197 virtual-service interface interface_name external-ip {address\|object} external-service service` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdVirtualServiceInterfaceInterface()` |
| Ch 23 (HTTP Redirect) | `ip http-redirect activate description` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdIpHttpRedirectActivate()` |
| Ch 23 (HTTP Redirect) | `ip http-redirect deactivate description` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdIpHttpRedirectDeactivate()` |
| Ch 23 (HTTP Redirect) | `ip http-redirect description interface interface_name redirect-to w.x.y.z <1..65535> 201 ip http-redirect description interface interface_name redirect-to w.x.y.z <1..65535> deactivate` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdIpHttpRedirectDescription()` |
| Ch 23 (HTTP Redirect) | `ip http-redirect flush` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdIpHttpRedirectFlush()` |
| Ch 23 (HTTP Redirect) | `no ip http-redirect description` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdNoIpHttpRedirect()` |
| Ch 23 (HTTP Redirect) | `show ip http-redirect [description]` | Envelope 5 | `read` | `ZyxelNatCmd::cmdShowIpHttpRedirect()` |
| Ch 24 (Redirect Service) | `[no] activate` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdActivate()` |
| Ch 24 (Redirect Service) | `[no] interface interface_name` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdInterfaceInterfaceName()` |
| Ch 24 (Redirect Service) | `[no] name profile_name` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdNameProfileName()` |
| Ch 24 (Redirect Service) | `[no] port <1..65535>` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdPort()` |
| Ch 24 (Redirect Service) | `[no] server <fqdn> <w.x.y.z>` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdServer()` |
| Ch 24 (Redirect Service) | `[no] service {http-redirect \| smtp-redirect}` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdService()` |
| Ch 24 (Redirect Service) | `[no] source profile_name` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdSourceProfileName()` |
| Ch 24 (Redirect Service) | `[no] user user_name` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdUserUserName()` |
| Ch 24 (Redirect Service) | `exit` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdExit()` |
| Ch 24 (Redirect Service) | `redirect-service <1..20>` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdRedirectService()` |
| Ch 24 (Redirect Service) | `redirect-service append <1..20>` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdRedirectServiceAppend()` |
| Ch 24 (Redirect Service) | `redirect-service flush` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdRedirectServiceFlush()` |
| Ch 24 (Redirect Service) | `redirect-service insert <1..20>` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdRedirectServiceInsert()` |
| Ch 24 (Redirect Service) | `redirect-service move <1..20> to <1..20>` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdRedirectServiceMoveTo()` |
| Ch 24 (Redirect Service) | `show redirect-service <1..20>` | Envelope 5 | `read` | `ZyxelNatCmd::cmdShowRedirectService()` |
| Ch 25 (ALG.................) | `Router(SIP Signaling Port)# [no] port <1025..65535>` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdRouterSipSignalingPort()` |
| Ch 26 (UPnP................) | `[no] bypass-firewall activate` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdBypassFirewallActivate()` |
| Ch 26 (UPnP................) | `[no] listen-interface interface_name` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdListenInterfaceInterfaceName()` |
| Ch 26 (UPnP................) | `[no] nat-pmp activate` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdNatPmpActivate()` |
| Ch 26 (UPnP................) | `[no] upnp-igd activate` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdUpnpIgdActivate()` |
| Ch 26 (UPnP................) | `link-sticking outgoing interface {interface_name \| all}` | Envelope 5 | `tier1` | `ZyxelNatCmd::cmdLinkStickingOutgoingInterface()` |
| Ch 27 (IP/MAC Binding) | `[no] ip ip-mac-binding interface_name activate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpIpMacBinding()` |
| Ch 27 (IP/MAC Binding) | `[no] ip ip-mac-binding interface_name log-unmatched-ip-mac` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpIpMacBinding()` |
| Ch 27 (IP/MAC Binding) | `show ip ip-mac-binding all` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowIpIpMac()` |
| Ch 27 (IP/MAC Binding) | `show ip ip-mac-binding exempt` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowIpIpMac()` |
| Ch 27 (IP/MAC Binding) | `show ip ip-mac-binding interface_name` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowIpIpMac()` |
| Ch 27 (IP/MAC Binding) | `show ip ip-mac-binding status [interface_name]` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowIpIpMac()` |
| Ch 27 (IP/MAC Binding) | `show ip ip-mac-binding status all` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowIpIpMac()` |
| Ch 28 (Layer 2 Isolation) | `[no] activate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdActivate()` |
| Ch 28 (Layer 2 Isolation) | `[no] activate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdActivate()` |
| Ch 28 (Layer 2 Isolation) | `[no] description description` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDescriptionDescription()` |
| Ch 28 (Layer 2 Isolation) | `[no] interface interface_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdInterfaceInterfaceName()` |
| Ch 28 (Layer 2 Isolation) | `[no] ip-address ip` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdIpAddressIp()` |
| Ch 28 (Layer 2 Isolation) | `l2-isolation` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdL2Isolation()` |
| Ch 28 (Layer 2 Isolation) | `no l2-isolation activate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNoL2IsolationActivate()` |
| Ch 28 (Layer 2 Isolation) | `no l2-isolation white-list activate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNoL2IsolationWhite()` |
| Ch 28 (Layer 2 Isolation) | `no l2-isolation white-list rule_number` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNoL2IsolationWhite()` |
| Ch 28 (Layer 2 Isolation) | `show l2-isolation` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowL2Isolation()` |
| Ch 28 (Layer 2 Isolation) | `show l2-isolation activation` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowL2IsolationActivation()` |
| Ch 28 (Layer 2 Isolation) | `show l2-isolation white-list [rule_number]` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowL2IsolationWhite()` |
| Ch 28 (Layer 2 Isolation) | `show l2-isolation white-list activation` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowL2IsolationWhite()` |
| Ch 28 (Layer 2 Isolation) | `white-list activate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdWhiteListActivate()` |
| Ch 28 (Layer 2 Isolation) | `white-list append` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdWhiteListAppend()` |
| Ch 28 (Layer 2 Isolation) | `white-list flush ..............................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdWhiteListFlush()` |
| Ch 28 (Layer 2 Isolation) | `white-list no activate ........................................................................................................` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdWhiteListNoActivate()` |
| Ch 28 (Layer 2 Isolation) | `white-list rule_number` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdWhiteListRuleNumber()` |
| Ch 29 (Secure Policy) | `<1..32> \| insert <1..32>}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdInsert()` |
| Ch 29 (Secure Policy) | `<1..32> \| move <1..32> to <1..32>}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdMoveTo()` |
| Ch 29 (Secure Policy) | `<profile name>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdGeneric()` |
| Ch 29 (Secure Policy) | `<profile1> <profile2>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdGeneric()` |
| Ch 29 (Secure Policy) | `[no] activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdActivate()` |
| Ch 29 (Secure Policy) | `[no] activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdActivate()` |
| Ch 29 (Secure Policy) | `[no] activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdActivate()` |
| Ch 29 (Secure Policy) | `[no] activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdActivate()` |
| Ch 29 (Secure Policy) | `[no] activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdActivate()` |
| Ch 29 (Secure Policy) | `[no] address address_object` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdAddressAddressObject()` |
| Ch 29 (Secure Policy) | `[no] address6 address6_object` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdAddress6Address6Object()` |
| Ch 29 (Secure Policy) | `[no] ctmatch {dnat \| snat} ....................................................................................................` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdCtmatch()` |
| Ch 29 (Secure Policy) | `[no] description <description>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDescription()` |
| Ch 29 (Secure Policy) | `[no] description description` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDescriptionDescription()` |
| Ch 29 (Secure Policy) | `[no] description description` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDescriptionDescription()` |
| Ch 29 (Secure Policy) | `[no] description description` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDescriptionDescription()` |
| Ch 29 (Secure Policy) | `[no] destinationip <profile name>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDestinationip()` |
| Ch 29 (Secure Policy) | `[no] destinationip address_object` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDestinationipAddressObject()` |
| Ch 29 (Secure Policy) | `[no] destinationip6 address_object` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDestinationip6AddressObject()` |
| Ch 29 (Secure Policy) | `[no] firewall-output activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdFirewallOutputActivate()` |
| Ch 29 (Secure Policy) | `[no] firewall-output default-rule establish enable` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdFirewallOutputDefaultRule()` |
| Ch 29 (Secure Policy) | `[no] flood-detection {tcp-flood \| udp-flood \| icmp-flood \| igmp-flood} {activate \| log [alert] \| block}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdFloodDetection()` |
| Ch 29 (Secure Policy) | `[no] from zone_object` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdFromZoneObject()` |
| Ch 29 (Secure Policy) | `[no] icmp-decoder {bad-icmp-l4-size \| icmp-fragment \| icmp-smurf} activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdIcmpDecoderActivate()` |
| Ch 29 (Secure Policy) | `[no] idp anomaly white-list activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdIdpAnomalyWhiteList()` |
| Ch 29 (Secure Policy) | `[no] ip-decoder {ip-spoof \| ip-teardrop} action {drop \| reject-sender \| reject-receiver \| reject-both}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdIpDecoderAction()` |
| Ch 29 (Secure Policy) | `[no] ip-decoder {ip-spoof \| ip-teardrop} activate .............................................................................` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdIpDecoderActivate()` |
| Ch 29 (Secure Policy) | `[no] ip-decoder {ip-spoof \| ip-teardrop} log ..................................................................................` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdIpDecoderLog()` |
| Ch 29 (Secure Policy) | `[no] limit <0...40000>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdLimit()` |
| Ch 29 (Secure Policy) | `[no] limit <0...40000>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdLimit()` |
| Ch 29 (Secure Policy) | `[no] log [alert]` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdLogAlert()` |
| Ch 29 (Secure Policy) | `[no] log [alert]` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdLogAlert()` |
| Ch 29 (Secure Policy) | `[no] scan-detection {tcp-xxx} {activate \| log [alert] \| block}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdScanDetection()` |
| Ch 29 (Secure Policy) | `[no] scan-detection {udp-portscan} {activate \| log [alert] \| block}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdScanDetection()` |
| Ch 29 (Secure Policy) | `[no] schedule schedule_object` | Envelope 6 | `tier2` | `ZyxelFirewallCmd::cmdScheduleScheduleObject()` |
| Ch 29 (Secure Policy) | `[no] secure-policy activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicyActivate()` |
| Ch 29 (Secure Policy) | `[no] secure-policy asymmetrical-route activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicyAsymmetricalRoute()` |
| Ch 29 (Secure Policy) | `[no] secure-policy6 activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicy6Activate()` |
| Ch 29 (Secure Policy) | `[no] secure-policy6 asymmetrical-route activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicy6AsymmetricalRoute()` |
| Ch 29 (Secure Policy) | `[no] service <profile name> ................................................................................................................` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdService()` |
| Ch 29 (Secure Policy) | `[no] service service_name` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdServiceServiceName()` |
| Ch 29 (Secure Policy) | `[no] session-limit activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionLimitActivate()` |
| Ch 29 (Secure Policy) | `[no] session-limit6 activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionLimit6Activate()` |
| Ch 29 (Secure Policy) | `[no] sourceip address_object` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSourceipAddressObject()` |
| Ch 29 (Secure Policy) | `[no] sourceip6 address_object` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSourceip6AddressObject()` |
| Ch 29 (Secure Policy) | `[no] sourceport {tcp\|udp} {eq <1..65535>\|range <1..65535> <1..65535>}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSourceport()` |
| Ch 29 (Secure Policy) | `[no] tcp-decoder {tcp-xxx} action {drop \| reject- sender \| reject-receiver \| reject-both}}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdTcpDecoderAction()` |
| Ch 29 (Secure Policy) | `[no] tcp-decoder {tcp-xxx} activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdTcpDecoderActivate()` |
| Ch 29 (Secure Policy) | `[no] to {zone_object\|ZyWALL}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdTo()` |
| Ch 29 (Secure Policy) | `[no] tointerface <interface name>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdTointerface()` |
| Ch 29 (Secure Policy) | `[no] udp-decoder {bad-udp-l4-size \| udp-land \| udp-smurf} activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdUdpDecoderActivate()` |
| Ch 29 (Secure Policy) | `[no] user user_name` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdUserUserName()` |
| Ch 29 (Secure Policy) | `[no] user user_name` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdUserUserName()` |
| Ch 29 (Secure Policy) | `[no] user user_name` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdUserUserName()` |
| Ch 29 (Secure Policy) | `action {allow\|deny\|reject}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdAction()` |
| Ch 29 (Secure Policy) | `action {allow\|deny\|reject}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdAction()` |
| Ch 29 (Secure Policy) | `activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdActivate()` |
| Ch 29 (Secure Policy) | `base {all \| everything \| none} ................................................................................................` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdBase()` |
| Ch 29 (Secure Policy) | `bind profile` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdBindProfile()` |
| Ch 29 (Secure Policy) | `deactivate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeactivate()` |
| Ch 29 (Secure Policy) | `description description` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDescriptionDescription()` |
| Ch 29 (Secure Policy) | `exit` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdExit()` |
| Ch 29 (Secure Policy) | `exit` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdExit()` |
| Ch 29 (Secure Policy) | `exit` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdExit()` |
| Ch 29 (Secure Policy) | `firewall icsa {icmp-destroy-session} {enable \| disable}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdFirewallIcsa()` |
| Ch 29 (Secure Policy) | `firewall-output append` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdFirewallOutputAppend()` |
| Ch 29 (Secure Policy) | `firewall-output delete rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdFirewallOutputDeleteRule()` |
| Ch 29 (Secure Policy) | `firewall-output insert rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdFirewallOutputInsertRule()` |
| Ch 29 (Secure Policy) | `firewall-output move rule_number to rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdFirewallOutputMoveRule()` |
| Ch 29 (Secure Policy) | `firewall-output rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdFirewallOutputRuleNumber()` |
| Ch 29 (Secure Policy) | `flood-detection block-period <1..3600>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdFloodDetectionBlockPeriod()` |
| Ch 29 (Secure Policy) | `from-zone zone_rule ...........................................................................................................` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdFromZoneZoneRule()` |
| Ch 29 (Secure Policy) | `icmp-decoder {bad-icmp-l4-size \| icmp-fragment \| icmp-smurf} action {drop \| reject-sender \| reject-receiver \| reject- both}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdIcmpDecoderAction()` |
| Ch 29 (Secure Policy) | `icmp-decoder {bad-icmp-l4-size \| icmp-fragment \| icmp-smurf} log [alert]` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdIcmpDecoderLogAlert()` |
| Ch 29 (Secure Policy) | `idp anomaly adp-profile [base {all \| everything \| none}]` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdIdpAnomalyAdpProfile()` |
| Ch 29 (Secure Policy) | `idp anomaly rule {append \|` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdIdpAnomalyRuleAppend()` |
| Ch 29 (Secure Policy) | `idp anomaly rule {delete` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdIdpAnomalyRuleDelete()` |
| Ch 29 (Secure Policy) | `idp anomaly white-list rename rule-name new-rule-name` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdIdpAnomalyWhiteList()` |
| Ch 29 (Secure Policy) | `idp anomaly white-list rule-name` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdIdpAnomalyWhiteList()` |
| Ch 29 (Secure Policy) | `idp rename anomaly` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdIdpRenameAnomaly()` |
| Ch 29 (Secure Policy) | `no bind .......................................................................................................................` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdNoBind()` |
| Ch 29 (Secure Policy) | `no description ................................................................................................................` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdNoDescription()` |
| Ch 29 (Secure Policy) | `no icmp-decoder {bad-icmp-l4-size icmp-fragment \| icmp-smurf} log` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdNoIcmpDecoderLog()` |
| Ch 29 (Secure Policy) | `no icmp-decoder {bad-icmp-l4-size \| icmp-fragment \| icmp-smurf} action` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdNoIcmpDecoderAction()` |
| Ch 29 (Secure Policy) | `no idp anomaly <profile3>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdNoIdpAnomaly()` |
| Ch 29 (Secure Policy) | `no idp anomaly rule <1..32>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdNoIdpAnomalyRule()` |
| Ch 29 (Secure Policy) | `no idp anomaly white-list rule-name` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdNoIdpAnomalyWhite()` |
| Ch 29 (Secure Policy) | `no scan-detection sensitivity .................................................................................................` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdNoScanDetectionSensitivity()` |
| Ch 29 (Secure Policy) | `no tcp-decoder {tcp-xxx} log` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdNoTcpDecoderLog()` |
| Ch 29 (Secure Policy) | `no udp-decoder {bad-udp-l4-size \| udp-land \| udp-smurf} action` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdNoUdpDecoderAction()` |
| Ch 29 (Secure Policy) | `scan-detection block-period <1..3600>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdScanDetectionBlockPeriod()` |
| Ch 29 (Secure Policy) | `scan-detection sensitivity {low \| medium \| high} ..............................................................................` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdScanDetectionSensitivity()` |
| Ch 29 (Secure Policy) | `secure-policy <profile name>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicy()` |
| Ch 29 (Secure Policy) | `secure-policy activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicyActivate()` |
| Ch 29 (Secure Policy) | `secure-policy append` | Envelope 6 | `tier2` | `ZyxelFirewallCmd::cmdSecurePolicyAppend()` |
| Ch 29 (Secure Policy) | `secure-policy backup activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicyBackupActivate()` |
| Ch 29 (Secure Policy) | `secure-policy default-rule action {allow \| deny \| reject} { no log \| log [alert] } 219 secure-policy delete rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicyDefaultRule()` |
| Ch 29 (Secure Policy) | `secure-policy flush` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicyFlush()` |
| Ch 29 (Secure Policy) | `secure-policy insert rule_number` | Envelope 6 | `tier2` | `ZyxelFirewallCmd::cmdSecurePolicyInsertRule()` |
| Ch 29 (Secure Policy) | `secure-policy move rule_number to rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicyMoveRule()` |
| Ch 29 (Secure Policy) | `secure-policy rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicyRuleNumber()` |
| Ch 29 (Secure Policy) | `secure-policy zone_object {zone_object\|ZyWALL} append` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicyZoneObject()` |
| Ch 29 (Secure Policy) | `secure-policy zone_object {zone_object\|ZyWALL} delete <1..5000>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicyZoneObject()` |
| Ch 29 (Secure Policy) | `secure-policy zone_object {zone_object\|ZyWALL} flush` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicyZoneObject()` |
| Ch 29 (Secure Policy) | `secure-policy zone_object {zone_object\|ZyWALL} insert rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicyZoneObject()` |
| Ch 29 (Secure Policy) | `secure-policy zone_object {zone_object\|ZyWALL} move rule_number to rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicyZoneObject()` |
| Ch 29 (Secure Policy) | `secure-policy zone_object {zone_object\|ZyWALL} rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicyZoneObject()` |
| Ch 29 (Secure Policy) | `secure-policy-style advance all-inspect-by-policy` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicyStyleAdvance()` |
| Ch 29 (Secure Policy) | `secure-policy-style {general \| advance}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicyStyle()` |
| Ch 29 (Secure Policy) | `secure-policy6 append` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicy6Append()` |
| Ch 29 (Secure Policy) | `secure-policy6 default-rule action {allow \| deny \| reject} { no log \| log [alert] } 221 secure-policy6 delete rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicy6DefaultRule()` |
| Ch 29 (Secure Policy) | `secure-policy6 flush` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicy6Flush()` |
| Ch 29 (Secure Policy) | `secure-policy6 insert rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicy6InsertRule()` |
| Ch 29 (Secure Policy) | `secure-policy6 move rule_number to rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicy6MoveRule()` |
| Ch 29 (Secure Policy) | `secure-policy6 rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicy6RuleNumber()` |
| Ch 29 (Secure Policy) | `secure-policy6 zone_object {zone_object\|ZyWALL} append` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicy6ZoneObject()` |
| Ch 29 (Secure Policy) | `secure-policy6 zone_object {zone_object\|ZyWALL} delete <1..5000>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicy6ZoneObject()` |
| Ch 29 (Secure Policy) | `secure-policy6 zone_object {zone_object\|ZyWALL} flush` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicy6ZoneObject()` |
| Ch 29 (Secure Policy) | `secure-policy6 zone_object {zone_object\|ZyWALL} insert rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicy6ZoneObject()` |
| Ch 29 (Secure Policy) | `secure-policy6 zone_object {zone_object\|ZyWALL} move rule_number to rule_number . 220 secure-policy6 zone_object {zone_object\|ZyWALL} rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSecurePolicy6ZoneObject()` |
| Ch 29 (Secure Policy) | `session-limit append` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionLimitAppend()` |
| Ch 29 (Secure Policy) | `session-limit delete rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionLimitDeleteRule()` |
| Ch 29 (Secure Policy) | `session-limit flush` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionLimitFlush()` |
| Ch 29 (Secure Policy) | `session-limit insert rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionLimitInsertRule()` |
| Ch 29 (Secure Policy) | `session-limit limit <0...20000>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionLimitLimit()` |
| Ch 29 (Secure Policy) | `session-limit move rule_number to rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionLimitMoveRule()` |
| Ch 29 (Secure Policy) | `session-limit rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionLimitRuleNumber()` |
| Ch 29 (Secure Policy) | `session-limit6 append` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionLimit6Append()` |
| Ch 29 (Secure Policy) | `session-limit6 delete rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionLimit6DeleteRule()` |
| Ch 29 (Secure Policy) | `session-limit6 flush` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionLimit6Flush()` |
| Ch 29 (Secure Policy) | `session-limit6 insert rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionLimit6InsertRule()` |
| Ch 29 (Secure Policy) | `session-limit6 limit <0...20000>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionLimit6Limit()` |
| Ch 29 (Secure Policy) | `session-limit6 move rule_number to rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionLimit6MoveRule()` |
| Ch 29 (Secure Policy) | `session-limit6 rule_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionLimit6RuleNumber()` |
| Ch 29 (Secure Policy) | `session-status-update alg {active\|inactive}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionStatusUpdateAlg()` |
| Ch 29 (Secure Policy) | `session-status-update reply-time <5..300>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSessionStatusUpdateReply()` |
| Ch 29 (Secure Policy) | `show firewall icsa status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowFirewallIcsaStatus()` |
| Ch 29 (Secure Policy) | `show firewall-output` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowFirewallOutput()` |
| Ch 29 (Secure Policy) | `show firewall-output status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowFirewallOutputStatus()` |
| Ch 29 (Secure Policy) | `show idp anomaly adp-profile ip-decoder all details` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowIdpAnomalyAdp()` |
| Ch 29 (Secure Policy) | `show idp anomaly adp-profile ip-decoder {ip-spoof \| ip-teardrop} details` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowIdpAnomalyAdp()` |
| Ch 29 (Secure Policy) | `show idp anomaly base profile` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowIdpAnomalyBase()` |
| Ch 29 (Secure Policy) | `show idp anomaly profile flood-detection [all details]` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowIdpAnomalyProfile()` |
| Ch 29 (Secure Policy) | `show idp anomaly profile flood-detection {tcp-flood \| udp-flood \| icmp-flood \| icmp-flood} de- tails` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowIdpAnomalyProfile()` |
| Ch 29 (Secure Policy) | `show idp anomaly profile icmp-decoder all details` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowIdpAnomalyProfile()` |
| Ch 29 (Secure Policy) | `show idp anomaly profile icmp-decoder {bad-icmp-l4-size \| icmp-smurf} details` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowIdpAnomalyProfile()` |
| Ch 29 (Secure Policy) | `show idp anomaly profile scan-detection [all details]` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowIdpAnomalyProfile()` |
| Ch 29 (Secure Policy) | `show idp anomaly profile scan-detection {tcp-portscan \| tcp-portscan-syn \| tcp-portsweep \| tcp- portscan-fin} details` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowIdpAnomalyProfile()` |
| Ch 29 (Secure Policy) | `show idp anomaly profile scan-detection {udp-portscan} details` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowIdpAnomalyProfile()` |
| Ch 29 (Secure Policy) | `show idp anomaly profile tcp-decoder {bad-tcp-flag \| bad-tcp-l4-size \| tcp-land} details 237 show idp anomaly profile tcp-decoder all details` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowIdpAnomalyProfile()` |
| Ch 29 (Secure Policy) | `show idp anomaly profile udp-decoder {bad-udp-l4-size \| udp-land \| udp-smurf} details 237 show idp anomaly profile udp-decoder all details` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowIdpAnomalyProfile()` |
| Ch 29 (Secure Policy) | `show idp anomaly profiles` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowIdpAnomalyProfiles()` |
| Ch 29 (Secure Policy) | `show idp anomaly rules` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowIdpAnomalyRules()` |
| Ch 29 (Secure Policy) | `show idp anomaly rules` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowIdpAnomalyRules()` |
| Ch 29 (Secure Policy) | `show idp anomaly white-list {all \| rule-name}` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowIdpAnomalyWhite()` |
| Ch 29 (Secure Policy) | `show secure-policy` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicy()` |
| Ch 29 (Secure Policy) | `show secure-policy _check-exposed-srv` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicyCheck()` |
| Ch 29 (Secure Policy) | `show secure-policy any ZyWALL` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicyAny()` |
| Ch 29 (Secure Policy) | `show secure-policy backup status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicyBackup()` |
| Ch 29 (Secure Policy) | `show secure-policy block_rules` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicyBlock()` |
| Ch 29 (Secure Policy) | `show secure-policy filter from zone_object to zone_object srcip <ip-address> dstip <ip> service {any \| tcp \| udp \| icmp \| gre \| esp \| user-defined} port-number user user_name sch sched- ule_object` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicyFilter()` |
| Ch 29 (Secure Policy) | `show secure-policy rule_number` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicyRule()` |
| Ch 29 (Secure Policy) | `show secure-policy status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicyStatus()` |
| Ch 29 (Secure Policy) | `show secure-policy zone_object {zone_object\|ZyWALL}` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicyZone()` |
| Ch 29 (Secure Policy) | `show secure-policy zone_object {zone_object\|ZyWALL} rule_number` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicyZone()` |
| Ch 29 (Secure Policy) | `show secure-policy-style status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicyStyle()` |
| Ch 29 (Secure Policy) | `show secure-policy6` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicy6()` |
| Ch 29 (Secure Policy) | `show secure-policy6 any ZyWALL` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicy6Any()` |
| Ch 29 (Secure Policy) | `show secure-policy6 block_rules` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicy6Block()` |
| Ch 29 (Secure Policy) | `show secure-policy6 filter from zone_object to zone_object srcip6 <ip-address> dstip6 <ip> ser- vice {any \| tcp \| udp \| icmp \| gre \| esp \| user-defined} port-number user user_name sch schedule_object` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicy6Filter()` |
| Ch 29 (Secure Policy) | `show secure-policy6 rule_number` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicy6Rule()` |
| Ch 29 (Secure Policy) | `show secure-policy6 status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicy6Status()` |
| Ch 29 (Secure Policy) | `show secure-policy6 zone_object {zone_object\|ZyWALL}` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicy6Zone()` |
| Ch 29 (Secure Policy) | `show secure-policy6 zone_object {zone_object\|ZyWALL} rule_number` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurePolicy6Zone()` |
| Ch 29 (Secure Policy) | `show security-service inspect status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSecurityServiceInspect()` |
| Ch 29 (Secure Policy) | `show session-limit` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSessionLimit()` |
| Ch 29 (Secure Policy) | `show session-limit begin rule_number end rule_number` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSessionLimitBegin()` |
| Ch 29 (Secure Policy) | `show session-limit rule_number` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSessionLimitRule()` |
| Ch 29 (Secure Policy) | `show session-limit status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSessionLimitStatus()` |
| Ch 29 (Secure Policy) | `show session-limit6` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSessionLimit6()` |
| Ch 29 (Secure Policy) | `show session-limit6 begin rule_number end rule_number` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSessionLimit6Begin()` |
| Ch 29 (Secure Policy) | `show session-limit6 rule_number` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSessionLimit6Rule()` |
| Ch 29 (Secure Policy) | `show session-limit6 status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSessionLimit6Status()` |
| Ch 29 (Secure Policy) | `show session-status-update reply-time` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowSessionStatusUpdate()` |
| Ch 29 (Secure Policy) | `source {src-ipv4-obj \| any} destination {dst-ipv4-obj \| any} service {service_obj \| any}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdSourceDestinationService()` |
| Ch 29 (Secure Policy) | `tcp-decoder {tcp-xxx} log [alert]` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdTcpDecoderLogAlert()` |
| Ch 29 (Secure Policy) | `udp-decoder {bad-udp-l4-size \| udp-land \| udp-smurf} action {drop \| reject-sender \| reject-receiver \| reject-both} udp-decoder {bad-udp-l4-size \| udp-land \| udp-smurf} log [alert]` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdUdpDecoderActionUdp()` |
| Ch 30 (Cloud CNM) | `(no) cnm-agent enable-cnm-id` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentEnableCnm()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent acs password tr069_acs_password` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentAcsPassword()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent acs username tr069_acs_username` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentAcsUsername()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentActivate()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent authentication enable` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentAuthenticationEnable()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent auto-get-acs activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentAutoGet()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent auto-get-acs activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentAutoGet()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent cnm-id <ID> ....................................................................................................` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentCnmId()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent encrypted-xmpp-password` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentEncryptedXmpp()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent manager {https_url\|http_url} ...................................................................................` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentManager()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent password` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentPassword()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent periodic-inform activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentPeriodicInform()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent periodic-inform interval <10…86400>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentPeriodicInform()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent trigger-inform <0..8640>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentTriggerInform()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent username` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentUsername()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent vantage certificate tr069_cert_file_name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentVantageCertificate()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent xmpp-domain xmpp_domain` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentXmppDomain()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent xmpp-host xmpp_host` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentXmppHost()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent xmpp-password xmpp_password` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentXmppPassword()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent xmpp-resource xmpp_resource` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentXmppResource()` |
| Ch 30 (Cloud CNM) | `[no] cnm-agent xmpp-username xmpp_username` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentXmppUsername()` |
| Ch 30 (Cloud CNM) | `[no] monitor-mode` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdMonitorMode()` |
| Ch 30 (Cloud CNM) | `[no] secumanager activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecumanagerActivate()` |
| Ch 30 (Cloud CNM) | `cnm-agent server-type [vantage \| tr069]` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCnmAgentServerType()` |
| Ch 30 (Cloud CNM) | `monitor-mode id <organization-id>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdMonitorModeId()` |
| Ch 30 (Cloud CNM) | `secu-reporter activate {no \| yes}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterActivate()` |
| Ch 30 (Cloud CNM) | `secu-reporter adp {activate\|deactivate}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterAdp()` |
| Ch 30 (Cloud CNM) | `secu-reporter anti-botnet {activate\|deactivate}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterAntiBotnet()` |
| Ch 30 (Cloud CNM) | `secu-reporter anti-spam {activate\|deactivate}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterAntiSpam()` |
| Ch 30 (Cloud CNM) | `secu-reporter anti-virus {activate\|deactivate}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterAntiVirus()` |
| Ch 30 (Cloud CNM) | `secu-reporter ap-managed {activate\|deactivate}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterApManaged()` |
| Ch 30 (Cloud CNM) | `secu-reporter app-patrol {activate\|deactivate}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterAppPatrol()` |
| Ch 30 (Cloud CNM) | `secu-reporter content-filter {activate\|deactivate}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterContentFilter()` |
| Ch 30 (Cloud CNM) | `secu-reporter idp {activate\|deactivate}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterIdp()` |
| Ch 30 (Cloud CNM) | `secu-reporter interface-statistics {activate\|deactivate}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterInterfaceStatistics()` |
| Ch 30 (Cloud CNM) | `secu-reporter on-cloud-config {dns-threat-filter\| ip-reputation\| url-threat-filter} update secu-reporter reputation-filter {activate\|deactivate}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterOnCloud()` |
| Ch 30 (Cloud CNM) | `secu-reporter sandbox {activate\|deactivate}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterSandbox()` |
| Ch 30 (Cloud CNM) | `secu-reporter traffic-log {activate \| deactivate}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterTrafficLog()` |
| Ch 30 (Cloud CNM) | `secu-reporter traffic-log {activate\|deactivate}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterTrafficLog()` |
| Ch 30 (Cloud CNM) | `secu-reporter upload-filesize <1..10>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterUploadFilesize()` |
| Ch 30 (Cloud CNM) | `secu-reporter upload-interval <60..600>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterUploadInterval()` |
| Ch 30 (Cloud CNM) | `secu-reporter vpn {activate\|deactivate}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecuReporterVpn()` |
| Ch 30 (Cloud CNM) | `secumanager server {IPv4\| FQDN} port <1…65535> ................................................................................` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecumanagerServerPort()` |
| Ch 30 (Cloud CNM) | `secumanager server-ca {default\| CERT_NAME}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecumanagerServerCa()` |
| Ch 30 (Cloud CNM) | `show cnm-agent configuration` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowCnmAgentConfiguration()` |
| Ch 30 (Cloud CNM) | `show monitor-mode` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowMonitorMode()` |
| Ch 30 (Cloud CNM) | `show secu-reporter category status` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowSecuReporterCategory()` |
| Ch 30 (Cloud CNM) | `show secu-reporter status` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowSecuReporterStatus()` |
| Ch 30 (Cloud CNM) | `show secumanager status` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowSecumanagerStatus()` |
| Ch 30 (Cloud CNM) | `show service-register status secu-reporter` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowServiceRegisterStatus()` |
| Ch 30 (Cloud CNM) | `show {address-object \| address6-object \| service-object \| schedule-object} [object_name] 459 show {ip-reputation\| dns-filter\| threat-website} sr-allow-list` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowObjectName459()` |
| Ch 31 (Web Authentication) | `[no] 8021x-sso` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmd8021xSso()` |
| Ch 31 (Web Authentication) | `[no] activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdActivate()` |
| Ch 31 (Web Authentication) | `[no] authentication {force \| required} ........................................................................................` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAuthentication()` |
| Ch 31 (Web Authentication) | `[no] description description` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdDescriptionDescription()` |
| Ch 31 (Web Authentication) | `[no] destination {address_object \| group_name}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdDestination()` |
| Ch 31 (Web Authentication) | `[no] entry {IPv4 \| IPv4_CIDR \| IPv4_RANGE \| IPv6 \| IPv6_PREFIX \| IPv6_RANGE \| SSL_INSPECTION_WILDCARD_CNAME} [no] error-url url` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdEntryErrorUrlUrl()` |
| Ch 31 (Web Authentication) | `[no] fbwifi activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdFbwifiActivate()` |
| Ch 31 (Web Authentication) | `[no] fbwifi idle-detection` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdFbwifiIdleDetection()` |
| Ch 31 (Web Authentication) | `[no] force ....................................................................................................................` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdForce()` |
| Ch 31 (Web Authentication) | `[no] google-auth` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdGoogleAuth()` |
| Ch 31 (Web Authentication) | `[no] internal-welcome-url url` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdInternalWelcomeUrlUrl()` |
| Ch 31 (Web Authentication) | `[no] ip <w.x.y.z>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdIp()` |
| Ch 31 (Web Authentication) | `[no] login-url url` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdLoginUrlUrl()` |
| Ch 31 (Web Authentication) | `[no] logout-ip ipv4_address` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdLogoutIpIpv4Address()` |
| Ch 31 (Web Authentication) | `[no] logout-url url` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdLogoutUrlUrl()` |
| Ch 31 (Web Authentication) | `[no] schedule schedule_name` | Envelope 9 | `tier2` | `ZyxelAuthCmd::cmdScheduleScheduleName()` |
| Ch 31 (Web Authentication) | `[no] session-url url` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSessionUrlUrl()` |
| Ch 31 (Web Authentication) | `[no] source {address_object \| group_name}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSource()` |
| Ch 31 (Web Authentication) | `[no] sso ......................................................................................................................` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSso()` |
| Ch 31 (Web Authentication) | `[no] terms-of-service` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTermsOfService()` |
| Ch 31 (Web Authentication) | `[no] web-auth activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWebAuthActivate()` |
| Ch 31 (Web Authentication) | `[no] web-auth redirect-fqdn host_str` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWebAuthRedirectFqdn()` |
| Ch 31 (Web Authentication) | `[no] welcome-url url` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWelcomeUrlUrl()` |
| Ch 31 (Web Authentication) | `exit` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdExit()` |
| Ch 31 (Web Authentication) | `fbwifi idle-detection timeout <1..60>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdFbwifiIdleDetectionTimeout()` |
| Ch 31 (Web Authentication) | `fbwifi reset-fbpage` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdFbwifiResetFbpage()` |
| Ch 31 (Web Authentication) | `fbwifi security {high \|low}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdFbwifiSecurity()` |
| Ch 31 (Web Authentication) | `interface interface_name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdInterfaceInterfaceName()` |
| Ch 31 (Web Authentication) | `no web-auth redirect-parameter` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdNoWebAuthRedirect()` |
| Ch 31 (Web Authentication) | `router(config-sso-primary)#` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdRouterConfigSsoPrimary()` |
| Ch 31 (Web Authentication) | `router(config-sso-primary)#` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdRouterConfigSsoPrimary()` |
| Ch 31 (Web Authentication) | `router(config-sso-secondary)#` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdRouterConfigSsoSecondary()` |
| Ch 31 (Web Authentication) | `router(config-sso-secondary)# [no] port <1025..65535>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdRouterConfigSsoSecondary()` |
| Ch 31 (Web Authentication) | `session-page {activate \| deactivate} ..........................................................................................` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSessionPage()` |
| Ch 31 (Web Authentication) | `show fbwifi activate` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowFbwifiActivate()` |
| Ch 31 (Web Authentication) | `show fbwifi service-register status` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowFbwifiServiceRegister()` |
| Ch 31 (Web Authentication) | `show fbwifi status` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowFbwifiStatus()` |
| Ch 31 (Web Authentication) | `show sso agent` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowSsoAgent()` |
| Ch 31 (Web Authentication) | `show sso agent primary` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowSsoAgentPrimary()` |
| Ch 31 (Web Authentication) | `show sso agent secondary` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowSsoAgentSecondary()` |
| Ch 31 (Web Authentication) | `show sso agent status` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowSsoAgentStatus()` |
| Ch 31 (Web Authentication) | `show sso port` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowSsoPort()` |
| Ch 31 (Web Authentication) | `show sso presharekey` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowSsoPresharekey()` |
| Ch 31 (Web Authentication) | `show sso { agent \| port \| presharekey}` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowSso()` |
| Ch 31 (Web Authentication) | `show web-auth activation` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowWebAuthActivation()` |
| Ch 31 (Web Authentication) | `show web-auth default-rule` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowWebAuthDefault()` |
| Ch 31 (Web Authentication) | `show web-auth exceptional-service` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowWebAuthExceptional()` |
| Ch 31 (Web Authentication) | `show web-auth method` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowWebAuthMethod()` |
| Ch 31 (Web Authentication) | `show web-auth policy {<1..1024> \| all}` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowWebAuthPolicy()` |
| Ch 31 (Web Authentication) | `show web-auth portal status` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowWebAuthPortal()` |
| Ch 31 (Web Authentication) | `show web-auth redirect-fqdn` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowWebAuthRedirect()` |
| Ch 31 (Web Authentication) | `show web-auth redirect-parameter` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowWebAuthRedirect()` |
| Ch 31 (Web Authentication) | `show web-auth status` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowWebAuthStatus()` |
| Ch 31 (Web Authentication) | `sso agent primary` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSsoAgentPrimary()` |
| Ch 31 (Web Authentication) | `sso agent secondary` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSsoAgentSecondary()` |
| Ch 31 (Web Authentication) | `sso encrypted-presharekey <ciphertext>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSsoEncryptedPresharekey()` |
| Ch 31 (Web Authentication) | `sso presharekey <preshared key>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSsoPresharekey()` |
| Ch 31 (Web Authentication) | `sso_port <1025..65535>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSsoPort()` |
| Ch 31 (Web Authentication) | `type {external \| internal}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdType()` |
| Ch 31 (Web Authentication) | `web-auth [no] exceptional-service service_name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWebAuthExceptionalService()` |
| Ch 31 (Web Authentication) | `web-auth default-rule authentication {required \| unnecessary} {no log \| log [alert]} 253 web-auth google-auth valid-time <1..5>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWebAuthDefaultRule()` |
| Ch 31 (Web Authentication) | `web-auth login setting` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWebAuthLoginSetting()` |
| Ch 31 (Web Authentication) | `web-auth method portal` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWebAuthMethodPortal()` |
| Ch 31 (Web Authentication) | `web-auth policy <1..1024>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWebAuthPolicy()` |
| Ch 31 (Web Authentication) | `web-auth policy append` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWebAuthPolicyAppend()` |
| Ch 31 (Web Authentication) | `web-auth policy delete <1..1024>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWebAuthPolicyDelete()` |
| Ch 31 (Web Authentication) | `web-auth policy flush` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWebAuthPolicyFlush()` |
| Ch 31 (Web Authentication) | `web-auth policy insert <1..1024>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWebAuthPolicyInsert()` |
| Ch 31 (Web Authentication) | `web-auth policy move <1..1024> to <1..1024>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWebAuthPolicyMove()` |
| Ch 31 (Web Authentication) | `web-auth redirect-parameter` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWebAuthRedirectParameter()` |
| Ch 31 (Web Authentication) | `web-auth web-portal` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWebAuthWebPortal()` |
| Ch 32 (Hotspot) | `[no] activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdActivate()` |
| Ch 32 (Hotspot) | `[no] activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdActivate()` |
| Ch 32 (Hotspot) | `[no] activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdActivate()` |
| Ch 32 (Hotspot) | `[no] advertisement activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAdvertisementActivate()` |
| Ch 32 (Hotspot) | `[no] advertisement name description url url` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAdvertisementNameDescriptionUrl()` |
| Ch 32 (Hotspot) | `[no] bandwidth activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBandwidthActivate()` |
| Ch 32 (Hotspot) | `[no] billing discount activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBillingDiscountActivate()` |
| Ch 32 (Hotspot) | `[no] billing discount unit <2..10> price price` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBillingDiscountUnitPrice()` |
| Ch 32 (Hotspot) | `[no] billing profile profile_name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBillingProfileProfileName()` |
| Ch 32 (Hotspot) | `[no] billing replenish activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBillingReplenishActivate()` |
| Ch 32 (Hotspot) | `[no] billing tax-rate activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBillingTaxRateActivate()` |
| Ch 32 (Hotspot) | `[no] billing wlan-ssid-profile profile_name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBillingWlanSsidProfile()` |
| Ch 32 (Hotspot) | `[no] domain-name walled_garden_fqdn` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdDomainNameWalledGarden()` |
| Ch 32 (Hotspot) | `[no] free-time activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdFreeTimeActivate()` |
| Ch 32 (Hotspot) | `[no] free-time auto-login` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdFreeTimeAutoLogin()` |
| Ch 32 (Hotspot) | `[no] free-time deliver-method onscreen` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdFreeTimeDeliverMethod()` |
| Ch 32 (Hotspot) | `[no] free-time deliver-method sms` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdFreeTimeDeliverMethod()` |
| Ch 32 (Hotspot) | `[no] free-time maximum-allowed-account <1..2000>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdFreeTimeMaximumAllowed()` |
| Ch 32 (Hotspot) | `[no] free-time maximum-register-number <1..5>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdFreeTimeMaximumRegister()` |
| Ch 32 (Hotspot) | `[no] free-time reset-register hh:mm` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdFreeTimeResetRegister()` |
| Ch 32 (Hotspot) | `[no] free-time time-period time_period` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdFreeTimeTimePeriod()` |
| Ch 32 (Hotspot) | `[no] hidden` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdHidden()` |
| Ch 32 (Hotspot) | `[no] interface interface_name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdInterfaceInterfaceName()` |
| Ch 32 (Hotspot) | `[no] ip ipnp activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdIpIpnpActivate()` |
| Ch 32 (Hotspot) | `[no] ip-address <w.x.y.z>/<1..32>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdIpAddress()` |
| Ch 32 (Hotspot) | `[no] name description` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdNameDescription()` |
| Ch 32 (Hotspot) | `[no] name description` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdNameDescription()` |
| Ch 32 (Hotspot) | `[no] payment-service activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceActivate()` |
| Ch 32 (Hotspot) | `[no] payment-service mobile-page-customization` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceMobilePage()` |
| Ch 32 (Hotspot) | `[no] payment-service page-customization` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServicePageCustomization()` |
| Ch 32 (Hotspot) | `[no] printer-manager activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPrinterManagerActivate()` |
| Ch 32 (Hotspot) | `[no] printer-manager encrypt activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPrinterManagerEncryptActivate()` |
| Ch 32 (Hotspot) | `[no] printer-manager printer <1..10>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPrinterManagerPrinter()` |
| Ch 32 (Hotspot) | `[no] type {domain\|ip}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdType()` |
| Ch 32 (Hotspot) | `[no] url url` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUrlUrl()` |
| Ch 32 (Hotspot) | `[no] walled-garden activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWalledGardenActivate()` |
| Ch 32 (Hotspot) | `[no] walled-garden rule <1..50>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWalledGardenRule()` |
| Ch 32 (Hotspot) | `activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdActivate()` |
| Ch 32 (Hotspot) | `advertisement flush` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAdvertisementFlush()` |
| Ch 32 (Hotspot) | `advertisement rename description_old description_new` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAdvertisementRenameDescriptionOld()` |
| Ch 32 (Hotspot) | `bandwidth {upload \| download} <0..1048576> priority <1..7>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBandwidthPriority()` |
| Ch 32 (Hotspot) | `billing accounting-method {accumulation \| time-to-finish }` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBillingAccountingMethod()` |
| Ch 32 (Hotspot) | `billing accumulation idle-detection timeout <1..60>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBillingAccumulationIdleDetection()` |
| Ch 32 (Hotspot) | `billing accumulation-expire {day <1..360> \| hour <1..24>}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBillingAccumulationExpire()` |
| Ch 32 (Hotspot) | `billing currency {eur \| gbp \| usd \| user-define currency_code }` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBillingCurrency()` |
| Ch 32 (Hotspot) | `billing decimal-places <2>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBillingDecimalPlaces()` |
| Ch 32 (Hotspot) | `billing decimal-symbol {comma \| dot}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBillingDecimalSymbol()` |
| Ch 32 (Hotspot) | `billing discount button {a \| b \| c} [charge-by-level]` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBillingDiscountButtonCharge()` |
| Ch 32 (Hotspot) | `billing profile rename profile_name profile_name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBillingProfileRenameProfile()` |
| Ch 32 (Hotspot) | `billing tax-rate <0..100>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBillingTaxRate()` |
| Ch 32 (Hotspot) | `billing unused-expire {minute <30..60> \| hour <1..24> \| day <1..365>}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBillingUnusedExpire()` |
| Ch 32 (Hotspot) | `billing username-password-length <4..6>` | Envelope 9 | `redline` | `ZyxelAuthCmd::cmdBillingUsernamePasswordLength()` |
| Ch 32 (Hotspot) | `deactivate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdDeactivate()` |
| Ch 32 (Hotspot) | `description description` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdDescriptionDescription()` |
| Ch 32 (Hotspot) | `ip ipnp config` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdIpIpnpConfig()` |
| Ch 32 (Hotspot) | `payment-service account-delivery delivery_method {deactivate \| activate}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceAccountDelivery()` |
| Ch 32 (Hotspot) | `payment-service check paypal-currency` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceCheckPaypal()` |
| Ch 32 (Hotspot) | `payment-service fail-page failed-message message` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceFailPage()` |
| Ch 32 (Hotspot) | `payment-service mobile-fail-page failed-message message` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceMobileFail()` |
| Ch 32 (Hotspot) | `payment-service mobile-profile-page selection-message message` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceMobileProfile()` |
| Ch 32 (Hotspot) | `payment-service mobile-sms-page info-message message` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceMobileSms()` |
| Ch 32 (Hotspot) | `payment-service mobile-success-page {notification-message \| successful-message \| notification- message-color {#00FF00 \| color_name \| rgb(0,0,255)}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceMobileSuccess()` |
| Ch 32 (Hotspot) | `payment-service profile-page selection-message message` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceProfilePage()` |
| Ch 32 (Hotspot) | `payment-service provider paypal` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceProviderPaypal()` |
| Ch 32 (Hotspot) | `payment-service provider paypal account e-mail` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceProviderPaypal()` |
| Ch 32 (Hotspot) | `payment-service provider paypal currency paypal_currency` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceProviderPaypal()` |
| Ch 32 (Hotspot) | `payment-service provider paypal exit` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceProviderPaypal()` |
| Ch 32 (Hotspot) | `payment-service provider paypal gateway payment_gw_url` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceProviderPaypal()` |
| Ch 32 (Hotspot) | `payment-service provider paypal identity-token paypal_token` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceProviderPaypal()` |
| Ch 32 (Hotspot) | `payment-service provider paypal no account` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceProviderPaypal()` |
| Ch 32 (Hotspot) | `payment-service provider paypal no identity-token .............................................................................` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceProviderPaypal()` |
| Ch 32 (Hotspot) | `payment-service provider select provider` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceProviderSelect()` |
| Ch 32 (Hotspot) | `payment-service success-page {account-message message \| format-date {dd-mm-yyyy \| mm-dd-yyyy \| yyyy-mm-dd} \| notification-message message \| notification-message-color {#00FF00 \| col- or_name \| rgb(0,0,255)} \| successful-message message}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentServiceSuccessPage()` |
| Ch 32 (Hotspot) | `price price` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPricePrice()` |
| Ch 32 (Hotspot) | `printer-ip ipv4_address` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPrinterIpIpv4Address()` |
| Ch 32 (Hotspot) | `printer-manager button {a \| b \| c} profile_name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPrinterManagerButtonProfile()` |
| Ch 32 (Hotspot) | `printer-manager discover` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPrinterManagerDiscover()` |
| Ch 32 (Hotspot) | `printer-manager encrypt secret-key secret_key` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPrinterManagerEncryptSecret()` |
| Ch 32 (Hotspot) | `printer-manager multi-printout <1..3>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPrinterManagerMultiPrintout()` |
| Ch 32 (Hotspot) | `printer-manager port <1..65535>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPrinterManagerPort()` |
| Ch 32 (Hotspot) | `printer-manager printer append` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPrinterManagerPrinterAppend()` |
| Ch 32 (Hotspot) | `printer-manager printout-type {customized \| default}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPrinterManagerPrintoutType()` |
| Ch 32 (Hotspot) | `quota type {total \| upload-download}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdQuotaType()` |
| Ch 32 (Hotspot) | `quota {total \| upload \| download} gigabytes <0..100>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdQuotaGigabytes()` |
| Ch 32 (Hotspot) | `quota {total \| upload \| download} megabytes <0..1023>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdQuotaMegabytes()` |
| Ch 32 (Hotspot) | `show advertisement` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowAdvertisement()` |
| Ch 32 (Hotspot) | `show advertisement activation` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowAdvertisementActivation()` |
| Ch 32 (Hotspot) | `show billing discount default rule` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowBillingDiscountDefault()` |
| Ch 32 (Hotspot) | `show billing discount rule` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowBillingDiscountRule()` |
| Ch 32 (Hotspot) | `show billing discount status` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowBillingDiscountStatus()` |
| Ch 32 (Hotspot) | `show billing profile [profile_name]` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowBillingProfileProfile()` |
| Ch 32 (Hotspot) | `show billing status` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowBillingStatus()` |
| Ch 32 (Hotspot) | `show free-time status` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowFreeTimeStatus()` |
| Ch 32 (Hotspot) | `show ip ipnp activation` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowIpIpnpActivation()` |
| Ch 32 (Hotspot) | `show ip ipnp interface` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowIpIpnpInterface()` |
| Ch 32 (Hotspot) | `show payment-service account-delivery` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPaymentServiceAccount()` |
| Ch 32 (Hotspot) | `show payment-service activation` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPaymentServiceActivation()` |
| Ch 32 (Hotspot) | `show payment-service check payment-all-currency` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPaymentServiceCheck()` |
| Ch 32 (Hotspot) | `show payment-service fail-page settings` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPaymentServiceFail()` |
| Ch 32 (Hotspot) | `show payment-service mobile-fail-page settings` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPaymentServiceMobile()` |
| Ch 32 (Hotspot) | `show payment-service mobile-page-customization` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPaymentServiceMobile()` |
| Ch 32 (Hotspot) | `show payment-service mobile-profile-page settings` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPaymentServiceMobile()` |
| Ch 32 (Hotspot) | `show payment-service mobile-sms-page settings` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPaymentServiceMobile()` |
| Ch 32 (Hotspot) | `show payment-service mobile-success-page settings` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPaymentServiceMobile()` |
| Ch 32 (Hotspot) | `show payment-service page-customization` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPaymentServicePage()` |
| Ch 32 (Hotspot) | `show payment-service profile-page settings` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPaymentServiceProfile()` |
| Ch 32 (Hotspot) | `show payment-service provider paypal` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPaymentServiceProvider()` |
| Ch 32 (Hotspot) | `show payment-service provider select` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPaymentServiceProvider()` |
| Ch 32 (Hotspot) | `show payment-service sms-page settings` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPaymentServiceSms()` |
| Ch 32 (Hotspot) | `show payment-service success-page settings` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPaymentServiceSuccess()` |
| Ch 32 (Hotspot) | `show printer-manager button` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPrinterManagerButton()` |
| Ch 32 (Hotspot) | `show printer-manager discover-printer-status` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPrinterManagerDiscover()` |
| Ch 32 (Hotspot) | `show printer-manager printer [<1..10>]` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPrinterManagerPrinter()` |
| Ch 32 (Hotspot) | `show printer-manager printer-status` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPrinterManagerPrinter()` |
| Ch 32 (Hotspot) | `show printer-manager printerfw version` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPrinterManagerPrinterfw()` |
| Ch 32 (Hotspot) | `show printer-manager printout-type` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPrinterManagerPrintout()` |
| Ch 32 (Hotspot) | `show printer-manager settings` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPrinterManagerSettings()` |
| Ch 32 (Hotspot) | `show printer-manager workableIP` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPrinterManagerWorkableip()` |
| Ch 32 (Hotspot) | `show walled-garden activation` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowWalledGardenActivation()` |
| Ch 32 (Hotspot) | `show walled-garden rule <1..50>` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowWalledGardenRule()` |
| Ch 32 (Hotspot) | `time-period {day <1..365> \| hour <1..24> \| minute <30..60>}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTimePeriod()` |
| Ch 32 (Hotspot) | `wac6553d-e} vlan_interface activate vid <1..4094> join ap_lan_port {tag \| untag} [ap_lan_port {tag \| untag}] [ap_lan_port {tag \| untag}] ................................................................................................................7 1 wac6553d-e} vlan_interface inactivate vid <1..4094> join ap_lan_port {tag \| untag} [ap_lan_port {tag \| untag}] [ap_lan_port {tag \| untag}] ................................................................................................................7 1 walled-garden domain-ip rule <1..50>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWac6553dEVlanInterface()` |
| Ch 32 (Hotspot) | `walled-garden domain-ip rule append` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWalledGardenDomainIp()` |
| Ch 32 (Hotspot) | `walled-garden domain-ip rule flush` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWalledGardenDomainIp()` |
| Ch 32 (Hotspot) | `walled-garden rule append` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWalledGardenRuleAppend()` |
| Ch 32 (Hotspot) | `walled-garden rule flush` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWalledGardenRuleFlush()` |
| Ch 32 (Hotspot) | `walled-garden rule insert <1..50>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWalledGardenRuleInsert()` |
| Ch 32 (Hotspot) | `walled-garden rule move <1..50> to <1..50>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdWalledGardenRuleMove()` |
| Ch 33 (IPSec VPN) | `[isakmp_algo]]` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdIsakmpAlgo()` |
| Ch 33 (IPSec VPN) | `[isakmp_algo]]` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdIsakmpAlgo()` |
| Ch 33 (IPSec VPN) | `[no] activate` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdActivate()` |
| Ch 33 (IPSec VPN) | `[no] configuration-payload-provide activate` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdConfigurationPayloadProvideActivate()` |
| Ch 33 (IPSec VPN) | `[no] configuration-payload-provide activate` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdConfigurationPayloadProvideActivate()` |
| Ch 33 (IPSec VPN) | `[no] configuration-payload-provide {first-dns IPv6\|second-dns IPv6}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdConfigurationPayloadProvide()` |
| Ch 33 (IPSec VPN) | `[no] configuration-payload-provide {first-dns IPv6\|second-dns IPv6}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdConfigurationPayloadProvide()` |
| Ch 33 (IPSec VPN) | `[no] crypto boost-tcp` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCryptoBoostTcp()` |
| Ch 33 (IPSec VPN) | `[no] crypto ignore-df-bit` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCryptoIgnoreDfBit()` |
| Ch 33 (IPSec VPN) | `[no] crypto map map_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCryptoMapMapName()` |
| Ch 33 (IPSec VPN) | `[no] crypto map map_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCryptoMapMapName()` |
| Ch 33 (IPSec VPN) | `[no] crypto map_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCryptoMapName()` |
| Ch 33 (IPSec VPN) | `[no] crypto map_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCryptoMapName()` |
| Ch 33 (IPSec VPN) | `[no] dpd` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdDpd()` |
| Ch 33 (IPSec VPN) | `[no] eap type {server AAA_method user-id {name\|any}\| client name username {password PASSWORD\| encrypted-pass- word PASSWORD}` | Envelope 8 | `redline` | `ZyxelVpnCmd::cmdEapTypeClientName()` |
| Ch 33 (IPSec VPN) | `[no] eap type {server auth_method user-id {name\|any}\| client name username {password PASSWORD\| encrypted-pass- word password} .............................................................................................................................` | Envelope 8 | `redline` | `ZyxelVpnCmd::cmdEapTypeClientName()` |
| Ch 33 (IPSec VPN) | `[no] fall-back` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdFallBack()` |
| Ch 33 (IPSec VPN) | `[no] fall-back` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdFallBack()` |
| Ch 33 (IPSec VPN) | `[no] fall-back` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdFallBack()` |
| Ch 33 (IPSec VPN) | `[no] ikev2 policy policy_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdIkev2PolicyPolicyName()` |
| Ch 33 (IPSec VPN) | `[no] ikev2 policy6 policy_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdIkev2Policy6PolicyName()` |
| Ch 33 (IPSec VPN) | `[no] in-dnat activate .........................................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdInDnatActivate()` |
| Ch 33 (IPSec VPN) | `[no] in-snat activate` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdInSnatActivate()` |
| Ch 33 (IPSec VPN) | `[no] isakmp policy policy_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdIsakmpPolicyPolicyName()` |
| Ch 33 (IPSec VPN) | `[no] mode-config {first-dns \| second-dns}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdModeConfig()` |
| Ch 33 (IPSec VPN) | `[no] mode-config {first-wins \| second-wins}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdModeConfig()` |
| Ch 33 (IPSec VPN) | `[no] nail-up` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdNailUp()` |
| Ch 33 (IPSec VPN) | `[no] nail-up` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdNailUp()` |
| Ch 33 (IPSec VPN) | `[no] narrowed .................................................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdNarrowed()` |
| Ch 33 (IPSec VPN) | `[no] narrowed .................................................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdNarrowed()` |
| Ch 33 (IPSec VPN) | `[no] natt` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdNatt()` |
| Ch 33 (IPSec VPN) | `[no] netbios-broadcast` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdNetbiosBroadcast()` |
| Ch 33 (IPSec VPN) | `[no] out-snat activate ........................................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdOutSnatActivate()` |
| Ch 33 (IPSec VPN) | `[no] policy-enforcement` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdPolicyEnforcement()` |
| Ch 33 (IPSec VPN) | `[no] policy-enforcement` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdPolicyEnforcement()` |
| Ch 33 (IPSec VPN) | `[no] protocol gre` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdProtocolGre()` |
| Ch 33 (IPSec VPN) | `[no] replay-detection` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdReplayDetection()` |
| Ch 33 (IPSec VPN) | `[no] replay-detection` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdReplayDetection()` |
| Ch 33 (IPSec VPN) | `[no] twofa-auth` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdTwofaAuth()` |
| Ch 33 (IPSec VPN) | `[no] twofa-auth` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdTwofaAuth()` |
| Ch 33 (IPSec VPN) | `[no] twofa-auth` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdTwofaAuth()` |
| Ch 33 (IPSec VPN) | `[no] ul-bandwidth-limit <1...1048576>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdUlBandwidthLimit()` |
| Ch 33 (IPSec VPN) | `[no] vpn-concentrator profile_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdVpnConcentratorProfileName()` |
| Ch 33 (IPSec VPN) | `[no] vpn-concentrator6 profile_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdVpnConcentrator6ProfileName()` |
| Ch 33 (IPSec VPN) | `[no] vpn-configuration-provision activate` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdVpnConfigurationProvisionActivate()` |
| Ch 33 (IPSec VPN) | `[no] vpn-configuration-provision iosfilter` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdVpnConfigurationProvisionIosfilter()` |
| Ch 33 (IPSec VPN) | `[no] vpn-service auto-disable` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdVpnServiceAutoDisable()` |
| Ch 33 (IPSec VPN) | `[no] vpn-service enable` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdVpnServiceEnable()` |
| Ch 33 (IPSec VPN) | `activate` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdActivate()` |
| Ch 33 (IPSec VPN) | `activate` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdActivate()` |
| Ch 33 (IPSec VPN) | `activate` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdActivate()` |
| Ch 33 (IPSec VPN) | `activate` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdActivate()` |
| Ch 33 (IPSec VPN) | `activate` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdActivate()` |
| Ch 33 (IPSec VPN) | `adjust-mss {auto \| <200..1500>} ...............................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdAdjustMss()` |
| Ch 33 (IPSec VPN) | `adjust-mss {auto \| <200..1500>} ...............................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdAdjustMss()` |
| Ch 33 (IPSec VPN) | `authentication {pre-share \| rsa-sig \| user-base-psk}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdAuthentication()` |
| Ch 33 (IPSec VPN) | `authentication {pre-share \| rsa-sig}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdAuthentication()` |
| Ch 33 (IPSec VPN) | `authentication {pre-share \| rsa-sig}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdAuthentication()` |
| Ch 33 (IPSec VPN) | `certificate certificate-name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCertificateCertificateName()` |
| Ch 33 (IPSec VPN) | `certificate certificate-name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCertificateCertificateName()` |
| Ch 33 (IPSec VPN) | `certificate certificate-name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCertificateCertificateName()` |
| Ch 33 (IPSec VPN) | `configuration-payload-provide address- {} .....................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdConfigurationPayloadProvideAddress()` |
| Ch 33 (IPSec VPN) | `configuration-payload-provide address- {} .....................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdConfigurationPayloadProvideAddress()` |
| Ch 33 (IPSec VPN) | `conn-check {ip address ip address \| first-and-last} method {icmp \| tcp} period <5...3600> timeout <1...10> fail- tolerance <1...10> action {log \| no-log} probe-condition {all \| any}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdConnCheckMethodPeriod()` |
| Ch 33 (IPSec VPN) | `crypto map dial map_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCryptoMapDialMap()` |
| Ch 33 (IPSec VPN) | `crypto map map_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCryptoMapMapName()` |
| Ch 33 (IPSec VPN) | `crypto map map_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCryptoMapMapName()` |
| Ch 33 (IPSec VPN) | `crypto map rename map_name map_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCryptoMapRenameMap()` |
| Ch 33 (IPSec VPN) | `crypto map rename map_name map_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCryptoMapRenameMap()` |
| Ch 33 (IPSec VPN) | `crypto map6 dial map_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCryptoMap6DialMap()` |
| Ch 33 (IPSec VPN) | `crypto map_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCryptoMapName()` |
| Ch 33 (IPSec VPN) | `deactivate` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdDeactivate()` |
| Ch 33 (IPSec VPN) | `deactivate` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdDeactivate()` |
| Ch 33 (IPSec VPN) | `deactivate` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdDeactivate()` |
| Ch 33 (IPSec VPN) | `deactivate` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdDeactivate()` |
| Ch 33 (IPSec VPN) | `deactivate` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdDeactivate()` |
| Ch 33 (IPSec VPN) | `debug conntrack flush` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdDebugConntrackFlush()` |
| Ch 33 (IPSec VPN) | `dpd-interval <15..60> .........................................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdDpdInterval()` |
| Ch 33 (IPSec VPN) | `eap auth_method AUTH_METHOD` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdEapAuthMethodAuth()` |
| Ch 33 (IPSec VPN) | `eap auth_method auth_method` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdEapAuthMethodAuth()` |
| Ch 33 (IPSec VPN) | `encapsulation {tunnel \| transport}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdEncapsulation()` |
| Ch 33 (IPSec VPN) | `encapsulation {tunnel \| transport}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdEncapsulation()` |
| Ch 33 (IPSec VPN) | `exit` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdExit()` |
| Ch 33 (IPSec VPN) | `fall-back-check-interval <60..86400>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdFallBackCheckInterval()` |
| Ch 33 (IPSec VPN) | `fall-back-check-interval <60..86400>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdFallBackCheckInterval()` |
| Ch 33 (IPSec VPN) | `fall-back-check-interval <60..86400>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdFallBackCheckInterval()` |
| Ch 33 (IPSec VPN) | `geo-ip database update weekly {fri \| mon \| sat \| sun \| thu \| tue \| wed} <0..23> .465 group1` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdGeoIpDatabaseUpdate()` |
| Ch 33 (IPSec VPN) | `group1` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdGroup1()` |
| Ch 33 (IPSec VPN) | `group1` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdGroup1()` |
| Ch 33 (IPSec VPN) | `group14` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdGroup14()` |
| Ch 33 (IPSec VPN) | `group14 .......................................................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdGroup14()` |
| Ch 33 (IPSec VPN) | `group15 .......................................................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdGroup15()` |
| Ch 33 (IPSec VPN) | `group16 .......................................................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdGroup16()` |
| Ch 33 (IPSec VPN) | `group17` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdGroup17()` |
| Ch 33 (IPSec VPN) | `group18 .......................................................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdGroup18()` |
| Ch 33 (IPSec VPN) | `group2` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdGroup2()` |
| Ch 33 (IPSec VPN) | `group2` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdGroup2()` |
| Ch 33 (IPSec VPN) | `group2` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdGroup2()` |
| Ch 33 (IPSec VPN) | `group5` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdGroup5()` |
| Ch 33 (IPSec VPN) | `group5` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdGroup5()` |
| Ch 33 (IPSec VPN) | `group5` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdGroup5()` |
| Ch 33 (IPSec VPN) | `ikev2 policy rename policy_name policy_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdIkev2PolicyRenamePolicy()` |
| Ch 33 (IPSec VPN) | `ikev2 policy rename policy_name policy_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdIkev2PolicyRenamePolicy()` |
| Ch 33 (IPSec VPN) | `in-dnat <1..10> protocol {all \| tcp \| udp} original-ip address_name <0..65535> <0..65535> mapped-ip address_name <0..65535> <0..65535> .........................................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdInDnatProtocolOriginal()` |
| Ch 33 (IPSec VPN) | `in-dnat append protocol {all \| tcp \| udp} original-ip address_name <0..65535> <0..65535> mapped-ip address_name <0..65535> <0..65535> .........................................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdInDnatAppendProtocol()` |
| Ch 33 (IPSec VPN) | `in-dnat delete <1..10>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdInDnatDelete()` |
| Ch 33 (IPSec VPN) | `in-dnat insert <1..10> protocol {all \| tcp \| udp} original-ip address_name <0..65535> <0..65535> mapped-ip ad- dress_name <0..65535> <0..65535>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdInDnatInsertProtocol()` |
| Ch 33 (IPSec VPN) | `in-dnat move <1..10> to <1..10> ...............................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdInDnatMoveTo()` |
| Ch 33 (IPSec VPN) | `in-snat source address_name destination address_name snat address_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdInSnatSourceAddress()` |
| Ch 33 (IPSec VPN) | `ipsec-isakmp policy_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdIpsecIsakmpPolicyName()` |
| Ch 33 (IPSec VPN) | `ipsec-isakmp policy_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdIpsecIsakmpPolicyName()` |
| Ch 33 (IPSec VPN) | `isakmp policy rename policy_name policy_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdIsakmpPolicyRenamePolicy()` |
| Ch 33 (IPSec VPN) | `keystring pre_shared_key` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdKeystringPreSharedKey()` |
| Ch 33 (IPSec VPN) | `keystring pre_shared_key` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdKeystringPreSharedKey()` |
| Ch 33 (IPSec VPN) | `keystring pre_shared_key` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdKeystringPreSharedKey()` |
| Ch 33 (IPSec VPN) | `lifetime <180..3000000>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdLifetime()` |
| Ch 33 (IPSec VPN) | `lifetime <180..3000000>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdLifetime()` |
| Ch 33 (IPSec VPN) | `lifetime <180..3000000>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdLifetime()` |
| Ch 33 (IPSec VPN) | `local-id type {ip IPv6 \| fqdn domain_name \| mail e_mail \| dn distinguished_name}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdLocalIdType()` |
| Ch 33 (IPSec VPN) | `local-id type {ip ip \| fqdn domain_name \| mail e_mail \| dn distinguished_name}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdLocalIdType()` |
| Ch 33 (IPSec VPN) | `local-id type {ip ip \| fqdn domain_name \| mail e_mail \| dn distinguished_name}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdLocalIdType()` |
| Ch 33 (IPSec VPN) | `local-ip ip` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdLocalIpIp()` |
| Ch 33 (IPSec VPN) | `local-ip {ip IPv6} ............................................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdLocalIp()` |
| Ch 33 (IPSec VPN) | `local-ip {ip {ip \| domain_name} \| interface interface_name}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdLocalIpInterfaceInterface()` |
| Ch 33 (IPSec VPN) | `local-ip {ip {ip \| domain_name} \| interface interface_name}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdLocalIpInterfaceInterface()` |
| Ch 33 (IPSec VPN) | `local-policy address_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdLocalPolicyAddressName()` |
| Ch 33 (IPSec VPN) | `local-policy address_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdLocalPolicyAddressName()` |
| Ch 33 (IPSec VPN) | `mode {main \| aggressive}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdMode()` |
| Ch 33 (IPSec VPN) | `mode-config activate ..........................................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdModeConfigActivate()` |
| Ch 33 (IPSec VPN) | `mode-config address- profile_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdModeConfigAddressProfile()` |
| Ch 33 (IPSec VPN) | `no sa spi spi` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdNoSaSpiSpi()` |
| Ch 33 (IPSec VPN) | `no sa tunnel-name map_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdNoSaTunnelName()` |
| Ch 33 (IPSec VPN) | `no user .......................................................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdNoUser()` |
| Ch 33 (IPSec VPN) | `no vpn-configuration-provision port` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdNoVpnConfigurationProvision()` |
| Ch 33 (IPSec VPN) | `out-snat source address_name destination address_name snat address_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdOutSnatSourceAddress()` |
| Ch 33 (IPSec VPN) | `peer-id type {any \| ip IPv6 \| fqdn domain_name \| mail e_mail \| dn distinguished_name}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdPeerIdType()` |
| Ch 33 (IPSec VPN) | `peer-id type {any \| ip ip \| fqdn domain_name \| mail e_mail \| dn distinguished_name} 280 peer-id type {any \| ip ip \| fqdn domain_name \| mail e_mail \| dn distinguished_name}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdPeerIdType280()` |
| Ch 33 (IPSec VPN) | `peer-ip ip` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdPeerIpIp()` |
| Ch 33 (IPSec VPN) | `peer-ip {ip IPv6]` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdPeerIpIpIpv6()` |
| Ch 33 (IPSec VPN) | `peer-ip {ip \| domain_name} [ip \| domain_name]` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdPeerIpIpDomain()` |
| Ch 33 (IPSec VPN) | `peer-ip {ip \| domain_name} [ip \| domain_name]` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdPeerIpIpDomain()` |
| Ch 33 (IPSec VPN) | `remote-policy address_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdRemotePolicyAddressName()` |
| Ch 33 (IPSec VPN) | `remote-policy address_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdRemotePolicyAddressName()` |
| Ch 33 (IPSec VPN) | `scenario {site-to-site-static\|site-to-site-dynamic\|remote-access-server\|remote-access-client}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdScenario()` |
| Ch 33 (IPSec VPN) | `scenario {site-to-site-static\|site-to-site-dynamic\|remote-access-server\|remote-access-client}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdScenario()` |
| Ch 33 (IPSec VPN) | `set pfs {group1 \| group2 \| group5 \| none} .....................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdSetPfs()` |
| Ch 33 (IPSec VPN) | `set pfs {group1 \| group2 \| group5 \| none} .....................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdSetPfs()` |
| Ch 33 (IPSec VPN) | `set security-association lifetime seconds <180..3000000>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdSetSecurityAssociationLifetime()` |
| Ch 33 (IPSec VPN) | `set security-association lifetime seconds <180..3000000>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdSetSecurityAssociationLifetime()` |
| Ch 33 (IPSec VPN) | `show crypto boost-tcp` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowCryptoBoostTcp()` |
| Ch 33 (IPSec VPN) | `show crypto map [map_name]` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowCryptoMapMap()` |
| Ch 33 (IPSec VPN) | `show crypto map6 [map_name]` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowCryptoMap6Map()` |
| Ch 33 (IPSec VPN) | `show ikev2 policy [policy_name]` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowIkev2PolicyPolicy()` |
| Ch 33 (IPSec VPN) | `show ikev2 policy6 [policy_name]` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowIkev2Policy6Policy()` |
| Ch 33 (IPSec VPN) | `show isakmp keepalive` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowIsakmpKeepalive()` |
| Ch 33 (IPSec VPN) | `show isakmp policy [policy_name]` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowIsakmpPolicyPolicy()` |
| Ch 33 (IPSec VPN) | `show isakmp sa` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowIsakmpSa()` |
| Ch 33 (IPSec VPN) | `show sa counter` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowSaCounter()` |
| Ch 33 (IPSec VPN) | `show sa monitor [{begin <1..1000>} \| {end <1..1000>} \| {crypto-map regexp} \| {policy regexp} \|{rsort sort_order} \| {sort sort_order}]` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowSaMonitor()` |
| Ch 33 (IPSec VPN) | `show vcp allowed crypto map` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowVcpAllowedCrypto()` |
| Ch 33 (IPSec VPN) | `show vcp allowed crypto map6` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowVcpAllowedCrypto()` |
| Ch 33 (IPSec VPN) | `show vcp allowed users` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowVcpAllowedUsers()` |
| Ch 33 (IPSec VPN) | `show vpn-concentrator [profile_name]` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowVpnConcentratorProfile()` |
| Ch 33 (IPSec VPN) | `show vpn-concentrator6 [profile_name]` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowVpnConcentrator6Profile()` |
| Ch 33 (IPSec VPN) | `show vpn-configuration-provision activation` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowVpnConfigurationProvision()` |
| Ch 33 (IPSec VPN) | `show vpn-configuration-provision authentication` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowVpnConfigurationProvision()` |
| Ch 33 (IPSec VPN) | `show vpn-configuration-provision iosfilter` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowVpnConfigurationProvision()` |
| Ch 33 (IPSec VPN) | `show vpn-configuration-provision port` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowVpnConfigurationProvision()` |
| Ch 33 (IPSec VPN) | `show vpn-configuration-provision rules` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowVpnConfigurationProvision()` |
| Ch 33 (IPSec VPN) | `show vpn-counters` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowVpnCounters()` |
| Ch 33 (IPSec VPN) | `show vpn-service status` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowVpnServiceStatus()` |
| Ch 33 (IPSec VPN) | `transform-set crypto_algo_ah [crypto_algo_ah [crypto_algo_ah]]` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdTransformSetCryptoAlgo()` |
| Ch 33 (IPSec VPN) | `transform-set crypto_algo_ah [crypto_algo_ah [crypto_algo_ah]]` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdTransformSetCryptoAlgo()` |
| Ch 33 (IPSec VPN) | `transform-set crypto_algo_esp [crypto_algo_esp [crypto_algo_esp]]` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdTransformSetCryptoAlgo()` |
| Ch 33 (IPSec VPN) | `transform-set crypto_algo_esp [crypto_algo_esp [crypto_algo_esp]]` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdTransformSetCryptoAlgo()` |
| Ch 33 (IPSec VPN) | `transform-set isakmp-algo [isakmp_algo ........................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdTransformSetIsakmpAlgo()` |
| Ch 33 (IPSec VPN) | `transform-set isakmp-algo [isakmp_algo ........................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdTransformSetIsakmpAlgo()` |
| Ch 33 (IPSec VPN) | `transform-set isakmp-algo [isakmp_algo [isakmp_algo]]` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdTransformSetIsakmpAlgo()` |
| Ch 33 (IPSec VPN) | `user username` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdUserUsername()` |
| Ch 33 (IPSec VPN) | `vpn-concentrator rename profile_name profile_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdVpnConcentratorRenameProfile()` |
| Ch 33 (IPSec VPN) | `vpn-concentrator6 rename profile_name profile_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdVpnConcentrator6RenameProfile()` |
| Ch 33 (IPSec VPN) | `vpn-configuration-provision authentication auth_method` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdVpnConfigurationProvisionAuthentication()` |
| Ch 33 (IPSec VPN) | `vpn-configuration-provision generate {ios \| windows \| android} ikev2-wizard profile <profile name>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdVpnConfigurationProvisionGenerate()` |
| Ch 33 (IPSec VPN) | `vpn-configuration-provision port <1...65535>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdVpnConfigurationProvisionPort()` |
| Ch 33 (IPSec VPN) | `vpn-configuration-provision rule { append \| conf_index \| insert conf_index }` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdVpnConfigurationProvisionRule()` |
| Ch 33 (IPSec VPN) | `xauth type {server auth_method [user-id {username \| any}] \| client name username password pass- word} [deactivate]` | Envelope 8 | `redline` | `ZyxelVpnCmd::cmdXauthTypeClientName()` |
| Ch 34 (SSL VPN) | `[no] activate` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdActivate()` |
| Ch 34 (SSL VPN) | `[no] application application_object ...........................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdApplicationApplicationObject()` |
| Ch 34 (SSL VPN) | `[no] description description ..................................................................................................` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdDescriptionDescription()` |
| Ch 34 (SSL VPN) | `[no] network-extension netbios-broadcast` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdNetworkExtensionNetbiosBroadcast()` |
| Ch 34 (SSL VPN) | `[no] network-extension traffic-enforcement` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdNetworkExtensionTrafficEnforcement()` |
| Ch 34 (SSL VPN) | `[no] network-extension {activate \| ip- address_object \| 1st-dns {address_object \| ip } \| 2nd- dns {address_object \| ip } \| 1st-wins {address_object \| ip } \| 2nd-wins {address_object \| ip } \| network address_object}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdNetworkExtension2ndDns()` |
| Ch 34 (SSL VPN) | `[no] user user_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdUserUserName()` |
| Ch 34 (SSL VPN) | `no sslvpn login-port` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdNoSslvpnLoginPort()` |
| Ch 34 (SSL VPN) | `no sslvpn policy profile_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdNoSslvpnPolicyProfile()` |
| Ch 34 (SSL VPN) | `show ssl-vpn network-extension local-ip` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowSslVpnNetwork()` |
| Ch 34 (SSL VPN) | `show sslvpn login-port` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowSslvpnLoginPort()` |
| Ch 34 (SSL VPN) | `show sslvpn monitor` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowSslvpnMonitor()` |
| Ch 34 (SSL VPN) | `show sslvpn policy [profile_name]` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowSslvpnPolicyProfile()` |
| Ch 34 (SSL VPN) | `show workspace application` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowWorkspaceApplication()` |
| Ch 34 (SSL VPN) | `show workspace cifs` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowWorkspaceCifs()` |
| Ch 34 (SSL VPN) | `sslvpn login message <description>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdSslvpnLoginMessage()` |
| Ch 34 (SSL VPN) | `sslvpn login-port <1..65535>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdSslvpnLoginPort()` |
| Ch 34 (SSL VPN) | `sslvpn logout message <description>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdSslvpnLogoutMessage()` |
| Ch 34 (SSL VPN) | `sslvpn network-extension local-ip ip` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdSslvpnNetworkExtensionLocal()` |
| Ch 34 (SSL VPN) | `sslvpn no connection username user_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdSslvpnNoConnectionUsername()` |
| Ch 34 (SSL VPN) | `sslvpn policy rename profile_name profile_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdSslvpnPolicyRenameProfile()` |
| Ch 34 (SSL VPN) | `sslvpn policy {profile_name \| profile_name append \| profile_name insert <1..16>} 296 sslvpn policy move <1..16> to <1..16>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdSslvpnPolicy296Sslvpn()` |
| Ch 35 (L2TP VPN) | `Interface dial wan1_ppp` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdInterfaceDialWan1Ppp()` |
| Ch 35 (L2TP VPN) | `Interface disconnect` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdInterfaceDisconnect()` |
| Ch 35 (L2TP VPN) | `Interface interface_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdInterfaceInterfaceName()` |
| Ch 35 (L2TP VPN) | `[no] account l2tp profile_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdAccountL2tpProfileName()` |
| Ch 35 (L2TP VPN) | `[no] l2tp-over-ipsec activate;` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdL2tpOverIpsecActivate()` |
| Ch 35 (L2TP VPN) | `[no] l2tp-over-ipsec first-dns-server {ip \| interface_name} {1st-dns\|2nd-dns\|3rd-dns}\| {ppp_in- terface}{1st-dns\|2nd-dns}}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdL2tpOverIpsecFirst()` |
| Ch 35 (L2TP VPN) | `[no] l2tp-over-ipsec first-wins-server ip` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdL2tpOverIpsecFirst()` |
| Ch 35 (L2TP VPN) | `[no] l2tp-over-ipsec keepalive-timer <1..180>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdL2tpOverIpsecKeepalive()` |
| Ch 35 (L2TP VPN) | `[no] l2tp-over-ipsec second-dns-server {ip \| interface_name} {1st-dns\|2nd-dns\|3rd-dns}\| {ppp_interface}{1st-dns\|2nd-dns}}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdL2tpOverIpsecSecond()` |
| Ch 35 (L2TP VPN) | `[no] l2tp-over-ipsec second-wins-server ip` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdL2tpOverIpsecSecond()` |
| Ch 35 (L2TP VPN) | `[no] l2tp-over-ipsec user user_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdL2tpOverIpsecUser()` |
| Ch 35 (L2TP VPN) | `account profile_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdAccountProfileName()` |
| Ch 35 (L2TP VPN) | `authentication {chap \| chap-pap \| mschap \| mschap-v2 \| pap}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdAuthentication()` |
| Ch 35 (L2TP VPN) | `certificate cert_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdCertificateCertName()` |
| Ch 35 (L2TP VPN) | `encrypted-password ciphertext` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdEncryptedPasswordCiphertext()` |
| Ch 35 (L2TP VPN) | `idle <0..360>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdIdle()` |
| Ch 35 (L2TP VPN) | `l2tp-over-ipsec address-object` | Envelope 8 | `tier2` | `ZyxelVpnCmd::cmdL2tpOverIpsecAddress()` |
| Ch 35 (L2TP VPN) | `l2tp-over-ipsec authentication authentication profile_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdL2tpOverIpsecAuthentication()` |
| Ch 35 (L2TP VPN) | `l2tp-over-ipsec crypto map_name` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdL2tpOverIpsecCrypto()` |
| Ch 35 (L2TP VPN) | `l2tp-over-ipsec recover default-ipsec-policy` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdL2tpOverIpsecRecover()` |
| Ch 35 (L2TP VPN) | `local-address w.x.y.z` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdLocalAddressWX()` |
| Ch 35 (L2TP VPN) | `no l2tp-over-ipsec session tunnel-id <0..65535>` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdNoL2tpOverIpsec()` |
| Ch 35 (L2TP VPN) | `password isp_account_password` | Envelope 8 | `redline` | `ZyxelVpnCmd::cmdPasswordIspAccountPassword()` |
| Ch 35 (L2TP VPN) | `server {domain_name \| w.x.y.z}` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdServer()` |
| Ch 35 (L2TP VPN) | `show account l2tp [profile_name]` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowAccountL2tpProfile()` |
| Ch 35 (L2TP VPN) | `show interface ppp` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowInterfacePpp()` |
| Ch 35 (L2TP VPN) | `show l2tp-over-ipsec` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowL2tpOverIpsec()` |
| Ch 35 (L2TP VPN) | `show l2tp-over-ipsec session` | Envelope 8 | `read` | `ZyxelVpnCmd::cmdShowL2tpOverIpsec()` |
| Ch 35 (L2TP VPN) | `user isp_account_username` | Envelope 8 | `tier1` | `ZyxelVpnCmd::cmdUserIspAccountUsername()` |
| Ch 36 (Bandwidth Management) | `[no] activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdActivate()` |
| Ch 36 (Bandwidth Management) | `[no] bwm activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBwmActivate()` |
| Ch 36 (Bandwidth Management) | `[no] bwm highest sip bandwidth priority` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBwmHighestSipBandwidth()` |
| Ch 36 (Bandwidth Management) | `[no] description description` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDescriptionDescription()` |
| Ch 36 (Bandwidth Management) | `[no] destination address_object` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDestinationAddressObject()` |
| Ch 36 (Bandwidth Management) | `[no] dscp {<0..63> \| any \| class {af11 \| af12 \| af13 \| af21 \| af22 \| af23 \| af31 \| af32 \| af33 \| af41 \| af42 \| af43 \| cs0 \| cs1 \| cs2 \| cs3 \| cs4 \| cs5 \| cs6 \| cs7 \| default \| wmm_be0 \| wmm_be24 \| wmm_bk16 \| wmm_bk8 \| wmm_vi32 \| wm- m_vi40 \| wmm_vo48 \| wmm_vo56}}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDscp()` |
| Ch 36 (Bandwidth Management) | `[no] inbound ceiling {<0..1048576> \| maximize-bandwidth-usage}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdInboundCeiling()` |
| Ch 36 (Bandwidth Management) | `[no] inbound guarantee-bandwidth <0..1048576> priority <1..7>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdInboundGuaranteeBandwidthPriority()` |
| Ch 36 (Bandwidth Management) | `[no] inbound-dscp-mark {<0..63> \| class {af11 \| af12 \| af13 \| af21 \| af22 \| af23 \| af31 \| af32 \| af33 \| af41 \| af42 \| af43 \| cs0 \| cs1 \| cs2 \| cs3 \| cs4 \| cs5 \| cs6 \| cs7 \| default \| wmm_be0 \| wmm_be24 \| wmm_bk16 \| wmm_bk8 \| wmm_vi32 \| wmm_vi40 \| wmm_vo48 \| wmm_vo56}}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdInboundDscpMark()` |
| Ch 36 (Bandwidth Management) | `[no] incoming-interface {interface interface_name \| trunk group_name}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIncomingInterface()` |
| Ch 36 (Bandwidth Management) | `[no] log [alert] ..............................................................................................................` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdLogAlert()` |
| Ch 36 (Bandwidth Management) | `[no] outbound ceiling {<0..1048576> \| maximize-bandwidth-usage}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdOutboundCeiling()` |
| Ch 36 (Bandwidth Management) | `[no] outbound guarantee-bandwidth <0..1048576> priority <1..7> ..............................................................30 9 [no] outbound-dscp-mark {<0..63> \| class {af11 \| af12 \| af13 \| af21 \| af22 \| af23 \| af31 \| af32 \| af33 \| af41 \| af42 \| af43 \| cs0 \| cs1 \| cs2 \| cs3 \| cs4 \| cs5 \| cs6 \| cs7 \| default \| wmm_be0 \| wmm_be24 \| wmm_bk16 \| wmm_bk8 \| wmm_vi32 \| wmm_vi40 \| wmm_vo48 \| wmm_vo56}}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdOutboundGuaranteeBandwidthPriority()` |
| Ch 36 (Bandwidth Management) | `[no] outgoing-interface {interface interface_name \| trunk group_name}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdOutgoingInterface()` |
| Ch 36 (Bandwidth Management) | `[no] schedule schedule_object` | Envelope 7 | `tier2` | `ZyxelSecurityCmd::cmdScheduleScheduleObject()` |
| Ch 36 (Bandwidth Management) | `[no] service application-group app_name` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdServiceApplicationGroupApp()` |
| Ch 36 (Bandwidth Management) | `[no] service service-object {service_name \| any}` | Envelope 7 | `tier2` | `ZyxelSecurityCmd::cmdServiceServiceObject()` |
| Ch 36 (Bandwidth Management) | `[no] source address_object` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSourceAddressObject()` |
| Ch 36 (Bandwidth Management) | `[no] type {per-user \| shared \| per-ip-source} .................................................................................` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdType()` |
| Ch 36 (Bandwidth Management) | `[no] user user_name` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdUserUserName()` |
| Ch 36 (Bandwidth Management) | `bwm <1..127>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBwm()` |
| Ch 36 (Bandwidth Management) | `bwm <1..127>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBwm()` |
| Ch 36 (Bandwidth Management) | `bwm append` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBwmAppend()` |
| Ch 36 (Bandwidth Management) | `bwm default inbound priority <1..7>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBwmDefaultInboundPriority()` |
| Ch 36 (Bandwidth Management) | `bwm default outbound priority <1..7>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBwmDefaultOutboundPriority()` |
| Ch 36 (Bandwidth Management) | `bwm delete <1..127>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBwmDelete()` |
| Ch 36 (Bandwidth Management) | `bwm insert <1..127>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBwmInsert()` |
| Ch 36 (Bandwidth Management) | `bwm modify <1..127>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBwmModify()` |
| Ch 36 (Bandwidth Management) | `bwm move <1..127> to <1..127>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBwmMoveTo()` |
| Ch 36 (Bandwidth Management) | `marked-interface any` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdMarkedInterfaceAny()` |
| Ch 36 (Bandwidth Management) | `marked-interface interface vlan<1..4064>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdMarkedInterfaceInterfaceVlan()` |
| Ch 36 (Bandwidth Management) | `marked-interface none` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdMarkedInterfaceNone()` |
| Ch 36 (Bandwidth Management) | `marked-interface trunk trunk_name` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdMarkedInterfaceTrunkTrunk()` |
| Ch 36 (Bandwidth Management) | `priority-code <0..7>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdPriorityCode()` |
| Ch 36 (Bandwidth Management) | `show` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdShow()` |
| Ch 36 (Bandwidth Management) | `show bwm activation` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowBwmActivation()` |
| Ch 36 (Bandwidth Management) | `show bwm all` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowBwmAll()` |
| Ch 36 (Bandwidth Management) | `show bwm applications list` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowBwmApplicationsList()` |
| Ch 36 (Bandwidth Management) | `show bwm default` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowBwmDefault()` |
| Ch 36 (Bandwidth Management) | `show bwm highest sip bandwidth priority` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowBwmHighestSip()` |
| Ch 36 (Bandwidth Management) | `vlan-priority-code <0..7>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdVlanPriorityCode()` |
| Ch 37 (Application Patrol) | `[no] app <profile-name>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdApp()` |
| Ch 37 (Application Patrol) | `[no] app log_sid` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAppLogSid()` |
| Ch 37 (Application Patrol) | `[no] app statistics collect` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAppStatisticsCollect()` |
| Ch 37 (Application Patrol) | `[no] app update auto` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAppUpdateAuto()` |
| Ch 37 (Application Patrol) | `[no] description DESCRIPTION` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDescriptionDescription()` |
| Ch 37 (Application Patrol) | `[no] security-service app-patrol activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSecurityServiceAppPatrol()` |
| Ch 37 (Application Patrol) | `app reload signatures` | Envelope 7 | `redline` | `ZyxelSecurityCmd::cmdAppReloadSignatures()` |
| Ch 37 (Application Patrol) | `app rename <profile-name> <profile-name>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAppRename()` |
| Ch 37 (Application Patrol) | `app statistics flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAppStatisticsFlush()` |
| Ch 37 (Application Patrol) | `app update` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAppUpdate()` |
| Ch 37 (Application Patrol) | `app update daily <0..23>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAppUpdateDaily()` |
| Ch 37 (Application Patrol) | `app update hourly` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAppUpdateHourly()` |
| Ch 37 (Application Patrol) | `app update weekly {sun \| mon \| tue \| wed \| thu \| fri \| sat} <0..23>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAppUpdateWeekly()` |
| Ch 37 (Application Patrol) | `application <profile-name> action {forward\|drop\|reject} {no log\|log [alert]}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdApplicationAction()` |
| Ch 37 (Application Patrol) | `no application-object <profile-name>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoApplicationObject()` |
| Ch 37 (Application Patrol) | `show app category <category_id>` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAppCategory()` |
| Ch 37 (Application Patrol) | `show app profiles` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAppProfiles()` |
| Ch 37 (Application Patrol) | `show app profiles <profile-name>` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAppProfiles()` |
| Ch 37 (Application Patrol) | `show app profiles <profile-name> application` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAppProfilesApplication()` |
| Ch 37 (Application Patrol) | `show app profiles <profile-name> application category {category_id \| all}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAppProfilesApplication()` |
| Ch 37 (Application Patrol) | `show app search-name <application_keyword>` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAppSearchName()` |
| Ch 37 (Application Patrol) | `show app signature update` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAppSignatureUpdate()` |
| Ch 37 (Application Patrol) | `show app signatures date` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAppSignaturesDate()` |
| Ch 37 (Application Patrol) | `show app signatures status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAppSignaturesStatus()` |
| Ch 37 (Application Patrol) | `show app signatures version` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAppSignaturesVersion()` |
| Ch 37 (Application Patrol) | `show app statistics collect` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAppStatisticsCollect()` |
| Ch 37 (Application Patrol) | `show app statistics summary` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAppStatisticsSummary()` |
| Ch 37 (Application Patrol) | `show app tag info` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAppTagInfo()` |
| Ch 37 (Application Patrol) | `show app update status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAppUpdateStatus()` |
| Ch 37 (Application Patrol) | `show security-service signature status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSecurityServiceSignature()` |
| Ch 37 (Application Patrol) | `show security-service status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSecurityServiceStatus()` |
| Ch 38 (Anti-Virus) | `[no] anti-virus activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusActivate()` |
| Ch 38 (Anti-Virus) | `[no] anti-virus black-list activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusBlackList()` |
| Ch 38 (Anti-Virus) | `[no] anti-virus cloud-query activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusCloudQuery()` |
| Ch 38 (Anti-Virus) | `[no] anti-virus cloud-query ftype-identify file_type` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusCloudQuery()` |
| Ch 38 (Anti-Virus) | `[no] anti-virus eicar activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusEicarActivate()` |
| Ch 38 (Anti-Virus) | `[no] anti-virus skip-unknown-file-type activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusSkipUnknown()` |
| Ch 38 (Anti-Virus) | `[no] anti-virus statistics collect` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusStatisticsCollect()` |
| Ch 38 (Anti-Virus) | `[no] anti-virus update auto` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusUpdateAuto()` |
| Ch 38 (Anti-Virus) | `[no] anti-virus white-list activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusWhiteList()` |
| Ch 38 (Anti-Virus) | `[no] bypass {white-list \| black-list}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBypass()` |
| Ch 38 (Anti-Virus) | `[no] file-decompression [unsupported destroy]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdFileDecompressionUnsupportedDestroy()` |
| Ch 38 (Anti-Virus) | `[no] infected-action {destroy \| send-win-msg}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdInfectedAction()` |
| Ch 38 (Anti-Virus) | `[no] log [alert]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdLogAlert()` |
| Ch 38 (Anti-Virus) | `[no] scan {http \| ftp \| imap4 \| smtp \| pop3}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdScan()` |
| Ch 38 (Anti-Virus) | `[no] security-service anti-virus activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSecurityServiceAntiVirus()` |
| Ch 38 (Anti-Virus) | `anti-virus black-list {md5-hash md5-pattern \| sha256-hash sha256-pattern \| file-pattern file- pattern} {activate\|deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusBlackList()` |
| Ch 38 (Anti-Virus) | `anti-virus black-list {replace <1..256> \| file-pattern file-pattern md5-hash md5-pattern \| sha256-hash sha256-pattern}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusBlackList()` |
| Ch 38 (Anti-Virus) | `anti-virus mail-infect-ext activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusMailInfect()` |
| Ch 38 (Anti-Virus) | `anti-virus profile_name` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusProfileName()` |
| Ch 38 (Anti-Virus) | `anti-virus reload signatures` | Envelope 7 | `redline` | `ZyxelSecurityCmd::cmdAntiVirusReloadSignatures()` |
| Ch 38 (Anti-Virus) | `anti-virus rename old_profile_name new_profile_name` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusRenameOld()` |
| Ch 38 (Anti-Virus) | `anti-virus scan mode {express \| hybrid \| stream}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusScanMode()` |
| Ch 38 (Anti-Virus) | `anti-virus statistics flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusStatisticsFlush()` |
| Ch 38 (Anti-Virus) | `anti-virus update ctdb` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusUpdateCtdb()` |
| Ch 38 (Anti-Virus) | `anti-virus update daily <0..23>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusUpdateDaily()` |
| Ch 38 (Anti-Virus) | `anti-virus update hourly` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusUpdateHourly()` |
| Ch 38 (Anti-Virus) | `anti-virus update signatures` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusUpdateSignatures()` |
| Ch 38 (Anti-Virus) | `anti-virus update weekly {sun \| mon \| tue \| wed \| thu \| fri \| sat} <0..23>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusUpdateWeekly()` |
| Ch 38 (Anti-Virus) | `anti-virus white-list {md5-hash md5-pattern \| sha256-hash sha256-pattern \| file-pattern file- pattern} {activate\|deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusWhiteList()` |
| Ch 38 (Anti-Virus) | `anti-virus white-list {replace <1..256> \| file-pattern file-pattern md5-hash md5-pattern \| sha256-hash sha256-pattern}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiVirusWhiteList()` |
| Ch 38 (Anti-Virus) | `description profile_description` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDescriptionProfileDescription()` |
| Ch 38 (Anti-Virus) | `exit` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdExit()` |
| Ch 38 (Anti-Virus) | `no anti-virus black-list {md5-hash md5-pattern \| sha256-hash sha256-pattern \| file-pattern file-pattern}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoAntiVirusBlack()` |
| Ch 38 (Anti-Virus) | `no anti-virus mail-infect-ext activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoAntiVirusMail()` |
| Ch 38 (Anti-Virus) | `no anti-virus white-list {md5-hash md5-pattern \| sha256-hash sha256-pattern \| file-pattern file-pattern}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoAntiVirusWhite()` |
| Ch 38 (Anti-Virus) | `security-service anti-virus inspect {all-traffic \| by-policy}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSecurityServiceAntiVirus()` |
| Ch 38 (Anti-Virus) | `show [all]` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAll()` |
| Ch 38 (Anti-Virus) | `show anti-virus black-list` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusBlack()` |
| Ch 38 (Anti-Virus) | `show anti-virus black-list status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusBlack()` |
| Ch 38 (Anti-Virus) | `show anti-virus cloud-query ftype-identify status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusCloud()` |
| Ch 38 (Anti-Virus) | `show anti-virus cloud-query status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusCloud()` |
| Ch 38 (Anti-Virus) | `show anti-virus eicar activation` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusEicar()` |
| Ch 38 (Anti-Virus) | `show anti-virus profile` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusProfile()` |
| Ch 38 (Anti-Virus) | `show anti-virus profile [profile_name]` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusProfile()` |
| Ch 38 (Anti-Virus) | `show anti-virus scan mode status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusScan()` |
| Ch 38 (Anti-Virus) | `show anti-virus search signature {all \| name virus_name} [{from id to id}]` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusSearch()` |
| Ch 38 (Anti-Virus) | `show anti-virus signatures status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusSignatures()` |
| Ch 38 (Anti-Virus) | `show anti-virus skip-unknown-file-type activation` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusSkip()` |
| Ch 38 (Anti-Virus) | `show anti-virus statistics collect` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusStatistics()` |
| Ch 38 (Anti-Virus) | `show anti-virus statistics ranking {destination \| destination6 \| source \| source6 \| virus-name} show anti-virus statistics summary` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusStatistics()` |
| Ch 38 (Anti-Virus) | `show anti-virus update` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusUpdate()` |
| Ch 38 (Anti-Virus) | `show anti-virus update status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusUpdate()` |
| Ch 38 (Anti-Virus) | `show anti-virus white-list` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusWhite()` |
| Ch 38 (Anti-Virus) | `show anti-virus white-list status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiVirusWhite()` |
| Ch 38 (Anti-Virus) | `show security-service signature status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSecurityServiceSignature()` |
| Ch 38 (Anti-Virus) | `show security-service status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSecurityServiceStatus()` |
| Ch 39 (RTLS ...............) | `[no] rtls ekahau activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdRtlsEkahauActivate()` |
| Ch 39 (RTLS ...............) | `rtls ekahau ip address <ip>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdRtlsEkahauIpAddress()` |
| Ch 39 (RTLS ...............) | `rtls ekahau ip port <1..65535>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdRtlsEkahauIpPort()` |
| Ch 39 (RTLS ...............) | `show rtls ekahau cli` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowRtlsEkahauCli()` |
| Ch 39 (RTLS ...............) | `show rtls ekahau config` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowRtlsEkahauConfig()` |
| Ch 40 (Reputation Filter) | `[no] anti-botnet log [alert]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiBotnetLogAlert()` |
| Ch 40 (Reputation Filter) | `[no] anti-botnet statistics collect` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiBotnetStatisticsCollect()` |
| Ch 40 (Reputation Filter) | `[no] anti-botnet update auto` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiBotnetUpdateAuto()` |
| Ch 40 (Reputation Filter) | `[no] dns-filter black-list activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterBlackList()` |
| Ch 40 (Reputation Filter) | `[no] dns-filter drop-malform-packet activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterDropMalform()` |
| Ch 40 (Reputation Filter) | `[no] dns-filter drop-malform-packet log` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterDropMalform()` |
| Ch 40 (Reputation Filter) | `[no] dns-filter statistics collect` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterStatisticsCollect()` |
| Ch 40 (Reputation Filter) | `[no] dns-filter white-list activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterWhiteList()` |
| Ch 40 (Reputation Filter) | `[no] ip-reputation black-list activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationBlackList()` |
| Ch 40 (Reputation Filter) | `[no] ip-reputation ebl activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationEblActivate()` |
| Ch 40 (Reputation Filter) | `[no] ip-reputation ebl update auto` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationEblUpdate()` |
| Ch 40 (Reputation Filter) | `[no] ip-reputation log [alert]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationLogAlert()` |
| Ch 40 (Reputation Filter) | `[no] ip-reputation log-all` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationLogAll()` |
| Ch 40 (Reputation Filter) | `[no] ip-reputation statistics collect` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationStatisticsCollect()` |
| Ch 40 (Reputation Filter) | `[no] ip-reputation system-protect activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationSystemProtect()` |
| Ch 40 (Reputation Filter) | `[no] ip-reputation update auto` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationUpdateAuto()` |
| Ch 40 (Reputation Filter) | `[no] ip-reputation webroot incoming-category {botnets \| denial-of-service \| exploits \| phishing \| proxy \| reputation \| scanners \| spam-sources \| tor-proxy \| web-attacks} .328 [no] ip-reputation webroot outgoing-category {botnets \| phishing}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationWebrootIncoming()` |
| Ch 40 (Reputation Filter) | `[no] ip-reputation white-list activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationWhiteList()` |
| Ch 40 (Reputation Filter) | `[no] security-service anti-botnet-IP activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSecurityServiceAntiBotnet()` |
| Ch 40 (Reputation Filter) | `[no] security-service dns-filter activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSecurityServiceDnsFilter()` |
| Ch 40 (Reputation Filter) | `[no] security-service ip-reputation activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSecurityServiceIpReputation()` |
| Ch 40 (Reputation Filter) | `[no] third-dns-server {ip \| interface_name {1st-dns \| 2nd-dns \| 3rd-dns} \| ZyWALL} 129 [no] threat-website action {block \| log \| pass \| warn}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThirdDnsServerZywall()` |
| Ch 40 (Reputation Filter) | `[no] threat-website block message message` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteBlockMessage()` |
| Ch 40 (Reputation Filter) | `[no] threat-website block redirect url` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteBlockRedirect()` |
| Ch 40 (Reputation Filter) | `[no] threat-website category {anonymizers \| browser-exploits \| botnets \| compromised \| mali- cious-downloads \| malicious-sites \| malware \| phishing \| phishing-fraud \| spam-sites \| spam-urls \| spyware-adware-keyloggers}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteCategory()` |
| Ch 40 (Reputation Filter) | `[no] threat-website ebl activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteEblActivate()` |
| Ch 40 (Reputation Filter) | `[no] threat-website ebl update auto` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteEblUpdate()` |
| Ch 40 (Reputation Filter) | `[no] threat-website forbid-list activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteForbidList()` |
| Ch 40 (Reputation Filter) | `[no] threat-website profile <profile name> category {anonymizers \| malware \| botnets \| phishing [no] threat-website profile <profile name> description <description>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteProfileCategory()` |
| Ch 40 (Reputation Filter) | `[no] threat-website profile <profile name> log` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteProfileLog()` |
| Ch 40 (Reputation Filter) | `[no] threat-website statistics collect` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteStatisticsCollect()` |
| Ch 40 (Reputation Filter) | `[no] threat-website trust-list activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteTrustList()` |
| Ch 40 (Reputation Filter) | `[no] utm-manager {doh\| dot\| system-protect} defaultport port number` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdUtmManagerDefaultportPort()` |
| Ch 40 (Reputation Filter) | `[no] {ipv4 \| ipv4_cidr \| ipv4_range \| wildcard_domainname \| tld \|ipv6 \| ipv6_range \| ipv6_prefix } [no] {ipv4 \| ipv4_cidr \| ipv4_range \| wildcard_domainname \| top_level_domain}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdGeneric()` |
| Ch 40 (Reputation Filter) | `anti-botnet action {forward \| reject-both \| reject-receiver \| reject-sender}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiBotnetAction()` |
| Ch 40 (Reputation Filter) | `anti-botnet statistics flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiBotnetStatisticsFlush()` |
| Ch 40 (Reputation Filter) | `anti-botnet update daily <0..23>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiBotnetUpdateDaily()` |
| Ch 40 (Reputation Filter) | `anti-botnet update hourly` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiBotnetUpdateHourly()` |
| Ch 40 (Reputation Filter) | `anti-botnet update signatures` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiBotnetUpdateSignatures()` |
| Ch 40 (Reputation Filter) | `anti-botnet update weekly {sun \| mon \| tue \| wed \| thu \| fri \| sat} <0..23>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiBotnetUpdateWeekly()` |
| Ch 40 (Reputation Filter) | `dns-filter black-list FQDN {activate\|deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterBlackList()` |
| Ch 40 (Reputation Filter) | `dns-filter black-list replace <1..256> FQDN {activate\|deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterBlackList()` |
| Ch 40 (Reputation Filter) | `dns-filter fake-dns-response-ttl <300...86400>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterFakeDns()` |
| Ch 40 (Reputation Filter) | `dns-filter profile profilename` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterProfileProfilename()` |
| Ch 40 (Reputation Filter) | `dns-filter redirect-ip custom IPv4` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterRedirectIp()` |
| Ch 40 (Reputation Filter) | `dns-filter redirect-ip default` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterRedirectIp()` |
| Ch 40 (Reputation Filter) | `dns-filter rename old_profile_name new_profile_name` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterRenameOld()` |
| Ch 40 (Reputation Filter) | `dns-filter secure-dns action {drop \| pass}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterSecureDns()` |
| Ch 40 (Reputation Filter) | `dns-filter secure-dns {log \| no log}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterSecureDns()` |
| Ch 40 (Reputation Filter) | `dns-filter statistics flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterStatisticsFlush()` |
| Ch 40 (Reputation Filter) | `dns-filter white-list FQDN {activate\|deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterWhiteList()` |
| Ch 40 (Reputation Filter) | `dns-filter white-list replace <1..256> FQDN {activate\|deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsFilterWhiteList()` |
| Ch 40 (Reputation Filter) | `exit` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdExit()` |
| Ch 40 (Reputation Filter) | `ip-reputation action {block \| pass}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationAction()` |
| Ch 40 (Reputation Filter) | `ip-reputation action-level {high \| medium \| low}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationActionLevel()` |
| Ch 40 (Reputation Filter) | `ip-reputation black-list replace <1..256> IPv4 {activate\|deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationBlackList()` |
| Ch 40 (Reputation Filter) | `ip-reputation black-list {IPv4\|IPv4CIDR} {activate\|deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationBlackList()` |
| Ch 40 (Reputation Filter) | `ip-reputation ebl <profile name>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationEbl()` |
| Ch 40 (Reputation Filter) | `ip-reputation ebl rename old_profile_name new_profile_name` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationEblRename()` |
| Ch 40 (Reputation Filter) | `ip-reputation ebl update` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationEblUpdate()` |
| Ch 40 (Reputation Filter) | `ip-reputation ebl update daily <0..23>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationEblUpdate()` |
| Ch 40 (Reputation Filter) | `ip-reputation ebl update hourly` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationEblUpdate()` |
| Ch 40 (Reputation Filter) | `ip-reputation ebl update weekly {sun \|mon\|tue\|wed\|thu\|fri\|sat} <0..23>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationEblUpdate()` |
| Ch 40 (Reputation Filter) | `ip-reputation statistics flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationStatisticsFlush()` |
| Ch 40 (Reputation Filter) | `ip-reputation update daily <0..23>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationUpdateDaily()` |
| Ch 40 (Reputation Filter) | `ip-reputation update hourly` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationUpdateHourly()` |
| Ch 40 (Reputation Filter) | `ip-reputation update signatures` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationUpdateSignatures()` |
| Ch 40 (Reputation Filter) | `ip-reputation update weekly {sun \| mon \| tue \| wed \| thu \| fri \| sat} <0..23>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationUpdateWeekly()` |
| Ch 40 (Reputation Filter) | `ip-reputation white-list replace <1..256> IPv4 {activate\|deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationWhiteList()` |
| Ch 40 (Reputation Filter) | `ip-reputation white-list {IPv4\|IPv4CIDR} {activate\|deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIpReputationWhiteList()` |
| Ch 40 (Reputation Filter) | `no dns-filter black-list FQDN` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoDnsFilterBlack()` |
| Ch 40 (Reputation Filter) | `no dns-filter white-list FQDN` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoDnsFilterWhite()` |
| Ch 40 (Reputation Filter) | `no ip-reputation black-list IPv4` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoIpReputationBlack()` |
| Ch 40 (Reputation Filter) | `no ip-reputation ebl <profile name>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoIpReputationEbl()` |
| Ch 40 (Reputation Filter) | `no ip-reputation white-list IPv4` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoIpReputationWhite()` |
| Ch 40 (Reputation Filter) | `no threat-website ebl <profile name>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoThreatWebsiteEbl()` |
| Ch 40 (Reputation Filter) | `security-service dns-filter inspect {all-traffic \| by-policy}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSecurityServiceDnsFilter()` |
| Ch 40 (Reputation Filter) | `security-service threat-website inspect {all-traffic \| by-policy}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSecurityServiceThreatWebsite()` |
| Ch 40 (Reputation Filter) | `show anti-botnet signature update` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiBotnetSignature()` |
| Ch 40 (Reputation Filter) | `show anti-botnet signatures date` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiBotnetSignatures()` |
| Ch 40 (Reputation Filter) | `show anti-botnet signatures number` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiBotnetSignatures()` |
| Ch 40 (Reputation Filter) | `show anti-botnet signatures version` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiBotnetSignatures()` |
| Ch 40 (Reputation Filter) | `show anti-botnet statistics collect status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiBotnetStatistics()` |
| Ch 40 (Reputation Filter) | `show anti-botnet statistics recent-activities` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiBotnetStatistics()` |
| Ch 40 (Reputation Filter) | `show anti-botnet statistics summary` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiBotnetStatistics()` |
| Ch 40 (Reputation Filter) | `show anti-botnet status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiBotnetStatus()` |
| Ch 40 (Reputation Filter) | `show anti-botnet update status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiBotnetUpdate()` |
| Ch 40 (Reputation Filter) | `show dns-filter dashboard statistics summary` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsFilterDashboard()` |
| Ch 40 (Reputation Filter) | `show dns-filter fake-dns-response-ttl` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsFilterFake()` |
| Ch 40 (Reputation Filter) | `show dns-filter profile {all \| profilename}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsFilterProfile()` |
| Ch 40 (Reputation Filter) | `show dns-filter search FQDN` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsFilterSearch()` |
| Ch 40 (Reputation Filter) | `show dns-filter statistics collect` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsFilterStatistics()` |
| Ch 40 (Reputation Filter) | `show dns-filter statistics list` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsFilterStatistics()` |
| Ch 40 (Reputation Filter) | `show dns-filter statistics summary` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsFilterStatistics()` |
| Ch 40 (Reputation Filter) | `show dns-filter status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsFilterStatus()` |
| Ch 40 (Reputation Filter) | `show dns-filter {white-list\|black-list}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsFilter()` |
| Ch 40 (Reputation Filter) | `show ip-reputation ebl` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputationEbl()` |
| Ch 40 (Reputation Filter) | `show ip-reputation ebl <1..4> {date \| number}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputationEbl()` |
| Ch 40 (Reputation Filter) | `show ip-reputation ebl <profile name>` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputationEbl()` |
| Ch 40 (Reputation Filter) | `show ip-reputation ebl signature update` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputationEbl()` |
| Ch 40 (Reputation Filter) | `show ip-reputation search {Ipv6Address \| Ipv4Address}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputationSearch()` |
| Ch 40 (Reputation Filter) | `show ip-reputation signature update` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputationSignature()` |
| Ch 40 (Reputation Filter) | `show ip-reputation signatures date` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputationSignatures()` |
| Ch 40 (Reputation Filter) | `show ip-reputation signatures number` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputationSignatures()` |
| Ch 40 (Reputation Filter) | `show ip-reputation signatures version` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputationSignatures()` |
| Ch 40 (Reputation Filter) | `show ip-reputation statistics collect status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputationStatistics()` |
| Ch 40 (Reputation Filter) | `show ip-reputation statistics recent-activities` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputationStatistics()` |
| Ch 40 (Reputation Filter) | `show ip-reputation statistics summary` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputationStatistics()` |
| Ch 40 (Reputation Filter) | `show ip-reputation status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputationStatus()` |
| Ch 40 (Reputation Filter) | `show ip-reputation update status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputationUpdate()` |
| Ch 40 (Reputation Filter) | `show ip-reputation webroot {incoming-category \| outgoing-category}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputationWebroot()` |
| Ch 40 (Reputation Filter) | `show ip-reputation {white-list\|black-list}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputation()` |
| Ch 40 (Reputation Filter) | `show ip-reputation {white-list\|black-list} status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIpReputationStatus()` |
| Ch 40 (Reputation Filter) | `show secure-dns search {FQDN\|IP Address}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSecureDnsSearch()` |
| Ch 40 (Reputation Filter) | `show security-service signature status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSecurityServiceSignature()` |
| Ch 40 (Reputation Filter) | `show security-service status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSecurityServiceStatus()` |
| Ch 40 (Reputation Filter) | `show security-service status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSecurityServiceStatus()` |
| Ch 40 (Reputation Filter) | `show security-service status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSecurityServiceStatus()` |
| Ch 40 (Reputation Filter) | `show threat-website ebl` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowThreatWebsiteEbl()` |
| Ch 40 (Reputation Filter) | `show threat-website ebl <1..4> {date \| number}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowThreatWebsiteEbl()` |
| Ch 40 (Reputation Filter) | `show threat-website ebl <profile name>` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowThreatWebsiteEbl()` |
| Ch 40 (Reputation Filter) | `show threat-website ebl signature update` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowThreatWebsiteEbl()` |
| Ch 40 (Reputation Filter) | `show threat-website search {ipv6address \| ipv4address}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowThreatWebsiteSearch()` |
| Ch 40 (Reputation Filter) | `show threat-website statistics collect` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowThreatWebsiteStatistics()` |
| Ch 40 (Reputation Filter) | `show threat-website statistics list` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowThreatWebsiteStatistics()` |
| Ch 40 (Reputation Filter) | `show threat-website statistics summary` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowThreatWebsiteStatistics()` |
| Ch 40 (Reputation Filter) | `show threat-website status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowThreatWebsiteStatus()` |
| Ch 40 (Reputation Filter) | `show threat-website {trust\|forbid}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowThreatWebsite()` |
| Ch 40 (Reputation Filter) | `show utm-manager {doh\| dot\| system-protect} defaultport` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowUtmManagerDefaultport()` |
| Ch 40 (Reputation Filter) | `source <url>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSource()` |
| Ch 40 (Reputation Filter) | `source <url>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSource()` |
| Ch 40 (Reputation Filter) | `threat-website ebl <profile name>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteEbl()` |
| Ch 40 (Reputation Filter) | `threat-website ebl rename old_profile_name new_profile_name` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteEblRename()` |
| Ch 40 (Reputation Filter) | `threat-website ebl update` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteEblUpdate()` |
| Ch 40 (Reputation Filter) | `threat-website ebl update daily <0..23>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteEblUpdate()` |
| Ch 40 (Reputation Filter) | `threat-website ebl update hourly` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteEblUpdate()` |
| Ch 40 (Reputation Filter) | `threat-website ebl update weekly {sun \|mon\|tue\|wed\|thu\|fri\|sat} <0..23>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteEblUpdate()` |
| Ch 40 (Reputation Filter) | `threat-website profile <profile name> action {block \| pass \| warn}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteProfileAction()` |
| Ch 40 (Reputation Filter) | `threat-website profile profile_name` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteProfileProfile()` |
| Ch 40 (Reputation Filter) | `threat-website rename old_profile_name new_profile_name` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteRenameOld()` |
| Ch 40 (Reputation Filter) | `threat-website statistics flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsiteStatisticsFlush()` |
| Ch 40 (Reputation Filter) | `threat-website {trust \| forbid}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdThreatWebsite()` |
| Ch 40 (Reputation Filter) | `\| browser-exploits \| phishing-fraud \| compromised \| spam-sites \| malicious-downloads \| spam- urls \| malicious-sites \| spyware-adware-keyloggers}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBrowserExploitsPhishingFraud()` |
| Ch 41 (Sandboxing) | `[no] sandbox file-type {archives \| chm \| eicar \| executables \| macromedia-flash-data \| ms-of- fice-document \| pdf \| rtf \| unknow-type}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSandboxFileType()` |
| Ch 41 (Sandboxing) | `[no] sandbox queue-packet` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSandboxQueuePacket()` |
| Ch 41 (Sandboxing) | `[no] sandbox statistics collect` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSandboxStatisticsCollect()` |
| Ch 41 (Sandboxing) | `[no] security-service sandbox activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSecurityServiceSandboxActivate()` |
| Ch 41 (Sandboxing) | `sandbox dashboard statistics flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSandboxDashboardStatisticsFlush()` |
| Ch 41 (Sandboxing) | `sandbox dashboard statistics flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSandboxDashboardStatisticsFlush()` |
| Ch 41 (Sandboxing) | `sandbox file-scanning-log {log \| log-alert \| no}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSandboxFileScanningLog()` |
| Ch 41 (Sandboxing) | `sandbox file-send-log {log \| log-alert \| no}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSandboxFileSendLog()` |
| Ch 41 (Sandboxing) | `sandbox malicious-action malicious {allow \| destroy} {log \| log-alert \| no}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSandboxMaliciousActionMalicious()` |
| Ch 41 (Sandboxing) | `sandbox malicious-action suspicious {allow \| destroy} {log \| log-alert \| no}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSandboxMaliciousActionSuspicious()` |
| Ch 41 (Sandboxing) | `sandbox mdb flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSandboxMdbFlush()` |
| Ch 41 (Sandboxing) | `sandbox response-clean-log {log \| log-alert \| no}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSandboxResponseCleanLog()` |
| Ch 41 (Sandboxing) | `sandbox server-file {delete \| keep}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSandboxServerFile()` |
| Ch 41 (Sandboxing) | `sandbox statistics flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSandboxStatisticsFlush()` |
| Ch 41 (Sandboxing) | `show sandbox file-type all` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSandboxFileType()` |
| Ch 41 (Sandboxing) | `show sandbox file-type status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSandboxFileType()` |
| Ch 41 (Sandboxing) | `show sandbox statistics collect` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSandboxStatisticsCollect()` |
| Ch 41 (Sandboxing) | `show sandbox statistics dashboard summary` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSandboxStatisticsDashboard()` |
| Ch 41 (Sandboxing) | `show sandbox statistics ranking file-name` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSandboxStatisticsRanking()` |
| Ch 41 (Sandboxing) | `show sandbox statistics summary` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSandboxStatisticsSummary()` |
| Ch 41 (Sandboxing) | `show sandbox status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSandboxStatus()` |
| Ch 41 (Sandboxing) | `show security-service status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSecurityServiceStatus()` |
| Ch 42 (IDP Commands) | `[no] idp` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdp()` |
| Ch 42 (IDP Commands) | `[no] idp signature update auto` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpSignatureUpdateAuto()` |
| Ch 42 (IDP Commands) | `[no] idp statistics collect` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpStatisticsCollect()` |
| Ch 42 (IDP Commands) | `[no] security-service ips activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSecurityServiceIpsActivate()` |
| Ch 42 (IDP Commands) | `[no] sid {1–4294967295}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSid()` |
| Ch 42 (IDP Commands) | `[no] signature sid activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSignatureSidActivate()` |
| Ch 42 (IDP Commands) | `description description2` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDescriptionDescription2()` |
| Ch 42 (IDP Commands) | `exit` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdExit()` |
| Ch 42 (IDP Commands) | `idp customize signature edit quoted_string` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpCustomizeSignatureEdit()` |
| Ch 42 (IDP Commands) | `idp customize signature quoted_string` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpCustomizeSignatureQuoted()` |
| Ch 42 (IDP Commands) | `idp customize_import name sig_name` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpCustomizeImportName()` |
| Ch 42 (IDP Commands) | `idp packet-capture default setting` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpPacketCaptureDefault()` |
| Ch 42 (IDP Commands) | `idp packet-capture select {add-id sid \| del-id sid}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpPacketCaptureSelect()` |
| Ch 42 (IDP Commands) | `idp packet-capture select {enable \| disable}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpPacketCaptureSelect()` |
| Ch 42 (IDP Commands) | `idp packet-capture show status` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpPacketCaptureShow()` |
| Ch 42 (IDP Commands) | `idp packet-capture {enable \| disable}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpPacketCapture()` |
| Ch 42 (IDP Commands) | `idp reload` | Envelope 7 | `redline` | `ZyxelSecurityCmd::cmdIdpReload()` |
| Ch 42 (IDP Commands) | `idp rename signature profile1 profile2` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpRenameSignatureProfile1()` |
| Ch 42 (IDP Commands) | `idp search signature my_profile name quoted_string sid SID severity severity_mask platform plat- form_mask classtype classtype_mask service service_mask activate {any \| yes \| no} log {any \| no \| log \| log-alert} action action_mask` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpSearchSignatureMy()` |
| Ch 42 (IDP Commands) | `idp session-block period {1–3600}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpSessionBlockPeriod()` |
| Ch 42 (IDP Commands) | `idp session-block {activate \| deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpSessionBlock()` |
| Ch 42 (IDP Commands) | `idp signature default_profile` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpSignatureDefaultProfile()` |
| Ch 42 (IDP Commands) | `idp signature mode {detection \| prevention}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpSignatureMode()` |
| Ch 42 (IDP Commands) | `idp signature newpro [base {all \| lan \| wan \| dmz \| none}]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpSignatureNewproBase()` |
| Ch 42 (IDP Commands) | `idp signature profile signature sid {activate \| log [alert] \| action {drop \| reject-sender \| reject-receiver \| reject-both}}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpSignatureProfileSignature()` |
| Ch 42 (IDP Commands) | `idp signature update daily <0..23>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpSignatureUpdateDaily()` |
| Ch 42 (IDP Commands) | `idp signature update hourly` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpSignatureUpdateHourly()` |
| Ch 42 (IDP Commands) | `idp signature update signatures` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpSignatureUpdateSignatures()` |
| Ch 42 (IDP Commands) | `idp signature update weekly {sun \| mon \| tue \| wed \| thu \| fri \| sat} <0..23>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpSignatureUpdateWeekly()` |
| Ch 42 (IDP Commands) | `idp statistics flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpStatisticsFlush()` |
| Ch 42 (IDP Commands) | `idp system-protect {activate\| deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpSystemProtect()` |
| Ch 42 (IDP Commands) | `idp white-list` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdIdpWhiteList()` |
| Ch 42 (IDP Commands) | `no idp customize signature custom_sid` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoIdpCustomizeSignature()` |
| Ch 42 (IDP Commands) | `no idp signature profile3` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoIdpSignatureProfile3()` |
| Ch 42 (IDP Commands) | `no signature sid action` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoSignatureSidAction()` |
| Ch 42 (IDP Commands) | `no signature sid action` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoSignatureSidAction()` |
| Ch 42 (IDP Commands) | `no signature sid log` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoSignatureSidLog()` |
| Ch 42 (IDP Commands) | `security-service ips inspect {all-traffic \| by-policy}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSecurityServiceIpsInspect()` |
| Ch 42 (IDP Commands) | `show idp` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdp()` |
| Ch 42 (IDP Commands) | `show idp engine version` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpEngineVersion()` |
| Ch 42 (IDP Commands) | `show idp rate_based_sig <profile name>` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpRateBased()` |
| Ch 42 (IDP Commands) | `show idp search signature my_profile name quoted_string sid SID severity severity_mask platform platform_mask classtype classtype_mask service service_mask activate {any \| yes \| no} log {any \| no \| log \| log-alert} action action_mask` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpSearchSignature()` |
| Ch 42 (IDP Commands) | `show idp signature all details` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpSignatureAll()` |
| Ch 42 (IDP Commands) | `show idp signature base profile {all\|none\|wan\|lan\|dmz} settings` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpSignatureBase()` |
| Ch 42 (IDP Commands) | `show idp signature mode` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpSignatureMode()` |
| Ch 42 (IDP Commands) | `show idp signature profile signature all details` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpSignatureProfile()` |
| Ch 42 (IDP Commands) | `show idp signature profile signature sid details` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpSignatureProfile()` |
| Ch 42 (IDP Commands) | `show idp signature profiles` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpSignatureProfiles()` |
| Ch 42 (IDP Commands) | `show idp signature signatures {version \| date \| number}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpSignatureSignatures()` |
| Ch 42 (IDP Commands) | `show idp signature update` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpSignatureUpdate()` |
| Ch 42 (IDP Commands) | `show idp signature update status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpSignatureUpdate()` |
| Ch 42 (IDP Commands) | `show idp signatures custom-signature all details` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpSignaturesCustom()` |
| Ch 42 (IDP Commands) | `show idp signatures custom-signature custom_sid {details \| contents \| non-contents} 356 show idp signatures custom-signature number` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpSignaturesCustom()` |
| Ch 42 (IDP Commands) | `show idp signatures date` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpSignaturesDate()` |
| Ch 42 (IDP Commands) | `show idp signatures number` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpSignaturesNumber()` |
| Ch 42 (IDP Commands) | `show idp signatures version` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpSignaturesVersion()` |
| Ch 42 (IDP Commands) | `show idp statistics collect` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpStatisticsCollect()` |
| Ch 42 (IDP Commands) | `show idp statistics collect status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpStatisticsCollect()` |
| Ch 42 (IDP Commands) | `show idp statistics ranking {signature-name \| source \| source6 \| destination \|destination6 \| rate-based}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpStatisticsRanking()` |
| Ch 42 (IDP Commands) | `show idp statistics summary` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpStatisticsSummary()` |
| Ch 42 (IDP Commands) | `show idp white-list` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpWhiteList()` |
| Ch 42 (IDP Commands) | `show idp {signature \| anomaly} base profile` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowIdpBaseProfile()` |
| Ch 42 (IDP Commands) | `show security-service status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSecurityServiceStatus()` |
| Ch 42 (IDP Commands) | `signature sid action {drop \| reject-sender \| reject-receiver \| reject-both}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSignatureSidAction()` |
| Ch 42 (IDP Commands) | `signature sid action {drop \| reject-sender \| reject-receiver \| reject-both}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSignatureSidAction()` |
| Ch 42 (IDP Commands) | `signature sid block_period` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSignatureSidBlockPeriod()` |
| Ch 42 (IDP Commands) | `signature sid counts` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSignatureSidCounts()` |
| Ch 42 (IDP Commands) | `signature sid log [alert] ..................................................................................................................` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSignatureSidLogAlert()` |
| Ch 42 (IDP Commands) | `signature sid seconds ......................................................................................................................` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSignatureSidSeconds()` |
| Ch 42 (IDP Commands) | `{anomaly \| signature \| system-protect} activation` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdActivation()` |
| Ch 42 (IDP Commands) | `{anomaly \| signature} activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdActivate()` |
| Ch 43 (Content Filtering) | `[no] content-filter block message message` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterBlockMessage()` |
| Ch 43 (Content Filtering) | `[no] content-filter block redirect redirect_url` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterBlockRedirect()` |
| Ch 43 (Content Filtering) | `[no] content-filter https-domain-filter activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterHttpsDomain()` |
| Ch 43 (Content Filtering) | `[no] content-filter https-domain-filter block-page activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterHttpsDomain()` |
| Ch 43 (Content Filtering) | `[no] content-filter profile <filtering_profile> safesearch` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileSafesearch()` |
| Ch 43 (Content Filtering) | `[no] content-filter profile filtering_profile` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileFiltering()` |
| Ch 43 (Content Filtering) | `[no] content-filter profile filtering_profile category {category_name}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileFiltering()` |
| Ch 43 (Content Filtering) | `[no] content-filter profile filtering_profile commtouch-url category {category_name} 372 [no] content-filter profile filtering_profile custom` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileFiltering()` |
| Ch 43 (Content Filtering) | `[no] content-filter profile filtering_profile custom activex` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileFiltering()` |
| Ch 43 (Content Filtering) | `[no] content-filter profile filtering_profile custom cookie` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileFiltering()` |
| Ch 43 (Content Filtering) | `[no] content-filter profile filtering_profile custom java` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileFiltering()` |
| Ch 43 (Content Filtering) | `[no] content-filter profile filtering_profile custom proxy` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileFiltering()` |
| Ch 43 (Content Filtering) | `[no] content-filter profile filtering_profile custom trust-allow-features` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileFiltering()` |
| Ch 43 (Content Filtering) | `[no] content-filter profile filtering_profile custom trust-only` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileFiltering()` |
| Ch 43 (Content Filtering) | `[no] content-filter profile filtering_profile log-level-info` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileFiltering()` |
| Ch 43 (Content Filtering) | `[no] content-filter profile filtering_profile url-server` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileFiltering()` |
| Ch 43 (Content Filtering) | `[no] content-filter safesearch <name>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterSafesearch()` |
| Ch 43 (Content Filtering) | `[no] content-filter service-timeout service_timeout` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterServiceTimeout()` |
| Ch 43 (Content Filtering) | `[no] content-filter sslv3 action block` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterSslv3Action()` |
| Ch 43 (Content Filtering) | `[no] content-filter sslv3 action block` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterSslv3Action()` |
| Ch 43 (Content Filtering) | `[no] content-filter statistics collect` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterStatisticsCollect()` |
| Ch 43 (Content Filtering) | `[no] dns-content-filter black-list activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsContentFilterBlack()` |
| Ch 43 (Content Filtering) | `[no] dns-content-filter statistics collect` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsContentFilterStatistics()` |
| Ch 43 (Content Filtering) | `[no] dns-content-filter white-list activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsContentFilterWhite()` |
| Ch 43 (Content Filtering) | `[no] forbid_hosts` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdForbidHosts()` |
| Ch 43 (Content Filtering) | `[no] keyword` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdKeyword()` |
| Ch 43 (Content Filtering) | `[no] trust_hosts` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdTrustHosts()` |
| Ch 43 (Content Filtering) | `content-filter cf-queue flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterCfQueue()` |
| Ch 43 (Content Filtering) | `content-filter common-list {trust\|forbid}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterCommonList()` |
| Ch 43 (Content Filtering) | `content-filter https-domain-filter block-cache-ttl <1~60>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterHttpsDomain()` |
| Ch 43 (Content Filtering) | `content-filter https-domain-filter block-page port <port>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterHttpsDomain()` |
| Ch 43 (Content Filtering) | `content-filter https-domain-filter forward-cache-ttl <1~1440>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterHttpsDomain()` |
| Ch 43 (Content Filtering) | `content-filter passed warning flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterPassedWarning()` |
| Ch 43 (Content Filtering) | `content-filter passed warning timeout <1..1440>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterPassedWarning()` |
| Ch 43 (Content Filtering) | `content-filter profile filtering_profile [commtouch-url] log-all` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileFiltering()` |
| Ch 43 (Content Filtering) | `content-filter profile filtering_profile [commtouch-url] match {block \| log \| pass} 373 content-filter profile filtering_profile [commtouch-url] offline {block \| log \| warn \| pass} content-filter profile filtering_profile [commtouch-url] unrate {block \| log \| warn \| pass} content-filter profile filtering_profile commtouch-url log-all` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileFiltering()` |
| Ch 43 (Content Filtering) | `content-filter profile filtering_profile commtouch-url match {block \| log \| pass} 372 content-filter profile filtering_profile commtouch-url match-unsafe {block \| log \| warn \|pass} content-filter profile filtering_profile commtouch-url offline {block \| log \| warn \| pass} content-filter profile filtering_profile commtouch-url unrate {block \| log \| warn \| pass} 373 content-filter profile filtering_profile custom-list forbid` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileFiltering()` |
| Ch 43 (Content Filtering) | `content-filter profile filtering_profile custom-list keyword` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileFiltering()` |
| Ch 43 (Content Filtering) | `content-filter profile filtering_profile custom-list trust` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterProfileFiltering()` |
| Ch 43 (Content Filtering) | `content-filter report deactivate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterReportDeactivate()` |
| Ch 43 (Content Filtering) | `content-filter report server {ip_address \| hostname}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterReportServer()` |
| Ch 43 (Content Filtering) | `content-filter statistics flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterStatisticsFlush()` |
| Ch 43 (Content Filtering) | `content-filter url-cache clear` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterUrlCache()` |
| Ch 43 (Content Filtering) | `content-filter url-cache clear url` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterUrlCache()` |
| Ch 43 (Content Filtering) | `content-filter url-server test` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterUrlServer()` |
| Ch 43 (Content Filtering) | `content-filter url-server test commtouch` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdContentFilterUrlServer()` |
| Ch 43 (Content Filtering) | `cookie match <string>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCookieMatch()` |
| Ch 43 (Content Filtering) | `cookie parameter <string>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCookieParameter()` |
| Ch 43 (Content Filtering) | `cookie value <string>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCookieValue()` |
| Ch 43 (Content Filtering) | `dns-content-filter black-list FQDN {activate \| deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsContentFilterBlack()` |
| Ch 43 (Content Filtering) | `dns-content-filter black-list replace <1..256> FQDN {activate \| deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsContentFilterBlack()` |
| Ch 43 (Content Filtering) | `dns-content-filter fake-dns-response-ttl <300...86400>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsContentFilterFake()` |
| Ch 43 (Content Filtering) | `dns-content-filter profile <profilename>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsContentFilterProfile()` |
| Ch 43 (Content Filtering) | `dns-content-filter redirect-ip custom IPv4` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsContentFilterRedirect()` |
| Ch 43 (Content Filtering) | `dns-content-filter redirect-ip default` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsContentFilterRedirect()` |
| Ch 43 (Content Filtering) | `dns-content-filter statistics flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsContentFilterStatistics()` |
| Ch 43 (Content Filtering) | `dns-content-filter white-list FQDN {activate \| deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsContentFilterWhite()` |
| Ch 43 (Content Filtering) | `dns-content-filter white-list replace <1..256> FQDN {activate \| deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDnsContentFilterWhite()` |
| Ch 43 (Content Filtering) | `domain match <string> .............................................................................................................................` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDomainMatch()` |
| Ch 43 (Content Filtering) | `domain not-match <string>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDomainNotMatch()` |
| Ch 43 (Content Filtering) | `exit` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdExit()` |
| Ch 43 (Content Filtering) | `exit` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdExit()` |
| Ch 43 (Content Filtering) | `exit` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdExit()` |
| Ch 43 (Content Filtering) | `exit` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdExit()` |
| Ch 43 (Content Filtering) | `exit` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdExit()` |
| Ch 43 (Content Filtering) | `no content-filter profile filtering_profile [commtouch-url] match {log}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoContentFilterProfile()` |
| Ch 43 (Content Filtering) | `no content-filter profile filtering_profile [commtouch-url] offline {log}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoContentFilterProfile()` |
| Ch 43 (Content Filtering) | `no content-filter profile filtering_profile [commtouch-url] unrate {log}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoContentFilterProfile()` |
| Ch 43 (Content Filtering) | `no content-filter profile filtering_profile commtouch-url match {log}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoContentFilterProfile()` |
| Ch 43 (Content Filtering) | `no content-filter profile filtering_profile commtouch-url match-unsafe {log}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoContentFilterProfile()` |
| Ch 43 (Content Filtering) | `no content-filter profile filtering_profile commtouch-url offline {log}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoContentFilterProfile()` |
| Ch 43 (Content Filtering) | `no content-filter profile filtering_profile commtouch-url unrate {log}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoContentFilterProfile()` |
| Ch 43 (Content Filtering) | `no dns-content-filter black-list FQDN` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoDnsContentFilter()` |
| Ch 43 (Content Filtering) | `no dns-content-filter white-list FQDN` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoDnsContentFilter()` |
| Ch 43 (Content Filtering) | `show content-filter common-list {trust\|forbid}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowContentFilterCommon()` |
| Ch 43 (Content Filtering) | `show content-filter https-domain-filter status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowContentFilterHttps()` |
| Ch 43 (Content Filtering) | `show content-filter passed warning` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowContentFilterPassed()` |
| Ch 43 (Content Filtering) | `show content-filter profile` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowContentFilterProfile()` |
| Ch 43 (Content Filtering) | `show content-filter profile [filtering_profile]` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowContentFilterProfile()` |
| Ch 43 (Content Filtering) | `show content-filter profile [filtering_profile] commtouch` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowContentFilterProfile()` |
| Ch 43 (Content Filtering) | `show content-filter profile commtouch` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowContentFilterProfile()` |
| Ch 43 (Content Filtering) | `show content-filter safesearch` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowContentFilterSafesearch()` |
| Ch 43 (Content Filtering) | `show content-filter settings` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowContentFilterSettings()` |
| Ch 43 (Content Filtering) | `show content-filter statistics collect` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowContentFilterStatistics()` |
| Ch 43 (Content Filtering) | `show content-filter statistics summary` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowContentFilterStatistics()` |
| Ch 43 (Content Filtering) | `show content-filter statistics summary` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowContentFilterStatistics()` |
| Ch 43 (Content Filtering) | `show dns-content-filter dashboard statistics summary` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsContentFilter()` |
| Ch 43 (Content Filtering) | `show dns-content-filter fake-dns-response-ttl` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsContentFilter()` |
| Ch 43 (Content Filtering) | `show dns-content-filter profile {all \| profileName}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsContentFilter()` |
| Ch 43 (Content Filtering) | `show dns-content-filter search FQDN` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsContentFilter()` |
| Ch 43 (Content Filtering) | `show dns-content-filter statistics collect` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsContentFilter()` |
| Ch 43 (Content Filtering) | `show dns-content-filter statistics list` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsContentFilter()` |
| Ch 43 (Content Filtering) | `show dns-content-filter statistics summary` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsContentFilter()` |
| Ch 43 (Content Filtering) | `show dns-content-filter status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsContentFilter()` |
| Ch 43 (Content Filtering) | `show dns-content-filter {white-list \| black-list}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowDnsContentFilter()` |
| Ch 43 (Content Filtering) | `url [timeout query_timeout]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdUrlTimeoutQueryTimeout()` |
| Ch 43 (Content Filtering) | `url match <string> .............................................................................................................................` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdUrlMatch()` |
| Ch 43 (Content Filtering) | `url not-match <string> .............................................................................................................................` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdUrlNotMatch()` |
| Ch 43 (Content Filtering) | `url parameter <string>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdUrlParameter()` |
| Ch 43 (Content Filtering) | `url value <string>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdUrlValue()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam black-list [rule_number] e-mail email {activate\|deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamBlackList()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam black-list [rule_number] ip6-address ipv6_subnet {activate\|deactivate} 405 [no] anti-spam black-list [rule_number] ip-address ip subnet_mask {activate\|deactivate} 405 [no] anti-spam black-list [rule_number] mail-header mail-header mail-header-value {activate\|de- activate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamBlackList()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam black-list [rule_number] subject subject {activate\|deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamBlackList()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam black-list activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamBlackList()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam dnsbl activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamDnsblActivate()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam ip-reputation activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamIpReputation()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam ip-reputation private-check activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamIpReputation()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam mail-content activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamMailContent()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam mail-phishing activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamMailPhishing()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam statistics collect` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamStatisticsCollect()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam virus-outbreak activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamVirusOutbreak()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam white-list [rule_number] e-mail email {activate\|deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamWhiteList()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam white-list [rule_number] ip6-address ipv6_subnet {activate\|deactivate} 404 [no] anti-spam white-list [rule_number] ip-address ip subnet_mask {activate\|deactivate} 404 [no] anti-spam white-list [rule_number] mail-header mail-header mail-header-value {activate\|de- activate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamWhiteList()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam white-list [rule_number] subject subject {activate\|deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamWhiteList()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam white-list activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamWhiteList()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam xheader dnsbl mail-header mail-header-value` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamXheaderDnsbl()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam xheader query-timeout xheader-name xheader-value` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamXheaderQuery()` |
| Ch 44 (Anti-Spam) | `[no] anti-spam xheader {white-list \| black-list} mail-header mail-header-value` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamXheaderMail()` |
| Ch 44 (Anti-Spam) | `[no] bypass mail-phishing` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBypassMailPhishing()` |
| Ch 44 (Anti-Spam) | `[no] bypass {ip-reputation \| mail-content \| virus-outbreak}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBypass()` |
| Ch 44 (Anti-Spam) | `[no] bypass {white-list \| black-list \| dnsbl}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBypass()` |
| Ch 44 (Anti-Spam) | `[no] log [alert]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdLogAlert()` |
| Ch 44 (Anti-Spam) | `[no] match-action pop3 {forward \| forward-with-tag}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdMatchActionPop3()` |
| Ch 44 (Anti-Spam) | `[no] match-action smtp {drop \| forward \| forward-with-tag}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdMatchActionSmtp()` |
| Ch 44 (Anti-Spam) | `[no] scan {smtp \| pop3}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdScan()` |
| Ch 44 (Anti-Spam) | `[no] security-service anti-spam activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSecurityServiceAntiSpam()` |
| Ch 44 (Anti-Spam) | `anti-spam dnsbl [1..5] domain dnsbl_domain {activate\|deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamDnsbl1()` |
| Ch 44 (Anti-Spam) | `anti-spam dnsbl ip-check-order {forward \| backward}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamDnsblIp()` |
| Ch 44 (Anti-Spam) | `anti-spam dnsbl max-query-ip [1..5]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamDnsblMax()` |
| Ch 44 (Anti-Spam) | `anti-spam dnsbl query-timeout pop3 {forward \| forward-with-tag}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamDnsblQuery()` |
| Ch 44 (Anti-Spam) | `anti-spam dnsbl query-timeout smtp {drop \| forward \| forward-with-tag}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamDnsblQuery()` |
| Ch 44 (Anti-Spam) | `anti-spam dnsbl query-timeout time [1..10]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamDnsblQuery()` |
| Ch 44 (Anti-Spam) | `anti-spam dnsbl statistics flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamDnsblStatistics()` |
| Ch 44 (Anti-Spam) | `anti-spam ip-reputation query-timeout time [timeout]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamIpReputation()` |
| Ch 44 (Anti-Spam) | `anti-spam ip-reputation statistics flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamIpReputation()` |
| Ch 44 (Anti-Spam) | `anti-spam mail-phishing query-timeout pop3 {forward \| forward-with-tag}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamMailPhishing()` |
| Ch 44 (Anti-Spam) | `anti-spam mail-phishing query-timeout smtp {drop \| forward \| forward-with-tag}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamMailPhishing()` |
| Ch 44 (Anti-Spam) | `anti-spam mail-phishing query-timeout time [timeout]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamMailPhishing()` |
| Ch 44 (Anti-Spam) | `anti-spam mail-scan query-timeout pop3 {forward \| forward-with-tag}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamMailScan()` |
| Ch 44 (Anti-Spam) | `anti-spam mail-scan query-timeout smtp {drop \| forward \| forward-with-tag}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamMailScan()` |
| Ch 44 (Anti-Spam) | `anti-spam mail-scan query-timeout time [timeout]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamMailScan()` |
| Ch 44 (Anti-Spam) | `anti-spam profile append` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamProfileAppend()` |
| Ch 44 (Anti-Spam) | `anti-spam profile delete rule_number` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamProfileDelete()` |
| Ch 44 (Anti-Spam) | `anti-spam profile insert rule_number` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamProfileInsert()` |
| Ch 44 (Anti-Spam) | `anti-spam profile move rule_number to rule_number` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamProfileMove()` |
| Ch 44 (Anti-Spam) | `anti-spam profile rule_number` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamProfileRule()` |
| Ch 44 (Anti-Spam) | `anti-spam statistics flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamStatisticsFlush()` |
| Ch 44 (Anti-Spam) | `anti-spam tag black-list [tag]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamTagBlack()` |
| Ch 44 (Anti-Spam) | `anti-spam tag mail-phishing [tag]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamTagMail()` |
| Ch 44 (Anti-Spam) | `anti-spam tag query-timeout [tag]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamTagQuery()` |
| Ch 44 (Anti-Spam) | `anti-spam tag {dnsbl \| dnsbl-timeout} [tag]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamTagTag()` |
| Ch 44 (Anti-Spam) | `anti-spam tag {mail-content \| mail-phishing \| virus-outbreak} [tag]` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamTagTag()` |
| Ch 44 (Anti-Spam) | `anti-spam xheader mail-phishing xheader-name xheader-value` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamXheaderMail()` |
| Ch 44 (Anti-Spam) | `anti-spam xheader {mail-content \| virus-outbreak} xheader-name xheader-value` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdAntiSpamXheaderXheader()` |
| Ch 44 (Anti-Spam) | `no anti-spam dnsbl domain dnsbl_domain` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoAntiSpamDnsbl()` |
| Ch 44 (Anti-Spam) | `no anti-spam xheader mail-phishing` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoAntiSpamXheader()` |
| Ch 44 (Anti-Spam) | `no anti-spam xheader {mail-content \| virus-outbreak}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoAntiSpamXheader()` |
| Ch 44 (Anti-Spam) | `security-service anti-spam inspect {all-traffic \| by-policy}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSecurityServiceAntiSpam()` |
| Ch 44 (Anti-Spam) | `show` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdShow()` |
| Ch 44 (Anti-Spam) | `show anti-spam black-list [status]` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamBlack()` |
| Ch 44 (Anti-Spam) | `show anti-spam dnsbl domain` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamDnsbl()` |
| Ch 44 (Anti-Spam) | `show anti-spam dnsbl ip-check-order` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamDnsbl()` |
| Ch 44 (Anti-Spam) | `show anti-spam dnsbl max-query-ip` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamDnsbl()` |
| Ch 44 (Anti-Spam) | `show anti-spam dnsbl query-timeout time` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamDnsbl()` |
| Ch 44 (Anti-Spam) | `show anti-spam dnsbl query-timeout {smtp \| pop3}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamDnsbl()` |
| Ch 44 (Anti-Spam) | `show anti-spam dnsbl statistics` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamDnsbl()` |
| Ch 44 (Anti-Spam) | `show anti-spam dnsbl status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamDnsbl()` |
| Ch 44 (Anti-Spam) | `show anti-spam ip-reputation high-sensitivity` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamIp()` |
| Ch 44 (Anti-Spam) | `show anti-spam ip-reputation private-check` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamIp()` |
| Ch 44 (Anti-Spam) | `show anti-spam ip-reputation query-timeout time` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamIp()` |
| Ch 44 (Anti-Spam) | `show anti-spam ip-reputation statistics` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamIp()` |
| Ch 44 (Anti-Spam) | `show anti-spam mail-phishing query-timeout pop3` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamMail()` |
| Ch 44 (Anti-Spam) | `show anti-spam mail-phishing query-timeout smtp` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamMail()` |
| Ch 44 (Anti-Spam) | `show anti-spam mail-phishing query-timeout time` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamMail()` |
| Ch 44 (Anti-Spam) | `show anti-spam mail-phishing status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamMail()` |
| Ch 44 (Anti-Spam) | `show anti-spam mail-scan query-timeout pop3` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamMail()` |
| Ch 44 (Anti-Spam) | `show anti-spam mail-scan query-timeout smtp` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamMail()` |
| Ch 44 (Anti-Spam) | `show anti-spam mail-scan query-timeout time` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamMail()` |
| Ch 44 (Anti-Spam) | `show anti-spam mail-scan statistics` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamMail()` |
| Ch 44 (Anti-Spam) | `show anti-spam mail-scan status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamMail()` |
| Ch 44 (Anti-Spam) | `show anti-spam profile [rule_number]` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamProfile()` |
| Ch 44 (Anti-Spam) | `show anti-spam statistics collect` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamStatistics()` |
| Ch 44 (Anti-Spam) | `show anti-spam statistics ranking {source \| mail-address}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamStatistics()` |
| Ch 44 (Anti-Spam) | `show anti-spam statistics summary` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamStatistics()` |
| Ch 44 (Anti-Spam) | `show anti-spam tag black-list` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamTag()` |
| Ch 44 (Anti-Spam) | `show anti-spam tag mail-phishing` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamTag()` |
| Ch 44 (Anti-Spam) | `show anti-spam tag query-timeout` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamTag()` |
| Ch 44 (Anti-Spam) | `show anti-spam tag {dnsbl \| dnsbl-timeout}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamTag()` |
| Ch 44 (Anti-Spam) | `show anti-spam tag {mail-content \| mail-phishing \| virus-outbreak}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamTag()` |
| Ch 44 (Anti-Spam) | `show anti-spam white-list [status]` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamWhite()` |
| Ch 44 (Anti-Spam) | `show anti-spam xheader dnsbl` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamXheader()` |
| Ch 44 (Anti-Spam) | `show anti-spam xheader mail-phishing` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamXheader()` |
| Ch 44 (Anti-Spam) | `show anti-spam xheader query-timeout` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamXheader()` |
| Ch 44 (Anti-Spam) | `show anti-spam xheader {mail-content \| mail-phishing \| virus-outbreak}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamXheader()` |
| Ch 44 (Anti-Spam) | `show anti-spam xheader {white-list \| black-list}` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowAntiSpamXheader()` |
| Ch 44 (Anti-Spam) | `show security-service status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSecurityServiceStatus()` |
| Ch 45 (Collaborative Detect) | `<1..4094>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdGeneric()` |
| Ch 45 (Collaborative Detect) | `[no] cdr activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrActivate()` |
| Ch 45 (Collaborative Detect) | `[no] cdr block block-wireless-client` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrBlockBlockWireless()` |
| Ch 45 (Collaborative Detect) | `[no] cdr counter-reset activate` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrCounterResetActivate()` |
| Ch 45 (Collaborative Detect) | `[no] cdr update auto` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrUpdateAuto()` |
| Ch 45 (Collaborative Detect) | `[no] cdr white-list ipv4 ip_address` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrWhiteListIpv4()` |
| Ch 45 (Collaborative Detect) | `[no] cdr white-list mac mac_address` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrWhiteListMac()` |
| Ch 45 (Collaborative Detect) | `cdr block http-service-port <1..65535>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrBlockHttpService()` |
| Ch 45 (Collaborative Detect) | `cdr block https-service-port <1..65535>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrBlockHttpsService()` |
| Ch 45 (Collaborative Detect) | `cdr block message denied_message` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrBlockMessageDenied()` |
| Ch 45 (Collaborative Detect) | `cdr block period <0..1440>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrBlockPeriod()` |
| Ch 45 (Collaborative Detect) | `cdr block redirect <url>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrBlockRedirect()` |
| Ch 45 (Collaborative Detect) | `cdr block url {message\|redirect}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrBlockUrl()` |
| Ch 45 (Collaborative Detect) | `cdr blocked-by {ip\|mac}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrBlockedBy()` |
| Ch 45 (Collaborative Detect) | `cdr quarantine period <0..1440>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrQuarantinePeriod()` |
| Ch 45 (Collaborative Detect) | `cdr quarantine vlan-id` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrQuarantineVlanId()` |
| Ch 45 (Collaborative Detect) | `cdr rule rule_id threshold occurence duration duration action {alert\| block\| quarantine\| block- alert\| quarantine-alert}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrRuleRuleId()` |
| Ch 45 (Collaborative Detect) | `cdr send-alerts-to email_address` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrSendAlertsTo()` |
| Ch 45 (Collaborative Detect) | `cdr signature reload` | Envelope 7 | `redline` | `ZyxelSecurityCmd::cmdCdrSignatureReload()` |
| Ch 45 (Collaborative Detect) | `cdr signature update` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrSignatureUpdate()` |
| Ch 45 (Collaborative Detect) | `cdr unblock ipv4 ip_address` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrUnblockIpv4Ip()` |
| Ch 45 (Collaborative Detect) | `cdr unblock mac mac_address` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrUnblockMacMac()` |
| Ch 45 (Collaborative Detect) | `cdr update daily <0..23>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrUpdateDaily()` |
| Ch 45 (Collaborative Detect) | `cdr update hourly` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrUpdateHourly()` |
| Ch 45 (Collaborative Detect) | `cdr update weekly {sun \| mon \| tue \| wed \| thu \| fri \| sat} <0..23>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrUpdateWeekly()` |
| Ch 45 (Collaborative Detect) | `cdr white-list replace <1..512> ipv4 ip_address` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrWhiteListReplace()` |
| Ch 45 (Collaborative Detect) | `cdr white-list replace <1..512> mac mac_address` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCdrWhiteListReplace()` |
| Ch 45 (Collaborative Detect) | `show cdr block-list` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowCdrBlockList()` |
| Ch 45 (Collaborative Detect) | `show cdr event-list` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowCdrEventList()` |
| Ch 45 (Collaborative Detect) | `show cdr rules` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowCdrRules()` |
| Ch 45 (Collaborative Detect) | `show cdr signature` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowCdrSignature()` |
| Ch 45 (Collaborative Detect) | `show cdr status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowCdrStatus()` |
| Ch 45 (Collaborative Detect) | `show cdr update` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowCdrUpdate()` |
| Ch 45 (Collaborative Detect) | `show cdr white-list` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowCdrWhiteList()` |
| Ch 46 (SSL Inspection) | `[no] category <category_name>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCategory()` |
| Ch 46 (SSL Inspection) | `[no] certificate cert_name .....................................................................................................................` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdCertificateCertName()` |
| Ch 46 (SSL Inspection) | `[no] description description` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdDescriptionDescription()` |
| Ch 46 (SSL Inspection) | `[no] log` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdLog()` |
| Ch 46 (SSL Inspection) | `[no] ssl-inspection cert-update auto` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSslInspectionCertUpdate()` |
| Ch 46 (SSL Inspection) | `[no] ssl-inspection statistics collect` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSslInspectionStatisticsCollect()` |
| Ch 46 (SSL Inspection) | `bind-ipv4-addr ipv4` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBindIpv4AddrIpv4()` |
| Ch 46 (SSL Inspection) | `bind-ipv6-addr ipv6` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdBindIpv6AddrIpv6()` |
| Ch 46 (SSL Inspection) | `exit` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdExit()` |
| Ch 46 (SSL Inspection) | `exit` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdExit()` |
| Ch 46 (SSL Inspection) | `exit` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdExit()` |
| Ch 46 (SSL Inspection) | `follow-real-client-routing {yes \| no} .........................................................................................` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdFollowRealClientRouting()` |
| Ch 46 (SSL Inspection) | `no ssl-inspection profile SSI_profile_name` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdNoSslInspectionProfile()` |
| Ch 46 (SSL Inspection) | `show ssl-inspection cert-list` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSslInspectionCert()` |
| Ch 46 (SSL Inspection) | `show ssl-inspection cert-update status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSslInspectionCert()` |
| Ch 46 (SSL Inspection) | `show ssl-inspection default-cert update` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSslInspectionDefault()` |
| Ch 46 (SSL Inspection) | `show ssl-inspection default-cert version` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSslInspectionDefault()` |
| Ch 46 (SSL Inspection) | `show ssl-inspection exclude-list` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSslInspectionExclude()` |
| Ch 46 (SSL Inspection) | `show ssl-inspection exclude-list address` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSslInspectionExclude()` |
| Ch 46 (SSL Inspection) | `show ssl-inspection exclude-list settings` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSslInspectionExclude()` |
| Ch 46 (SSL Inspection) | `show ssl-inspection exclude-list web-category` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSslInspectionExclude()` |
| Ch 46 (SSL Inspection) | `show ssl-inspection profile [SSI_profile_name]` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSslInspectionProfile()` |
| Ch 46 (SSL Inspection) | `show ssl-inspection statistics collect` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSslInspectionStatistics()` |
| Ch 46 (SSL Inspection) | `show ssl-inspection statistics summary` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSslInspectionStatistics()` |
| Ch 46 (SSL Inspection) | `show ssl-inspection status` | Envelope 7 | `read` | `ZyxelSecurityCmd::cmdShowSslInspectionStatus()` |
| Ch 46 (SSL Inspection) | `ssl-inspection cache flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSslInspectionCacheFlush()` |
| Ch 46 (SSL Inspection) | `ssl-inspection cert-update now` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSslInspectionCertUpdate()` |
| Ch 46 (SSL Inspection) | `ssl-inspection exclude-list` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSslInspectionExcludeList()` |
| Ch 46 (SSL Inspection) | `ssl-inspection exclude-list-settings` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSslInspectionExcludeList()` |
| Ch 46 (SSL Inspection) | `ssl-inspection pkt-enc-mss <536..1460>` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSslInspectionPktEnc()` |
| Ch 46 (SSL Inspection) | `ssl-inspection profile rename SSI_profile_name1 SSI_profile_name2` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSslInspectionProfileRename()` |
| Ch 46 (SSL Inspection) | `ssl-inspection profile ssi_profile_name` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSslInspectionProfileSsi()` |
| Ch 46 (SSL Inspection) | `ssl-inspection server-sign-cert mode {default \| rsa-1024 \| rsa-2048}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSslInspectionServerSign()` |
| Ch 46 (SSL Inspection) | `ssl-inspection server-sign-cert mode {ecdsa-rsa-1024\|ecdsa-rsa-2048}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSslInspectionServerSign()` |
| Ch 46 (SSL Inspection) | `ssl-inspection statistics flush` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSslInspectionStatisticsFlush()` |
| Ch 46 (SSL Inspection) | `ssl-inspection tls1-2 aesgcm {activate \| deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSslInspectionTls12()` |
| Ch 46 (SSL Inspection) | `ssl-inspection tls1-3 {activate \| deactivate}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSslInspectionTls13()` |
| Ch 46 (SSL Inspection) | `sslv2 action {pass \| block} {no log \| log [alert]} ............................................................................` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSslv2Action()` |
| Ch 46 (SSL Inspection) | `support-version-max {ssl3 \| tls1_0 \| tls1_1 \| tls1_2 \| tls1_3}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSupportVersionMax()` |
| Ch 46 (SSL Inspection) | `support-version-min {ssl3 \| tls1_0 \| tls1_1 \| tls1_2 \| tls1_3}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdSupportVersionMin()` |
| Ch 46 (SSL Inspection) | `unsupported-suite action {pass \| block} {no log \| log [alert]} ................................................................` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdUnsupportedSuiteAction()` |
| Ch 46 (SSL Inspection) | `untrusted-cert-chain action {block \| inspect \| pass} {no log \| log [alert]}` | Envelope 7 | `tier1` | `ZyxelSecurityCmd::cmdUntrustedCertChainAction()` |
| Ch 47 (IP Exception) | `no security-service ip-exception profile_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNoSecurityServiceIp()` |
| Ch 47 (IP Exception) | `no security-service ip6-exception profile_name` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdNoSecurityServiceIp6()` |
| Ch 47 (IP Exception) | `security-service ip-exception {profile_name}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdSecurityServiceIpException()` |
| Ch 47 (IP Exception) | `security-service ip6-exception {profile_name}` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdSecurityServiceIp6Exception()` |
| Ch 47 (IP Exception) | `show security-service ip-exception` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowSecurityServiceIp()` |
| Ch 47 (IP Exception) | `show security-service ip6-exception` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowSecurityServiceIp6()` |
| Ch 48 (Device HA) | `[no] device-ha activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHaActivate()` |
| Ch 48 (Device HA) | `[no] device-ha ap-mode authentication {string key \| ah-md5 key}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHaApMode()` |
| Ch 48 (Device HA) | `[no] device-ha ap-mode backup sync authentication password password` | Envelope 6 | `redline` | `ZyxelFirewallCmd::cmdDeviceHaApMode()` |
| Ch 48 (Device HA) | `[no] device-ha ap-mode backup sync auto` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHaApMode()` |
| Ch 48 (Device HA) | `[no] device-ha ap-mode backup sync from master_address port port` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHaApMode()` |
| Ch 48 (Device HA) | `[no] device-ha ap-mode backup sync interval <5..1440>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHaApMode()` |
| Ch 48 (Device HA) | `[no] device-ha ap-mode interface_name activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHaApMode()` |
| Ch 48 (Device HA) | `[no] device-ha ap-mode interface_name manage-ip ip subnet_mask` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHaApMode()` |
| Ch 48 (Device HA) | `[no] device-ha ap-mode master sync authentication password password` | Envelope 6 | `redline` | `ZyxelFirewallCmd::cmdDeviceHaApMode()` |
| Ch 48 (Device HA) | `[no] device-ha ap-mode preempt` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHaApMode()` |
| Ch 48 (Device HA) | `[no] device-ha2 activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2Activate()` |
| Ch 48 (Device HA) | `[no] device-ha2 connchk-monitor` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2ConnchkMonitor()` |
| Ch 48 (Device HA) | `[no] device-ha2 disable-session-sync` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2DisableSession()` |
| Ch 48 (Device HA) | `[no] device-ha2 interface_name activate` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2InterfaceName()` |
| Ch 48 (Device HA) | `[no] device-ha2 manage-ip ip1 ip2 subnet_mask` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2ManageIp()` |
| Ch 48 (Device HA) | `[no] device-ha2 srv-monitor` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2SrvMonitor()` |
| Ch 48 (Device HA) | `[no] device-ha2 sync password` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2SyncPassword()` |
| Ch 48 (Device HA) | `device-ha ap-mode backup sync now` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHaApMode()` |
| Ch 48 (Device HA) | `device-ha ap-mode cluster-id <1..32>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHaApMode()` |
| Ch 48 (Device HA) | `device-ha ap-mode priority <1..254>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHaApMode()` |
| Ch 48 (Device HA) | `device-ha ap-mode role {master\|backup}` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHaApMode()` |
| Ch 48 (Device HA) | `device-ha mode active-passive` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHaModeActive()` |
| Ch 48 (Device HA) | `device-ha2 ap-firmware-sync` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2ApFirmware()` |
| Ch 48 (Device HA) | `device-ha2 failover connchk-hold-time <60..86400>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2FailoverConnchk()` |
| Ch 48 (Device HA) | `device-ha2 failover reset-interval <1..30>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2FailoverReset()` |
| Ch 48 (Device HA) | `device-ha2 failover-count <5 ..50>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2FailoverCount()` |
| Ch 48 (Device HA) | `device-ha2 firmware-update check-timeout` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2FirmwareUpdate()` |
| Ch 48 (Device HA) | `device-ha2 firmware-update delay` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2FirmwareUpdate()` |
| Ch 48 (Device HA) | `device-ha2 heartbeat period <1..10> fail-tolerance <1..10>` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2HeartbeatPeriod()` |
| Ch 48 (Device HA) | `device-ha2 license-sync serial_number` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2LicenseSync()` |
| Ch 48 (Device HA) | `device-ha2 sync password password` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2SyncPassword()` |
| Ch 48 (Device HA) | `device-ha2 sync_from_active` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2SyncFrom()` |
| Ch 48 (Device HA) | `device-ha2 sync_to_passive` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2SyncTo()` |
| Ch 48 (Device HA) | `device-ha2 virtual-mac zynos_style_mac_address` | Envelope 6 | `tier1` | `ZyxelFirewallCmd::cmdDeviceHa2VirtualMac()` |
| Ch 48 (Device HA) | `show device-ha ap-mode backup sync` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHaAp()` |
| Ch 48 (Device HA) | `show device-ha ap-mode backup sync status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHaAp()` |
| Ch 48 (Device HA) | `show device-ha ap-mode backup sync summary` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHaAp()` |
| Ch 48 (Device HA) | `show device-ha ap-mode forwarding-port interface_name` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHaAp()` |
| Ch 48 (Device HA) | `show device-ha ap-mode interfaces` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHaAp()` |
| Ch 48 (Device HA) | `show device-ha ap-mode master sync` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHaAp()` |
| Ch 48 (Device HA) | `show device-ha ap-mode next-sync-time` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHaAp()` |
| Ch 48 (Device HA) | `show device-ha ap-mode status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHaAp()` |
| Ch 48 (Device HA) | `show device-ha mode` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHaMode()` |
| Ch 48 (Device HA) | `show device-ha status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHaStatus()` |
| Ch 48 (Device HA) | `show device-ha2 activation` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Activation()` |
| Ch 48 (Device HA) | `show device-ha2 device-status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Device()` |
| Ch 48 (Device HA) | `show device-ha2 firmware-update check-timeout` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Firmware()` |
| Ch 48 (Device HA) | `show device-ha2 firmware-update delay` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Firmware()` |
| Ch 48 (Device HA) | `show device-ha2 firmware-update status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Firmware()` |
| Ch 48 (Device HA) | `show device-ha2 interfaces` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Interfaces()` |
| Ch 48 (Device HA) | `show device-ha2 log` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Log()` |
| Ch 48 (Device HA) | `show device-ha2 mgnt-iface` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Mgnt()` |
| Ch 48 (Device HA) | `show device-ha2 mode` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Mode()` |
| Ch 48 (Device HA) | `show device-ha2 passive device-status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Passive()` |
| Ch 48 (Device HA) | `show device-ha2 passive log` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Passive()` |
| Ch 48 (Device HA) | `show device-ha2 passive trace-log` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Passive()` |
| Ch 48 (Device HA) | `show device-ha2 status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Status()` |
| Ch 48 (Device HA) | `show device-ha2 sync status` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Sync()` |
| Ch 48 (Device HA) | `show device-ha2 sync summary` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Sync()` |
| Ch 48 (Device HA) | `show device-ha2 trace-log` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Trace()` |
| Ch 48 (Device HA) | `show device-ha2 virtual-mac` | Envelope 6 | `read` | `ZyxelFirewallCmd::cmdShowDeviceHa2Virtual()` |
| Ch 49 (Device Insight) | `[no] device block mac <mac address>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDeviceBlockMac()` |
| Ch 49 (Device Insight) | `[no] device identify activate` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDeviceIdentifyActivate()` |
| Ch 49 (Device Insight) | `[no] device mac <mac address> description <description>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDeviceMacDescription()` |
| Ch 49 (Device Insight) | `[no] device profile <profile name>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDeviceProfile()` |
| Ch 49 (Device Insight) | `[no] device profile <profile name> category <category>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDeviceProfileCategory()` |
| Ch 49 (Device Insight) | `[no] device profile <profile name> description <description>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDeviceProfileDescription()` |
| Ch 49 (Device Insight) | `[no] device profile <profile name> os <os>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDeviceProfileOs()` |
| Ch 49 (Device Insight) | `device feedback mac <mac address> category <category> os <os> type <type>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDeviceFeedbackMacCategory()` |
| Ch 49 (Device Insight) | `device profile rename <profile name> <profile name>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDeviceProfileRename()` |
| Ch 49 (Device Insight) | `device remove mac <mac address>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDeviceRemoveMac()` |
| Ch 49 (Device Insight) | `device remove mac <mac address>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdDeviceRemoveMac()` |
| Ch 49 (Device Insight) | `secure policy <1...500> device <profile name>` | Envelope 2 | `tier1` | `ZyxelNetworkCmd::cmdSecurePolicyDevice()` |
| Ch 49 (Device Insight) | `show device identify status` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowDeviceIdentifyStatus()` |
| Ch 49 (Device Insight) | `show device info all` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowDeviceInfoAll()` |
| Ch 49 (Device Insight) | `show device info ip <ip address>` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowDeviceInfoIp()` |
| Ch 49 (Device Insight) | `show device info mac <mac address>` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowDeviceInfoMac()` |
| Ch 49 (Device Insight) | `show device profile <profile name>` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowDeviceProfile()` |
| Ch 49 (Device Insight) | `show device profile all` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowDeviceProfileAll()` |
| Ch 49 (Device Insight) | `show reference object device <profile name>` | Envelope 2 | `read` | `ZyxelNetworkCmd::cmdShowReferenceObjectDevice()` |
| Ch 50 (User/Group) | `[no] description description` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdDescriptionDescription()` |
| Ch 50 (User/Group) | `[no] groupname groupname` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdGroupnameGroupname()` |
| Ch 50 (User/Group) | `[no] groupname groupname` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdGroupnameGroupname()` |
| Ch 50 (User/Group) | `[no] mac-auth database mac mac_address type ext-mac-address mac-role mac-users description de- scription` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdMacAuthDatabaseMac()` |
| Ch 50 (User/Group) | `[no] mac-auth database mac mac_address type int-mac-address mac-role mac-users description de- scription` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdMacAuthDatabaseMac()` |
| Ch 50 (User/Group) | `[no] password complexity-verify` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPasswordComplexityVerify()` |
| Ch 50 (User/Group) | `[no] pwd-expiry expiration days <1..365>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPwdExpiryExpirationDays()` |
| Ch 50 (User/Group) | `[no] pwd-expiry force-to-change-pwd activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPwdExpiryForceTo()` |
| Ch 50 (User/Group) | `[no] user username` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUserUsername()` |
| Ch 50 (User/Group) | `[no] users idle-detection` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsersIdleDetection()` |
| Ch 50 (User/Group) | `[no] users idle-detection timeout <1..60>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsersIdleDetectionTimeout()` |
| Ch 50 (User/Group) | `[no] users lockout-period <1..65535>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsersLockoutPeriod()` |
| Ch 50 (User/Group) | `[no] users retry-count <1..99>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsersRetryCount()` |
| Ch 50 (User/Group) | `[no] users retry-limit` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsersRetryLimit()` |
| Ch 50 (User/Group) | `[no] users simultaneous-logon {administration \| access} enforce` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsersSimultaneousLogonEnforce()` |
| Ch 50 (User/Group) | `[no] users simultaneous-logon {administration \| access} limit <1..1024>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsersSimultaneousLogonLimit()` |
| Ch 50 (User/Group) | `[no] users update-lease automation` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsersUpdateLeaseAutomation()` |
| Ch 50 (User/Group) | `groupname rename groupname groupname` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdGroupnameRenameGroupnameGroupname()` |
| Ch 50 (User/Group) | `no username username` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdNoUsernameUsername()` |
| Ch 50 (User/Group) | `pwd-expiry expiration send-now` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPwdExpiryExpirationSend()` |
| Ch 50 (User/Group) | `show groupname [groupname]` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowGroupnameGroupname()` |
| Ch 50 (User/Group) | `show lockout-users` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowLockoutUsers()` |
| Ch 50 (User/Group) | `show password complexity-verify status` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPasswordComplexityVerify()` |
| Ch 50 (User/Group) | `show pwd-expiry {all \| expiration \| force-to-change-pwd \| link-to-device}` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowPwdExpiry()` |
| Ch 50 (User/Group) | `show username [username]` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowUsernameUsername()` |
| Ch 50 (User/Group) | `show users default-setting {all \| user-type {admin\|user\|guest\|limited-admin\|ext-user\| ext- group-user}}` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowUsersDefaultSetting()` |
| Ch 50 (User/Group) | `show users idle-detection-settings` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowUsersIdleDetection()` |
| Ch 50 (User/Group) | `show users retry-settings` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowUsersRetrySettings()` |
| Ch 50 (User/Group) | `show users simultaneous-logon-settings` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowUsersSimultaneousLogon()` |
| Ch 50 (User/Group) | `show users update-lease-settings` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowUsersUpdateLease()` |
| Ch 50 (User/Group) | `show users {username \| all \| current}` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowUsers()` |
| Ch 50 (User/Group) | `sms-service _two-factor-auth-admin-send phone phone user username verification-code verifica- tion_code` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSmsServiceTwoFactor()` |
| Ch 50 (User/Group) | `unlock lockout-users {ip \| console\| ipv6_addr}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUnlockLockoutUsers()` |
| Ch 50 (User/Group) | `username rename username username` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameRenameUsernameUsername()` |
| Ch 50 (User/Group) | `username username [no] description description` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsernameDescriptionDescription()` |
| Ch 50 (User/Group) | `username username [no] email <1..2> email-address` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsernameEmailEmail()` |
| Ch 50 (User/Group) | `username username [no] logon-lease-time <0..1440>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsernameLogonLease()` |
| Ch 50 (User/Group) | `username username [no] logon-re-auth-time <0..1440>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsernameLogonRe()` |
| Ch 50 (User/Group) | `username username [no] phone phone_number` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsernamePhonePhone()` |
| Ch 50 (User/Group) | `username username [no] phone-verify` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsernamePhoneVerify()` |
| Ch 50 (User/Group) | `username username [no] {email1-verify\|email2-verify}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsername()` |
| Ch 50 (User/Group) | `username username encrypted-password <password>` | Envelope 9 | `redline` | `ZyxelAuthCmd::cmdUsernameUsernameEncryptedPassword()` |
| Ch 50 (User/Group) | `username username logon-time-setting <default \| manual>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsernameLogonTime()` |
| Ch 50 (User/Group) | `username username nopassword user-type {admin \| guest \| limited-admin \| user}` | Envelope 9 | `redline` | `ZyxelAuthCmd::cmdUsernameUsernameNopasswordUser()` |
| Ch 50 (User/Group) | `username username password password user-type {admin \| guest \| limited-admin \| user} 444 username username user-type ext-group-user associated-aaa-server server_profile group-id id username username user-type ext-user` | Envelope 9 | `redline` | `ZyxelAuthCmd::cmdUsernameUsernamePasswordPassword()` |
| Ch 50 (User/Group) | `username username user-type mac-address` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsernameUserType()` |
| Ch 50 (User/Group) | `username username vlan activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsernameVlanActivate()` |
| Ch 50 (User/Group) | `username username vlan id <1..4094>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsernameVlanId()` |
| Ch 50 (User/Group) | `users default-setting [no] logon-lease-time <0..1440>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsersDefaultSettingLogon()` |
| Ch 50 (User/Group) | `users default-setting [no] logon-re-auth-time <0..1440>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsersDefaultSettingLogon()` |
| Ch 50 (User/Group) | `users default-setting [no] user-type <admin \|ext-user\|guest\|limited-admin\|user\|ext-group-user>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsersDefaultSettingUser()` |
| Ch 50 (User/Group) | `users default-setting [no] user-type <admin \|ext-user\|guest\|limited-admin\|user\|ext-group-user> logon-lease-time <0..1440>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsersDefaultSettingUser()` |
| Ch 50 (User/Group) | `users default-setting [no] user-type <admin \|ext-user\|guest\|limited-admin\|user\|ext-group-user> logon-re-auth-time <0..1440>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsersDefaultSettingUser()` |
| Ch 50 (User/Group) | `users force-logout {username \| ip \| ipv6_addr}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsersForceLogout()` |
| Ch 51 (Application Object) | `[no] application <sid>` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdApplication()` |
| Ch 51 (Application Object) | `[no] application-object <object>` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdApplicationObject()` |
| Ch 51 (Application Object) | `[no] description <description>` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdDescription()` |
| Ch 51 (Application Object) | `[no] description <description>` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdDescription()` |
| Ch 51 (Application Object) | `[no] object-group <object>` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdObjectGroup()` |
| Ch 51 (Application Object) | `application-object <object>` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdApplicationObject()` |
| Ch 51 (Application Object) | `application-object rename <object> <object>` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdApplicationObjectRename()` |
| Ch 51 (Application Object) | `no application-object <object>` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdNoApplicationObject()` |
| Ch 51 (Application Object) | `no object-group application <object>` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdNoObjectGroupApplication()` |
| Ch 51 (Application Object) | `object-group application <object>` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdObjectGroupApplication()` |
| Ch 51 (Application Object) | `object-group application rename <object> <object>` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdObjectGroupApplicationRename()` |
| Ch 51 (Application Object) | `show application-object <object>` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowApplicationObject()` |
| Ch 51 (Application Object) | `show object-group application <object>` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowObjectGroupApplication()` |
| Ch 52 (Addresses) | `[no] address-object object_name` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdAddressObjectObjectName()` |
| Ch 52 (Addresses) | `[no] address6-object OBJECT_NAME interface-ip interface {dhcpv6 \| link-local \| slaac \| static} {addr_index}` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdAddress6ObjectObjectName()` |
| Ch 52 (Addresses) | `[no] address6-object object_name interface-subnet interface {dhcpv6 \| slaac \| static} {addr_in- dex}` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdAddress6ObjectObjectName()` |
| Ch 52 (Addresses) | `[no] address6-object object_name {ipv6_address \| ipv6_range \| ipv6_subnet}` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdAddress6ObjectObjectName()` |
| Ch 52 (Addresses) | `[no] description description` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdDescriptionDescription()` |
| Ch 52 (Addresses) | `[no] geo-ip database update auto` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdGeoIpDatabaseUpdate()` |
| Ch 52 (Addresses) | `[no] object-group address group_name` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdObjectGroupAddressGroup()` |
| Ch 52 (Addresses) | `[no] object-group group_name` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdObjectGroupGroupName()` |
| Ch 52 (Addresses) | `address-object object_name geography <country code> all` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdAddressObjectObjectName()` |
| Ch 52 (Addresses) | `address-object object_name {ip \| ip_range \| ip_subnet \| fqdn fqdn\| geography country code \| interface-ip \| interface-subnet \| interface-gateway} {interface_name \| virtual interface name}` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdAddressObjectObjectName()` |
| Ch 52 (Addresses) | `address-object rename object_name object_name` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdAddressObjectRenameObject()` |
| Ch 52 (Addresses) | `address6-object object_name geography <country code> all` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdAddress6ObjectObjectName()` |
| Ch 52 (Addresses) | `address6-object object_name {ip \| ip_range \| ip_subnet \| fqdn fqdn\| geography country code \| interface-ip \| interface-subnet \| interface-gateway} {interface_name \| virtual interface name}` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdAddress6ObjectObjectName()` |
| Ch 52 (Addresses) | `fqdn-object query-period <1..1440>` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdFqdnObjectQueryPeriod()` |
| Ch 52 (Addresses) | `fqdn-object sync-period <1..5>` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdFqdnObjectSyncPeriod()` |
| Ch 52 (Addresses) | `fqdn-object test fqdn` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdFqdnObjectTestFqdn()` |
| Ch 52 (Addresses) | `gateway ipv6_addr metric <0..15> ............................................................................................................... .120 geo-ip database update country` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdGatewayIpv6AddrMetric()` |
| Ch 52 (Addresses) | `geo-ip [no] geography <country_code> all address {ipv4 \| ip6}` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdGeoIpGeographyAll()` |
| Ch 52 (Addresses) | `no address-object object_name` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdNoAddressObjectObject()` |
| Ch 52 (Addresses) | `object-group address rename group_name group_name` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdObjectGroupAddressRename()` |
| Ch 52 (Addresses) | `show fqdn` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowFqdn()` |
| Ch 52 (Addresses) | `show fqdn-object all` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowFqdnObjectAll()` |
| Ch 52 (Addresses) | `show fqdn-object query-period` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowFqdnObjectQuery()` |
| Ch 52 (Addresses) | `show fqdn-object sync-period` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowFqdnObjectSync()` |
| Ch 52 (Addresses) | `show fqdn-object6 all` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowFqdnObject6All()` |
| Ch 52 (Addresses) | `show geo-ip country-code` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowGeoIpCountry()` |
| Ch 52 (Addresses) | `show geo-ip country-list region code` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowGeoIpCountry()` |
| Ch 52 (Addresses) | `show geo-ip database update` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowGeoIpDatabase()` |
| Ch 52 (Addresses) | `show geo-ip database version` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowGeoIpDatabase()` |
| Ch 52 (Addresses) | `show geo-ip database version country` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowGeoIpDatabase()` |
| Ch 52 (Addresses) | `show geo-ip geography` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowGeoIpGeography()` |
| Ch 52 (Addresses) | `show geo-ip geography6` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowGeoIpGeography6()` |
| Ch 52 (Addresses) | `show geo-ip region-code` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowGeoIpRegion()` |
| Ch 52 (Addresses) | `show object-group {address \| address6} [group_name]` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowObjectGroupGroup()` |
| Ch 53 (Services) | `[no] description description` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdDescriptionDescription()` |
| Ch 53 (Services) | `[no] object-group group_name` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdObjectGroupGroupName()` |
| Ch 53 (Services) | `[no] object-group service group_name` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdObjectGroupServiceGroup()` |
| Ch 53 (Services) | `[no] service-object object_name` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdServiceObjectObjectName()` |
| Ch 53 (Services) | `no service-object object_name` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdNoServiceObjectObject()` |
| Ch 53 (Services) | `object-group service rename group_name group_name` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdObjectGroupServiceRename()` |
| Ch 53 (Services) | `service-object object_name icmpv6 {<0..255> \| neighbor-solicitation \| router-advertisement \| echo \| packet-toobig \| router-solicitation \| echo-reply \| parameter-problem \| time-ex- ceeded \| neighbor-advertisement \| redirect \| unreachable}` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdServiceObjectObjectName()` |
| Ch 53 (Services) | `service-object object_name protocol <1..255>` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdServiceObjectObjectName()` |
| Ch 53 (Services) | `service-object object_name {tcp \| udp} {eq <1..65535> \| range <1..65535> <1..65535>} 467 service-object object_name icmp icmp_value` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdServiceObjectObjectName()` |
| Ch 53 (Services) | `service-object rename object_name object_name` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdServiceObjectRenameObject()` |
| Ch 53 (Services) | `show object-group service group_name` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowObjectGroupService()` |
| Ch 53 (Services) | `show service-object [object_name]` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowServiceObjectObject()` |
| Ch 54 (Schedules) | `no schedule-object object_name` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdNoScheduleObjectObject()` |
| Ch 54 (Schedules) | `schedule-object object_name date time date time` | Envelope 4 | `tier2` | `ZyxelObjectCmd::cmdScheduleObjectObjectName()` |
| Ch 54 (Schedules) | `show schedule-object` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowScheduleObject()` |
| Ch 55 (AAA Server) | `[no] aaa group server ad group-name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAaaGroupServerAd()` |
| Ch 55 (AAA Server) | `[no] aaa group server ldap group-name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAaaGroupServerLdap()` |
| Ch 55 (AAA Server) | `[no] aaa group server radius group-name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAaaGroupServerRadius()` |
| Ch 55 (AAA Server) | `[no] ad-server basedn basedn` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAdServerBasednBasedn()` |
| Ch 55 (AAA Server) | `[no] ad-server binddn binddn` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAdServerBinddnBinddn()` |
| Ch 55 (AAA Server) | `[no] ad-server cn-identifier uid` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAdServerCnIdentifier()` |
| Ch 55 (AAA Server) | `[no] ad-server host ad_server` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAdServerHostAd()` |
| Ch 55 (AAA Server) | `[no] ad-server password password` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAdServerPasswordPassword()` |
| Ch 55 (AAA Server) | `[no] ad-server password-encrypted password` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAdServerPasswordEncrypted()` |
| Ch 55 (AAA Server) | `[no] ad-server port port_no` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAdServerPortPort()` |
| Ch 55 (AAA Server) | `[no] ad-server search-time-limit time` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAdServerSearchTime()` |
| Ch 55 (AAA Server) | `[no] ad-server ssl` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAdServerSsl()` |
| Ch 55 (AAA Server) | `[no] case-sensitive` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCaseSensitive()` |
| Ch 55 (AAA Server) | `[no] case-sensitive` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCaseSensitive()` |
| Ch 55 (AAA Server) | `[no] case-sensitive` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCaseSensitive()` |
| Ch 55 (AAA Server) | `[no] ldap-server basedn basedn` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdLdapServerBasednBasedn()` |
| Ch 55 (AAA Server) | `[no] ldap-server binddn binddn` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdLdapServerBinddnBinddn()` |
| Ch 55 (AAA Server) | `[no] ldap-server cn-identifier uid` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdLdapServerCnIdentifier()` |
| Ch 55 (AAA Server) | `[no] ldap-server host ldap_server` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdLdapServerHostLdap()` |
| Ch 55 (AAA Server) | `[no] ldap-server password password` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdLdapServerPasswordPassword()` |
| Ch 55 (AAA Server) | `[no] ldap-server password-encrypted password` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdLdapServerPasswordEncrypted()` |
| Ch 55 (AAA Server) | `[no] ldap-server port port_no` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdLdapServerPortPort()` |
| Ch 55 (AAA Server) | `[no] ldap-server search-time-limit time` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdLdapServerSearchTime()` |
| Ch 55 (AAA Server) | `[no] ldap-server ssl` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdLdapServerSsl()` |
| Ch 55 (AAA Server) | `[no] radius-server host radius_server auth-port auth_port` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdRadiusServerHostRadius()` |
| Ch 55 (AAA Server) | `[no] radius-server key secret` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdRadiusServerKeySecret()` |
| Ch 55 (AAA Server) | `[no] radius-server timeout time` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdRadiusServerTimeoutTime()` |
| Ch 55 (AAA Server) | `[no] server alternative-cn-identifier uid` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerAlternativeCnIdentifier()` |
| Ch 55 (AAA Server) | `[no] server alternative-cn-identifier uid` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerAlternativeCnIdentifier()` |
| Ch 55 (AAA Server) | `[no] server basedn basedn` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerBasednBasedn()` |
| Ch 55 (AAA Server) | `[no] server basedn basedn` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerBasednBasedn()` |
| Ch 55 (AAA Server) | `[no] server binddn binddn` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerBinddnBinddn()` |
| Ch 55 (AAA Server) | `[no] server binddn binddn` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerBinddnBinddn()` |
| Ch 55 (AAA Server) | `[no] server cn-identifier uid` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerCnIdentifierUid()` |
| Ch 55 (AAA Server) | `[no] server cn-identifier uid` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerCnIdentifierUid()` |
| Ch 55 (AAA Server) | `[no] server description description` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerDescriptionDescription()` |
| Ch 55 (AAA Server) | `[no] server description description` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerDescriptionDescription()` |
| Ch 55 (AAA Server) | `[no] server description description` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerDescriptionDescription()` |
| Ch 55 (AAA Server) | `[no] server group-attribute <1-255>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerGroupAttribute()` |
| Ch 55 (AAA Server) | `[no] server group-attribute group-attribute` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerGroupAttributeGroup()` |
| Ch 55 (AAA Server) | `[no] server group-attribute group-attribute` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerGroupAttributeGroup()` |
| Ch 55 (AAA Server) | `[no] server host ad_server` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerHostAdServer()` |
| Ch 55 (AAA Server) | `[no] server host ldap_server` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerHostLdapServer()` |
| Ch 55 (AAA Server) | `[no] server host radius_server` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerHostRadiusServer()` |
| Ch 55 (AAA Server) | `[no] server key secret` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerKeySecret()` |
| Ch 55 (AAA Server) | `[no] server password password` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerPasswordPassword()` |
| Ch 55 (AAA Server) | `[no] server password password` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerPasswordPassword()` |
| Ch 55 (AAA Server) | `[no] server port port_no` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerPortPortNo()` |
| Ch 55 (AAA Server) | `[no] server port port_no` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerPortPortNo()` |
| Ch 55 (AAA Server) | `[no] server search-time-limit time` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerSearchTimeLimit()` |
| Ch 55 (AAA Server) | `[no] server search-time-limit time` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerSearchTimeLimit()` |
| Ch 55 (AAA Server) | `[no] server ssl` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerSsl()` |
| Ch 55 (AAA Server) | `[no] server ssl` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerSsl()` |
| Ch 55 (AAA Server) | `[no] server timeout time` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdServerTimeoutTime()` |
| Ch 55 (AAA Server) | `aaa group server ad group-name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAaaGroupServerAd()` |
| Ch 55 (AAA Server) | `aaa group server ad rename group-name group-name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAaaGroupServerAd()` |
| Ch 55 (AAA Server) | `aaa group server ldap group-name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAaaGroupServerLdap()` |
| Ch 55 (AAA Server) | `aaa group server ldap rename group-name group-name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAaaGroupServerLdap()` |
| Ch 55 (AAA Server) | `aaa group server radius group-name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAaaGroupServerRadius()` |
| Ch 55 (AAA Server) | `aaa group server radius rename {group-name-old} group-name-new` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAaaGroupServerRadius()` |
| Ch 55 (AAA Server) | `clear aaa group server ad [group-name]` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdClearAaaGroupServer()` |
| Ch 55 (AAA Server) | `clear aaa group server ldap [group-name]` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdClearAaaGroupServer()` |
| Ch 55 (AAA Server) | `clear aaa group server radius group-name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdClearAaaGroupServer()` |
| Ch 55 (AAA Server) | `show aaa group server ad group-name` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowAaaGroupServer()` |
| Ch 55 (AAA Server) | `show aaa group server ldap group-name` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowAaaGroupServer()` |
| Ch 55 (AAA Server) | `show aaa group server radius group-name` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowAaaGroupServer()` |
| Ch 55 (AAA Server) | `show ad-server` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowAdServer()` |
| Ch 55 (AAA Server) | `show ldap-server` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowLdapServer()` |
| Ch 55 (AAA Server) | `show radius-server` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowRadiusServer()` |
| Ch 56 (Authentication Objec) | `[no] aaa authentication profile-name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAaaAuthenticationProfileName()` |
| Ch 56 (Authentication Objec) | `[no] two-factor-auth activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthActivate()` |
| Ch 56 (Authentication Objec) | `[no] two-factor-auth admin-access activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthAdmin()` |
| Ch 56 (Authentication Objec) | `[no] two-factor-auth admin-access deliver-method {sms\|email}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthAdmin()` |
| Ch 56 (Authentication Objec) | `[no] two-factor-auth admin-access service {ssh\|telnet\|web}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthAdmin()` |
| Ch 56 (Authentication Objec) | `[no] two-factor-auth admin-access user username` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthAdmin()` |
| Ch 56 (Authentication Objec) | `[no] two-factor-auth admin-access valid-time <1..5>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthAdmin()` |
| Ch 56 (Authentication Objec) | `[no] two-factor-auth deliver-method {sms \| email \| google-auth}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthDeliver()` |
| Ch 56 (Authentication Objec) | `[no] two-factor-auth http activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthHttp()` |
| Ch 56 (Authentication Objec) | `[no] two-factor-auth service {sslvpn\|ipsec\|l2tp}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthService()` |
| Ch 56 (Authentication Objec) | `[no] two-factor-auth user username` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthUser()` |
| Ch 56 (Authentication Objec) | `[no] two-factor-auth valid-time <1..15>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthValid()` |
| Ch 56 (Authentication Objec) | `aaa authentication [no] match-default-group` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAaaAuthenticationMatchDefault()` |
| Ch 56 (Authentication Objec) | `aaa authentication default member1 [member2] [member3] [member4]` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAaaAuthenticationDefaultMember1()` |
| Ch 56 (Authentication Objec) | `aaa authentication profile-name member1 [member2] [member3] [member4]` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAaaAuthenticationProfileName()` |
| Ch 56 (Authentication Objec) | `aaa authentication rename profile-name-old profile-name-new` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAaaAuthenticationRenameProfile()` |
| Ch 56 (Authentication Objec) | `clear aaa authentication profile-name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdClearAaaAuthenticationProfile()` |
| Ch 56 (Authentication Objec) | `show aaa authentication {group-name\|default}` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowAaaAuthentication()` |
| Ch 56 (Authentication Objec) | `show two-factor-auth` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowTwoFactorAuth()` |
| Ch 56 (Authentication Objec) | `show two-factor-auth admin-access` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowTwoFactorAuth()` |
| Ch 56 (Authentication Objec) | `show two-factor-auth admin-access` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowTwoFactorAuth()` |
| Ch 56 (Authentication Objec) | `show username username google-auth backup-code` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowUsernameUsernameGoogle()` |
| Ch 56 (Authentication Objec) | `show username username google-auth qrcode` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowUsernameUsernameGoogle()` |
| Ch 56 (Authentication Objec) | `test aaa {server\|secure-server} {ad\|ldap} host {hostname\|ipv4-address} [host {hostname\|ipv4- address}] port <1..65535> base-dn base-dn-string [bind-dn bind-dn-string password pass- word] login-name-attribute attribute [alternative-login-name-attribute attribute] ac- count account-name` | Envelope 9 | `redline` | `ZyxelAuthCmd::cmdTestAaaHostHost()` |
| Ch 56 (Authentication Objec) | `two-factor-auth admin-access auth-method {google-auth\|pin-code}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthAdmin()` |
| Ch 56 (Authentication Objec) | `two-factor-auth allow-access-url-thru-tunnel [activate \| deactivate]` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthAllow()` |
| Ch 56 (Authentication Objec) | `two-factor-auth http port <1...65535>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthHttp()` |
| Ch 56 (Authentication Objec) | `two-factor-auth message {message_quoted \| message}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthMessage()` |
| Ch 56 (Authentication Objec) | `two-factor-auth message-type {default \| file}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthMessage()` |
| Ch 56 (Authentication Objec) | `two-factor-auth server interface interface_name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthServer()` |
| Ch 56 (Authentication Objec) | `two-factor-auth server user-defined {ipv4\|domain_name}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthServer()` |
| Ch 56 (Authentication Objec) | `two-factor-auth sms message {message_quoted \| message}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTwoFactorAuthSms()` |
| Ch 56 (Authentication Objec) | `username username 2fa-auth-method {default\|google-auth\|pin-code}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsername2faAuth()` |
| Ch 56 (Authentication Objec) | `username username [no] google-auth` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsernameGoogleAuth()` |
| Ch 56 (Authentication Objec) | `username username [no] phone-verify` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsernamePhoneVerify()` |
| Ch 56 (Authentication Objec) | `username username [no] {email1-verify\|email2-verify}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsername()` |
| Ch 56 (Authentication Objec) | `username username google-auth backup-code create` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsernameGoogleAuth()` |
| Ch 56 (Authentication Objec) | `username username google-auth verify-code <verification code>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdUsernameUsernameGoogleAuth()` |
| Ch 57 (Authentication Serve) | `[no] activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdActivate()` |
| Ch 57 (Authentication Serve) | `[no] auth-server activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAuthServerActivate()` |
| Ch 57 (Authentication Serve) | `[no] auth-server cert certificate_name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAuthServerCertCertificate()` |
| Ch 57 (Authentication Serve) | `[no] auth-server trusted-client profile_name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAuthServerTrustedClient()` |
| Ch 57 (Authentication Serve) | `[no] description description` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdDescriptionDescription()` |
| Ch 57 (Authentication Serve) | `[no] ip address ip subnet_mask` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdIpAddressIpSubnet()` |
| Ch 57 (Authentication Serve) | `[no] secret secret` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSecretSecret()` |
| Ch 57 (Authentication Serve) | `auth-server authentication` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAuthServerAuthentication()` |
| Ch 57 (Authentication Serve) | `authentication-type {<profile name> \| default-user-agreement \| default-web-portal \| facebook-wifi} . 256 auth_method` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdAuthenticationType256Auth()` |
| Ch 57 (Authentication Serve) | `no auth-server authentication` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdNoAuthServerAuthentication()` |
| Ch 57 (Authentication Serve) | `show auth-server status` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowAuthServerStatus()` |
| Ch 57 (Authentication Serve) | `show auth-server trusted-client` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowAuthServerTrusted()` |
| Ch 57 (Authentication Serve) | `show auth-server trusted-client profile_name` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowAuthServerTrusted()` |
| Ch 58 (Certificates) | `ca generate pkcs10 name certificate_name cn-type {ip cn cn_ipv4_address \| ipv6 cn cn_ipv6_ad- dress \|fqdn cn cn_domain_name \| mail cn cn_email} [ou organizational_unit] [o organiza- tion] [l town] [s state] [c country] [usr-def user_definition] key-type {dsa \| dsa-sha256 \| ecdsa \| ecdsa-sha256 \| ecdsa-sha384 \| rsa \| rsa-sha256 \| rsa-sha512} key-len key_length [extend-key extend_key] year lifetimes` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCaGeneratePkcs10Name()` |
| Ch 58 (Certificates) | `ca generate pkcs12 name name password password` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCaGeneratePkcs12Name()` |
| Ch 58 (Certificates) | `ca generate x509 name certificate_name cn-type {ip cn ipv4 \| ipv6 cn cn_ipv6_address \| fqdn cn cn_domain_name \| mail cn cn_email} [ou organizational_unit] [o organization] [l town] [s state] [c country] [usr-def user_definition] key-type {dsa \| dsa-sha256 \| ecdsa \| ecdsa- sha256 \| ecdsa-sha384 \| rsa \| rsa-sha256 \| rsa-sha512} key-len key_length . 494 ca rename category {local\|remote} old_name new_name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCaGenerateX509Name()` |
| Ch 58 (Certificates) | `ca validation remote_certificate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCaValidationRemoteCertificate()` |
| Ch 58 (Certificates) | `cdp {activate\|deactivate} .....................................................................................................` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCdp()` |
| Ch 58 (Certificates) | `ldap ip {ip\|fqdn} port <1..65535> [id name password password] [deactivate]` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdLdapIpPortId()` |
| Ch 58 (Certificates) | `ldap {activate\|deactivate} ....................................................................................................` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdLdap()` |
| Ch 58 (Certificates) | `no ca category {local\|remote} certificate_name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdNoCaCategoryCertificate()` |
| Ch 58 (Certificates) | `no ca validation name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdNoCaValidationName()` |
| Ch 58 (Certificates) | `ocsp url url [id name password password] [deactivate]` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdOcspUrlUrlId()` |
| Ch 58 (Certificates) | `ocsp {activate\|deactivate} ....................................................................................................` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdOcsp()` |
| Ch 58 (Certificates) | `show ca category {local\|remote}` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowCaCategory()` |
| Ch 58 (Certificates) | `show ca category {local\|remote} name certificate_name certpath` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowCaCategoryName()` |
| Ch 58 (Certificates) | `show ca category {local\|remote} name certificate_name format {text\|pem}]` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowCaCategoryName()` |
| Ch 58 (Certificates) | `show ca hierarchy name certificate_name [format all \| cn \| file]` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowCaHierarchyName()` |
| Ch 58 (Certificates) | `show ca spaceusage` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowCaSpaceusage()` |
| Ch 58 (Certificates) | `show ca validation name name` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowCaValidationName()` |
| Ch 59 (ISP Accounts) | `[no] account cellular profile_name` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdAccountCellularProfileName()` |
| Ch 59 (ISP Accounts) | `[no] account {pppoe \| pptp} profile_name` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdAccountProfileName()` |
| Ch 59 (ISP Accounts) | `[no] apn access_point_name` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdApnAccessPointName()` |
| Ch 59 (ISP Accounts) | `[no] as-profile <profile name> {[no log]\|[log by-profile]} {activate \| deactivate} 223 [no] authentication {chap-pap \| chap \| pap \| mschap \| mschap-v2}` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdAsProfile223Authentication()` |
| Ch 59 (ISP Accounts) | `[no] authentication {none \| pap \| chap}` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdAuthentication()` |
| Ch 59 (ISP Accounts) | `[no] compression {yes \| no}` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdCompression()` |
| Ch 59 (ISP Accounts) | `[no] connection-id connection_id` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdConnectionIdConnectionId()` |
| Ch 59 (ISP Accounts) | `[no] dial-string isp_dial_string` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdDialStringIspDial()` |
| Ch 59 (ISP Accounts) | `[no] encryption {nomppe \| mppe-40 \| mppe-128}` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdEncryption()` |
| Ch 59 (ISP Accounts) | `[no] idle <0..360>` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdIdle()` |
| Ch 59 (ISP Accounts) | `[no] idle <0..360>` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdIdle()` |
| Ch 59 (ISP Accounts) | `[no] password password` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdPasswordPassword()` |
| Ch 59 (ISP Accounts) | `[no] password password` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdPasswordPassword()` |
| Ch 59 (ISP Accounts) | `[no] server ip` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdServerIp()` |
| Ch 59 (ISP Accounts) | `[no] service-name {ip \| hostname \| service_name}` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdServiceName()` |
| Ch 59 (ISP Accounts) | `[no] user username` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdUserUsername()` |
| Ch 59 (ISP Accounts) | `[no] user username` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdUserUsername()` |
| Ch 59 (ISP Accounts) | `encrypted-password ciphertext` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdEncryptedPasswordCiphertext()` |
| Ch 59 (ISP Accounts) | `show account [pppoe profile_name \| pptp profile_name]` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowAccountPppoeProfile()` |
| Ch 59 (ISP Accounts) | `show account cellular profile_name` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowAccountCellularProfile()` |
| Ch 60 (SSL Application) | `[no] ssl-profile <profile name> {[no log]\|[log by-profile]} {activate \| deactivate} 223 [no] sslvpn application application_object` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdSslProfile223Sslvpn()` |
| Ch 60 (SSL Application) | `[no] webpage-encrypt` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdWebpageEncrypt()` |
| Ch 60 (SSL Application) | `no server-type` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdNoServerType()` |
| Ch 60 (SSL Application) | `port <1..65535> ending-port <1..65535>]` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdPortEndingPort()` |
| Ch 60 (SSL Application) | `port <1..65535> ending-port <1..65535>] [program-path program-path]` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdPortEndingPortProgram()` |
| Ch 60 (SSL Application) | `server-type rdp server-address server-address [starting-` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdServerTypeRdpServer()` |
| Ch 60 (SSL Application) | `server-type vnc server-address server-address [starting-` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdServerTypeVncServer()` |
| Ch 60 (SSL Application) | `server-type weblink url url` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdServerTypeWeblinkUrl()` |
| Ch 60 (SSL Application) | `server-type {file-sharing \| owa \| web-server} url URL [entry-point entry_point] . 501 server-type file-sharing share-path share-path` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdServerTypeUrlUrl()` |
| Ch 60 (SSL Application) | `show sslvpn application [application_object]` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowSslvpnApplicationApplication()` |
| Ch 61 (DHCPv6 Objects) | `dhcp6-lease-object dhcp6_profile address ipv6_addr duid duid` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdDhcp6LeaseObjectDhcp6()` |
| Ch 61 (DHCPv6 Objects) | `dhcp6-lease-object dhcp6_profile address- ipv6_addr ipv6_addr` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdDhcp6LeaseObjectDhcp6()` |
| Ch 61 (DHCPv6 Objects) | `dhcp6-lease-object dhcp6_profile prefix-delegation ipv6_addr_prefix duid duid` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdDhcp6LeaseObjectDhcp6()` |
| Ch 61 (DHCPv6 Objects) | `dhcp6-lease-object dhcp6_profile { sip-server \| ntp-server \| dns-server } { ipv6_addr \| dhcp6_- profile }` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdDhcp6LeaseObjectDhcp6()` |
| Ch 61 (DHCPv6 Objects) | `dhcp6-lease-object rename dhcp6_profile dhcp6_profile` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdDhcp6LeaseObjectRename()` |
| Ch 61 (DHCPv6 Objects) | `dhcp6-request-object dhcp6_profile { dns-server \| ntp-server \| prefix-delegation \| sip-server } dhcp6-request-object rename dhcp6_profile dhcp6_profile` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdDhcp6RequestObjectDhcp6()` |
| Ch 61 (DHCPv6 Objects) | `no dhcp6-lease-object dhcp6_profile` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdNoDhcp6LeaseObject()` |
| Ch 61 (DHCPv6 Objects) | `no dhcp6-request-object dhcp6_profile` | Envelope 4 | `tier1` | `ZyxelObjectCmd::cmdNoDhcp6RequestObject()` |
| Ch 61 (DHCPv6 Objects) | `show dhcp6 interface` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowDhcp6Interface()` |
| Ch 61 (DHCPv6 Objects) | `show dhcp6 lease-object [dhcp6_profile]` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowDhcp6LeaseObject()` |
| Ch 61 (DHCPv6 Objects) | `show dhcp6 object-binding interface_name` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowDhcp6ObjectBinding()` |
| Ch 61 (DHCPv6 Objects) | `show dhcp6 request-object [dhcp6_profile]` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowDhcp6RequestObject()` |
| Ch 61 (DHCPv6 Objects) | `show ipv6 dhcp6 binding` | Envelope 4 | `read` | `ZyxelObjectCmd::cmdShowIpv6Dhcp6Binding()` |
| Ch 62 (Dynamic Guest Accoun) | `[no] bandwidth activate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBandwidthActivate()` |
| Ch 62 (Dynamic Guest Accoun) | `[no] dynamic-guest user_name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdDynamicGuestUserName()` |
| Ch 62 (Dynamic Guest Accoun) | `bandwidth {upload \| download} <0..1048576> priority <1..7>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdBandwidthPriority()` |
| Ch 62 (Dynamic Guest Accoun) | `charge price` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdChargePrice()` |
| Ch 62 (Dynamic Guest Accoun) | `create-time yyyy-mm-dd hh:mm` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCreateTimeYyyyMm()` |
| Ch 62 (Dynamic Guest Accoun) | `currency {eur \|gbp \|usd \| user-define curreny_code}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdCurrency()` |
| Ch 62 (Dynamic Guest Accoun) | `dynamic-guest freeuser user_name` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdDynamicGuestFreeuserUser()` |
| Ch 62 (Dynamic Guest Accoun) | `dynamic-guest generate` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdDynamicGuestGenerate()` |
| Ch 62 (Dynamic Guest Accoun) | `dynamic-guest generate-freeuser` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdDynamicGuestGenerateFreeuser()` |
| Ch 62 (Dynamic Guest Accoun) | `dynamic-guest keep-user-logged-in` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdDynamicGuestKeepUser()` |
| Ch 62 (Dynamic Guest Accoun) | `e-mail email_address` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdEMailEmailAddress()` |
| Ch 62 (Dynamic Guest Accoun) | `encrypted-password password` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdEncryptedPasswordPassword()` |
| Ch 62 (Dynamic Guest Accoun) | `expire-time yyyy-mm-dd hh:mm` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdExpireTimeYyyyMm()` |
| Ch 62 (Dynamic Guest Accoun) | `login-mac mac_address` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdLoginMacMacAddress()` |
| Ch 62 (Dynamic Guest Accoun) | `name description` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdNameDescription()` |
| Ch 62 (Dynamic Guest Accoun) | `password password` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPasswordPassword()` |
| Ch 62 (Dynamic Guest Accoun) | `payment-info {cash \| payment-service}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPaymentInfo()` |
| Ch 62 (Dynamic Guest Accoun) | `phone phone_number` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPhonePhoneNumber()` |
| Ch 62 (Dynamic Guest Accoun) | `printer-ip ip_address` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdPrinterIpIpAddress()` |
| Ch 62 (Dynamic Guest Accoun) | `quota type {total \| upload-download}` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdQuotaType()` |
| Ch 62 (Dynamic Guest Accoun) | `quota {total \| upload \| download} gigabytes <0..100>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdQuotaGigabytes()` |
| Ch 62 (Dynamic Guest Accoun) | `quota {total \| upload \| download} megabytes <0..1023>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdQuotaMegabytes()` |
| Ch 62 (Dynamic Guest Accoun) | `remaining-time <1..25920000>` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdRemainingTime()` |
| Ch 62 (Dynamic Guest Accoun) | `replenish enable` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdReplenishEnable()` |
| Ch 62 (Dynamic Guest Accoun) | `serial-number number` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdSerialNumberNumber()` |
| Ch 62 (Dynamic Guest Accoun) | `show dynamic-guest log` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowDynamicGuestLog()` |
| Ch 62 (Dynamic Guest Accoun) | `show dynamic-guest log create-time begin yyyy-mm-dd hh:mm end yyyy-mm-dd hh:mm` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowDynamicGuestLog()` |
| Ch 62 (Dynamic Guest Accoun) | `show dynamic-guest users` | Envelope 9 | `read` | `ZyxelAuthCmd::cmdShowDynamicGuestUsers()` |
| Ch 62 (Dynamic Guest Accoun) | `time-period <1..432000> .......................................................................................................` | Envelope 9 | `tier1` | `ZyxelAuthCmd::cmdTimePeriod()` |
| Ch 63 (System) | `Router(config)#` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRouterConfig()` |
| Ch 63 (System) | `Router(config)# zon lldp server` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRouterConfigZonLldp()` |
| Ch 63 (System) | `Router(config)# zon lldp server status` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRouterConfigZonLldp()` |
| Ch 63 (System) | `[no] access-page color-window-background` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAccessPageColorWindow()` |
| Ch 63 (System) | `[no] access-page message-text message` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAccessPageMessageText()` |
| Ch 63 (System) | `[no] activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdActivate()` |
| Ch 63 (System) | `[no] auth-server activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAuthServerActivate()` |
| Ch 63 (System) | `[no] auth-server cert certificate_name` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAuthServerCertCertificate()` |
| Ch 63 (System) | `[no] auth-server trusted-client profile_name` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAuthServerTrustedClient()` |
| Ch 63 (System) | `[no] clock auto-sync-daylight-saving` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdClockAutoSyncDaylight()` |
| Ch 63 (System) | `[no] clock auto-sync-timezone` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdClockAutoSyncTimezone()` |
| Ch 63 (System) | `[no] clock daylight-saving` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdClockDaylightSaving()` |
| Ch 63 (System) | `[no] clock saving-interval begin {apr\|aug\|dec\|feb\|jan\|jul\|jun\|mar\|may\|nov\|oct\|sep} {1\|2\|3\|4\|last} {fri\|mon\|sat\|sun\|thu\|tue\|wed} hh:mm end {apr\|aug\|dec\|feb\|jan\|jul\|jun\|mar\|may\|nov\|oct\|sep} {1\|2\|3\|4\|last} {fri\|mon\|sat\|sun\|thu\|tue\|wed} hh:mm offset` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdClockSavingIntervalBegin()` |
| Ch 63 (System) | `[no] clock time-zone {-\|+hh:mm} [+\|-]HH:MM.` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdClockTimeZoneHh()` |
| Ch 63 (System) | `[no] console baud baud_rate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdConsoleBaudBaudRate()` |
| Ch 63 (System) | `[no] description description` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDescriptionDescription()` |
| Ch 63 (System) | `[no] domainname domain_name` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDomainnameDomainName()` |
| Ch 63 (System) | `[no] hostname hostname` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdHostnameHostname()` |
| Ch 63 (System) | `[no] ip address ip subnet_mask` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpAddressIpSubnet()` |
| Ch 63 (System) | `[no] ip dns server a-record fqdn w.x.y.z` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpDnsServerA()` |
| Ch 63 (System) | `[no] ip dns server mx-record domain_name {w.x.y.z\|fqdn}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpDnsServerMx()` |
| Ch 63 (System) | `[no] ip dns server zone-forwarder {<1..32>\|append\|insert <1..32>} {domain_zone_name\|*} inter- face interface_name` | Envelope 1 | `tier2` | `ZyxelSystemCmd::cmdIpDnsServerZone()` |
| Ch 63 (System) | `[no] ipv6 activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpv6Activate()` |
| Ch 63 (System) | `[no] language update auto` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLanguageUpdateAuto()` |
| Ch 63 (System) | `[no] login-page color-background` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoginPageColorBackground()` |
| Ch 63 (System) | `[no] login-page color-window-background` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoginPageColorWindow()` |
| Ch 63 (System) | `[no] login-page message-text % message` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoginPageMessageText()` |
| Ch 63 (System) | `[no] mail-from email_address` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMailFromEmailAddress()` |
| Ch 63 (System) | `[no] mail-subject append date-time` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMailSubjectAppendDate()` |
| Ch 63 (System) | `[no] mail-subject append system-name` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMailSubjectAppendSystem()` |
| Ch 63 (System) | `[no] ntp` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNtp()` |
| Ch 63 (System) | `[no] ntp server {fqdn\|w.x.y.z}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNtpServer()` |
| Ch 63 (System) | `[no] password password` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdPasswordPassword()` |
| Ch 63 (System) | `[no] respmsg url-filter block-page customized activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRespmsgUrlFilterBlock()` |
| Ch 63 (System) | `[no] secret secret` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSecretSecret()` |
| Ch 63 (System) | `[no] sms-service activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSmsServiceActivate()` |
| Ch 63 (System) | `[no] smtp-address {ip \| hostname}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSmtpAddress()` |
| Ch 63 (System) | `[no] smtp-auth activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSmtpAuthActivate()` |
| Ch 63 (System) | `[no] smtp-auth username username password password` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdSmtpAuthUsernameUsername()` |
| Ch 63 (System) | `[no] smtp-port <1..65535>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSmtpPort()` |
| Ch 63 (System) | `[no] smtp-tls activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSmtpTlsActivate()` |
| Ch 63 (System) | `[no] smtp-tls authenticate-server` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSmtpTlsAuthenticateServer()` |
| Ch 63 (System) | `[no] smtp-tls starttls-off` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSmtpTlsStarttlsOff()` |
| Ch 63 (System) | `[no] username e-mail` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdUsernameEMail()` |
| Ch 63 (System) | `access-page message-color {color-rgb \| color-name \| color-number}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAccessPageMessageColor()` |
| Ch 63 (System) | `access-page title title` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAccessPageTitleTitle()` |
| Ch 63 (System) | `access-page window-color {color-rgb \| color-name \| color-number}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAccessPageWindowColor()` |
| Ch 63 (System) | `auth-server authentication` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAuthServerAuthentication()` |
| Ch 63 (System) | `auth_method` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAuthMethod()` |
| Ch 63 (System) | `clock date yyyy-mm-dd time hh:mm:ss` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdClockDateYyyyMm()` |
| Ch 63 (System) | `clock time hh:mm:ss` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdClockTimeHhMm()` |
| Ch 63 (System) | `fast forwarding {activate \| deactivate}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdFastForwarding()` |
| Ch 63 (System) | `ip dns security-options {default \| 1}]` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpDnsSecurityOptions()` |
| Ch 63 (System) | `ip dns server aaaa-record {FQDN_DNS \| FQDN_WILDCARD_DNS} IPv6` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpDnsServerAaaa()` |
| Ch 63 (System) | `ip dns server cache-flush` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpDnsServerCache()` |
| Ch 63 (System) | `ip dns server cname-record {FQDN_DNS \| FQDN_WILDCARD_DNS} {FQDN_DNS}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpDnsServerCname()` |
| Ch 63 (System) | `ip dns server rule move <1..32> to <1..32>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpDnsServerRule()` |
| Ch 63 (System) | `ip dns server rule {<1..32>\|append\|insert <1..32>} access-group {ALL\|address_object} zone {ALL\|address_object} action {accept\|deny}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpDnsServerRule()` |
| Ch 63 (System) | `ip dns server zone-forwarder move <1..32> to <1..32>` | Envelope 1 | `tier2` | `ZyxelSystemCmd::cmdIpDnsServerZone()` |
| Ch 63 (System) | `ip dns server zone-forwarder {<1..32>\|append\|insert <1..32>} {domain_zone_name\|*} {interface interface_name \| user-defined ipv4_address [interface {interface_name \| auto}]} 606 ip dns server zone-forwarder {<1..32>\|append\|insert <1..32>} {domain_zone_name\|*} user-defined w.x.y.z {ip_type} [private \| interface {interface_name \| auto}]` | Envelope 1 | `tier2` | `ZyxelSystemCmd::cmdIpDnsServerZone()` |
| Ch 63 (System) | `language language_name` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLanguageLanguageName()` |
| Ch 63 (System) | `language update daily <0..23>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLanguageUpdateDaily()` |
| Ch 63 (System) | `language update hourly` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLanguageUpdateHourly()` |
| Ch 63 (System) | `language update package` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLanguageUpdatePackage()` |
| Ch 63 (System) | `language update weekly {sun \| mon \| tue \| wed \| thu \| fri \| sat} <0..23>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLanguageUpdateWeekly()` |
| Ch 63 (System) | `login-page background-color {color-rgb \| color-name \| color-number}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoginPageBackgroundColor()` |
| Ch 63 (System) | `login-page message-color {color-rgb \| color-name \| color-number}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoginPageMessageColor()` |
| Ch 63 (System) | `login-page title title` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoginPageTitleTitle()` |
| Ch 63 (System) | `login-page title-color {color-rgb \| color-name \| color-number}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoginPageTitleColor()` |
| Ch 63 (System) | `login-page window-color {color-rgb \| color-name \| color-number}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoginPageWindowColor()` |
| Ch 63 (System) | `logo background-color {color-rgb \| color-name \| color-number}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLogoBackgroundColor()` |
| Ch 63 (System) | `mail-server` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMailServer()` |
| Ch 63 (System) | `myzyxel-service get-cloud-timezone` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMyzyxelServiceGetCloud()` |
| Ch 63 (System) | `myzyxel-service set-timezone-according-cloud` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMyzyxelServiceSetTimezone()` |
| Ch 63 (System) | `name DNS_OPTIONS_NAME .........................................................................................................` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNameDnsOptionsName()` |
| Ch 63 (System) | `no additional-from-cache activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNoAdditionalFromCache()` |
| Ch 63 (System) | `no address-object-group {any \| PROFILE}` | Envelope 1 | `tier2` | `ZyxelSystemCmd::cmdNoAddressObjectGroup()` |
| Ch 63 (System) | `no auth-server authentication` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNoAuthServerAuthentication()` |
| Ch 63 (System) | `no ip dns server rule <1..32>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNoIpDnsServer()` |
| Ch 63 (System) | `no recursion activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNoRecursionActivate()` |
| Ch 63 (System) | `ntp sync` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNtpSync()` |
| Ch 63 (System) | `schedule hour <0..23> minute <00..59>` | Envelope 1 | `tier2` | `ZyxelSystemCmd::cmdScheduleHourMinute()` |
| Ch 63 (System) | `show` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdShow()` |
| Ch 63 (System) | `show access-page settings` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowAccessPageSettings()` |
| Ch 63 (System) | `show auth-server status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowAuthServerStatus()` |
| Ch 63 (System) | `show auth-server trusted-client` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowAuthServerTrusted()` |
| Ch 63 (System) | `show auth-server trusted-client profile_name` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowAuthServerTrusted()` |
| Ch 63 (System) | `show clock date` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowClockDate()` |
| Ch 63 (System) | `show clock status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowClockStatus()` |
| Ch 63 (System) | `show clock time` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowClockTime()` |
| Ch 63 (System) | `show console` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowConsole()` |
| Ch 63 (System) | `show fast forwarding status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowFastForwardingStatus()` |
| Ch 63 (System) | `show fqdn` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowFqdn()` |
| Ch 63 (System) | `show ip dns security-options all` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowIpDnsSecurity()` |
| Ch 63 (System) | `show ip dns server` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowIpDnsServer()` |
| Ch 63 (System) | `show ip dns server database` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowIpDnsServer()` |
| Ch 63 (System) | `show ip dns server status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowIpDnsServer()` |
| Ch 63 (System) | `show ipv6 status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowIpv6Status()` |
| Ch 63 (System) | `show language {setting \| all}` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowLanguage()` |
| Ch 63 (System) | `show login-page default-title` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowLoginPageDefault()` |
| Ch 63 (System) | `show login-page settings` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowLoginPageSettings()` |
| Ch 63 (System) | `show logo settings` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowLogoSettings()` |
| Ch 63 (System) | `show myzyxel-service get-cloud-timezone` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowMyzyxelServiceGet()` |
| Ch 63 (System) | `show ntp server` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowNtpServer()` |
| Ch 63 (System) | `show page-customization` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowPageCustomization()` |
| Ch 63 (System) | `show respmsg url-filter block-page` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowRespmsgUrlFilter()` |
| Ch 63 (System) | `show sms-service` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSmsService()` |
| Ch 63 (System) | `show sms-service activation` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSmsServiceActivation()` |
| Ch 63 (System) | `show sms-service default-country-code` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSmsServiceDefault()` |
| Ch 63 (System) | `show sms-service provider email-to-sms` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSmsServiceProvider()` |
| Ch 63 (System) | `show sms-service provider vianett` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSmsServiceProvider()` |
| Ch 63 (System) | `show zon lldp neighbors` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowZonLldpNeighbors()` |
| Ch 63 (System) | `show zon lldp server config` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowZonLldpServer()` |
| Ch 63 (System) | `show zon lldp server statistics` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowZonLldpServer()` |
| Ch 63 (System) | `show zon lldp server status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowZonLldpServer()` |
| Ch 63 (System) | `show zon zdp server status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowZonZdpServer()` |
| Ch 63 (System) | `sms-service account-send phone phone_number account user_name password password . 520 sms-service default-country-code country_code` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdSmsServiceAccountSend()` |
| Ch 63 (System) | `sms-service provider email-to-sms` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSmsServiceProviderEmail()` |
| Ch 63 (System) | `sms-service provider vianett` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSmsServiceProviderVianett()` |
| Ch 63 (System) | `sms-service provider-select vianett {vianett\|email-to-sms}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSmsServiceProviderSelect()` |
| Ch 63 (System) | `sms-service test-send phone phone_number msg message` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSmsServiceTestSend()` |
| Ch 63 (System) | `status: active` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdStatusActive()` |
| Ch 63 (System) | `zon lldp server` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdZonLldpServer()` |
| Ch 63 (System) | `zon lldp server tx-hold <1..10>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdZonLldpServerTx()` |
| Ch 63 (System) | `zon lldp server tx-interval <1..600>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdZonLldpServerTx()` |
| Ch 63 (System) | `zon zdp server` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdZonZdpServer()` |
| Ch 64 (System Remote Manage) | `[no] ip ftp server` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpFtpServer()` |
| Ch 64 (System Remote Manage) | `[no] ip ftp server cert certificate_name` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpFtpServerCert()` |
| Ch 64 (System Remote Manage) | `[no] ip ftp server cipher-suite {3des\| des\| rc4\| dhe}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpFtpServerCipher()` |
| Ch 64 (System Remote Manage) | `[no] ip ftp server port <1..65535>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpFtpServerPort()` |
| Ch 64 (System Remote Manage) | `[no] ip ftp server tls-required` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpFtpServerTls()` |
| Ch 64 (System Remote Manage) | `[no] ip http authentication auth_method` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpAuthenticationAuth()` |
| Ch 64 (System Remote Manage) | `[no] ip http content-security-policy` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpContentSecurity()` |
| Ch 64 (System Remote Manage) | `[no] ip http port <1..65535>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpPort()` |
| Ch 64 (System Remote Manage) | `[no] ip http secure-port <1..65535>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpSecurePort()` |
| Ch 64 (System Remote Manage) | `[no] ip http secure-server` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpSecureServer()` |
| Ch 64 (System Remote Manage) | `[no] ip http secure-server auth-client` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpSecureServer()` |
| Ch 64 (System Remote Manage) | `[no] ip http secure-server cert certificate_name` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpSecureServer()` |
| Ch 64 (System Remote Manage) | `[no] ip http secure-server force-redirect` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpSecureServer()` |
| Ch 64 (System Remote Manage) | `[no] ip http secure-server sslv3` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpSecureServer()` |
| Ch 64 (System Remote Manage) | `[no] ip http server` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpServer()` |
| Ch 64 (System Remote Manage) | `[no] ip http x-frame-options` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpXFrame()` |
| Ch 64 (System Remote Manage) | `[no] ip ssh server` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpSshServer()` |
| Ch 64 (System Remote Manage) | `[no] ip ssh server cert certificate_name` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpSshServerCert()` |
| Ch 64 (System Remote Manage) | `[no] ip ssh server kexalg dhe` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpSshServerKexalg()` |
| Ch 64 (System Remote Manage) | `[no] ip ssh server port <1..65535>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpSshServerPort()` |
| Ch 64 (System Remote Manage) | `[no] ip ssh server v1` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpSshServerV1()` |
| Ch 64 (System Remote Manage) | `[no] ip telnet server` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpTelnetServer()` |
| Ch 64 (System Remote Manage) | `[no] ip telnet server port <1..65535>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpTelnetServerPort()` |
| Ch 64 (System Remote Manage) | `[no] snmp-server` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSnmpServer()` |
| Ch 64 (System Remote Manage) | `[no] snmp-server community community_string {ro\|rw}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSnmpServerCommunityCommunity()` |
| Ch 64 (System Remote Manage) | `[no] snmp-server contact description` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSnmpServerContactDescription()` |
| Ch 64 (System Remote Manage) | `[no] snmp-server enable {informs\|traps}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSnmpServerEnable()` |
| Ch 64 (System Remote Manage) | `[no] snmp-server host {w.x.y.z\|fqdn\|ipv6 address} [community_string]` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSnmpServerHostCommunity()` |
| Ch 64 (System Remote Manage) | `[no] snmp-server location description` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSnmpServerLocationDescription()` |
| Ch 64 (System Remote Manage) | `[no] snmp-server port <1..65535>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSnmpServerPort()` |
| Ch 64 (System Remote Manage) | `ip ftp server rule move rule_number to rule_number` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpFtpServerRule()` |
| Ch 64 (System Remote Manage) | `ip ftp server rule {rule_number\|append\|insert rule_number} access-group {ALL\|address_object} zone {ALL\|zone_object} action {accept\|deny}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpFtpServerRule()` |
| Ch 64 (System Remote Manage) | `ip http secure-server cipher-suite {aes\| rc4\| des\| 3des\| dhe}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpSecureServer()` |
| Ch 64 (System Remote Manage) | `ip http secure-server table {admin\|user} rule move rule_number to rule_number` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpSecureServer()` |
| Ch 64 (System Remote Manage) | `ip http secure-server table {admin\|user} rule {rule_number\|append\|insert rule_number} access- group {ALL\|address_object} zone {ALL\|zone_object} action {accept\|deny}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpSecureServer()` |
| Ch 64 (System Remote Manage) | `ip http server table {admin\|user} rule move rule_number to rule_number` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpServerTable()` |
| Ch 64 (System Remote Manage) | `ip http server table {admin\|user} rule {rule_number\|append\|insert rule_number} access-group {ALL\|address_object} zone {ALL\|zone_object} action {accept\|deny}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpServerTable()` |
| Ch 64 (System Remote Manage) | `ip http skip-csrf-check` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpHttpSkipCsrf()` |
| Ch 64 (System Remote Manage) | `ip ssh server rule move rule_number to rule_number` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpSshServerRule()` |
| Ch 64 (System Remote Manage) | `ip ssh server rule {rule_number\|append\|insert rule_number} access-group {ALL\|address_object} zone {ALL\|zone_object} action {accept\|deny}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpSshServerRule()` |
| Ch 64 (System Remote Manage) | `ip telnet server rule move rule_number to rule_number` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpTelnetServerRule()` |
| Ch 64 (System Remote Manage) | `ip telnet server rule {rule_number\|append\|insert rule_number} access-group {ALL\|address_object} zone {ALL\|zone_object} action {accept\|deny}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpTelnetServerRule()` |
| Ch 64 (System Remote Manage) | `no ip ftp server rule rule_number` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNoIpFtpServer()` |
| Ch 64 (System Remote Manage) | `no ip http secure-server table {admin\|user} rule rule_number` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNoIpHttpSecure()` |
| Ch 64 (System Remote Manage) | `no ip http server table {admin\|user} rule rule_number` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdNoIpHttpServer()` |
| Ch 64 (System Remote Manage) | `no ip http skip-csrf-check` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNoIpHttpSkip()` |
| Ch 64 (System Remote Manage) | `no ip ssh server rule rule_number` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdNoIpSshServer()` |
| Ch 64 (System Remote Manage) | `no ip telnet server rule rule_number` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdNoIpTelnetServer()` |
| Ch 64 (System Remote Manage) | `no snmp-server rule rule_number` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNoSnmpServerRule()` |
| Ch 64 (System Remote Manage) | `show ip ftp server status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowIpFtpServer()` |
| Ch 64 (System Remote Manage) | `show ip http server secure status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowIpHttpServer()` |
| Ch 64 (System Remote Manage) | `show ip http server status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowIpHttpServer()` |
| Ch 64 (System Remote Manage) | `show ip http skip-csrf-check` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowIpHttpSkip()` |
| Ch 64 (System Remote Manage) | `show ip ssh server status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowIpSshServer()` |
| Ch 64 (System Remote Manage) | `show ip telnet server status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowIpTelnetServer()` |
| Ch 64 (System Remote Manage) | `show snmp status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSnmpStatus()` |
| Ch 64 (System Remote Manage) | `show snmp-server v3user status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSnmpServerV3user()` |
| Ch 64 (System Remote Manage) | `snmp-server rule move rule_number to rule_number` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSnmpServerRuleMove()` |
| Ch 64 (System Remote Manage) | `snmp-server rule {rule_number\|append\|insert rule_number} access-group {ALL\|address_object} zone {ALL\|zone_object} action {accept\|deny}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSnmpServerRuleAccess()` |
| Ch 64 (System Remote Manage) | `snmp-server v3user username description authentication {md5 \| sha} privacy {none \| des \| aes} privilege {ro \| rw}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSnmpServerV3userUsername()` |
| Ch 64 (System Remote Manage) | `snmp-server version {v2c \| v3}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSnmpServerVersion()` |
| Ch 64 (System Remote Manage) | `ssh {user@W.X.Y.Z \| or W.X.Y.Z)` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSshUserWX()` |
| Ch 65 (File Manager) | `[no] cloud-helper firmware update auto` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperFirmwareUpdate()` |
| Ch 65 (File Manager) | `[no] cloud-helper-notify activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperNotifyActivate()` |
| Ch 65 (File Manager) | `[no] private-encryption-key <encryption-key>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdPrivateEncryptionKey()` |
| Ch 65 (File Manager) | `apply /conf/file_name.conf [ignore-error] [rollback]` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdApplyConfFileName()` |
| Ch 65 (File Manager) | `cloud-helper check all` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckAll()` |
| Ch 65 (File Manager) | `cloud-helper check app` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckApp()` |
| Ch 65 (File Manager) | `cloud-helper check app_incr` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckApp()` |
| Ch 65 (File Manager) | `cloud-helper check av` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckAv()` |
| Ch 65 (File Manager) | `cloud-helper check botnet` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckBotnet()` |
| Ch 65 (File Manager) | `cloud-helper check ctdb` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckCtdb()` |
| Ch 65 (File Manager) | `cloud-helper check firmware` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckFirmware()` |
| Ch 65 (File Manager) | `cloud-helper check geoip` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckGeoip()` |
| Ch 65 (File Manager) | `cloud-helper check idp` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckIdp()` |
| Ch 65 (File Manager) | `cloud-helper check rf` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckRf()` |
| Ch 65 (File Manager) | `cloud-helper check sslca` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckSslca()` |
| Ch 65 (File Manager) | `cloud-helper check-notify new_features_cdr` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckNotify()` |
| Ch 65 (File Manager) | `cloud-helper check-notify new_features_dns_cf` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckNotify()` |
| Ch 65 (File Manager) | `cloud-helper check-notify new_features_rap` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckNotify()` |
| Ch 65 (File Manager) | `cloud-helper check-notify now` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckNotify()` |
| Ch 65 (File Manager) | `cloud-helper check-notify service_expired` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckNotify()` |
| Ch 65 (File Manager) | `cloud-helper check-notify whats_new` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCheckNotify()` |
| Ch 65 (File Manager) | `cloud-helper clean-download firmware` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperCleanDownload()` |
| Ch 65 (File Manager) | `cloud-helper firmware update daily <0..23> reboot {no \| yes}` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdCloudHelperFirmwareUpdate()` |
| Ch 65 (File Manager) | `cloud-helper firmware update weekly {fri \| mon \| sat \| sun \| thu \| tue \| wed} <0..23> reboot {no \| yes}` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdCloudHelperFirmwareUpdate()` |
| Ch 65 (File Manager) | `cloud-helper get app` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperGetApp()` |
| Ch 65 (File Manager) | `cloud-helper get av` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperGetAv()` |
| Ch 65 (File Manager) | `cloud-helper get botnet` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperGetBotnet()` |
| Ch 65 (File Manager) | `cloud-helper get firmware <1..2>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperGetFirmware()` |
| Ch 65 (File Manager) | `cloud-helper get idp` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperGetIdp()` |
| Ch 65 (File Manager) | `cloud-helper get sslca` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperGetSslca()` |
| Ch 65 (File Manager) | `cloud-helper pause-download firmware <1..2>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperPauseDownload()` |
| Ch 65 (File Manager) | `cloud-helper set remind {every-time \| never}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperSetRemind()` |
| Ch 65 (File Manager) | `cloud-helper set {[retry_times <1..10>]} {[retry_period <2..60>]} {[retry_fail_period <180..720>]}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperSet()` |
| Ch 65 (File Manager) | `cloud-helper set-read new_features_cdr` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperSetRead()` |
| Ch 65 (File Manager) | `cloud-helper set-read new_features_dns_cf` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperSetRead()` |
| Ch 65 (File Manager) | `cloud-helper set-read service_expired` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperSetRead()` |
| Ch 65 (File Manager) | `cloud-helper set-read whats_new` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperSetRead()` |
| Ch 65 (File Manager) | `cloud-helper update firmware <1..2>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCloudHelperUpdateFirmware()` |
| Ch 65 (File Manager) | `copy running-config /conf/file_name.conf` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCopyRunningConfigConf()` |
| Ch 65 (File Manager) | `copy running-config startup-config` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCopyRunningConfigStartup()` |
| Ch 65 (File Manager) | `copy {/conf \| /idp \| /packet_trace \| /script \| /tmp}file_name-a.conf {/conf \| /idp \| /pack- et_trace \| /script \| /tmp}/file_name-b.conf` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdCopyFileNameA()` |
| Ch 65 (File Manager) | `delete {/conf \| /idp \| /packet_trace \| /script \| /tmp}/file_name` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDeleteFileName()` |
| Ch 65 (File Manager) | `dir {/conf \| /idp \| /packet_trace \| /script \| /tmp}` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdDir()` |
| Ch 65 (File Manager) | `rename /script/old-file_name /script/new-file_name` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRenameScriptOldFile()` |
| Ch 65 (File Manager) | `rename {/conf \| /idp \| /packet_trace \| /script \| /tmp}/old-file_name {/conf \| /idp \| /pack- et_trace \| /script \| /tmp}/new-file_name` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRenameOldFileName()` |
| Ch 65 (File Manager) | `run /script/file_name.zysh` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRunScriptFileName()` |
| Ch 65 (File Manager) | `schedule-run 1 file_name.zysh {daily \| monthly \| weekly} time {date \| sun \| mon \| tue \| wed \| thu \| fri \| sat}` | Envelope 1 | `tier2` | `ZyxelSystemCmd::cmdScheduleRun1File()` |
| Ch 65 (File Manager) | `set firmware boot number <1..2>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSetFirmwareBootNumber()` |
| Ch 65 (File Manager) | `set firmware boot option <0..1>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSetFirmwareBootOption()` |
| Ch 65 (File Manager) | `setenv-startup stop-on-error off` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSetenvStartupStopOn()` |
| Ch 65 (File Manager) | `show cloud-helper autoupdate firmware` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowCloudHelperAutoupdate()` |
| Ch 65 (File Manager) | `show cloud-helper firmware` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowCloudHelperFirmware()` |
| Ch 65 (File Manager) | `show cloud-helper highlight` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowCloudHelperHighlight()` |
| Ch 65 (File Manager) | `show cloud-helper notify_all` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowCloudHelperNotify()` |
| Ch 65 (File Manager) | `show cloud-helper remind` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowCloudHelperRemind()` |
| Ch 65 (File Manager) | `show cloud-helper retry` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowCloudHelperRetry()` |
| Ch 65 (File Manager) | `show firmware image boot option` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowFirmwareImageBoot()` |
| Ch 65 (File Manager) | `show private-encryption-key status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowPrivateEncryptionKey()` |
| Ch 65 (File Manager) | `show running-config` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowRunningConfig()` |
| Ch 65 (File Manager) | `show setenv-startup` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSetenvStartup()` |
| Ch 65 (File Manager) | `write` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdWrite()` |
| Ch 66 (Logs................) | `FACILITY` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdFacility()` |
| Ch 66 (Logs................) | `HOSTNAME` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdHostname()` |
| Ch 66 (Logs................) | `MODULE_NAME_WTP` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdModuleNameWtp()` |
| Ch 66 (Logs................) | `MODULE_NAME_WTP_` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdModuleNameWtp()` |
| Ch 66 (Logs................) | `USER_NAME_` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdUserName()` |
| Ch 66 (Logs................) | `WEEKDAYS` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdWeekdays()` |
| Ch 66 (Logs................) | `ZYLOG_SUBJECT` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdZylogSubject()` |
| Ch 66 (Logs................) | `[no ]logging mail <1..2> tls authenticate-server` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNoLoggingMailTls()` |
| Ch 66 (Logs................) | `[no] connectivity-check continuous-log activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdConnectivityCheckContinuousLog()` |
| Ch 66 (Logs................) | `[no] logging cef-format include year` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingCefFormatInclude()` |
| Ch 66 (Logs................) | `[no] logging console` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingConsole()` |
| Ch 66 (Logs................) | `[no] logging console category module_name` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingConsoleCategoryModule()` |
| Ch 66 (Logs................) | `[no] logging debug suppression` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingDebugSuppression()` |
| Ch 66 (Logs................) | `[no] logging debug suppression interval <10..600>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingDebugSuppressionInterval()` |
| Ch 66 (Logs................) | `[no] logging mail <1..2>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingMail()` |
| Ch 66 (Logs................) | `[no] logging mail <1..2> address {ip \| hostname}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingMailAddress()` |
| Ch 66 (Logs................) | `[no] logging mail <1..2> authentication` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingMailAuthentication()` |
| Ch 66 (Logs................) | `[no] logging mail <1..2> authentication username username password password` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdLoggingMailAuthenticationUsername()` |
| Ch 66 (Logs................) | `[no] logging mail <1..2> category module_name level {alert \| all}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingMailCategoryModule()` |
| Ch 66 (Logs................) | `[no] logging mail <1..2> port <1..65535>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingMailPort()` |
| Ch 66 (Logs................) | `[no] logging mail <1..2> schedule {full \| hourly}` | Envelope 1 | `tier2` | `ZyxelSystemCmd::cmdLoggingMailSchedule()` |
| Ch 66 (Logs................) | `[no] logging mail <1..2> subject subject` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingMailSubjectSubject()` |
| Ch 66 (Logs................) | `[no] logging mail <1..2> tls activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingMailTlsActivate()` |
| Ch 66 (Logs................) | `[no] logging mail <1..2> tls starttls-off` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingMailTlsStarttls()` |
| Ch 66 (Logs................) | `[no] logging mail <1..2> {send-log-to \| send-alerts-to} e_mail` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingMailEMail()` |
| Ch 66 (Logs................) | `[no] logging syslog <1..4>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingSyslog()` |
| Ch 66 (Logs................) | `[no] logging syslog <1..4> address {ip \| hostname}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingSyslogAddress()` |
| Ch 66 (Logs................) | `[no] logging syslog <1..4> facility {local_1 \| local_2 \| local_3 \| local_4 \| local_5 \| local_6 \| local_7}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingSyslogFacility()` |
| Ch 66 (Logs................) | `[no] logging syslog <1..4> format {cef \| vrpt}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingSyslogFormat()` |
| Ch 66 (Logs................) | `[no] logging syslog <1..4> {disable \| level normal \| level all}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingSyslog()` |
| Ch 66 (Logs................) | `[no] logging system-log suppression` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingSystemLogSuppression()` |
| Ch 66 (Logs................) | `[no] logging system-log suppression interval <10..600>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingSystemLogSuppression()` |
| Ch 66 (Logs................) | `[no] logging usb-storage` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingUsbStorage()` |
| Ch 66 (Logs................) | `[no] logging usb-storage keep-duration` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingUsbStorageKeep()` |
| Ch 66 (Logs................) | `clear logging debug buffer` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdClearLoggingDebugBuffer()` |
| Ch 66 (Logs................) | `clear logging system-log buffer` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdClearLoggingSystemLog()` |
| Ch 66 (Logs................) | `logging console category module_name level {alert \| crit \| debug \| emerg \| error \| info \| notice \| warn}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingConsoleCategoryModule()` |
| Ch 66 (Logs................) | `logging mail <1..2> schedule daily hour <0..23> minute <0..59>` | Envelope 1 | `tier2` | `ZyxelSystemCmd::cmdLoggingMailScheduleDaily()` |
| Ch 66 (Logs................) | `logging mail <1..2> schedule weekly day day hour <0..23> minute <0..59>` | Envelope 1 | `tier2` | `ZyxelSystemCmd::cmdLoggingMailScheduleWeekly()` |
| Ch 66 (Logs................) | `logging mail <1..2> sending_now` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingMailSendingNow()` |
| Ch 66 (Logs................) | `logging system-log category module_name {disable \| level normal \| level all}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingSystemLogCategory()` |
| Ch 66 (Logs................) | `logging usb-storage category module_name level {all \| normal}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingUsbStorageCategory()` |
| Ch 66 (Logs................) | `logging usb-storage delete over-keep-duration` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingUsbStorageDelete()` |
| Ch 66 (Logs................) | `logging usb-storage flushThreshold <1..100>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingUsbStorageFlushthreshold()` |
| Ch 66 (Logs................) | `logging usb-storage keep-duration day <1..365>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdLoggingUsbStorageKeep()` |
| Ch 66 (Logs................) | `show connectivity-check continuous-log status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowConnectivityCheckContinuous()` |
| Ch 66 (Logs................) | `show logging debug entries [priority pri] [category module_name] [srcip ip] [srcip6 ipv6_addr] [dstip ip] [dstip6 ipv6_addr] [service service_name] [srciface interface_name] [dstiface interface_name] [protocol protocol] [begin <1..512> end <1..512>] [keyword keyword] show logging debug entries field field [begin <1..1024> end <1..1024>]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowLoggingDebugEntries()` |
| Ch 66 (Logs................) | `show logging debug status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowLoggingDebugStatus()` |
| Ch 66 (Logs................) | `show logging entries [priority pri] [category module_name] [srcip ip] [srcip6 ipv6_addr] [dstip ip] [dstip6 ipv6_addr] [service service_name] [begin <1..512> end <1..512>] [keyword key- word] [srciface interface_name] [dstiface interface_name] [protocol protocol] 566 show logging entries field field [begin <1..512> end <1..512>]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowLoggingEntriesPriority()` |
| Ch 66 (Logs................) | `show logging status console` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowLoggingStatusConsole()` |
| Ch 66 (Logs................) | `show logging status mail` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowLoggingStatusMail()` |
| Ch 66 (Logs................) | `show logging status syslog` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowLoggingStatusSyslog()` |
| Ch 66 (Logs................) | `show logging status system-log` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowLoggingStatusSystem()` |
| Ch 66 (Logs................) | `show vrpt send device information interval` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowVrptSendDevice()` |
| Ch 66 (Logs................) | `show vrpt send interface statistics interval` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowVrptSendInterface()` |
| Ch 66 (Logs................) | `show vrpt send system status interval` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowVrptSendSystem()` |
| Ch 66 (Logs................) | `vrpt send device information interval <15..3600>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdVrptSendDeviceInformation()` |
| Ch 66 (Logs................) | `vrpt send interface statistics interval <15..3600>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdVrptSendInterfaceStatistics()` |
| Ch 66 (Logs................) | `vrpt send system status interval <15..3600>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdVrptSendSystemStatus()` |
| Ch 67 (Reports and Reboot) | `[no] activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdActivate()` |
| Ch 67 (Reports and Reboot) | `[no] item as-report` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdItemAsReport()` |
| Ch 67 (Reports and Reboot) | `[no] item av-report` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdItemAvReport()` |
| Ch 67 (Reports and Reboot) | `[no] item cf-report` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdItemCfReport()` |
| Ch 67 (Reports and Reboot) | `[no] item cpu-usage ...........................................................................................................` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdItemCpuUsage()` |
| Ch 67 (Reports and Reboot) | `[no] item idp-report ..........................................................................................................` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdItemIdpReport()` |
| Ch 67 (Reports and Reboot) | `[no] item mem-usage ...........................................................................................................` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdItemMemUsage()` |
| Ch 67 (Reports and Reboot) | `[no] item port-usage` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdItemPortUsage()` |
| Ch 67 (Reports and Reboot) | `[no] item session-usage` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdItemSessionUsage()` |
| Ch 67 (Reports and Reboot) | `[no] item traffic-report` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdItemTrafficReport()` |
| Ch 67 (Reports and Reboot) | `[no] mac-auth database mac oui type ext-oui mac-role mac-users description description 450 [no] mac-auth database mac oui type int-oui mac-role mac-users description description 450 [no] mail-from e_mail` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMacAuthDatabaseMac()` |
| Ch 67 (Reports and Reboot) | `[no] mail-subject append date-time` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMailSubjectAppendDate()` |
| Ch 67 (Reports and Reboot) | `[no] mail-subject append system-name` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMailSubjectAppendSystem()` |
| Ch 67 (Reports and Reboot) | `[no] mail-to-1 e_mail` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMailTo1E()` |
| Ch 67 (Reports and Reboot) | `[no] mail-to-2 e_mail` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMailTo2E()` |
| Ch 67 (Reports and Reboot) | `[no] mail-to-3 e_mail` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMailTo3E()` |
| Ch 67 (Reports and Reboot) | `[no] mail-to-4 e_mail` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMailTo4E()` |
| Ch 67 (Reports and Reboot) | `[no] mail-to-5 e_mail` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMailTo5E()` |
| Ch 67 (Reports and Reboot) | `[no] report` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdReport()` |
| Ch 67 (Reports and Reboot) | `[no] report packet size statistics` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdReportPacketSizeStatistics()` |
| Ch 67 (Reports and Reboot) | `[no] reset-counter ............................................................................................................` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdResetCounter()` |
| Ch 67 (Reports and Reboot) | `clear report [interface_name]` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdClearReportInterfaceName()` |
| Ch 67 (Reports and Reboot) | `daily-report` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDailyReport()` |
| Ch 67 (Reports and Reboot) | `draw-usage-graphics` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDrawUsageGraphics()` |
| Ch 67 (Reports and Reboot) | `exit` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdExit()` |
| Ch 67 (Reports and Reboot) | `mail-subject set subject` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMailSubjectSetSubject()` |
| Ch 67 (Reports and Reboot) | `no mail-subject set ...........................................................................................................` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNoMailSubjectSet()` |
| Ch 67 (Reports and Reboot) | `report packet size statistics clear` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdReportPacketSizeStatistics()` |
| Ch 67 (Reports and Reboot) | `reset-counter-now` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdResetCounterNow()` |
| Ch 67 (Reports and Reboot) | `schedule hour <0..23> minute <00..59>` | Envelope 1 | `tier2` | `ZyxelSystemCmd::cmdScheduleHourMinute()` |
| Ch 67 (Reports and Reboot) | `send-now` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSendNow()` |
| Ch 67 (Reports and Reboot) | `show conn [user {username\|any\|unknown}] [service {service-name\|any\|unknown}] [source {ip\|any}] [destination {ip\|any}] [begin <1..128000>] [end <1..128000>] [dstcc {country-code\|any}] [srtcc {country-code\|any}] fastpath` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowConnUserService()` |
| Ch 67 (Reports and Reboot) | `show conn ip-traffic destination` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowConnIpTraffic()` |
| Ch 67 (Reports and Reboot) | `show conn ip-traffic source` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowConnIpTraffic()` |
| Ch 67 (Reports and Reboot) | `show conn status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowConnStatus()` |
| Ch 67 (Reports and Reboot) | `show daily-report status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowDailyReportStatus()` |
| Ch 67 (Reports and Reboot) | `show report [interface_name {ip \| service \| url}]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReportInterfaceName()` |
| Ch 67 (Reports and Reboot) | `show report [interface_name] https-url` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReportInterfaceName()` |
| Ch 67 (Reports and Reboot) | `show report packet size statistics status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReportPacketSize()` |
| Ch 67 (Reports and Reboot) | `show report packet size statistics {interface_name} [interval interval]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReportPacketSize()` |
| Ch 67 (Reports and Reboot) | `show report status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowReportStatus()` |
| Ch 68 (Diagnostics and Remo) | `(no) remote-assistance activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRemoteAssistanceActivate()` |
| Ch 68 (Diagnostics and Remo) | `diag-info collect` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDiagInfoCollect()` |
| Ch 68 (Diagnostics and Remo) | `diaginfo collect ac` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDiaginfoCollectAc()` |
| Ch 68 (Diagnostics and Remo) | `diaginfo collect wtp` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDiaginfoCollectWtp()` |
| Ch 68 (Diagnostics and Remo) | `diaginfo delete /ac` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDiaginfoDeleteAc()` |
| Ch 68 (Diagnostics and Remo) | `diaginfo delete /wtp` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDiaginfoDeleteWtp()` |
| Ch 68 (Diagnostics and Remo) | `nslookup {ipv4 \| hostname} [server ipv4] [extension filter-extension]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdNslookupServerIpv4Extension()` |
| Ch 68 (Diagnostics and Remo) | `nslookup6 {ipv6 \| hostname} [server ipv6] [extension filter-extension]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdNslookup6ServerIpv6Extension()` |
| Ch 68 (Diagnostics and Remo) | `remote-assistance [address1\|address2] ipv4` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRemoteAssistanceAddress1Address2()` |
| Ch 68 (Diagnostics and Remo) | `remote-assistance [https\|ssh] port port` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRemoteAssistanceHttpsSsh()` |
| Ch 68 (Diagnostics and Remo) | `remote-assistance generate user-password` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdRemoteAssistanceGenerateUser()` |
| Ch 68 (Diagnostics and Remo) | `remote-assistance remove {address1\|address2}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRemoteAssistanceRemove()` |
| Ch 68 (Diagnostics and Remo) | `remote-assistance schedule DATE TIME date time` | Envelope 1 | `tier2` | `ZyxelSystemCmd::cmdRemoteAssistanceScheduleDate()` |
| Ch 68 (Diagnostics and Remo) | `remote-assistance settings [random\|manual]` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRemoteAssistanceSettingsRandom()` |
| Ch 68 (Diagnostics and Remo) | `remote-assistance user-object user` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRemoteAssistanceUserObject()` |
| Ch 68 (Diagnostics and Remo) | `show cpu average` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowCpuAverage()` |
| Ch 68 (Diagnostics and Remo) | `show diag-info` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowDiagInfo()` |
| Ch 68 (Diagnostics and Remo) | `show mem status all` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowMemStatusAll()` |
| Ch 68 (Diagnostics and Remo) | `show remote-assistance` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowRemoteAssistance()` |
| Ch 68 (Diagnostics and Remo) | `show remote-assistance generate` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowRemoteAssistanceGenerate()` |
| Ch 69 (Session Timeout) | `session timeout session {tcp-established \| tcp-synrecv \| tcp-close \| tcp-finwait \| tcp-synsent \| tcp-closewait \| tcp-lastack \| tcp-timewait} <1..300>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSessionTimeoutSession()` |
| Ch 69 (Session Timeout) | `session timeout {udp-connect <1..300> \| udp-deliver <1..300> \| icmp <1..300>}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSessionTimeout()` |
| Ch 69 (Session Timeout) | `show session timeout {icmp \| tcp-timewait \| udp}` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSessionTimeout()` |
| Ch 70 (Packet Flow Explore) | `show ip route static-dynamic` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowIpRouteStatic()` |
| Ch 70 (Packet Flow Explore) | `show route order` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowRouteOrder()` |
| Ch 70 (Packet Flow Explore) | `show system route default-wan-trunk` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSystemRouteDefault()` |
| Ch 70 (Packet Flow Explore) | `show system route dynamic-vpn` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSystemRouteDynamic()` |
| Ch 70 (Packet Flow Explore) | `show system route nat-1-1` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSystemRouteNat()` |
| Ch 70 (Packet Flow Explore) | `show system route policy-route` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSystemRoutePolicy()` |
| Ch 70 (Packet Flow Explore) | `show system route site-to-site-vpn` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSystemRouteSite()` |
| Ch 70 (Packet Flow Explore) | `show system snat default-snat` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSystemSnatDefault()` |
| Ch 70 (Packet Flow Explore) | `show system snat nat-1-1` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSystemSnatNat()` |
| Ch 70 (Packet Flow Explore) | `show system snat nat-loopback` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSystemSnatNat()` |
| Ch 70 (Packet Flow Explore) | `show system snat order` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSystemSnatOrder()` |
| Ch 70 (Packet Flow Explore) | `show system snat policy-route` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSystemSnatPolicy()` |
| Ch 71 (Maintenance Tools) | `Ping {ipv4 \| hostname} [source ipv4] [size <0..65507>] [forever\| count <1..4096>] 587 ping {ipv4_addr \| hostname} [source ipv4] [size <0..65507>] [forever \| count <1..4096>] [in- terface interface_name] [extension filter-extension]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdPingSourceIpv4Size()` |
| Ch 71 (Maintenance Tools) | `The conf-backup commands automatically backup the current Zyxel Device configuration file ac- cording to a schedule, and then send it to an email address.` | Envelope 1 | `tier2` | `ZyxelSystemCmd::cmdTheConfBackupCommands()` |
| Ch 71 (Maintenance Tools) | `The conf-mail commands send a user-specified configuration file immediately to an email address.` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdTheConfMailCommands()` |
| Ch 71 (Maintenance Tools) | `[no] config-backup scheduler activate` | Envelope 1 | `tier2` | `ZyxelSystemCmd::cmdConfigBackupSchedulerActivate()` |
| Ch 71 (Maintenance Tools) | `[no] mail-send` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMailSend()` |
| Ch 71 (Maintenance Tools) | `[no] packet-capture activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdPacketCaptureActivate()` |
| Ch 71 (Maintenance Tools) | `[no] schedule reboot activate` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdScheduleRebootActivate()` |
| Ch 71 (Maintenance Tools) | `arp IP mac_address` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdArpIpMacAddress()` |
| Ch 71 (Maintenance Tools) | `conf-mail mail-content <mail-content>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdConfMailMailContent()` |
| Ch 71 (Maintenance Tools) | `conf-mail mail-subject <subject>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdConfMailMailSubject()` |
| Ch 71 (Maintenance Tools) | `conf-mail no mail-content` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdConfMailNoMail()` |
| Ch 71 (Maintenance Tools) | `conf-mail no {mail-to-1\|mail-to-2\|mail-to-3\|mail-to-4\|mail-to-5}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdConfMailNo()` |
| Ch 71 (Maintenance Tools) | `conf-mail send-now <configfile>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdConfMailSendNow()` |
| Ch 71 (Maintenance Tools) | `conf-mail {mail-to-1\|mail-to-2\|mail-to-3\|mail-to-4\|mail-to-5} <user@domainname> .595 conf-mail attach password <attachment password>` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdConfMail595Conf()` |
| Ch 71 (Maintenance Tools) | `config-backup run` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdConfigBackupRun()` |
| Ch 71 (Maintenance Tools) | `config-backup setting` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdConfigBackupSetting()` |
| Ch 71 (Maintenance Tools) | `device-ha2 schedule-reboot check-timeout <1..3600>` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdDeviceHa2ScheduleReboot()` |
| Ch 71 (Maintenance Tools) | `device-ha2 schedule-reboot delay <1..300>` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdDeviceHa2ScheduleReboot()` |
| Ch 71 (Maintenance Tools) | `duration <0..300>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdDuration()` |
| Ch 71 (Maintenance Tools) | `exit` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdExit()` |
| Ch 71 (Maintenance Tools) | `file-suffix <profile_name>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdFileSuffix()` |
| Ch 71 (Maintenance Tools) | `files-size <1..10000> .........................................................................................................` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdFilesSize()` |
| Ch 71 (Maintenance Tools) | `host-ip {ip-address \| profile_name \| any>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdHostIpIpAddress()` |
| Ch 71 (Maintenance Tools) | `host-port <0..65535>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdHostPort()` |
| Ch 71 (Maintenance Tools) | `iface {add \| del} {interface_name \| virtual_interface_name}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIface()` |
| Ch 71 (Maintenance Tools) | `ip-version {ip\|ip6\|any}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpVersion()` |
| Ch 71 (Maintenance Tools) | `ipv6 neighbor flush {ipv6 \| all}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdIpv6NeighborFlush()` |
| Ch 71 (Maintenance Tools) | `mail-attach password <attachment password>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMailAttachPassword()` |
| Ch 71 (Maintenance Tools) | `mail-info content <mail-content>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMailInfoContent()` |
| Ch 71 (Maintenance Tools) | `mail-info {mail-to-1\|mail-to-2\|mail-to-3\|mail-to-4\|mail-to-5} <user@domainname>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMailInfo()` |
| Ch 71 (Maintenance Tools) | `no arp ip` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNoArpIp()` |
| Ch 71 (Maintenance Tools) | `no mail-attach password` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNoMailAttachPassword()` |
| Ch 71 (Maintenance Tools) | `no mail-info content` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNoMailInfoContent()` |
| Ch 71 (Maintenance Tools) | `no {mail-to-1\|mail-to-2\|mail-to-3\|mail-to-4\|mail-to-5}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdNo()` |
| Ch 71 (Maintenance Tools) | `packet-capture configure` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdPacketCaptureConfigure()` |
| Ch 71 (Maintenance Tools) | `packet-trace [interface interface_name] [[ip-proto\|ipv6-proto] \| protocol_name \| any}] [src- host {ip \| hostname \| any}] [dst-host {ip \| hostname \| any}] [host {ip\| hostname\| any}] [port {<1..65535> \| any}] [file] [duration <1..3600>] [extension-filter arp\| link-header\| data-header\| match-port port port\| match-host host host]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdPacketTraceInterfaceInterface()` |
| Ch 71 (Maintenance Tools) | `ping6{ipv6 \| hostname} [source ipv6] [size <0..65527>] [forever\| count <1..4096>] [interface {interface_name \| virtual_interface_name}][extension filter_extension]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdPing6SourceIpv6Size()` |
| Ch 71 (Maintenance Tools) | `proto-type {icmp \| icmp6 \| igmp \| igrp \| pim \| ah \| esp \| vrrp \| udp \| tcp \| any}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdProtoType()` |
| Ch 71 (Maintenance Tools) | `ring-buffer <enable\|disable>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdRingBuffer()` |
| Ch 71 (Maintenance Tools) | `schedule reboot daily <time,hh:mm>` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdScheduleRebootDaily()` |
| Ch 71 (Maintenance Tools) | `schedule reboot monthly <time,hh:mm> <day,dd>` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdScheduleRebootMonthly()` |
| Ch 71 (Maintenance Tools) | `schedule reboot weekly <time,hh:mm> {sun\|mon\|tue\|wed\|thu\|fri\|sat}` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdScheduleRebootWeekly()` |
| Ch 71 (Maintenance Tools) | `schedule-object object_name time time [day] [day] [day] [day] [day] [day] [day] .471 scheduler daily <time,hh:mm>` | Envelope 1 | `tier2` | `ZyxelSystemCmd::cmdScheduleObjectObjectName()` |
| Ch 71 (Maintenance Tools) | `scheduler monthly <time,hh:mm> <day,dd>` | Envelope 1 | `tier2` | `ZyxelSystemCmd::cmdSchedulerMonthly()` |
| Ch 71 (Maintenance Tools) | `scheduler weekly <time,hh:mm> {sun\|mon\|tue\|wed\|thu\|fri\|sat}` | Envelope 1 | `tier2` | `ZyxelSystemCmd::cmdSchedulerWeekly()` |
| Ch 71 (Maintenance Tools) | `show arp-table` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowArpTable()` |
| Ch 71 (Maintenance Tools) | `show config-backup status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowConfigBackupStatus()` |
| Ch 71 (Maintenance Tools) | `show config-backup status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowConfigBackupStatus()` |
| Ch 71 (Maintenance Tools) | `show device-ha2 schedule-reboot check-timeout` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdShowDeviceHa2Schedule()` |
| Ch 71 (Maintenance Tools) | `show device-ha2 schedule-reboot delay` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdShowDeviceHa2Schedule()` |
| Ch 71 (Maintenance Tools) | `show ipv6 neighbor-list` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowIpv6NeighborList()` |
| Ch 71 (Maintenance Tools) | `show packet-capture config` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowPacketCaptureConfig()` |
| Ch 71 (Maintenance Tools) | `show packet-capture status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowPacketCaptureStatus()` |
| Ch 71 (Maintenance Tools) | `show schedule reboot status` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdShowScheduleRebootStatus()` |
| Ch 71 (Maintenance Tools) | `snaplen <68..1512>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSnaplen()` |
| Ch 71 (Maintenance Tools) | `split-size <1..2048>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSplitSize()` |
| Ch 71 (Maintenance Tools) | `storage <internal\|usbstorage>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdStorage()` |
| Ch 71 (Maintenance Tools) | `tracepath6 {ipv6 \| hostname}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdTracepath6()` |
| Ch 71 (Maintenance Tools) | `traceroute {ip \| hostname}` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdTraceroute()` |
| Ch 71 (Maintenance Tools) | `traceroute {ipv4 \| hostname} [source ipv4] [interface interface_name] [extension filter-exten- sion]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdTracerouteSourceIpv4Interface()` |
| Ch 71 (Maintenance Tools) | `traceroute6 {ipv6 \| hostname}` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdTraceroute6()` |
| Ch 71 (Maintenance Tools) | `traceroute6 {ipv6 \| hostname} [source ipv6] [interface interface_name] [extension filter-ex- tension]` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdTraceroute6SourceIpv6Interface()` |
| Ch 72 (Miscellaneous) | `[no] app-profile <profile name> {[no log]\|[log by-profile]} {activate \| deactivate} 223 [no] app-watch-dog activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAppProfile223App()` |
| Ch 72 (Miscellaneous) | `[no] app-watch-dog alert` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAppWatchDogAlert()` |
| Ch 72 (Miscellaneous) | `[no] app-watch-dog auto-recover` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAppWatchDogAuto()` |
| Ch 72 (Miscellaneous) | `[no] app-watch-dog console-print {always\|once}` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAppWatchDogConsole()` |
| Ch 72 (Miscellaneous) | `[no] app-watch-dog cpu-threshold min <1..100> max <1..100>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAppWatchDogCpu()` |
| Ch 72 (Miscellaneous) | `[no] app-watch-dog disk-threshold min <1..100> max <1..100>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAppWatchDogDisk()` |
| Ch 72 (Miscellaneous) | `[no] app-watch-dog interval <6..300>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAppWatchDogInterval()` |
| Ch 72 (Miscellaneous) | `[no] app-watch-dog mem-threshold min <1..100> max <1..100>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAppWatchDogMem()` |
| Ch 72 (Miscellaneous) | `[no] app-watch-dog retry-count <1..5>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdAppWatchDogRetry()` |
| Ch 72 (Miscellaneous) | `[no] app-watch-dog sys-reboot` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdAppWatchDogSys()` |
| Ch 72 (Miscellaneous) | `[no] gui-visibility gui-visibility policy-route-fromlocal-snat` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdGuiVisibilityGuiVisibility()` |
| Ch 72 (Miscellaneous) | `[no] gui-visibility show-advanced` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdGuiVisibilityShowAdvanced()` |
| Ch 72 (Miscellaneous) | `[no] hardware-watchdog-timer <4..37>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdHardwareWatchdogTimer()` |
| Ch 72 (Miscellaneous) | `[no] mem-conserve activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMemConserveActivate()` |
| Ch 72 (Miscellaneous) | `[no] mem-conserve av-bypass falling-threshold <1..4000>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMemConserveAvBypass()` |
| Ch 72 (Miscellaneous) | `[no] mem-conserve av-bypass rising-threshold <1..4000>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMemConserveAvBypass()` |
| Ch 72 (Miscellaneous) | `[no] mem-conserve av-bypass sustained-time <1..60>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMemConserveAvBypass()` |
| Ch 72 (Miscellaneous) | `[no] mem-conserve utm-bypass falling-threshold <1..4000>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMemConserveUtmBypass()` |
| Ch 72 (Miscellaneous) | `[no] mem-conserve utm-bypass rising-threshold <1..4000>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMemConserveUtmBypass()` |
| Ch 72 (Miscellaneous) | `[no] mem-conserve utm-bypass sustained-time <1..60>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdMemConserveUtmBypass()` |
| Ch 72 (Miscellaneous) | `[no] software-watchdog-timer <10..600>` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdSoftwareWatchdogTimer()` |
| Ch 72 (Miscellaneous) | `[no] web-google-analytics activate` | Envelope 1 | `tier1` | `ZyxelSystemCmd::cmdWebGoogleAnalyticsActivate()` |
| Ch 72 (Miscellaneous) | `app-watch-dog reboot-log flush` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdAppWatchDogReboot()` |
| Ch 72 (Miscellaneous) | `show app-watch-dog config` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowAppWatchDog()` |
| Ch 72 (Miscellaneous) | `show app-watch-dog monitor-list` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowAppWatchDog()` |
| Ch 72 (Miscellaneous) | `show app-watch-dog reboot-log` | Envelope 1 | `redline` | `ZyxelSystemCmd::cmdShowAppWatchDog()` |
| Ch 72 (Miscellaneous) | `show gui-visability status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowGuiVisabilityStatus()` |
| Ch 72 (Miscellaneous) | `show hardware-watchdog-timer status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowHardwareWatchdogTimer()` |
| Ch 72 (Miscellaneous) | `show mem-conserve status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowMemConserveStatus()` |
| Ch 72 (Miscellaneous) | `show sdwan oncloudst` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSdwanOncloudst()` |
| Ch 72 (Miscellaneous) | `show software-watchdog-timer log` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSoftwareWatchdogTimer()` |
| Ch 72 (Miscellaneous) | `show software-watchdog-timer status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowSoftwareWatchdogTimer()` |
| Ch 72 (Miscellaneous) | `show web-google-analytics status` | Envelope 1 | `read` | `ZyxelSystemCmd::cmdShowWebGoogleAnalytics()` |
| Ch 73 (Managed AP Commands) | `capwap ap ac-ip auto` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApAcIp()` |
| Ch 73 (Managed AP Commands) | `capwap ap vlan ip address ip netmask` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApVlanIp()` |
| Ch 73 (Managed AP Commands) | `capwap ap vlan ip gateway gateway` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApVlanIp()` |
| Ch 73 (Managed AP Commands) | `capwap ap vlan no ip gateway` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApVlanNo()` |
| Ch 73 (Managed AP Commands) | `capwap ap vlan vlan-id vid { tag \| untag }` | Envelope 10 | `tier1` | `ZyxelWlanCmd::cmdCapwapApVlanVlan()` |
| Ch 73 (Managed AP Commands) | `ip dns server zone-forwarder move <1..32> to <1..32>` | Envelope 10 | `tier2` | `ZyxelWlanCmd::cmdIpDnsServerZone()` |
| Ch 73 (Managed AP Commands) | `no ip dns server zone-forwarder <1..4>` | Envelope 10 | `tier2` | `ZyxelWlanCmd::cmdNoIpDnsServer()` |
| Ch 73 (Managed AP Commands) | `show capwap ap ac-ip` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapApAc()` |
| Ch 73 (Managed AP Commands) | `show capwap ap discovery-type` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapApDiscovery()` |
| Ch 73 (Managed AP Commands) | `show capwap ap info` | Envelope 10 | `read` | `ZyxelWlanCmd::cmdShowCapwapApInfo()` |
