/*
 * WebServer.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "WebServer.hxx"
#include "WebAssets.hxx"
#include "Config.hxx"
#include "AuthManager.hxx"
#include "SnmpAggregator.hxx"
#include "SnmpDatabase.hxx"
#include "LanSniffer.hxx"
#include "DeviceRegistry.hxx"
#include "AimonGatewayClient.hxx"
#include "SecurityCheckpoint.hxx"
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
    , _adminBindAddress("127.0.0.1")
    , _adminPort(3886)
    , _endpointsEnabled(false)
    , _running(false)
    , _server(std::make_unique<httplib::Server>())
    , _adminServer(std::make_unique<httplib::Server>()) {
}

WebServer::~WebServer() {
    stop();
    join();
}

bool WebServer::isReservedAdminPort(int port) {
    return (port == 3883 || port == 3884 || port == 3885 || port == 16880);
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

bool WebServer::isValidAdminSession(const httplib::Request &req, std::string &outToken) const {
    outToken.clear();
    if (req.has_header("Authorization")) {
        std::string auth = req.get_header_value("Authorization");
        if (auth.rfind("Bearer ", 0) == 0) {
            outToken = auth.substr(7);
        }
    } else if (req.has_header("X-Admin-Token")) {
        outToken = req.get_header_value("X-Admin-Token");
    } else if (req.has_header("Cookie")) {
        std::string cookie = req.get_header_value("Cookie");
        size_t pos = cookie.find("netmon_admin_session=");
        if (pos != std::string::npos) {
            size_t start = pos + 21;
            size_t end = cookie.find(';', start);
            outToken = (end == std::string::npos) ? cookie.substr(start) : cookie.substr(start, end - start);
        }
    }

    if (outToken.empty()) {
        return false;
    }

    return AuthManager::getInstance().validateSession(outToken);
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

bool WebServer::start(const std::string &bindAddress, int port, int adminPort) {
    if (_running.load()) {
        return false;
    }
    join();

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

    if (adminPort > 0) {
        _adminPort = adminPort;
    } else {
        _adminPort = Config::getInstance().getAdminPort();
    }

    if (isReservedAdminPort(_adminPort) || _adminPort == _port) {
        std::cerr << "WebServer: Error: Admin port " << _adminPort
                  << " is reserved or conflicts with dashboard port " << _port << std::endl;
        return false;
    }

    _endpointsEnabled = Config::getInstance().getWebConfig().endpointsEnabled;

    if (!Config::getInstance().getWebConfig().enabled) {
        std::cout << "WebServer: Disabled in configuration" << std::endl;
        return false;
    }

    _server = std::make_unique<httplib::Server>();
    _adminServer = std::make_unique<httplib::Server>();

    setupDashboardRoutes();
    setupAdminRoutes();

    _running.store(true);
    _thread = std::thread(&WebServer::runDashboard, this);
    _adminThread = std::thread(&WebServer::runAdmin, this);

    std::cout << "WebServer: Dashboard available at http://" << _bindAddress << ":" << _port << std::endl;
    std::cout << "WebServer: Loopback admin available at http://" << _adminBindAddress << ":" << _adminPort << std::endl;
    return true;
}

void WebServer::stop() {
    if (!_running.load()) {
        return;
    }
    _running.store(false);
    if (_server) {
        _server->stop();
    }
    if (_adminServer) {
        _adminServer->stop();
    }
}

void WebServer::join() {
    if (_thread.joinable()) {
        _thread.join();
    }
    if (_adminThread.joinable()) {
        _adminThread.join();
    }
    _server.reset();
    _adminServer.reset();
    std::lock_guard<std::mutex> lock(_sessionMutex);
    std::map<std::string, time_t>().swap(_uiSessions);
}

bool WebServer::isRunning() const {
    return _running.load();
}

int WebServer::getPort() const {
    return _port;
}

int WebServer::getAdminPort() const {
    return _adminPort;
}

void WebServer::runDashboard() {
    _server->set_read_timeout(1, 0);
    _server->set_write_timeout(5, 0);
    _server->listen(_bindAddress.c_str(), _port);
}

void WebServer::runAdmin() {
    _adminServer->set_read_timeout(1, 0);
    _adminServer->set_write_timeout(5, 0);
    _adminServer->listen(_adminBindAddress.c_str(), _adminPort);
}

void WebServer::setupAdminRoutes() {
    _adminServer->Post("/api/auth/login", [this](const httplib::Request &req, httplib::Response &res) {
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            res.status = 400;
            res.set_content(json{{"error", "Invalid JSON body"}}.dump(), "application/json");
            return;
        }

        std::string password = body.value("password", "");
        std::string token = AuthManager::getInstance().login(password);
        if (token.empty()) {
            res.status = 401;
            res.set_content(json{{"status", "unauthorized"}, {"error", "Invalid password"}}.dump(), "application/json");
            return;
        }

        res.status = 200;
        res.set_header("Set-Cookie", "netmon_admin_session=" + token + "; Path=/; HttpOnly; SameSite=Strict");
        res.set_content(json{{"status", "ok"}, {"token", token}}.dump(), "application/json");
    });

    _adminServer->Post("/api/auth/logout", [this](const httplib::Request &req, httplib::Response &res) {
        std::string token;
        if (!isValidAdminSession(req, token)) {
            res.status = 401;
            res.set_content(json{{"error", "Unauthorized"}}.dump(), "application/json");
            return;
        }
        AuthManager::getInstance().logout(token);
        res.status = 200;
        res.set_content(json{{"status", "ok"}}.dump(), "application/json");
    });

    _adminServer->Get("/api/admin/status", [this](const httplib::Request &req, httplib::Response &res) {
        std::string token;
        if (!isValidAdminSession(req, token)) {
            res.status = 401;
            res.set_content(json{{"error", "Unauthorized"}}.dump(), "application/json");
            return;
        }
        json j = {
            {"status", "ok"},
            {"subsystem", "netmon_admin"},
            {"admin_port", _adminPort}
        };
        res.set_content(j.dump(), "application/json");
    });

    _adminServer->Get("/api/admin/pending", [this](const httplib::Request &req, httplib::Response &res) {
        std::string token;
        if (!isValidAdminSession(req, token)) {
            res.status = 401;
            res.set_content(json{{"error", "Unauthorized"}}.dump(), "application/json");
            return;
        }
        std::string filter = req.has_param("status") ? req.get_param_value("status") : "pending";
        auto tickets = SecurityCheckpoint::getInstance().getPendingTickets(filter);
        json arr = json::array();
        for (const auto &t : tickets) {
            arr.push_back({
                {"id", t.id},
                {"created_at", t.createdAt},
                {"expires_at", t.expiresAt},
                {"requester", t.requester},
                {"tool", t.tool},
                {"payload", t.payload},
                {"status", t.status}
            });
        }
        res.status = 200;
        res.set_content(json{{"status", "ok"}, {"pending", arr}}.dump(), "application/json");
    });

    _adminServer->Post("/api/admin/approve", [this](const httplib::Request &req, httplib::Response &res) {
        std::string token;
        if (!isValidAdminSession(req, token)) {
            res.status = 401;
            res.set_content(json{{"error", "Unauthorized"}}.dump(), "application/json");
            return;
        }
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            res.status = 400;
            res.set_content(json{{"error", "Invalid JSON"}}.dump(), "application/json");
            return;
        }
        int64_t id = body.value("ticket_id", 0);
        if (id <= 0 && body.contains("id")) {
            id = body.value("id", 0);
        }
        std::string outErr;
        if (SecurityCheckpoint::getInstance().approve(id, outErr)) {
            res.status = 200;
            res.set_content(json{{"status", "ok"}}.dump(), "application/json");
        } else {
            res.status = 400;
            res.set_content(json{{"status", "error"}, {"error", outErr}}.dump(), "application/json");
        }
    });

    _adminServer->Post("/api/admin/deny", [this](const httplib::Request &req, httplib::Response &res) {
        std::string token;
        if (!isValidAdminSession(req, token)) {
            res.status = 401;
            res.set_content(json{{"error", "Unauthorized"}}.dump(), "application/json");
            return;
        }
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            res.status = 400;
            res.set_content(json{{"error", "Invalid JSON"}}.dump(), "application/json");
            return;
        }
        int64_t id = body.value("ticket_id", 0);
        if (id <= 0 && body.contains("id")) {
            id = body.value("id", 0);
        }
        std::string reason = body.value("reason", "Operator denied");
        if (SecurityCheckpoint::getInstance().deny(id, reason)) {
            res.status = 200;
            res.set_content(json{{"status", "ok"}}.dump(), "application/json");
        } else {
            res.status = 400;
            res.set_content(json{{"status", "error"}, {"error", "Failed to deny ticket"}}.dump(), "application/json");
        }
    });

    _adminServer->Post("/api/admin/reconcile", [this](const httplib::Request &req, httplib::Response &res) {
        std::string token;
        if (!isValidAdminSession(req, token)) {
            res.status = 401;
            res.set_content(json{{"error", "Unauthorized"}}.dump(), "application/json");
            return;
        }
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            res.status = 400;
            res.set_content(json{{"error", "Invalid JSON"}}.dump(), "application/json");
            return;
        }
        int64_t id = body.value("ticket_id", 0);
        if (id <= 0 && body.contains("id")) {
            id = body.value("id", 0);
        }
        std::string action = body.value("action", "");
        std::string outErr;
        if (SecurityCheckpoint::getInstance().reconcile(id, action, outErr)) {
            res.status = 200;
            res.set_content(json{{"status", "ok"}}.dump(), "application/json");
        } else {
            res.status = 400;
            res.set_content(json{{"status", "error"}, {"error", outErr}}.dump(), "application/json");
        }
    });

    _adminServer->Get("/api/admin/policy", [this](const httplib::Request &req, httplib::Response &res) {
        std::string token;
        if (!isValidAdminSession(req, token)) {
            res.status = 401;
            res.set_content(json{{"error", "Unauthorized"}}.dump(), "application/json");
            return;
        }
        std::string pol = SecurityCheckpoint::policyModeToString(SecurityCheckpoint::getInstance().getPolicyMode());
        res.status = 200;
        res.set_content(json{{"status", "ok"}, {"policy", pol}}.dump(), "application/json");
    });

    _adminServer->Post("/api/admin/policy", [this](const httplib::Request &req, httplib::Response &res) {
        std::string token;
        if (!isValidAdminSession(req, token)) {
            res.status = 401;
            res.set_content(json{{"error", "Unauthorized"}}.dump(), "application/json");
            return;
        }
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            res.status = 400;
            res.set_content(json{{"error", "Invalid JSON"}}.dump(), "application/json");
            return;
        }
        std::string pol = body.value("policy", "");
        PolicyMode mode = SecurityCheckpoint::stringToPolicyMode(pol);
        SecurityCheckpoint::getInstance().setPolicyMode(mode);
        res.status = 200;
        res.set_content(json{{"status", "ok"}, {"policy", SecurityCheckpoint::policyModeToString(mode)}}.dump(), "application/json");
    });

    _adminServer->Get("/api/admin/audit", [this](const httplib::Request &req, httplib::Response &res) {
        std::string token;
        if (!isValidAdminSession(req, token)) {
            res.status = 401;
            res.set_content(json{{"error", "Unauthorized"}}.dump(), "application/json");
            return;
        }
        auto records = SnmpDatabase::getInstance().getAllAuditOutbox();
        json arr = json::array();
        for (const auto &r : records) {
            arr.push_back({
                {"sequence", r.sequence},
                {"timestamp", r.timestamp},
                {"prev_hash", r.prevHash},
                {"record_hash", SecurityCheckpoint::computeRecordHash(r)},
                {"action_id", r.actionId},
                {"tool", r.tool},
                {"requester", r.requester},
                {"payload", r.payload},
                {"decision", r.decision},
                {"reason", r.reason},
                {"exported", r.exported}
            });
        }
        res.status = 200;
        res.set_content(json{{"status", "ok"}, {"records", arr}}.dump(), "application/json");
    });

    _adminServer->Get("/api/admin/syslog", [this](const httplib::Request &req, httplib::Response &res) {
        std::string token;
        if (!isValidAdminSession(req, token)) {
            res.status = 401;
            res.set_content(json{{"error", "Unauthorized"}}.dump(), "application/json");
            return;
        }

        size_t limit = 50;
        if (req.has_param("limit")) {
            try {
                limit = std::stoul(req.get_param_value("limit"));
            } catch (...) {
                limit = 50;
            }
        }

        auto events = SnmpDatabase::getInstance().getSyslogEvents(limit);
        json arr = json::array();
        for (const auto &ev : events) {
            arr.push_back({
                {"id", ev.id},
                {"timestamp", ev.timestamp},
                {"source_ip", ev.sourceIp},
                {"facility", ev.facility},
                {"severity", ev.severity},
                {"tag", ev.tag},
                {"message", ev.message},
                {"time_adjacent", ev.timeAdjacent},
                {"adjacent_audit_seq", ev.adjacentAuditSeq},
                {"raw", ev.raw}
            });
        }

        res.status = 200;
        res.set_content(json{{"status", "ok"}, {"events", arr}}.dump(), "application/json");
    });

    _adminServer->Post("/api/auth/change-password", [this](const httplib::Request &req, httplib::Response &res) {
        std::string token;
        if (!isValidAdminSession(req, token)) {
            res.status = 401;
            res.set_content(json{{"error", "Unauthorized"}}.dump(), "application/json");
            return;
        }
        json body;
        try {
            body = json::parse(req.body);
        } catch (...) {
            res.status = 400;
            res.set_content(json{{"error", "Invalid JSON"}}.dump(), "application/json");
            return;
        }
        std::string currentPass = body.value("current_password", "");
        std::string newPass = body.value("new_password", "");
        if (newPass.empty()) {
            res.status = 400;
            res.set_content(json{{"status", "error"}, {"error", "New password cannot be empty"}}.dump(), "application/json");
            return;
        }
        if (!AuthManager::getInstance().setPassword(newPass, currentPass, true)) {
            res.status = 400;
            res.set_content(json{{"status", "error"}, {"error", "Failed to change password: invalid current password"}}.dump(), "application/json");
            return;
        }
        res.status = 200;
        res.set_content(json{{"status", "ok"}}.dump(), "application/json");
    });
}

void WebServer::setupDashboardRoutes() {
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
