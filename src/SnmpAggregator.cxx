/*
 * SnmpAggregator.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "SnmpAggregator.hxx"

SnmpAggregator &SnmpAggregator::getInstance() {
    static SnmpAggregator instance;
    return instance;
}

SnmpAggregator::SnmpAggregator()
    : _configured(false) {
}

nlohmann::json SnmpAggregator::getDeviceMetrics(const std::string &targetIp) {
    nlohmann::json res;
    if (!_configured) {
        res["status"] = "unconfigured";
        res["error"] = "No SNMP targets configured in netmon.cfg";
        res["target_ip"] = targetIp;
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
