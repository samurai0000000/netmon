/*
 * WebServer.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "WebServer.hxx"
#include "WebAssets.hxx"
#include "Config.hxx"
#include "SnmpAggregator.hxx"
#include "SnmpDatabase.hxx"
#include "LanSniffer.hxx"
#include "DeviceRegistry.hxx"
#include "AimonGatewayClient.hxx"
#include "Version.hxx"

#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wshadow"
#include <httplib.h>
#pragma GCC diagnostic pop

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;
using json = nlohmann::json;

WebServer &WebServer::getInstance() {
    static WebServer instance;
    return instance;
}

WebServer::WebServer()
    : _bindAddress("0.0.0.0")
    , _port(3884)
    , _endpointsEnabled(false)
    , _running(false)
    , _server(std::make_unique<httplib::Server>()) {
}

WebServer::~WebServer() {
    stop();
    join();
}

std::string WebServer::createUiSession() {
    static thread_local std::random_device rd;
    static thread_local std::mt19937_64 gen(rd());
    static thread_local std::uniform_int_distribution<uint64_t> dis;

    uint64_t p1 = dis(gen);
    uint64_t p2 = dis(gen);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << p1 << std::setw(16) << p2;
    std::string token = oss.str();

    std::lock_guard<std::mutex> lock(_sessionMutex);
    time_t now = time(nullptr);
    for (auto it = _uiSessions.begin(); it != _uiSessions.end(); ) {
        if (now - it->second > 86400 * 7) {
            it = _uiSessions.erase(it);
        } else {
            ++it;
        }
    }
    _uiSessions[token] = now;
    return token;
}

bool WebServer::isValidUiSession(const httplib::Request &req) const {
    if (req.has_header("User-Agent")) {
        std::string ua = req.get_header_value("User-Agent");
        for (char &c : ua) c = tolower(c);
        if (ua.find("curl/") != std::string::npos ||
            ua.find("python") != std::string::npos ||
            ua.find("wget/") != std::string::npos ||
            ua.find("httpie") != std::string::npos ||
            ua.find("aiohttp") != std::string::npos ||
            ua.find("go-http-client") != std::string::npos) {
            return false;
        }
    } else {
        return false;
    }

    std::string token;
    if (req.has_header("X-UI-Session")) {
        token = req.get_header_value("X-UI-Session");
    } else if (req.has_header("Cookie")) {
        std::string cookie = req.get_header_value("Cookie");
        size_t pos = cookie.find("netmon_session=");
        if (pos != std::string::npos) {
            size_t start = pos + 15;
            size_t end = cookie.find(';', start);
            token = (end == std::string::npos) ? cookie.substr(start) : cookie.substr(start, end - start);
        }
    }

    if (token.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(_sessionMutex);
    auto it = _uiSessions.find(token);
    if (it != _uiSessions.end()) {
        time_t now = time(nullptr);
        if (now - it->second < 86400 * 7) {
            return true;
        }
    }

    return false;
}

std::string WebServer::getMcpHintForPath(const std::string &path, const std::string &body) {
    (void)body;
    if (path == "/api/status") {
        return "Use official MCP tool 'snmp_get_wan_status' or 'lan_get_traffic_summary'.";
    }
    if (path == "/api/snmp/wan") {
        return "Use official MCP tool 'snmp_get_wan_status'.";
    }
    if (path == "/api/snmp/devices") {
        return "Use official MCP tool 'snmp_get_device_metrics' with argument {\"filter\": \"monitored\"}.";
    }
    if (path.find("/stats") != std::string::npos || path.find("/events") != std::string::npos) {
        return "Use official MCP tool 'snmp_get_device_metrics' with argument {\"filter\": \"all\"}.";
    }
    if (path.rfind("/api/snmp", 0) == 0) {
        return "Use official MCP tool 'snmp_query_oid' or 'snmp_get_interface_counters'.";
    }
    if (path == "/api/traffic" || path.rfind("/api/traffic", 0) == 0) {
        return "Use official MCP tool 'lan_get_top_talkers' or 'lan_get_traffic_summary'.";
    }
    if (path == "/api/devices") {
        return "Use official MCP tool 'lan_get_devices' or 'lan_get_unregistered_devices'.";
    }
    if (path == "/api/devices/name") {
        return "Use official MCP tool 'lan_name_device' with argument {\"mac\": \"...\", \"name\": \"...\"}.";
    }
    if (path == "/api/firewall/status") {
        return "Use official MCP tool 'firewall_get_status'.";
    }
    if (path == "/api/firewall/sessions") {
        return "Use official MCP tool 'firewall_get_sessions'.";
    }
    if (path == "/api/firewall/block") {
        return "Use official MCP tool 'firewall_block_ip' with argument {\"ip\": \"...\"}.";
    }
    if (path == "/api/firewall/unblock") {
        return "Use official MCP tool 'firewall_unblock_ip' with argument {\"ip\": \"...\"}.";
    }
    return "Use official MCP tools ('snmp_*', 'lan_*', 'firewall_*'). Direct REST API access is disabled.";
}

bool WebServer::start(const std::string &bindAddress, int port) {
    if (_running.load()) {
        return false;
    }

    if (!bindAddress.empty()) {
        _bindAddress = bindAddress;
    } else {
        _bindAddress = Config::getInstance().getWebConfig().bindAddress;
    }

    if (port > 0) {
        _port = port;
    } else {
        _port = Config::getInstance().getWebConfig().port;
    }

    _endpointsEnabled = Config::getInstance().getWebConfig().endpointsEnabled;

    if (!Config::getInstance().getWebConfig().enabled) {
        std::cout << "WebServer: Disabled in configuration" << std::endl;
        return false;
    }

    setupRoutes();
    _running.store(true);
    _thread = std::thread(&WebServer::run, this);
    std::cout << "WebServer: Dashboard available at http://" << _bindAddress << ":" << _port << std::endl;
    return true;
}

void WebServer::stop() {
    if (!_running.load()) return;
    _running.store(false);
    if (_server) {
        _server->stop();
    }
}

void WebServer::join() {
    if (_thread.joinable()) {
        _thread.join();
    }
}

bool WebServer::isRunning() const {
    return _running.load();
}

int WebServer::getPort() const {
    return _port;
}

void WebServer::run() {
    _server->set_read_timeout(1, 0);
    _server->set_write_timeout(5, 0);
    _server->listen(_bindAddress.c_str(), _port);
    _running.store(false);
}

void WebServer::setupRoutes() {
    // Gate REST API endpoints if disabled by configuration, unless request is from an authenticated Web UI session
    if (!_endpointsEnabled) {
        _server->set_pre_routing_handler([this](const httplib::Request &req, httplib::Response &res) {
            if (req.path == "/api" || req.path.rfind("/api/", 0) == 0) {
                if (!isValidUiSession(req)) {
                    res.status = 403;
                    std::string hint = getMcpHintForPath(req.path, req.body);
                    json err = {
                        {"error", "Direct REST API endpoint access is disabled by configuration."},
                        {"hint", hint},
                        {"daemon", "netmon"}
                    };
                    res.set_content(err.dump(2), "application/json");
                    return httplib::Server::HandlerResponse::Handled;
                }
            }
            return httplib::Server::HandlerResponse::Unhandled;
        });
    }

    auto serveFileOrFallback = [](const std::string &diskPath,
                                  const char *fallbackAsset,
                                  const std::string &contentType,
                                  httplib::Response &res) {
        if (fs::exists(diskPath)) {
            std::ifstream f(diskPath);
            if (f.is_open()) {
                std::string content((std::istreambuf_iterator<char>(f)),
                                    std::istreambuf_iterator<char>());
                res.set_content(content, contentType.c_str());
                return;
            }
        }
        res.set_content(fallbackAsset, contentType.c_str());
    };

    // Static Web Assets
    _server->Get("/", [this, serveFileOrFallback](const httplib::Request &, httplib::Response &res) {
        std::string token = createUiSession();
        res.set_header("Set-Cookie", "netmon_session=" + token + "; Path=/; SameSite=Strict");

        std::string html;
        if (fs::exists("web/index.html")) {
            std::ifstream f("web/index.html");
            if (f.is_open()) {
                html = std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
            }
        }
        if (html.empty()) {
            html = assets::INDEX_HTML;
        }

        std::string sessionScript = "<script>\n"
            "window.__UI_SESSION_TOKEN__ = \"" + token + "\";\n"
            "(function() {\n"
            "    const originalFetch = window.fetch;\n"
            "    window.fetch = function(url, options) {\n"
            "        options = options || {};\n"
            "        options.headers = options.headers || {};\n"
            "        if (window.__UI_SESSION_TOKEN__) {\n"
            "            if (options.headers instanceof Headers) {\n"
            "                options.headers.set('X-UI-Session', window.__UI_SESSION_TOKEN__);\n"
            "            } else if (Array.isArray(options.headers)) {\n"
            "                options.headers.push(['X-UI-Session', window.__UI_SESSION_TOKEN__]);\n"
            "            } else {\n"
            "                options.headers['X-UI-Session'] = window.__UI_SESSION_TOKEN__;\n"
            "            }\n"
            "        }\n"
            "        return originalFetch(url, options);\n"
            "    };\n"
            "})();\n"
            "</script>\n";

        size_t headPos = html.find("</head>");
        if (headPos != std::string::npos) {
            html.insert(headPos, sessionScript);
        } else {
            html = sessionScript + html;
        }

        res.set_content(html, "text/html");
    });

    _server->Get("/style.css", [serveFileOrFallback](const httplib::Request &, httplib::Response &res) {
        serveFileOrFallback("web/style.css", assets::STYLE_CSS, "text/css", res);
    });

    _server->Get("/app.js", [serveFileOrFallback](const httplib::Request &, httplib::Response &res) {
        serveFileOrFallback("web/app.js", assets::APP_JS, "application/javascript", res);
    });

    // 1. System Status API
    _server->Get("/api/status", [](const httplib::Request &, httplib::Response &res) {
        static const auto s_startTime = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        int64_t uptimeSec = std::chrono::duration_cast<std::chrono::seconds>(now - s_startTime).count();

        json j = {
            {"status", "ok"},
            {"subsystem", "netmon"},
            {"version", NETMON_VERSION_STRING},
            {"uptime_seconds", uptimeSec},
            {"db_size_bytes", SnmpDatabase::getInstance().getDatabaseSizeBytes()},
            {"snmp_running", SnmpAggregator::getInstance().isRunning()},
            {"pcap_running", LanSniffer::getInstance().isRunning()},
            {"aimon_connected", AimonGatewayClient::getInstance().isConnected()},
            {"known_devices", DeviceRegistry::getInstance().getDeviceCount()}
        };
        res.set_content(j.dump(), "application/json");
    });

    // 2. SNMP WAN Status API
    _server->Get("/api/snmp/wan", [](const httplib::Request &req, httplib::Response &res) {
        std::string ip = req.has_param("target_ip") ? req.get_param_value("target_ip") : "";
        json j = SnmpAggregator::getInstance().getWanStatus(ip);
        res.set_content(j.dump(), "application/json");
    });

    // 3. SNMP Devices Inventory API
    _server->Get("/api/snmp/devices", [](const httplib::Request &req, httplib::Response &res) {
        std::string ip = req.has_param("target_ip") ? req.get_param_value("target_ip") : "";
        std::string filter = req.has_param("filter") ? req.get_param_value("filter") : "monitored";
        json j = SnmpAggregator::getInstance().getDeviceMetrics(ip, filter);
        res.set_content(j.dump(), "application/json");
    });

    // 4. SNMP History API (powered by SQLite)
    _server->Get("/api/snmp/history", [](const httplib::Request &req, httplib::Response &res) {
        std::string targetIp = req.has_param("target_ip") ? req.get_param_value("target_ip") : "";
        if (targetIp.empty()) {
            auto targets = Config::getInstance().getSnmpTargets();
            if (!targets.empty()) targetIp = targets[0].ip;
        }

        std::string ifName = req.has_param("iface") ? req.get_param_value("iface") : "eth1";

        int64_t now = static_cast<int64_t>(time(nullptr));
        int64_t startTs = now - 86400; // default 24h
        int64_t endTs = now;

        if (req.has_param("start")) {
            startTs = std::stoll(req.get_param_value("start"));
        } else if (req.has_param("hours")) {
            int h = std::stoi(req.get_param_value("hours"));
            if (h > 0) startTs = now - (h * 3600);
        }

        if (req.has_param("end")) {
            endTs = std::stoll(req.get_param_value("end"));
        }

        int maxPoints = req.has_param("max_points") ? std::stoi(req.get_param_value("max_points")) : 240;

        json history = SnmpDatabase::getInstance().queryHistory(targetIp, ifName, startTs, endTs, maxPoints);
        res.set_content(history.dump(), "application/json");
    });

    // 5. LAN Traffic Summary API
    _server->Get("/api/traffic", [](const httplib::Request &, httplib::Response &res) {
        json j = LanSniffer::getInstance().getTrafficSummary();
        json tt = LanSniffer::getInstance().getTopTalkers(5, 15);
        if (tt.contains("top_talkers")) {
            j["top_talkers"] = tt["top_talkers"];
        } else {
            j["top_talkers"] = json::array();
        }
        res.set_content(j.dump(), "application/json");
    });

    _server->Get("/api/traffic/top-talkers", [](const httplib::Request &req, httplib::Response &res) {
        int limit = req.has_param("limit") ? std::stoi(req.get_param_value("limit")) : 10;
        int window = req.has_param("window") ? std::stoi(req.get_param_value("window")) : 15;
        json j = LanSniffer::getInstance().getTopTalkers(limit, window);
        res.set_content(j.dump(), "application/json");
    });

    // 6. LAN Devices Registry API
    _server->Get("/api/devices", [](const httplib::Request &, httplib::Response &res) {
        json j = LanSniffer::getInstance().getDevicesJson();
        res.set_content(j.dump(), "application/json");
    });
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
