/*
 * AimonGatewayClient.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "AimonGatewayClient.hxx"
#include "Config.hxx"
#include "LanSniffer.hxx"
#include "DeviceRegistry.hxx"
#include "OuiDatabase.hxx"
#include "SecurityCheckpoint.hxx"
#include "SnmpAggregator.hxx"
#include <nlohmann/json.hpp>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>
#include <errno.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <chrono>

using json = nlohmann::json;

AimonGatewayClient &AimonGatewayClient::getInstance() {
    static AimonGatewayClient instance;
    return instance;
}

AimonGatewayClient::AimonGatewayClient()
    : _routerDriver(nullptr)
    , _host("")
    , _port(0)
    , _running(false)
    , _connected(false)
    , _sockFd(-1) {
}

AimonGatewayClient::~AimonGatewayClient() {
    stop();
    join();
}

void AimonGatewayClient::setRouterDriver(std::shared_ptr<RouterDriver> driver) {
    _routerDriver = driver;
}

bool AimonGatewayClient::start(const std::string &host, uint16_t port) {
    if (_running.load()) {
        return false;
    }

    if (!host.empty()) {
        _host = host;
    } else {
        _host = Config::getInstance().getGatewayHost();
    }

    if (port != 0) {
        _port = port;
    } else {
        _port = static_cast<uint16_t>(Config::getInstance().getGatewayPort());
    }

    _running.store(true);
    _thread = std::thread(&AimonGatewayClient::run, this);
    return true;
}

void AimonGatewayClient::stop() {
    if (!_running.load()) {
        return;
    }

    _running.store(false);
    disconnect();
}

void AimonGatewayClient::join() {
    if (_thread.joinable()) {
        _thread.join();
    }
}

bool AimonGatewayClient::isConnected() const {
    return _connected.load();
}

void AimonGatewayClient::disconnect() {
    _connected.store(false);
    std::lock_guard<std::mutex> lock(_sendMutex);
    if (_sockFd != -1) {
        shutdown(_sockFd, SHUT_RDWR);
        close(_sockFd);
        _sockFd = -1;
    }
}

bool AimonGatewayClient::connectToGateway() {
    disconnect();

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *res = nullptr;
    std::string portStr = std::to_string(_port);
    int rc = getaddrinfo(_host.c_str(), portStr.c_str(), &hints, &res);
    if (rc != 0 || res == nullptr) {
        return false;
    }

    int fd = -1;
    for (struct addrinfo *rp = res; rp != nullptr; rp = rp->ai_next) {
        fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (fd == -1) {
            continue;
        }

        int flag = 1;
        setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));
        setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)&flag, sizeof(flag));

        if (connect(fd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;
        }

        close(fd);
        fd = -1;
    }

    freeaddrinfo(res);

    if (fd == -1) {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(_sendMutex);
        _sockFd = fd;
    }

    return true;
}

void AimonGatewayClient::sendResponse(const std::string &msg) {
    std::lock_guard<std::mutex> lock(_sendMutex);
    if (_sockFd == -1) {
        return;
    }

    std::string payload = msg;
    if (payload.empty() || payload.back() != '\n') {
        payload += "\n";
    }

    const char *ptr = payload.c_str();
    size_t rem = payload.size();
    while (rem > 0) {
        ssize_t n = send(_sockFd, ptr, rem, MSG_NOSIGNAL);
        if (n <= 0) {
            break;
        }
        ptr += n;
        rem -= n;
    }
}

bool AimonGatewayClient::sendRegistration() {
    json reg = {
        {"jsonrpc", "2.0"},
        {"method", "gateway/register"},
        {"params", {
            {"subsystem", "netmon"},
            {"name", "netmon"},
            {"description", "Local Area Network (LAN) monitor, packet sniffer, and firewall telemetry engine"},
            {"tools", json::array({
                {
                    {"name", "lan_get_devices"},
                    {"description", "Get all unique MAC and IP addresses discovered on the LAN (from ARP cache and live frame sniffer), vendor OUI, device category (infrastructure, known, visitor, or unregistered), and first/last seen timestamps."},
                    {"inputSchema", {
                        {"type", "object"},
                        {"properties", {
                            {"category", {
                                {"type", "string"},
                                {"description", "Optional category filter: 'all', 'infrastructure', 'known', 'visitor', or 'unregistered' (default: 'all')"}
                            }}
                        }}
                    }}
                },
                {
                    {"name", "lan_get_unregistered_devices"},
                    {"description", "List all unregistered devices or visitor phones connected to WiFi, with vendor identification and connection timestamps."},
                    {"inputSchema", {
                        {"type", "object"},
                        {"properties", json::object()}
                    }}
                },
                {
                    {"name", "lan_name_device"},
                    {"description", "Assign a human-readable friendly name and category (e.g. 'known', 'visitor', 'infrastructure') to a MAC address, persisting it to devices.cfg."},
                    {"inputSchema", {
                        {"type", "object"},
                        {"properties", {
                            {"mac", {
                                {"type", "string"},
                                {"description", "Hardware MAC address to name"}
                            }},
                            {"name", {
                                {"type", "string"},
                                {"description", "Friendly name to assign to the device (e.g. 'alice_iphone', 'living_room_tv')"}
                            }},
                            {"category", {
                                {"type", "string"},
                                {"description", "Device classification category: 'known', 'visitor', or 'infrastructure' (default: 'known')"}
                            }}
                        }},
                        {"required", json::array({"mac", "name"})}
                    }}
                },
                {
                    {"name", "lan_get_top_talkers"},
                    {"description", "Return the top bandwidth-consuming internal hosts over a time window (in minutes, up to 60) with transfer rates directly from in-memory ring buffers."},
                    {"inputSchema", {
                        {"type", "object"},
                        {"properties", {
                            {"limit", {
                                {"type", "integer"},
                                {"description", "Number of top talkers to return (default: 10)"}
                            }},
                            {"window_minutes", {
                                {"type", "integer"},
                                {"description", "Time window in minutes (1 to 60, or 0 for cumulative, default: 15)"}
                            }}
                        }}
                    }}
                },
                {
                    {"name", "lan_get_traffic_summary"},
                    {"description", "Return total LAN throughput, packet rates, active capture status, and protocol distribution (DNS, HTTPS, SSH, HTTP, ARP, Broadcast, Multicast, ICMP)."},
                    {"inputSchema", {
                        {"type", "object"},
                        {"properties", json::object()}
                    }}
                },
                {
                    {"name", "firewall_get_status"},
                    {"description", "Get router/firewall hardware status, model, firmware, and interface link states."},
                    {"inputSchema", {
                        {"type", "object"},
                        {"properties", json::object()}
                    }}
                },
                {
                    {"name", "firewall_get_sessions"},
                    {"description", "Query active router firewall connection tracking sessions."},
                    {"inputSchema", {
                        {"type", "object"},
                        {"properties", json::object()}
                    }}
                },
                {
                    {"name", "firewall_block_ip"},
                    {"description", "Insert a firewall drop rule to block an IP address. Validated against protected infrastructure invariants (router, servers, gateways)."},
                    {"inputSchema", {
                        {"type", "object"},
                        {"properties", {
                            {"ip", {
                                {"type", "string"},
                                {"description", "Target IPv4 address to block"}
                            }},
                            {"reason", {
                                {"type", "string"},
                                {"description", "Reason or security ticket justifying the firewall block"}
                            }}
                        }},
                        {"required", json::array({"ip", "reason"})}
                    }}
                },
                {
                    {"name", "firewall_unblock_ip"},
                    {"description", "Remove a firewall drop rule for an IP address."},
                    {"inputSchema", {
                        {"type", "object"},
                        {"properties", {
                            {"ip", {
                                {"type", "string"},
                                {"description", "Target IPv4 address to unblock"}
                            }}
                        }},
                        {"required", json::array({"ip"})}
                    }}
                },
                {
                    {"name", "snmp_get_device_metrics"},
                    {"description", "Query SNMP metrics for network switches, access points, or routers."},
                    {"inputSchema", {
                        {"type", "object"},
                        {"properties", {
                            {"target_ip", {
                                {"type", "string"},
                                {"description", "IP address of the target network device to query"}
                            }}
                        }},
                        {"required", json::array({"target_ip"})}
                    }}
                }
            })}
        }}
    };

    sendResponse(reg.dump());
    return true;
}

void AimonGatewayClient::processIncoming() {
    char buffer[4096];
    std::string lineBuffer;

    while (_running.load() && _connected.load() && (_sockFd != -1)) {
        struct pollfd pfd;
        pfd.fd = _sockFd;
        pfd.events = POLLIN;
        pfd.revents = 0;

        int pr = poll(&pfd, 1, 500);
        if (pr < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (pr == 0) {
            continue;
        }
        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            break;
        }
        if (pfd.revents & POLLIN) {
            ssize_t n = recv(_sockFd, buffer, sizeof(buffer), 0);
            if (n <= 0) {
                break;
            }
            lineBuffer.append(buffer, n);

            size_t newlinePos;
            while ((newlinePos = lineBuffer.find('\n')) != std::string::npos) {
                std::string line = lineBuffer.substr(0, newlinePos);
                lineBuffer.erase(0, newlinePos + 1);
                if (!line.empty() && (line.back() == '\r')) {
                    line.pop_back();
                }
                if (!line.empty()) {
                    handleRequest(line);
                }
            }
        }
    }
}

void AimonGatewayClient::handleRequest(const std::string &line) {
    try {
        json req = json::parse(line);
        if (!req.contains("id")) {
            return;
        }
        if (req.contains("result") || req.contains("error")) {
            return;
        }

        auto reqId = req["id"];
        std::string method = req.value("method", "");

        if (method == "tools/call") {
            json params = req.value("params", json::object());
            std::string toolName = params.value("name", "");
            json args = params.value("arguments", json::object());

            std::string resultText;
            if (toolName == "lan_get_devices") {
                std::string catFilter = args.value("category", "all");
                resultText = toolLanGetDevices(catFilter);
            } else if (toolName == "lan_get_unregistered_devices") {
                resultText = toolLanGetUnregisteredDevices();
            } else if (toolName == "lan_name_device") {
                std::string mac = args.value("mac", "");
                std::string name = args.value("name", "");
                std::string category = args.value("category", "known");
                resultText = toolLanNameDevice(mac, name, category);
            } else if (toolName == "lan_get_top_talkers") {
                int limit = args.value("limit", 10);
                int windowMinutes = args.value("window_minutes", 15);
                resultText = toolLanGetTopTalkers(limit, windowMinutes);
            } else if (toolName == "lan_get_traffic_summary") {
                resultText = toolLanGetTrafficSummary();
            } else if (toolName == "firewall_get_status") {
                resultText = toolFirewallGetStatus();
            } else if (toolName == "firewall_get_sessions") {
                resultText = toolFirewallGetSessions();
            } else if (toolName == "firewall_block_ip") {
                std::string ip = args.value("ip", "");
                std::string reason = args.value("reason", "");
                resultText = toolFirewallBlockIp(ip, reason);
            } else if (toolName == "firewall_unblock_ip") {
                std::string ip = args.value("ip", "");
                resultText = toolFirewallUnblockIp(ip);
            } else if (toolName == "snmp_get_device_metrics") {
                std::string targetIp = args.value("target_ip", "");
                resultText = toolSnmpGetDeviceMetrics(targetIp);
            } else {
                json errResp = {
                    {"jsonrpc", "2.0"},
                    {"id", reqId},
                    {"error", {
                        {"code", -32601},
                        {"message", "Unknown tool: " + toolName}
                    }}
                };
                sendResponse(errResp.dump());
                return;
            }

            json resp = {
                {"jsonrpc", "2.0"},
                {"id", reqId},
                {"result", {
                    {"content", json::array({
                        {
                            {"type", "text"},
                            {"text", resultText}
                        }
                    })}
                }}
            };
            sendResponse(resp.dump());
        } else {
            json errResp = {
                {"jsonrpc", "2.0"},
                {"id", reqId},
                {"error", {
                    {"code", -32601},
                    {"message", "Unknown method: " + method}
                }}
            };
            sendResponse(errResp.dump());
        }
    } catch (const std::exception &ex) {
        std::cerr << "AimonGatewayClient: Error handling JSON-RPC: " << ex.what() << std::endl;
    }
}

void AimonGatewayClient::run() {
    int retryIntervalSec = Config::getInstance().getReconnectIntervalSec();
    if (retryIntervalSec < 1) {
        retryIntervalSec = 5;
    }

    while (_running.load()) {
        std::cout << "AimonGatewayClient: Connecting to aimon gateway at "
                  << _host << ":" << _port << "..." << std::endl;

        if (connectToGateway()) {
            _connected.store(true);
            std::cout << "AimonGatewayClient: Connected to aimon! Registering netmon toolset..." << std::endl;

            if (sendRegistration()) {
                std::cout << "AimonGatewayClient: Registration sent successfully." << std::endl;
            }

            processIncoming();

            _connected.store(false);
            std::cout << "AimonGatewayClient: Disconnected from aimon gateway." << std::endl;
        } else {
            std::cout << "AimonGatewayClient: Connection failed. Retrying in "
                      << retryIntervalSec << "s..." << std::endl;
        }

        if (_running.load()) {
            for (int i = 0; i < retryIntervalSec * 10 && _running.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
}

std::string AimonGatewayClient::toolLanGetDevices(const std::string &categoryFilter) {
    auto fullJson = LanSniffer::getInstance().getDevicesJson();
    if (categoryFilter.empty() || categoryFilter == "all") {
        return fullJson.dump(2);
    }

    json filtered = fullJson;
    json filteredList = json::array();
    for (const auto &dev : fullJson["devices"]) {
        if (dev.value("category", "") == categoryFilter) {
            filteredList.push_back(dev);
        }
    }
    filtered["devices"] = filteredList;
    filtered["filtered_count"] = filteredList.size();
    filtered["filter_category"] = categoryFilter;
    return filtered.dump(2);
}

std::string AimonGatewayClient::toolLanGetUnregisteredDevices() {
    return LanSniffer::getInstance().getUnregisteredDevicesJson().dump(2);
}

std::string AimonGatewayClient::toolLanNameDevice(const std::string &mac,
                                                  const std::string &name,
                                                  const std::string &category) {
    if (mac.empty() || name.empty()) {
        json err = {
            {"status", "error"},
            {"error", "Both 'mac' and 'name' are required parameters"}
        };
        return err.dump(2);
    }

    bool ok = DeviceRegistry::getInstance().nameDevice(mac, name, category);
    json res;
    if (ok) {
        res["status"] = "ok";
        res["mac"] = OuiDatabase::normalizeMac(mac);
        res["name"] = name;
        res["category"] = category.empty() ? "known" : category;
        res["persisted_to"] = DeviceRegistry::getInstance().getFilePath();
    } else {
        res["status"] = "error";
        res["error"] = "MAC address not found in registry (run lan_get_devices to discover active devices)";
        res["mac"] = mac;
    }
    return res.dump(2);
}

std::string AimonGatewayClient::toolLanGetTopTalkers(int limit, int windowMinutes) {
    if (limit <= 0) limit = 10;
    if (windowMinutes < 0 || windowMinutes > 60) windowMinutes = 15;
    return LanSniffer::getInstance().getTopTalkers(static_cast<size_t>(limit), windowMinutes).dump(2);
}

std::string AimonGatewayClient::toolLanGetTrafficSummary() {
    return LanSniffer::getInstance().getTrafficSummary().dump(2);
}

std::string AimonGatewayClient::toolFirewallGetStatus() {
    if (_routerDriver) {
        return _routerDriver->getStatus().dump(2);
    }
    json err = {
        {"status", "unconfigured"},
        {"error", "Router driver not initialized in netmon"}
    };
    return err.dump(2);
}

std::string AimonGatewayClient::toolFirewallGetSessions() {
    if (_routerDriver) {
        return _routerDriver->getSessions().dump(2);
    }
    json err = {
        {"status", "unconfigured"},
        {"error", "Router driver not initialized in netmon"}
    };
    return err.dump(2);
}

std::string AimonGatewayClient::toolFirewallBlockIp(const std::string &ip, const std::string &reason) {
    std::string errReason;
    bool permitted = SecurityCheckpoint::getInstance().validateBlockRequest(ip, errReason);
    SecurityCheckpoint::getInstance().logAudit("firewall_block_ip", "ai_agent",
                                               "ip=" + ip + ", reason=" + reason, permitted, errReason);

    if (!permitted) {
        json err = {
            {"status", "denied"},
            {"target_ip", ip},
            {"reason", errReason}
        };
        return err.dump(2);
    }

    if (_routerDriver) {
        return _routerDriver->blockIp(ip, reason).dump(2);
    }

    json unconf = {
        {"status", "unconfigured"},
        {"error", "Router driver not configured"}
    };
    return unconf.dump(2);
}

std::string AimonGatewayClient::toolFirewallUnblockIp(const std::string &ip) {
    if (SecurityCheckpoint::getInstance().isIpProtected(ip)) {
        json err = {
            {"status", "denied"},
            {"target_ip", ip},
            {"reason", "Cannot modify protected core infrastructure invariant"}
        };
        return err.dump(2);
    }

    SecurityCheckpoint::getInstance().logAudit("firewall_unblock_ip", "ai_agent",
                                               "ip=" + ip, true, "Permitted");

    if (_routerDriver) {
        return _routerDriver->unblockIp(ip).dump(2);
    }

    json unconf = {
        {"status", "unconfigured"},
        {"error", "Router driver not configured"}
    };
    return unconf.dump(2);
}

std::string AimonGatewayClient::toolSnmpGetDeviceMetrics(const std::string &targetIp) {
    return SnmpAggregator::getInstance().getDeviceMetrics(targetIp).dump(2);
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
