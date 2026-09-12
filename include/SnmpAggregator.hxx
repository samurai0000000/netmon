/*
 * SnmpAggregator.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_SNMPAGGREGATOR_HXX
#define NETMON_SNMPAGGREGATOR_HXX

#include <string>
#include <nlohmann/json.hpp>

class SnmpAggregator {
public:
    static SnmpAggregator &getInstance();

    nlohmann::json getDeviceMetrics(const std::string &targetIp);

private:
    SnmpAggregator();
    ~SnmpAggregator() = default;
    SnmpAggregator(const SnmpAggregator &) = delete;
    SnmpAggregator &operator=(const SnmpAggregator &) = delete;

    bool _configured;
};

#endif /* NETMON_SNMPAGGREGATOR_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
