/*
 * NetMonShell.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "NetMonShell.hxx"
#include "Config.hxx"
#include "LanSniffer.hxx"
#include "DeviceRegistry.hxx"
#include "AimonGatewayClient.hxx"
#include "AuthManager.hxx"
#include "ZyxelDriver.hxx"
#include "AiSecurityClearance.hxx"
#include "UnixAuth.hxx"
#include "ZyxelLiveDiagnostic.hxx"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <ctime>
#include <unistd.h>
#include <termios.h>
#include <mutex>

static std::mutex g_pwMutex;
static NetMonShell::PasswordReaderFunc g_pwReaderForTesting = nullptr;

static std::string readPasswordInteractive(const std::string &prompt) {
    {
        std::lock_guard<std::mutex> lock(g_pwMutex);
        if (g_pwReaderForTesting) {
            return g_pwReaderForTesting(prompt);
        }
    }

    std::cout << prompt << std::flush;
    std::string password;

    if (isatty(STDIN_FILENO)) {
        struct termios oldt, newt;
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~ECHO;
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);

        std::getline(std::cin, password);

        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        std::cout << std::endl;
    } else {
        std::getline(std::cin, password);
    }

    while (!password.empty() && (password.back() == '\r' || password.back() == '\n')) {
        password.pop_back();
    }
    return password;
}

void NetMonShell::setPasswordReaderForTesting(PasswordReaderFunc reader) {
    std::lock_guard<std::mutex> lock(g_pwMutex);
    g_pwReaderForTesting = reader;
}

NetMonShell &NetMonShell::getInstance() {
    static NetMonShell instance;
    return instance;
}

NetMonShell::NetMonShell()
    : _running(false),
      _executingCommand(false) {
}

bool NetMonShell::isExecutingCommand() const {
    return _executingCommand.load();
}

void NetMonShell::cancelCurrentCommand() {
    ZyxelDriver::getInstance().cancelActiveCommand();
}

static std::string formatRelativeTime(time_t timestamp) {
    if (timestamp <= 0) {
        return "never";
    }
    time_t now = time(nullptr);
    if (now < timestamp) {
        return "just now";
    }
    long diff = static_cast<long>(now - timestamp);
    if (diff < 5) return "just now";
    if (diff < 60) return std::to_string(diff) + "s ago";
    if (diff < 3600) return std::to_string(diff / 60) + "m ago";
    if (diff < 86400) return std::to_string(diff / 3600) + "h ago";
    return std::to_string(diff / 86400) + "d ago";
}

void NetMonShell::printBanner() const {
    std::cout << "============================================================" << std::endl;
    std::cout << "  NetMon - Network Monitor & AI Telemetry Gateway           " << std::endl;
    std::cout << "  Type 'help' for commands, 'quit' to exit                  " << std::endl;
    std::cout << "============================================================" << std::endl;

    if (!LanSniffer::getInstance().isPcapActive()) {
        std::string err = LanSniffer::getInstance().getPcapError();
        std::string hint = LanSniffer::getInstance().getPcapRemediationHint();
        if (err.empty()) {
            err = "Packet capture interface failed to open.";
        }
        if (hint.empty()) {
            hint = "Run 'make setcap' or verify interface configuration in netmon.cfg";
        }
        std::cout << "\n************************************************************" << std::endl;
        std::cout << " [WARNING] LIVE PACKET CAPTURE (PCAP) IS INACTIVE" << std::endl;
        std::cout << " Status:  Degraded to kernel ARP and /proc/net/dev scraping" << std::endl;
        std::cout << " Cause:   " << err << std::endl;
        std::cout << " Action:  " << hint << std::endl;
        std::cout << "************************************************************\n" << std::endl;
    }
}

void NetMonShell::printHelp() const {
    std::cout << "Available Commands:" << std::endl;
    std::cout << "  status                       Show daemon & interface status" << std::endl;
    std::cout << "  devices [filter]             List devices (known/visitor/unregistered)" << std::endl;
    std::cout << "  unregistered                 List unregistered and visitor devices" << std::endl;
    std::cout << "  visitors                     Alias for unregistered" << std::endl;
    std::cout << "  toptalkers [limit] [mins]    Top bandwidth consumers (default: 10, 15m)" << std::endl;
    std::cout << "  traffic                      Show LAN throughput and protocol breakdown" << std::endl;
    std::cout << "  name <mac> <name> [category] Assign friendly name & category to device" << std::endl;
    std::cout << "  reload                       Reload configuration and devices registry" << std::endl;
    std::cout << "  auth login <password>        Authenticate local operator session" << std::endl;
    std::cout << "  auth set-password <old> <new>Change admin password" << std::endl;
    std::cout << "  auth list                    List active AI security clearances & timers" << std::endl;
    std::cout << "  audit [limit]                Show AI security clearance audit history" << std::endl;
    std::cout << "  grant conn-nnnn [sec] [tier] Approve AI clearance (requires UNIX password)" << std::endl;
    std::cout << "  deny conn-nnnn [seconds]     Deny pending AI clearance request" << std::endl;
    std::cout << "  firewall status              Show firewall model, firmware, and connection" << std::endl;
    std::cout << "  firewall ping <host> [count] Run firewall-side ICMP diagnostic ping" << std::endl;
    std::cout << "  firewall traceroute <host>   Run firewall-side route trace" << std::endl;
    std::cout << "  firewall set-password        Store firewall password in encrypted vault" << std::endl;
    std::cout << "  firewall clear-password      Clear firewall password from encrypted vault" << std::endl;
    std::cout << "  firewall-clear-password      Alias for firewall clear-password" << std::endl;
    std::cout << "  firewall diag [r|w|all] [flt]Run live qualification & benchmark suite" << std::endl;
    std::cout << "  help                         Show this help message" << std::endl;
    std::cout << "  quit / exit                  Exit the shell" << std::endl;
}

void NetMonShell::cmdStatus() {
    auto summary = LanSniffer::getInstance().getTrafficSummary();
    bool connected = AimonGatewayClient::getInstance().isConnected();
    bool pcapActive = summary.value("pcap_active", false);

    std::cout << "--- NetMon Daemon Status ---" << std::endl;
    std::cout << "Interface:        " << Config::getInstance().getInterface() << std::endl;
    if (pcapActive) {
        std::cout << "Capture Engine:   Live libpcap streaming (Zero-DB mode)" << std::endl;
    } else {
        std::cout << "Capture Engine:   [WARNING] Proc/ARP kernel telemetry (INACTIVE)" << std::endl;
        std::string warn = summary.value("pcap_warning", "");
        std::string remed = summary.value("pcap_remediation", "");
        if (!warn.empty()) {
            std::cout << "PCAP Error:       " << warn << std::endl;
        }
        if (!remed.empty()) {
            std::cout << "Remediation:      " << remed << std::endl;
        }
    }
    std::cout << "Gateway Client:   " << (connected ? ("CONNECTED (" + Config::getInstance().getGatewayHost() + ":" + std::to_string(Config::getInstance().getGatewayPort()) + ")") : "DISCONNECTED (reconnecting...)") << std::endl;
    std::cout << "Registered Devs:  " << DeviceRegistry::getInstance().getDeviceCount() << std::endl;
    std::cout << "Uptime:           " << summary.value("uptime_seconds", 0) << " seconds" << std::endl;
    std::cout << "Devices Config:   " << DeviceRegistry::getInstance().getFilePath() << std::endl;
}

void NetMonShell::cmdDevices(const std::string &catFilter) {
    auto jsonRes = LanSniffer::getInstance().getDevicesJson();
    auto &devs = jsonRes["devices"];

    std::cout << std::left
              << std::setw(18) << "MAC Address"
              << std::setw(16) << "IP Address"
              << std::setw(16) << "Device Name"
              << std::setw(12) << "Category"
              << "Last Seen" << std::endl;
    std::cout << std::string(72, '-') << std::endl;

    size_t printed = 0;
    for (const auto &d : devs) {
        std::string cat = d.value("category", "unregistered");
        if (!catFilter.empty() && catFilter != "all" && cat != catFilter) {
            continue;
        }

        time_t ls = d.value("last_seen", 0);
        std::string name = d.value("name", "");
        if (name.length() > 15) name = name.substr(0, 12) + "...";
        if (cat.length() > 11) cat = cat.substr(0, 9) + "...";

        std::cout << std::left
                  << std::setw(18) << d.value("mac", "")
                  << std::setw(16) << d.value("ip", "")
                  << std::setw(16) << name
                  << std::setw(12) << cat
                  << formatRelativeTime(ls) << std::endl;
        printed++;
    }

    std::cout << std::string(72, '-') << std::endl;
    std::cout << "Total displayed: " << printed << " (Total registered: " << jsonRes.value("total_devices", 0) << ")" << std::endl;
}

void NetMonShell::cmdUnregistered() {
    auto jsonRes = LanSniffer::getInstance().getUnregisteredDevicesJson();

    std::cout << "=== Visitor / Guest Mobile Devices (" << jsonRes.value("visitor_count", 0) << ") ===" << std::endl;
    std::cout << std::left
              << std::setw(18) << "MAC Address"
              << std::setw(16) << "IP Address"
              << std::setw(18) << "Device Name"
              << std::setw(10) << "Last Seen"
              << "Vendor" << std::endl;
    std::cout << std::string(74, '-') << std::endl;

    for (const auto &d : jsonRes["visitor_devices"]) {
        time_t ls = d.value("last_seen", 0);
        std::string name = d.value("name", "");
        if (name.length() > 17) name = name.substr(0, 14) + "...";
        std::string vendor = d.value("vendor", "");
        if (vendor.length() > 11) vendor = vendor.substr(0, 8) + "...";

        std::cout << std::left
                  << std::setw(18) << d.value("mac", "")
                  << std::setw(16) << d.value("ip", "")
                  << std::setw(18) << name
                  << std::setw(10) << formatRelativeTime(ls)
                  << vendor << std::endl;
    }

    std::cout << "\n=== Unregistered Devices (" << jsonRes.value("unregistered_count", 0) << ") ===" << std::endl;
    std::cout << std::left
              << std::setw(18) << "MAC Address"
              << std::setw(16) << "IP Address"
              << std::setw(18) << "Assigned Name"
              << std::setw(10) << "Last Seen"
              << "Vendor" << std::endl;
    std::cout << std::string(74, '-') << std::endl;

    for (const auto &d : jsonRes["unregistered_devices"]) {
        time_t ls = d.value("last_seen", 0);
        std::string name = d.value("name", "");
        if (name.length() > 17) name = name.substr(0, 14) + "...";
        std::string vendor = d.value("vendor", "");
        if (vendor.length() > 11) vendor = vendor.substr(0, 8) + "...";

        std::cout << std::left
                  << std::setw(18) << d.value("mac", "")
                  << std::setw(16) << d.value("ip", "")
                  << std::setw(18) << name
                  << std::setw(10) << formatRelativeTime(ls)
                  << vendor << std::endl;
    }
}

void NetMonShell::cmdTopTalkers(int limit, int windowMins) {
    auto jsonRes = LanSniffer::getInstance().getTopTalkers(limit, windowMins);
    auto &talkers = jsonRes["top_talkers"];

    std::cout << "--- Top Talkers (Window: " << windowMins << " minutes) ---" << std::endl;
    std::cout << std::left
              << std::setw(18) << "MAC Address"
              << std::setw(16) << "IP Address"
              << std::setw(16) << "Name"
              << std::setw(12) << "Bytes Tx"
              << "Bytes Rx" << std::endl;
    std::cout << std::string(72, '-') << std::endl;

    for (const auto &t : talkers) {
        std::string name = t.value("name", "");
        if (name.length() > 15) name = name.substr(0, 12) + "...";

        std::cout << std::left
                  << std::setw(18) << t.value("mac", "")
                  << std::setw(16) << t.value("ip", "")
                  << std::setw(16) << name
                  << std::setw(12) << t.value("bytes_tx", 0)
                  << t.value("bytes_rx", 0) << std::endl;
    }
}

void NetMonShell::cmdTraffic() {
    auto summary = LanSniffer::getInstance().getTrafficSummary();
    bool pcapActive = summary.value("pcap_active", false);

    std::cout << "--- LAN Traffic & Protocol Breakdown ---" << std::endl;
    if (!pcapActive) {
        std::cout << "[WARNING] PCAP packet capture is INACTIVE (permission denied)." << std::endl;
        std::cout << "          Protocol counts below require CAP_NET_RAW capabilities." << std::endl;
        std::cout << "          Run 'make setcap' to activate live packet analysis." << std::endl;
    }
    std::cout << "Total Transferred:   " << summary.value("total_bytes", 0) << " bytes ("
              << summary.value("total_packets", 0) << " packets)" << std::endl;
    std::cout << "Current Throughput:  " << std::fixed << std::setprecision(2)
              << summary.value("current_bytes_per_sec", 0.0) << " B/s ("
              << summary.value("current_packets_per_sec", 0.0) << " pps)" << std::endl;

    auto &p = summary["protocols"];
    std::cout << "Protocol Counts:" << std::endl;
    std::cout << "  HTTPS:     " << p.value("https", 0) << std::endl;
    std::cout << "  DNS:       " << p.value("dns", 0) << std::endl;
    std::cout << "  SSH:       " << p.value("ssh", 0) << std::endl;
    std::cout << "  HTTP:      " << p.value("http", 0) << std::endl;
    std::cout << "  ARP:       " << p.value("arp", 0) << std::endl;
    std::cout << "  Broadcast: " << p.value("broadcast", 0) << std::endl;
    std::cout << "  Multicast: " << p.value("multicast", 0) << std::endl;
    std::cout << "  ICMP:      " << p.value("icmp", 0) << std::endl;
}

void NetMonShell::cmdNameDevice(const std::string &mac, const std::string &name,
                                const std::string &category) {
    if (mac.empty() || name.empty()) {
        std::cout << "Usage: name <mac> <name> [category]" << std::endl;
        return;
    }

    bool ok = DeviceRegistry::getInstance().nameDevice(mac, name, category);
    if (ok) {
        std::cout << "Successfully named device " << mac << " -> '" << name
                  << "' (" << (category.empty() ? "known" : category) << ")" << std::endl;
    } else {
        std::cout << "Error: MAC address not found in registry. Has it been seen on the LAN?" << std::endl;
    }
}

void NetMonShell::cmdReload() {
    Config::getInstance().load();
    DeviceRegistry::getInstance().load();
    std::cout << "Reloaded configuration and devices registry." << std::endl;
}

int NetMonShell::executeCommand(const std::string &cmdLine) {
    std::istringstream iss(cmdLine);
    std::string cmd;
    if (!(iss >> cmd)) {
        return 0;
    }

    struct CommandExecutionGuard {
        std::atomic<bool> &_flag;
        CommandExecutionGuard(std::atomic<bool> &flag) : _flag(flag) {
            _flag.store(true);
        }
        ~CommandExecutionGuard() {
            _flag.store(false);
        }
    } guard(_executingCommand);

    if (cmd == "help" || cmd == "?") {
        printHelp();
    } else if (cmd == "status") {
        cmdStatus();
    } else if (cmd == "devices") {
        std::string filter;
        iss >> filter;
        cmdDevices(filter);
    } else if (cmd == "unregistered" || cmd == "visitors") {
        cmdUnregistered();
    } else if (cmd == "toptalkers") {
        int limit = 10, mins = 15;
        iss >> limit >> mins;
        cmdTopTalkers(limit, mins);
    } else if (cmd == "traffic") {
        cmdTraffic();
    } else if (cmd == "name") {
        std::string mac, name, cat;
        iss >> mac >> name >> cat;
        cmdNameDevice(mac, name, cat);
    } else if (cmd == "reload") {
        cmdReload();
    } else if (cmd == "auth") {
        std::string subCmd;
        iss >> subCmd;
        if (subCmd == "list") {
            cmdAuthList();
            return 0;
        } else if (subCmd == "login") {
            std::string pass;
            iss >> pass;
            cmdAuthLogin(pass);
        } else if (subCmd == "set-password") {
            std::string currPass, newPass;
            iss >> currPass >> newPass;
            if (newPass.empty() || !AuthManager::getInstance().setPassword(newPass, currPass, true)) {
                std::cout << "Failed to change password. Current password incorrect or new password empty." << std::endl;
                return 1;
            }
            std::cout << "Admin password changed successfully. Previous sessions revoked." << std::endl;
            _sessionToken = AuthManager::getInstance().login(newPass);
            return 0;
        } else {
            std::cout << "Usage: auth list | auth login <password> | auth set-password <current> <new>" << std::endl;
        }
    } else if (cmd == "firewall") {
        std::string subCmd;
        iss >> subCmd;
        if (subCmd == "set-password") {
            cmdFirewallSetPassword();
        } else if (subCmd == "clear-password") {
            cmdFirewallClearPassword();
        } else if (subCmd == "status") {
            cmdFirewallStatus();
        } else if (subCmd == "ping") {
            std::string host;
            iss >> host;
            int count = 4;
            if (!(iss >> count) || count <= 0) {
                count = 4;
            }
            if (host.empty()) {
                std::cout << "Usage: firewall ping <host> [count]" << std::endl;
            } else {
                cmdFirewallPing(host, count);
            }
        } else if (subCmd == "traceroute") {
            std::string host;
            iss >> host;
            if (host.empty()) {
                std::cout << "Usage: firewall traceroute <host>" << std::endl;
            } else {
                cmdFirewallTraceroute(host);
            }
        } else if (subCmd == "diag") {
            std::string mode = "read";
            std::string filter;
            std::string arg1;
            if (iss >> arg1) {
                if (arg1 == "read" || arg1 == "write" || arg1 == "all") {
                    mode = arg1;
                    iss >> filter;
                } else {
                    filter = arg1;
                }
            }
            cmdFirewallDiag(mode, filter);
        } else {
            std::cout << "Usage: firewall status | ping | traceroute | set-password | clear-password | diag" << std::endl;
        }
    } else if (cmd == "firewall-clear-password") {
        cmdFirewallClearPassword();
    } else if (cmd == "audit") {
        size_t limit = 20;
        if (iss >> limit) {
            if (limit == 0) limit = 20;
        } else {
            limit = 20;
        }
        auto logs = AiSecurityClearanceManager::getInstance().getAuditLog(limit);
        std::cout << "\n=== AI Security Clearance Audit Log (Recent " << logs.size() << ") ===" << std::endl;
        if (logs.empty()) {
            std::cout << "  (no audit entries recorded)" << std::endl;
        } else {
            for (const auto &e : logs) {
                char timeBuf[32];
                struct tm tmBuf;
                localtime_r(&e.timestamp, &tmBuf);
                strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &tmBuf);
                std::cout << "  [" << timeBuf << "] conn=" << AiSecurityClearanceManager::formatConnId(e.connectionId)
                          << " peer=" << e.peerAddress << ":" << e.peerPort
                          << " cmd=\"" << e.command << "\""
                          << " decision=" << e.decision
                          << " reason=\"" << e.reason << "\""
                          << " ssh_res=" << e.sshResult
                          << " (" << e.durationMs << "ms)" << std::endl;
            }
        }
        std::cout << std::endl;
        return 0;
    } else if (cmd == "approve" || cmd == "grant") {
        std::string targetStr;
        iss >> targetStr;
        uint64_t targetId = AiSecurityClearanceManager::parseConnId(targetStr);
        uint32_t seconds = 300;
        bool hasSeconds = false;
        bool hasExplicitTier = false;
        ClearanceTier tier = ClearanceTier::READ_WRITE;

        std::string optArg;
        while (iss >> optArg) {
            if (optArg == "read" || optArg == "READ") {
                tier = ClearanceTier::READ;
                hasExplicitTier = true;
            } else if (optArg == "write" || optArg == "WRITE" || optArg == "readwrite" || optArg == "read/write") {
                tier = ClearanceTier::READ_WRITE;
                hasExplicitTier = true;
            } else if (optArg == "password" || optArg == "elevated") {
                tier = ClearanceTier::READ_WRITE_PASSWORD;
                hasExplicitTier = true;
            } else {
                try {
                    seconds = std::stoul(optArg);
                    hasSeconds = true;
                } catch (...) {}
            }
        }

        if (targetId == 0) {
            std::cout << "Usage: grant conn-nnnn [seconds] [read|write]  (e.g., grant conn-0003 300)" << std::endl;
            return 1;
        }
        uint64_t pendingId = 0;
        std::string peerIp;
        uint16_t peerPort = 0;
        ClearanceTier requestedTier = ClearanceTier::READ_WRITE;
        if (!AiSecurityClearanceManager::getInstance().hasPendingApproval(pendingId, peerIp, peerPort, &requestedTier) || pendingId != targetId) {
            std::cout << "No pending clearance request for connection " << AiSecurityClearanceManager::formatConnId(targetId) << "." << std::endl;
            return 1;
        }

        if (!hasExplicitTier) {
            tier = requestedTier;
        }

        if (tier == ClearanceTier::READ && !hasSeconds) {
            seconds = 0; // Default indefinite for READ
        } else if (tier != ClearanceTier::READ && !hasSeconds) {
            seconds = 300; // Default 300s for READ/WRITE
        }

        std::string user = UnixAuth::getCurrentUsername();
        std::string pass = readPasswordInteractive("Password for " + user + ": ");
        if (pass.empty() || !UnixAuth::authenticate(pass)) {
            std::cout << "Authentication failed: invalid UNIX password. Clearance denied." << std::endl;
            AiSecurityClearanceManager::getInstance().consoleDeny(targetId);
            return 1;
        }

        if (AiSecurityClearanceManager::getInstance().consoleApprove(targetId, seconds, tier)) {
            std::cout << "Approved " << clearanceTierToString(tier) << " clearance for connection "
                      << AiSecurityClearanceManager::formatConnId(targetId)
                      << " (" << peerIp << ":" << peerPort << ") for "
                      << (seconds == 0 ? "indefinite (0s)" : (std::to_string(seconds) + "s (downgrades to READ upon expiry)"))
                      << "." << std::endl;
        } else {
            std::cout << "Approval failed (connection no longer pending or socket closed)." << std::endl;
        }
        return 0;
    } else if (cmd == "deny") {
        std::string targetStr;
        iss >> targetStr;
        uint64_t targetId = AiSecurityClearanceManager::parseConnId(targetStr);
        uint32_t seconds = 0;
        iss >> seconds;

        if (targetId == 0) {
            std::cout << "Usage: deny conn-nnnn [seconds]   (e.g., deny conn-0003)" << std::endl;
            return 1;
        }
        uint64_t pendingId = 0;
        std::string peerIp;
        uint16_t peerPort = 0;
        if (!AiSecurityClearanceManager::getInstance().hasPendingApproval(pendingId, peerIp, peerPort) || pendingId != targetId) {
            std::cout << "No pending clearance request for connection " << AiSecurityClearanceManager::formatConnId(targetId) << "." << std::endl;
            return 1;
        }
        if (AiSecurityClearanceManager::getInstance().consoleDeny(targetId, seconds)) {
            std::cout << "Denied Level 2 clearance for connection "
                      << AiSecurityClearanceManager::formatConnId(targetId)
                      << " (" << peerIp << ":" << peerPort << ")";
            if (seconds > 0) {
                std::cout << " with " << seconds << "s lockout.";
            } else {
                std::cout << ".";
            }
            std::cout << std::endl;
        } else {
            std::cout << "Denial failed (connection no longer pending)." << std::endl;
        }
        return 0;
    } else if (cmd == "quit" || cmd == "exit") {
        return -1;
    } else {
        std::cout << "Unknown command: '" << cmd << "'. Type 'help' for available commands." << std::endl;
    }

    return 0;
}

bool NetMonShell::isAuthenticated() const {
    if (_sessionToken.empty()) {
        return false;
    }
    return AuthManager::getInstance().validateSession(_sessionToken);
}

void NetMonShell::cmdAuthLogin(const std::string &password) {
    if (password.empty()) {
        std::cout << "Usage: auth login <password>" << std::endl;
        return;
    }
    std::string token = AuthManager::getInstance().login(password);
    if (token.empty()) {
        std::cout << "Authentication failed: invalid password." << std::endl;
    } else {
        _sessionToken = token;
        std::cout << "Authenticated successfully. Session active (15-min idle timeout)." << std::endl;
    }
}

void NetMonShell::cmdAuthSetPassword(const std::string &currentPass, const std::string &newPass) {
    if (newPass.empty()) {
        std::cout << "Usage: auth set-password <current_password> <new_password>" << std::endl;
        return;
    }
    if (AuthManager::getInstance().setPassword(newPass, currentPass, true)) {
        std::cout << "Admin password changed successfully. Previous sessions revoked." << std::endl;
        _sessionToken = AuthManager::getInstance().login(newPass);
    } else {
        std::cout << "Failed to change password. Current password incorrect or new password empty." << std::endl;
    }
}

void NetMonShell::cmdFirewallSetPassword() {
    std::string pass = readPasswordInteractive("Enter Zyxel firewall password: ");
    if (pass.empty()) {
        std::cout << "Firewall password cannot be empty." << std::endl;
        return;
    }
    std::string confirm = readPasswordInteractive("Confirm Zyxel firewall password: ");
    if (pass != confirm) {
        std::cout << "Passwords do not match." << std::endl;
        return;
    }
    if (AuthManager::getInstance().setRouterPassword(pass)) {
        std::cout << "Firewall password stored securely in encrypted vault (vault.enc)." << std::endl;
    } else {
        std::cout << "Error: Failed to store firewall password in vault." << std::endl;
    }
}

void NetMonShell::cmdFirewallClearPassword() {
    if (AuthManager::getInstance().clearRouterPassword()) {
        std::cout << "Firewall password cleared from vault." << std::endl;
    } else {
        std::cout << "Error: Failed to clear firewall password from vault." << std::endl;
    }
}

void NetMonShell::cmdFirewallStatus() {
    auto res = ZyxelDriver::getInstance().getStatus();
    std::cout << "--- Zyxel Firewall Status ---" << std::endl;
    std::cout << "Status:       " << res.value("status", "unknown") << std::endl;
    std::cout << "Driver:       " << res.value("driver", "") << std::endl;
    if (res.contains("model")) {
        std::cout << "Model:        " << res.value("model", "") << std::endl;
    }
    if (res.contains("firmware")) {
        std::cout << "Firmware:     " << res.value("firmware", "") << std::endl;
    }
    if (res.contains("build_date")) {
        std::cout << "Build Date:   " << res.value("build_date", "") << std::endl;
    }
    if (res.contains("serial_number") && !res["serial_number"].get<std::string>().empty()) {
        std::cout << "Serial:       " << res.value("serial_number", "") << std::endl;
    }
    if (res.contains("transport")) {
        std::cout << "Transport:    " << res.value("transport", "") << std::endl;
    }
    if (res.contains("error")) {
        std::cout << "Error:        " << res.value("error", "") << std::endl;
    }
    if (res.contains("note")) {
        std::cout << "Note:         " << res.value("note", "") << std::endl;
    }
}

void NetMonShell::cmdFirewallPing(const std::string &target, int count) {
    std::cout << "Pinging " << target << " (" << count << " packets) via Zyxel firewall (Ctrl+C to cancel)..." << std::endl;
    auto res = ZyxelDriver::getInstance().ping(target, count);
    if (res.value("status", "") == "canceled") {
        std::cout << "\n^C\nPing canceled by operator." << std::endl;
        return;
    }
    if (res.value("status", "") != "ok") {
        std::cout << "Error: " << res.value("error", "Unknown ping error") << std::endl;
        return;
    }

    std::cout << "--- Zyxel Firewall Ping Probe: " << target << " ---" << std::endl;
    std::cout << "Packets:      " << res.value("packets_transmitted", 0) << " transmitted, "
              << res.value("packets_received", 0) << " received, "
              << std::fixed << std::setprecision(1) << res.value("packet_loss_percent", 0.0) << "% loss" << std::endl;
    if (res.value("packets_received", 0) > 0) {
        std::cout << "Round-Trip:   min = " << std::fixed << std::setprecision(2) << res.value("min_latency_ms", 0.0)
                  << " ms, avg = " << res.value("avg_latency_ms", 0.0)
                  << " ms, max = " << res.value("max_latency_ms", 0.0) << " ms" << std::endl;
    }
}

void NetMonShell::cmdFirewallTraceroute(const std::string &target) {
    std::cout << "Traceroute to " << target << " via Zyxel firewall (up to 60s, Ctrl+C to cancel)..." << std::endl;
    auto res = ZyxelDriver::getInstance().traceroute(target);
    if (res.value("status", "") == "canceled") {
        std::cout << "\n^C\nTraceroute canceled by operator." << std::endl;
        return;
    }
    if (res.value("status", "") != "ok") {
        std::cout << "Error: " << res.value("error", "Unknown traceroute error") << std::endl;
        if (res.contains("raw_output") && !res["raw_output"].get<std::string>().empty()) {
            std::cout << res["raw_output"].get<std::string>() << std::endl;
        }
        return;
    }

    std::cout << "--- Zyxel Firewall Traceroute: " << target << " ---" << std::endl;
    if (res.contains("raw_output") && !res["raw_output"].get<std::string>().empty()) {
        std::cout << res["raw_output"].get<std::string>() << std::endl;
    } else {
        std::cout << "Hops received: " << res.value("packets_received", 0) << std::endl;
    }
}

void NetMonShell::cmdFirewallDiag(const std::string &mode, const std::string &filter) {
    runLiveFirewallDiagnostic(std::cout, mode, filter);
}

void NetMonShell::cmdAuthList() {
    auto grants = AiSecurityClearanceManager::getInstance().getActiveClearances();
    std::cout << "\n=== Active AI Security Clearances ===" << std::endl;
    if (grants.empty()) {
        std::cout << "  (no active AI security clearances)" << std::endl;
    } else {
        std::cout << "  "
                  << std::left
                  << std::setw(12) << "Connection"
                  << std::setw(22) << "Peer Address"
                  << std::setw(12) << "State"
                  << std::setw(12) << "Expires In"
                  << "Metadata" << std::endl;
        std::cout << "  " << std::string(70, '-') << std::endl;
        for (const auto &g : grants) {
            std::string stateStr;
            switch (g.clearanceState) {
            case ClientClearanceState::LEVEL2: stateStr = "READ/WRITE"; break;
            case ClientClearanceState::LEVEL3: stateStr = "READ"; break;
            case ClientClearanceState::PENDING: stateStr = "PENDING"; break;
            case ClientClearanceState::DENIED: stateStr = "DENIED"; break;
            case ClientClearanceState::NONE: stateStr = "NONE"; break;
            default: stateStr = "UNKNOWN"; break;
            }
            std::string expStr;
            if (g.clearanceState == ClientClearanceState::LEVEL2 || g.clearanceState == ClientClearanceState::DENIED) {
                expStr = std::to_string(g.remainingSeconds) + "s";
            } else if (g.clearanceState == ClientClearanceState::LEVEL3) {
                expStr = (g.remainingSeconds > 0) ? (std::to_string(g.remainingSeconds) + "s") : "indefinite";
            } else {
                expStr = "-";
            }
            std::string peer = g.peerAddress + ":" + std::to_string(g.peerPort);
            if (peer.length() > 21) peer = peer.substr(0, 18) + "...";
            std::string meta = g.metadata.empty() ? "-" : g.metadata;
            if (meta.length() > 12) meta = meta.substr(0, 9) + "...";

            std::cout << "  "
                      << std::left
                      << std::setw(12) << g.formattedConnId
                      << std::setw(22) << peer
                      << std::setw(12) << stateStr
                      << std::setw(12) << expStr
                      << meta << std::endl;
        }
    }
    std::cout << std::endl;
}

void NetMonShell::runInteractive() {
    printBanner();
    _running.store(true);

    while (_running.load()) {
        std::cout << "netmon> " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (executeCommand(line) < 0) {
            break;
        }
    }

    _running.store(false);
}

void NetMonShell::stop() {
    _running.store(false);
}

void NetMonShell::resetForTesting() {
    _running.store(false);
    _executingCommand.store(false);
    std::string().swap(_sessionToken);
    std::lock_guard<std::mutex> lock(g_pwMutex);
    g_pwReaderForTesting = nullptr;
}

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
