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
    , _running(false)
    , _server(std::make_unique<httplib::Server>()) {
}

WebServer::~WebServer() {
    stop();
    join();
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
    _server->listen(_bindAddress.c_str(), _port);
    _running.store(false);
}

void WebServer::setupRoutes() {
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
    _server->Get("/", [serveFileOrFallback](const httplib::Request &, httplib::Response &res) {
        serveFileOrFallback("web/index.html", assets::INDEX_HTML, "text/html", res);
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
