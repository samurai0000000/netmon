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
    static std::string cmdShowServiceGroup(const std::string &name = "");

    // Command generators - Schedule Objects
    static std::string cmdShowScheduleObjects(const std::string &name = "");
    static std::string cmdShowScheduleRecurring();
    static std::string cmdShowScheduleOneTime();
    static std::string cmdAddScheduleOneTime(const std::string &name,
                                            const std::string &startDate,
                                            const std::string &startTime,
                                            const std::string &endDate,
                                            const std::string &endTime);
    static std::string cmdDeleteSchedule(const std::string &name);
    static std::string cmdAddScheduleGroup(const std::string &name);
    static std::string cmdDeleteScheduleGroup(const std::string &name);

    // Command generators - IPv6 Address Objects & Groups
    static std::string cmdShowAddress6Objects(const std::string &name = "");
    static std::string cmdAddAddress6Host(const std::string &name, const std::string &ipv6);
    static std::string cmdAddAddress6Subnet(const std::string &name, const std::string &ipv6, int prefixLen);
    static std::string cmdAddAddress6Range(const std::string &name, const std::string &startIpv6, const std::string &endIpv6);
    static std::string cmdDeleteAddress6(const std::string &name);
    static std::string cmdShowAddress6Group(const std::string &name = "");
    static std::string cmdAddAddress6Group(const std::string &name);
    static std::string cmdDeleteAddress6Group(const std::string &name);

    // Command generators - FQDN and Geo-IP
    static std::string cmdShowFqdn();
    static std::string cmdShowFqdnObjects();
    static std::string cmdShowFqdnQueryPeriod();
    static std::string cmdShowFqdnSyncPeriod();
    static std::string cmdShowGeoIpCountryCode();
    static std::string cmdShowGeoIpDatabaseUpdate();
    static std::string cmdShowGeoIpDatabaseVersion();

    // Command generators - Service Groups
    static std::string cmdAddServiceGroup(const std::string &name);
    static std::string cmdDeleteServiceGroup(const std::string &name);

    // Command generators - ISP Accounts
    static std::string cmdShowAccountPppoe();
    static std::string cmdShowAccountPptp();
    static std::string cmdShowAccountCellular();

    // Command generators - SSL VPN Application
    static std::string cmdShowSslvpnApplication();

    // Command generators - DHCPv6 Objects
    static std::string cmdShowDhcp6Interface();
    static std::string cmdShowDhcp6LeaseObjects();
    static std::string cmdShowDhcp6RequestObjects();
    static std::string cmdShowIpv6Dhcp6Bindings();

    // Dry-fire / Invalid Object Queries
    static std::string cmdInvalidAddressObjectDryFire();
    static std::string cmdInvalidAddress6ObjectDryFire();
    static std::string cmdInvalidServiceObjectDryFire();
    static std::string cmdInvalidScheduleObjectDryFire();

    // Parsers
    static bool parseAddressObjects(const std::string &raw, std::vector<ZyxelAddressObject> &out);
    static bool parseAddress6Objects(const std::string &raw, std::vector<ZyxelAddress6Object> &out);
    static bool parseAddressGroups(const std::string &raw, std::vector<ZyxelAddressGroup> &out);
    static bool parseServiceObjects(const std::string &raw, std::vector<ZyxelServiceObject> &out);
    static bool parseServiceGroups(const std::string &raw, std::vector<ZyxelServiceGroup> &out);
    static bool parseScheduleObjects(const std::string &raw, std::vector<ZyxelScheduleObject> &out);
    static bool parseAccountPppoe(const std::string &raw, std::vector<ZyxelAccountPppoeEntry> &out);
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
