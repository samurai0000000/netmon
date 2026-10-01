/*
 * Main.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <iostream>
#include <csignal>
#include <cstdlib>
#include <memory>
#include <unistd.h>
#include "Version.hxx"
#include "Config.hxx"
#include "DeviceRegistry.hxx"
#include "LanSniffer.hxx"
#include "ZyxelDriver.hxx"
#include "SnmpDatabase.hxx"
#include "SnmpAggregator.hxx"
#include "WebServer.hxx"
#include "AimonGatewayClient.hxx"
#include "MacVendorResolver.hxx"
#include "DnsResolver.hxx"
#include "NetMonShell.hxx"
#include "AuthManager.hxx"
#include "SyslogServer.hxx"
#include "InstanceLock.hxx"
#include <libssh2.h>

static volatile sig_atomic_t g_shutdownRequested = 0;

static void signalHandler(int sig) {
    if (sig == SIGINT && NetMonShell::getInstance().isExecutingCommand()) {
        NetMonShell::getInstance().cancelCurrentCommand();
        return;
    }
    g_shutdownRequested = 1;
    WebServer::getInstance().stop();
    SnmpAggregator::getInstance().stop();
    NetMonShell::getInstance().stop();
    LanSniffer::getInstance().stop();
    AimonGatewayClient::getInstance().stop();
    DnsResolver::getInstance().stop();
    MacVendorResolver::getInstance().stop();
}

static void printUsage(const char *progName) {
    std::cout << "netmon: Network Monitor & AI Telemetry Gateway (v"
              << NETMON_VERSION_STRING << ")\n"
              << "Built: " << NETMON_WHOAMI << "@" << NETMON_HOSTNAME
              << " " << NETMON_DATE << "\n\n"
              << "Usage:\n"
              << "  " << progName << " [command] [options]\n\n"
              << "Commands:\n"
              << "  daemon         Start NetMon daemon with interactive screen CLI shell (default)\n"
              << "  run            Start NetMon daemon in non-interactive background mode\n"
              << "  status         Print current device registry and interface status\n"
              << "  set-password   Set or replace admin password (requires controlling terminal)\n"
              << "  router-set-password Set or replace Zyxel router password in encrypted vault\n"
              << "  version        Show NetMon version and build metadata\n"
              << "  help           Show this help message\n\n"
              << "Options:\n"
              << "  -c <path>      Path to custom netmon.cfg configuration file (default: ~/.config/netmon/netmon.cfg)\n"
              << "  -i <iface>     Override sniffing network interface (e.g. br0)\n"
              << "  -g <host>      Override aimon gateway host (default: 127.0.0.1)\n"
              << "  -p <port>      Override aimon gateway port (default: 3885)\n"
              << "  -w <port>      Override web server port (default: 3884)\n"
              << "  --db <path>    Override SQLite database file path\n"
              << "  --version, -v  Display version and build metadata\n"
              << "  --help, -h     Display this help message\n";
}

int main(int argc, char **argv) {
    // Process-global libssh2 initialization
    libssh2_init(0);

    // Crucial: ignore SIGPIPE to prevent crashes during terminal disconnection or screen detach
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    std::string customConfigPath;
    std::string overrideInterface;
    std::string overrideGateway;
    std::string overrideDb;
    int overridePort = 0;
    int overrideWebPort = 0;
    std::string mode = "daemon";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help" || arg == "help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version" || arg == "version") {
            std::cout << "netmon version " << NETMON_VERSION_STRING << "\n"
                      << "Built: " << NETMON_WHOAMI << "@" << NETMON_HOSTNAME
                      << " " << NETMON_DATE << "\n";
            return 0;
        } else if (arg == "status") {
            mode = "status";
        } else if (arg == "set-password") {
            mode = "set-password";
        } else if (arg == "router-set-password") {
            mode = "router-set-password";
        } else if (arg == "router" && i + 1 < argc && std::string(argv[i + 1]) == "set-password") {
            mode = "router-set-password";
            ++i;
        } else if (arg == "run") {
            mode = "run";
        } else if (arg == "daemon") {
            mode = "daemon";
        } else if (arg == "-c" && i + 1 < argc) {
            customConfigPath = argv[++i];
        } else if (arg == "-i" && i + 1 < argc) {
            overrideInterface = argv[++i];
        } else if (arg == "-g" && i + 1 < argc) {
            overrideGateway = argv[++i];
        } else if (arg == "-p" && i + 1 < argc) {
            overridePort = std::atoi(argv[++i]);
        } else if (arg == "-w" && i + 1 < argc) {
            overrideWebPort = std::atoi(argv[++i]);
        } else if (arg == "--db" && i + 1 < argc) {
            overrideDb = argv[++i];
        }
    }

    // Load configurations
    if (!Config::getInstance().load(customConfigPath)) {
        std::cerr << "Warning: Could not load config file, using built-in defaults" << std::endl;
    }

    if (!overrideInterface.empty()) {
        Config::getInstance().setInterface(overrideInterface);
    }
    if (!overrideGateway.empty()) {
        Config::getInstance().setGatewayHost(overrideGateway);
    }
    if (overridePort > 0) {
        Config::getInstance().setGatewayPort(overridePort);
    }
    if (overrideWebPort > 0) {
        Config::getInstance().setWebPort(overrideWebPort);
    }
    if (!overrideDb.empty()) {
        Config::getInstance().setDatabaseFile(overrideDb);
    }

    if (mode == "set-password") {
        if (!isatty(STDIN_FILENO)) {
            std::cerr << "Error: set-password requires a controlling terminal" << std::endl;
            return 1;
        }

        char *p1 = getpass("Enter new admin password: ");
        if (!p1 || std::strlen(p1) == 0) {
            std::cerr << "Error: Password cannot be empty" << std::endl;
            return 1;
        }
        std::string pass1 = p1;

        char *p2 = getpass("Confirm new admin password: ");
        if (!p2 || pass1 != p2) {
            std::cerr << "Error: Passwords do not match" << std::endl;
            return 1;
        }

        if (!AuthManager::getInstance().setPassword(pass1, "", false)) {
            std::cerr << "Error: Failed to set admin password" << std::endl;
            return 1;
        }

        std::cout << "Admin password successfully updated." << std::endl;
        return 0;
    }

    if (mode == "router-set-password") {
        if (!isatty(STDIN_FILENO)) {
            std::cerr << "Error: router-set-password requires a controlling terminal" << std::endl;
            return 1;
        }

        char *p1 = getpass("Enter new Zyxel router password: ");
        if (!p1 || std::strlen(p1) == 0) {
            std::cerr << "Error: Router password cannot be empty" << std::endl;
            return 1;
        }
        std::string pass1 = p1;

        char *p2 = getpass("Confirm new Zyxel router password: ");
        if (!p2 || pass1 != p2) {
            std::cerr << "Error: Passwords do not match" << std::endl;
            return 1;
        }

        if (!AuthManager::getInstance().setRouterPassword(pass1)) {
            std::cerr << "Error: Failed to store router password in vault" << std::endl;
            return 1;
        }

        std::cout << "Router password successfully stored in encrypted vault." << std::endl;
        return 0;
    }

    // Load persistent devices registry
    DeviceRegistry::getInstance().load();

    std::cout << "NetMon v" << NETMON_VERSION_STRING << " initializing..." << std::endl;
    std::cout << "  Interface:      " << Config::getInstance().getInterface() << std::endl;
    std::cout << "  Gateway:        " << Config::getInstance().getGatewayHost()
              << ":" << Config::getInstance().getGatewayPort() << std::endl;
    std::cout << "  Web Dashboard:  port " << Config::getInstance().getWebPort()
              << " (" << (Config::getInstance().isWebEnabled() ? "enabled" : "disabled") << ")" << std::endl;
    std::cout << "  Database:       " << Config::getInstance().getDatabaseFile() << std::endl;
    std::cout << "  Devices File:   " << DeviceRegistry::getInstance().getFilePath() << std::endl;
    std::cout << "  Known Devices:  " << DeviceRegistry::getInstance().getDeviceCount() << std::endl;

    if (mode == "status") {
        std::string pcapReason, pcapRemediation;
        bool pcapOk = LanSniffer::getInstance().checkPcapPermissions(pcapReason, pcapRemediation);
        std::cout << "\n--- PCAP Permission Preflight ---" << std::endl;
        if (pcapOk) {
            std::cout << "  Raw Capture Socket:  PERMITTED (CAP_NET_RAW active)" << std::endl;
        } else {
            std::cout << "  Raw Capture Socket:  [WARNING] DENIED" << std::endl;
            std::cout << "  Reason:              " << pcapReason << std::endl;
            std::cout << "  Remediation:         " << pcapRemediation << std::endl;
        }
        std::cout << std::endl;

        NetMonShell::getInstance().executeCommand("status");
        DnsResolver::getInstance().waitUntilDone(1000);
        MacVendorResolver::getInstance().waitUntilDone(500);
        NetMonShell::getInstance().executeCommand("devices");
        return 0;
    }

    // Enforce single instance per host
    InstanceLock instanceLock;
    if (!instanceLock.acquire()) {
        std::cerr << "[netmon] Error: Another instance of netmon is already running on host '"
                  << instanceLock.getHostname() << "' (PID "
                  << static_cast<int>(instanceLock.getExistingPid()) << ")\n"
                  << "[netmon] Lock file: " << instanceLock.getLockFilePath() << std::endl;
        return 1;
    }

    // Initialize SQLite time-series telemetry database
    SnmpDatabase::getInstance().open();

    // Start streaming sniffer and telemetry
    LanSniffer::getInstance().start();

    // Start background SNMP poller and rate calculator
    SnmpAggregator::getInstance().start();

    // Start embedded Web Server and live dashboard
    WebServer::getInstance().start();

    // Start UDP syslog receiver for router logs
    SyslogServer::getInstance().start();

    // Initialize router driver singleton
    if (!Config::getInstance().getSnmpTargets().empty() && !Config::getInstance().getRouterUser().empty()) {
        std::string routerIp = Config::getInstance().getSnmpTargets()[0].ip;
        std::string routerUser = Config::getInstance().getRouterUser();
        ZyxelDriver::getInstance().configure(routerIp, 22, routerUser);
    }
    ZyxelDriver::getInstance().start();
    AimonGatewayClient::getInstance().setRouterDriver(
        std::shared_ptr<RouterDriver>(&ZyxelDriver::getInstance(), [](RouterDriver *) {}));

    // Start gateway client to aimon hub
    AimonGatewayClient::getInstance().setWebPort(WebServer::getInstance().getPort());
    AimonGatewayClient::getInstance().start();

    if (mode == "daemon") {
        // Interactive shell mode for persistent GNU screen session
        NetMonShell::getInstance().runInteractive();
    } else {
        // Non-interactive background daemon
        std::cout << "NetMon daemon running. Press Ctrl+C to terminate." << std::endl;
        while (!g_shutdownRequested) {
            pause();
        }
    }

    std::cout << "\nShutting down NetMon subsystems..." << std::endl;
    SyslogServer::getInstance().stop();
    WebServer::getInstance().stop();
    SnmpAggregator::getInstance().stop();
    LanSniffer::getInstance().stop();
    AimonGatewayClient::getInstance().stop();
    DnsResolver::getInstance().stop();
    MacVendorResolver::getInstance().stop();
    ZyxelDriver::getInstance().stop();

    SyslogServer::getInstance().join();
    WebServer::getInstance().join();
    SnmpAggregator::getInstance().join();
    AimonGatewayClient::getInstance().join();
    SnmpDatabase::getInstance().close();

    instanceLock.release();

    libssh2_exit();

    std::cout << "NetMon shutdown complete." << std::endl;
    return 0;
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
