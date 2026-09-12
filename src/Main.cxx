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
#include "AimonGatewayClient.hxx"
#include "NetMonShell.hxx"

static volatile sig_atomic_t g_shutdownRequested = 0;

static void signalHandler(int sig) {
    (void)sig;
    g_shutdownRequested = 1;
    NetMonShell::getInstance().stop();
    LanSniffer::getInstance().stop();
    AimonGatewayClient::getInstance().stop();
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
              << "  version        Show NetMon version and build metadata\n"
              << "  help           Show this help message\n\n"
              << "Options:\n"
              << "  -c <path>      Path to custom netmon.cfg configuration file (default: ~/.config/netmon/netmon.cfg)\n"
              << "  -i <iface>     Override sniffing network interface (e.g. br0)\n"
              << "  -g <host>      Override aimon gateway host (default: 192.168.8.39)\n"
              << "  -p <port>      Override aimon gateway port (default: 3885)\n"
              << "  --version, -v  Display version and build metadata\n"
              << "  --help, -h     Display this help message\n";
}

int main(int argc, char **argv) {
    // Crucial: ignore SIGPIPE to prevent crashes during terminal disconnection or screen detach
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    std::string customConfigPath;
    std::string overrideInterface;
    std::string overrideGateway;
    int overridePort = 0;
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

    // Load persistent devices registry
    DeviceRegistry::getInstance().load();

    std::cout << "NetMon v" << NETMON_VERSION_STRING << " initializing..." << std::endl;
    std::cout << "  Interface:      " << Config::getInstance().getInterface() << std::endl;
    std::cout << "  Gateway:        " << Config::getInstance().getGatewayHost()
              << ":" << Config::getInstance().getGatewayPort() << std::endl;
    std::cout << "  Devices File:   " << DeviceRegistry::getInstance().getFilePath() << std::endl;
    std::cout << "  Known Devices:  " << DeviceRegistry::getInstance().getDeviceCount() << std::endl;

    if (mode == "status") {
        NetMonShell::getInstance().executeCommand("status");
        NetMonShell::getInstance().executeCommand("devices");
        return 0;
    }

    // Start streaming sniffer and telemetry
    LanSniffer::getInstance().start();

    // Initialize router driver (stubs until hardware credentials configured)
    auto zyxel = std::make_shared<ZyxelDriver>();
    AimonGatewayClient::getInstance().setRouterDriver(zyxel);

    // Start gateway client to aimon hub
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
    LanSniffer::getInstance().stop();
    AimonGatewayClient::getInstance().stop();
    AimonGatewayClient::getInstance().join();

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
