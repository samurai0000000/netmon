/*
 * Config.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "Config.hxx"
#include <cstdlib>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

Config &Config::getInstance() {
    static Config instance;
    return instance;
}

Config::Config()
    : _configPath("")
    , _interface("br0")
    , _gatewayHost("192.168.8.39")
    , _gatewayPort(3885)
    , _reconnectIntervalSec(5)
    , _devicesFile("~/.config/netmon/devices.cfg")
    , _logLevel("info")
    , _allowAiBlockIp(false)
    , _allowAiRawExec(false) {
}

std::string Config::resolveHomePath(const std::string &path) {
    if (path.empty() || path[0] != '~') {
        return path;
    }

    const char *home = getenv("HOME");
    if (!home) {
        return path;
    }

    if (path == "~") {
        return std::string(home);
    }

    if (path[1] == '/') {
        return std::string(home) + path.substr(1);
    }

    return path;
}

bool Config::load(const std::string &customPath) {
    if (!customPath.empty()) {
        _configPath = resolveHomePath(customPath);
    } else {
        std::string homeDir = resolveHomePath("~/.config/netmon");
        mkdir(homeDir.c_str(), 0755);
        _configPath = homeDir + "/netmon.cfg";
    }

    libconfig::Config cfg;

    try {
        cfg.readFile(_configPath.c_str());
    } catch (const libconfig::FileIOException &) {
        // File does not exist yet, save default configuration
        return save();
    } catch (const libconfig::ParseException &pex) {
        std::cerr << "Config parse error at " << pex.getFile() << ":"
                  << pex.getLine() << " - " << pex.getError() << std::endl;
        return false;
    }

    try {
        cfg.lookupValue("interface", _interface);
        cfg.lookupValue("gateway_host", _gatewayHost);
        cfg.lookupValue("gateway_port", _gatewayPort);
        cfg.lookupValue("reconnect_interval_sec", _reconnectIntervalSec);
        cfg.lookupValue("devices_file", _devicesFile);
        cfg.lookupValue("log_level", _logLevel);
        cfg.lookupValue("allow_ai_block_ip", _allowAiBlockIp);
        cfg.lookupValue("allow_ai_raw_exec", _allowAiRawExec);
    } catch (const libconfig::SettingNotFoundException &) {
        // Some settings were missing, keep defaults
    }

    return true;
}

bool Config::save() {
    if (_configPath.empty()) {
        _configPath = resolveHomePath("~/.config/netmon/netmon.cfg");
    }

    libconfig::Config cfg;
    libconfig::Setting &root = cfg.getRoot();

    try {
        if (!root.exists("interface")) {
            root.add("interface", libconfig::Setting::TypeString) = _interface;
        } else {
            root["interface"] = _interface;
        }

        if (!root.exists("gateway_host")) {
            root.add("gateway_host", libconfig::Setting::TypeString) = _gatewayHost;
        } else {
            root["gateway_host"] = _gatewayHost;
        }

        if (!root.exists("gateway_port")) {
            root.add("gateway_port", libconfig::Setting::TypeInt) = _gatewayPort;
        } else {
            root["gateway_port"] = _gatewayPort;
        }

        if (!root.exists("reconnect_interval_sec")) {
            root.add("reconnect_interval_sec", libconfig::Setting::TypeInt) = _reconnectIntervalSec;
        } else {
            root["reconnect_interval_sec"] = _reconnectIntervalSec;
        }

        if (!root.exists("devices_file")) {
            root.add("devices_file", libconfig::Setting::TypeString) = _devicesFile;
        } else {
            root["devices_file"] = _devicesFile;
        }

        if (!root.exists("log_level")) {
            root.add("log_level", libconfig::Setting::TypeString) = _logLevel;
        } else {
            root["log_level"] = _logLevel;
        }

        if (!root.exists("allow_ai_block_ip")) {
            root.add("allow_ai_block_ip", libconfig::Setting::TypeBoolean) = _allowAiBlockIp;
        } else {
            root["allow_ai_block_ip"] = _allowAiBlockIp;
        }

        if (!root.exists("allow_ai_raw_exec")) {
            root.add("allow_ai_raw_exec", libconfig::Setting::TypeBoolean) = _allowAiRawExec;
        } else {
            root["allow_ai_raw_exec"] = _allowAiRawExec;
        }

        cfg.writeFile(_configPath.c_str());
        return true;
    } catch (const std::exception &ex) {
        std::cerr << "Failed to write config: " << ex.what() << std::endl;
        return false;
    }
}

const std::string &Config::getInterface() const {
    return _interface;
}

void Config::setInterface(const std::string &interface) {
    _interface = interface;
}

const std::string &Config::getGatewayHost() const {
    return _gatewayHost;
}

void Config::setGatewayHost(const std::string &host) {
    _gatewayHost = host;
}

int Config::getGatewayPort() const {
    return _gatewayPort;
}

void Config::setGatewayPort(int port) {
    _gatewayPort = port;
}

int Config::getReconnectIntervalSec() const {
    return _reconnectIntervalSec;
}

void Config::setReconnectIntervalSec(int sec) {
    _reconnectIntervalSec = sec;
}

const std::string &Config::getDevicesFile() const {
    return _devicesFile;
}

void Config::setDevicesFile(const std::string &file) {
    _devicesFile = file;
}

const std::string &Config::getLogLevel() const {
    return _logLevel;
}

void Config::setLogLevel(const std::string &level) {
    _logLevel = level;
}

bool Config::getAllowAiBlockIp() const {
    return _allowAiBlockIp;
}

void Config::setAllowAiBlockIp(bool allow) {
    _allowAiBlockIp = allow;
}

bool Config::getAllowAiRawExec() const {
    return _allowAiRawExec;
}

void Config::setAllowAiRawExec(bool allow) {
    _allowAiRawExec = allow;
}

const std::string &Config::getConfigPath() const {
    return _configPath;
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
