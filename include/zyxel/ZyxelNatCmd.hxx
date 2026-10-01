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
    // Command generators
    static std::string cmdShowVirtualServers(const std::string &name = "");
    static std::string cmdAddVirtualServer(const ZyxelVirtualServerRule &rule);
    static std::string cmdDeleteVirtualServer(const std::string &name);

    // Parsers
    static bool parseVirtualServers(const std::string &raw,
                                   std::vector<ZyxelVirtualServerRule> &out);
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
