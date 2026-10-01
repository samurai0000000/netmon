/*
 * ZyxelObjectCmd.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXEL_OBJECT_CMD_HXX
#define NETMON_ZYXEL_OBJECT_CMD_HXX

#include "zyxel/ZyxelTypes.hxx"
#include <string>
#include <vector>

class ZyxelObjectCmd {
public:
    // Command generators - Address Objects
    static std::string cmdAddAddressHost(const std::string &name, const std::string &ip);
    static std::string cmdAddAddressRange(const std::string &name, const std::string &ipStart,
                                         const std::string &ipEnd);
    static std::string cmdAddAddressSubnet(const std::string &name, const std::string &ip,
                                          const std::string &mask);
    static std::string cmdDeleteAddress(const std::string &name);
    static std::string cmdShowAddressObjects(const std::string &name = "");

    // Command generators - Address Groups
    static std::string cmdAddAddressGroupMember(const std::string &group, const std::string &member);
    static std::string cmdDeleteAddressGroupMember(const std::string &group, const std::string &member);
    static std::string cmdShowAddressGroup(const std::string &name = "");

    // Command generators - Service Objects
    static std::string cmdAddService(const std::string &name, const std::string &proto, int port);
    static std::string cmdDeleteService(const std::string &name);
    static std::string cmdShowServiceObjects(const std::string &name = "");

    // Parsers
    static bool parseAddressObjects(const std::string &raw, std::vector<ZyxelAddressObject> &out);
    static bool parseAddressGroups(const std::string &raw, std::vector<ZyxelAddressGroup> &out);
    static bool parseServiceObjects(const std::string &raw, std::vector<ZyxelServiceObject> &out);
};

#endif /* NETMON_ZYXEL_OBJECT_CMD_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
