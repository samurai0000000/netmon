/*
 * ZyxelDriver.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXELDRIVER_HXX
#define NETMON_ZYXELDRIVER_HXX

#include "RouterDriver.hxx"

class ZyxelDriver : public RouterDriver {
public:
    static ZyxelDriver &getInstance();

    ZyxelDriver();
    virtual ~ZyxelDriver() override = default;

    virtual nlohmann::json getStatus() override;
    virtual nlohmann::json getSessions() override;
    virtual nlohmann::json blockIp(const std::string &ip, const std::string &reason) override;
    virtual nlohmann::json unblockIp(const std::string &ip) override;

    bool isConfigured() const;

private:
    bool _configured;
};

#endif /* NETMON_ZYXELDRIVER_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
