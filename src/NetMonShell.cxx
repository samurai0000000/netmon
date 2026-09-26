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
#include "SecurityCheckpoint.hxx"
#include "SnmpDatabase.hxx"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <ctime>
#include <unistd.h>

NetMonShell &NetMonShell::getInstance() {
    static NetMonShell instance;
    return instance;
}

NetMonShell::NetMonShell()
    : _running(false) {
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
    std::cout << "  status                       Show daemon, interface, and gateway status" << std::endl;
    std::cout << "  devices [filter]             List devices (optional: known, visitor, unregistered)" << std::endl;
    std::cout << "  unregistered                 List unregistered and visitor mobile devices" << std::endl;
    std::cout << "  visitors                     Alias for unregistered" << std::endl;
    std::cout << "  toptalkers [limit] [mins]    Show top bandwidth consumers (default: 10 hosts, 15 mins)" << std::endl;
    std::cout << "  traffic                      Show LAN throughput and protocol breakdown" << std::endl;
    std::cout << "  name <mac> <name> [category] Assign friendly name and category to a device" << std::endl;
    std::cout << "  reload                       Reload configuration and devices registry" << std::endl;
    std::cout << "  auth login <password>        Authenticate local operator session" << std::endl;
    std::cout << "  auth set-password <old> <new>Change admin password (requires old password)" << std::endl;
    std::cout << "  policy [mode]                View or update policy (disabled, dry_run, require_approval, live)" << std::endl;
    std::cout << "  pending                      List actions awaiting operator approval" << std::endl;
    std::cout << "  approve <id>                 Approve and execute a pending action" << std::endl;
    std::cout << "  deny <id> [reason]           Deny a pending action" << std::endl;
    std::cout << "  reconcile <id> <appl|retry>  Reconcile an interrupted action" << std::endl;
    std::cout << "  audit [limit]                View recent audit outbox decisions" << std::endl;
    std::cout << "  syslog [limit]               View recent router syslog events" << std::endl;
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
              << std::setw(20) << "Device Name"
              << std::setw(16) << "Category"
              << std::setw(12) << "Last Seen"
              << "Vendor / OUI" << std::endl;
    std::cout << std::string(105, '-') << std::endl;

    size_t printed = 0;
    for (const auto &d : devs) {
        std::string cat = d.value("category", "unregistered");
        if (!catFilter.empty() && catFilter != "all" && cat != catFilter) {
            continue;
        }

        time_t ls = d.value("last_seen", 0);
        std::cout << std::left
                  << std::setw(18) << d.value("mac", "")
                  << std::setw(16) << d.value("ip", "")
                  << std::setw(20) << d.value("name", "")
                  << std::setw(16) << cat
                  << std::setw(12) << formatRelativeTime(ls)
                  << d.value("vendor", "") << std::endl;
        printed++;
    }

    std::cout << std::string(105, '-') << std::endl;
    std::cout << "Total displayed: " << printed << " (Total registered: " << jsonRes.value("total_devices", 0) << ")" << std::endl;
}

void NetMonShell::cmdUnregistered() {
    auto jsonRes = LanSniffer::getInstance().getUnregisteredDevicesJson();

    std::cout << "=== Visitor / Guest Mobile Devices (" << jsonRes.value("visitor_count", 0) << ") ===" << std::endl;
    std::cout << std::left
              << std::setw(18) << "MAC Address"
              << std::setw(16) << "IP Address"
              << std::setw(22) << "Device Name"
              << std::setw(12) << "Last Seen"
              << "Vendor" << std::endl;
    std::cout << std::string(90, '-') << std::endl;

    for (const auto &d : jsonRes["visitor_devices"]) {
        time_t ls = d.value("last_seen", 0);
        std::cout << std::left
                  << std::setw(18) << d.value("mac", "")
                  << std::setw(16) << d.value("ip", "")
                  << std::setw(22) << d.value("name", "")
                  << std::setw(12) << formatRelativeTime(ls)
                  << d.value("vendor", "") << std::endl;
    }

    std::cout << "\n=== Unregistered Devices (" << jsonRes.value("unregistered_count", 0) << ") ===" << std::endl;
    std::cout << std::left
              << std::setw(18) << "MAC Address"
              << std::setw(16) << "IP Address"
              << std::setw(22) << "Assigned Name"
              << std::setw(12) << "Last Seen"
              << "Vendor" << std::endl;
    std::cout << std::string(90, '-') << std::endl;

    for (const auto &d : jsonRes["unregistered_devices"]) {
        time_t ls = d.value("last_seen", 0);
        std::cout << std::left
                  << std::setw(18) << d.value("mac", "")
                  << std::setw(16) << d.value("ip", "")
                  << std::setw(22) << d.value("name", "")
                  << std::setw(12) << formatRelativeTime(ls)
                  << d.value("vendor", "") << std::endl;
    }
}

void NetMonShell::cmdTopTalkers(int limit, int windowMins) {
    auto jsonRes = LanSniffer::getInstance().getTopTalkers(limit, windowMins);
    auto &talkers = jsonRes["top_talkers"];

    std::cout << "--- Top Talkers (Window: " << windowMins << " minutes) ---" << std::endl;
    std::cout << std::left
              << std::setw(18) << "MAC Address"
              << std::setw(16) << "IP Address"
              << std::setw(20) << "Name"
              << std::setw(14) << "Window Bytes"
              << std::setw(14) << "Bytes Tx"
              << std::setw(14) << "Bytes Rx"
              << "Vendor" << std::endl;
    std::cout << std::string(105, '-') << std::endl;

    for (const auto &t : talkers) {
        std::cout << std::left
                  << std::setw(18) << t.value("mac", "")
                  << std::setw(16) << t.value("ip", "")
                  << std::setw(20) << t.value("name", "")
                  << std::setw(14) << t.value("window_bytes", 0)
                  << std::setw(14) << t.value("bytes_tx", 0)
                  << std::setw(14) << t.value("bytes_rx", 0)
                  << t.value("vendor", "") << std::endl;
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
        if (subCmd == "login") {
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
            std::cout << "Usage: auth login <password> | auth set-password <current> <new>" << std::endl;
        }
    } else if (cmd == "policy") {
        std::string mode;
        iss >> mode;
        cmdPolicy(mode);
    } else if (cmd == "pending") {
        cmdPending();
    } else if (cmd == "approve") {
        int64_t id = 0;
        iss >> id;
        cmdApprove(id);
    } else if (cmd == "deny") {
        int64_t id = 0;
        std::string reason;
        iss >> id;
        std::getline(iss, reason);
        size_t first = reason.find_first_not_of(" \t");
        if (first != std::string::npos) {
            reason = reason.substr(first);
        }
        cmdDeny(id, reason);
    } else if (cmd == "reconcile") {
        int64_t id = 0;
        std::string action;
        iss >> id >> action;
        cmdReconcile(id, action);
    } else if (cmd == "audit") {
        size_t lim = 50;
        iss >> lim;
        cmdAudit(lim > 0 ? lim : 50);
    } else if (cmd == "syslog") {
        size_t lim = 50;
        iss >> lim;
        cmdSyslog(lim > 0 ? lim : 50);
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

void NetMonShell::cmdPolicy(const std::string &mode) {
    if (mode.empty()) {
        PolicyMode current = SecurityCheckpoint::getInstance().getPolicyMode();
        std::cout << "Current security policy mode: "
                  << SecurityCheckpoint::policyModeToString(current) << std::endl;
        return;
    }
    if (!isAuthenticated()) {
        std::cout << "Authentication required. Run 'auth login <password>' first." << std::endl;
        return;
    }
    PolicyMode m = SecurityCheckpoint::stringToPolicyMode(mode);
    SecurityCheckpoint::getInstance().setPolicyMode(m);
    std::cout << "Security policy mode updated to: "
              << SecurityCheckpoint::policyModeToString(m) << std::endl;
}

void NetMonShell::cmdPending() {
    if (!isAuthenticated()) {
        std::cout << "Authentication required. Run 'auth login <password>' first." << std::endl;
        return;
    }
    auto tickets = SecurityCheckpoint::getInstance().getPendingTickets("pending");
    if (tickets.empty()) {
        std::cout << "No pending action tickets in queue." << std::endl;
        return;
    }
    std::cout << "----------------------------------------------------------------------------------" << std::endl;
    std::cout << std::left << std::setw(6) << "ID"
              << std::setw(12) << "CREATED"
              << std::setw(12) << "REQUESTER"
              << std::setw(22) << "TOOL"
              << "PAYLOAD" << std::endl;
    std::cout << "----------------------------------------------------------------------------------" << std::endl;
    for (const auto &t : tickets) {
        std::cout << std::left << std::setw(6) << t.id
                  << std::setw(12) << formatRelativeTime(t.createdAt)
                  << std::setw(12) << t.requester
                  << std::setw(22) << t.tool
                  << t.payload << std::endl;
    }
    std::cout << "----------------------------------------------------------------------------------" << std::endl;
}

void NetMonShell::cmdApprove(int64_t id) {
    if (!isAuthenticated()) {
        std::cout << "Authentication required. Run 'auth login <password>' first." << std::endl;
        return;
    }
    if (id <= 0) {
        std::cout << "Usage: approve <ticket_id>" << std::endl;
        return;
    }
    std::string outErr;
    if (SecurityCheckpoint::getInstance().approve(id, outErr)) {
        std::cout << "Ticket #" << id << " successfully approved and executed on router." << std::endl;
    } else {
        std::cout << "Approval failed for ticket #" << id << ": " << outErr << std::endl;
    }
}

void NetMonShell::cmdDeny(int64_t id, const std::string &reason) {
    if (!isAuthenticated()) {
        std::cout << "Authentication required. Run 'auth login <password>' first." << std::endl;
        return;
    }
    if (id <= 0) {
        std::cout << "Usage: deny <ticket_id> [reason]" << std::endl;
        return;
    }
    std::string r = reason.empty() ? "Rejected by operator" : reason;
    if (SecurityCheckpoint::getInstance().deny(id, r)) {
        std::cout << "Ticket #" << id << " denied. (" << r << ")" << std::endl;
    } else {
        std::cout << "Failed to deny ticket #" << id << " (not in pending state or does not exist)." << std::endl;
    }
}

void NetMonShell::cmdReconcile(int64_t id, const std::string &action) {
    if (!isAuthenticated()) {
        std::cout << "Authentication required. Run 'auth login <password>' first." << std::endl;
        return;
    }
    if (id <= 0 || (action != "applied" && action != "retry")) {
        std::cout << "Usage: reconcile <ticket_id> <applied|retry>" << std::endl;
        return;
    }
    std::string outErr;
    if (SecurityCheckpoint::getInstance().reconcile(id, action, outErr)) {
        std::cout << "Ticket #" << id << " reconciled as: " << action << std::endl;
    } else {
        std::cout << "Reconciliation failed: " << outErr << std::endl;
    }
}

void NetMonShell::cmdAudit(size_t limit) {
    if (!isAuthenticated()) {
        std::cout << "Authentication required. Run 'auth login <password>' first." << std::endl;
        return;
    }
    auto records = SnmpDatabase::getInstance().getAllAuditOutbox();
    if (records.empty()) {
        std::cout << "No audit records found in SQLite authority." << std::endl;
        return;
    }
    size_t count = (limit < records.size()) ? limit : records.size();
    size_t start = records.size() - count;
    std::cout << "----------------------------------------------------------------------------------" << std::endl;
    std::cout << std::left << std::setw(6) << "SEQ"
              << std::setw(12) << "TIME"
              << std::setw(10) << "DECISION"
              << std::setw(20) << "TOOL"
              << "REASON" << std::endl;
    std::cout << "----------------------------------------------------------------------------------" << std::endl;
    for (size_t i = start; i < records.size(); ++i) {
        const auto &r = records[i];
        std::cout << std::left << std::setw(6) << r.sequence
                  << std::setw(12) << formatRelativeTime(r.timestamp)
                  << std::setw(10) << r.decision
                  << std::setw(20) << r.tool
                  << r.reason << std::endl;
    }
    std::cout << "----------------------------------------------------------------------------------" << std::endl;
}

void NetMonShell::cmdSyslog(size_t limit) {
    if (!isAuthenticated()) {
        std::cout << "Authentication required. Run 'auth login <password>' first." << std::endl;
        return;
    }
    auto events = SnmpDatabase::getInstance().getSyslogEvents(limit);
    if (events.empty()) {
        std::cout << "No syslog events logged." << std::endl;
        return;
    }
    std::cout << "----------------------------------------------------------------------------------" << std::endl;
    std::cout << std::left << std::setw(6) << "ID"
              << std::setw(12) << "TIME"
              << std::setw(16) << "SOURCE"
              << std::setw(6) << "PRI"
              << std::setw(16) << "TAG"
              << std::setw(8) << "TIME_ADJ"
              << "MESSAGE" << std::endl;
    std::cout << "----------------------------------------------------------------------------------" << std::endl;
    for (const auto &e : events) {
        std::cout << std::left << std::setw(6) << e.id
                  << std::setw(12) << formatRelativeTime(e.timestamp)
                  << std::setw(16) << e.sourceIp
                  << std::setw(6) << (e.facility * 8 + e.severity)
                  << std::setw(16) << e.tag
                  << std::setw(8) << (e.timeAdjacent ? "YES" : "NO")
                  << e.message << std::endl;
    }
    std::cout << "----------------------------------------------------------------------------------" << std::endl;
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
    std::string().swap(_sessionToken);
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
