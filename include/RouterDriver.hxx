/*
 * RouterDriver.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ROUTERDRIVER_HXX
#define NETMON_ROUTERDRIVER_HXX

#include <string>
#include <nlohmann/json.hpp>

class RouterDriver {
public:
    virtual ~RouterDriver() = default;

    virtual nlohmann::json getStatus() = 0;
    virtual nlohmann::json getSessions() = 0;
    virtual nlohmann::json blockIp(const std::string &ip, const std::string &reason) = 0;
    virtual nlohmann::json unblockIp(const std::string &ip) = 0;
};

#endif /* NETMON_ROUTERDRIVER_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
