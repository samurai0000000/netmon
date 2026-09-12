/*
 * ZyxelDriver.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "ZyxelDriver.hxx"

ZyxelDriver::ZyxelDriver()
    : _configured(false) {
}

bool ZyxelDriver::isConfigured() const {
    return _configured;
}

nlohmann::json ZyxelDriver::getStatus() {
    nlohmann::json res;
    if (!_configured) {
        res["status"] = "unconfigured";
        res["error"] = "Zyxel USG router driver not configured in netmon.cfg";
        res["model"] = "Zyxel USG (Pending credentials)";
        return res;
    }

    res["status"] = "ok";
    return res;
}

nlohmann::json ZyxelDriver::getSessions() {
    nlohmann::json res;
    if (!_configured) {
        res["status"] = "unconfigured";
        res["error"] = "Zyxel USG router driver not configured in netmon.cfg";
        res["sessions"] = nlohmann::json::array();
        return res;
    }

    res["status"] = "ok";
    return res;
}

nlohmann::json ZyxelDriver::blockIp(const std::string &ip, const std::string &reason) {
    nlohmann::json res;
    if (!_configured) {
        res["status"] = "unconfigured";
        res["error"] = "Zyxel USG router driver not configured in netmon.cfg";
        res["target_ip"] = ip;
        res["reason"] = reason;
        return res;
    }

    res["status"] = "ok";
    return res;
}

nlohmann::json ZyxelDriver::unblockIp(const std::string &ip) {
    nlohmann::json res;
    if (!_configured) {
        res["status"] = "unconfigured";
        res["error"] = "Zyxel USG router driver not configured in netmon.cfg";
        res["target_ip"] = ip;
        return res;
    }

    res["status"] = "ok";
    return res;
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
