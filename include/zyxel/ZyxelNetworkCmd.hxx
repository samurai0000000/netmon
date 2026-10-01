/*
 * ZyxelNetworkCmd.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXEL_NETWORK_CMD_HXX
#define NETMON_ZYXEL_NETWORK_CMD_HXX

#include "zyxel/ZyxelTypes.hxx"
#include <string>
#include <vector>

class ZyxelNetworkCmd {
public:
    // Command generators
    static std::string cmdShowInterfaces();
    static std::string cmdShowInterface(const std::string &name);
    static std::string cmdShowIpRoute();
    static std::string cmdAddRoute(const std::string &dest, const std::string &mask,
                                  const std::string &gw, int metric = 1);
    static std::string cmdDeleteRoute(const std::string &dest, const std::string &mask,
                                     const std::string &gw);
    static std::string cmdShowZones();
    static std::string cmdShowArp();

    // Parsers
    static bool parseInterfaces(const std::string &raw, std::vector<ZyxelInterfaceInfo> &out);
    static bool parseIpRoutes(const std::string &raw, std::vector<ZyxelRouteEntry> &out);
    static bool parseZones(const std::string &raw, std::vector<ZyxelZoneInfo> &out);
    static bool parseArp(const std::string &raw, std::vector<ZyxelArpEntry> &out);
};

#endif /* NETMON_ZYXEL_NETWORK_CMD_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
