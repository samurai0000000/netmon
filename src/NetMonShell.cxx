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
    std::cout << "  help                         Show this help message" << std::endl;
    std::cout << "  quit / exit                  Exit the shell" << std::endl;
}

void NetMonShell::cmdStatus() {
    auto summary = LanSniffer::getInstance().getTrafficSummary();
    bool connected = AimonGatewayClient::getInstance().isConnected();

    std::cout << "--- NetMon Daemon Status ---" << std::endl;
    std::cout << "Interface:        " << Config::getInstance().getInterface() << std::endl;
    std::cout << "Capture Engine:   " << (summary.value("pcap_active", false) ? "Live libpcap streaming (Zero-DB mode)" : "Proc/ARP kernel telemetry (restricted permissions)") << std::endl;
    std::cout << "Gateway Client:   " << (connected ? "CONNECTED (builder:3885)" : "DISCONNECTED (reconnecting...)") << std::endl;
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

    std::cout << "--- LAN Traffic & Protocol Breakdown ---" << std::endl;
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
    } else if (cmd == "quit" || cmd == "exit") {
        return -1;
    } else {
        std::cout << "Unknown command: '" << cmd << "'. Type 'help' for available commands." << std::endl;
    }

    return 0;
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

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
