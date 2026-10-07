/*
 * ZyxelNatCmd.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXEL_NAT_CMD_HXX
#define NETMON_ZYXEL_NAT_CMD_HXX

#include "zyxel/ZyxelTypes.hxx"
#include <string>
#include <vector>

class ZyxelNatCmd {
public:
    // Command generators - Virtual Servers
    static std::string cmdShowVirtualServers(const std::string &name = "");
    static std::string cmdShowVirtualServerStatus();
    static std::string cmdShowVirtualServerLoadBalancer(const std::string &name = "");
    static std::string cmdAddVirtualServer(const ZyxelVirtualServerRule &rule);
    static std::string cmdDeleteVirtualServer(const std::string &name);
    static std::string cmdInvalidVirtualServerDryFire();

    // Command generators - DDNS
    static std::string cmdShowDdns(const std::string &profileName = "");
    static std::string cmdShowDdnsStatus();
    static std::string cmdAddDdnsProfile(const std::string &name);
    static std::string cmdDeleteDdnsProfile(const std::string &name);
    static std::string cmdInvalidDdnsDryFire();

    // Command generators - HTTP Redirect
    static std::string cmdShowHttpRedirect(const std::string &desc = "");
    static std::string cmdAddHttpRedirect(const std::string &desc);
    static std::string cmdDeleteHttpRedirect(const std::string &desc);
    static std::string cmdInvalidHttpRedirectDryFire();

    // Command generators - Redirect Service
    static std::string cmdShowRedirectService(int ruleIndex = 0);
    static std::string cmdInvalidRedirectServiceDryFire();

    // Command generators - ALG
    static std::string cmdShowAlg(const std::string &proto);
    static std::string cmdShowAlgFtp();
    static std::string cmdShowAlgSip();
    static std::string cmdShowAlgH323();
    static std::string cmdInvalidAlgDryFire();

    // Command generators - UPnP
    static std::string cmdShowUpnp();
    static std::string cmdShowUpnpIgd();
    static std::string cmdShowNatPmp();
    static std::string cmdInvalidUpnpDryFire();

    // Parsers
    static bool parseVirtualServers(const std::string &raw,
                                   std::vector<ZyxelVirtualServerRule> &out);
    static bool parseDdnsStatus(const std::string &raw,
                                std::vector<ZyxelDdnsStatusEntry> &out);
    static bool parseAlgStatus(const std::string &raw,
                               const std::string &proto,
                               ZyxelAlgStatus &out);
};

#endif /* NETMON_ZYXEL_NAT_CMD_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
