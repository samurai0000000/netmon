/*
 * TestZyxelObjectCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelObjectCmd.hxx"
#include <CppUTest/TestHarness.h>

TEST_GROUP(ZyxelObjectCmdTest) {
    void setup() {}
    void teardown() {}
};

TEST(ZyxelObjectCmdTest, CommandGeneratorsReturnExpectedStrings) {
    STRCMP_EQUAL("address-object A0 192.168.1.1",
                 ZyxelObjectCmd::cmdAddAddressHost("A0", "192.168.1.1").c_str());
    STRCMP_EQUAL("address-object A1 192.168.1.1-192.168.1.20",
                 ZyxelObjectCmd::cmdAddAddressRange("A1", "192.168.1.1", "192.168.1.20").c_str());
    STRCMP_EQUAL("address-object A2 192.168.1.0/24",
                 ZyxelObjectCmd::cmdAddAddressSubnet("A2", "192.168.1.0", "24").c_str());
    STRCMP_EQUAL("no address-object A0",
                 ZyxelObjectCmd::cmdDeleteAddress("A0").c_str());
    STRCMP_EQUAL("show address-object",
                 ZyxelObjectCmd::cmdShowAddressObjects().c_str());
    STRCMP_EQUAL("show address-object A0",
                 ZyxelObjectCmd::cmdShowAddressObjects("A0").c_str());

    STRCMP_EQUAL("address-group BlkGroup A0",
                 ZyxelObjectCmd::cmdAddAddressGroupMember("BlkGroup", "A0").c_str());
    STRCMP_EQUAL("no address-group BlkGroup A0",
                 ZyxelObjectCmd::cmdDeleteAddressGroupMember("BlkGroup", "A0").c_str());

    STRCMP_EQUAL("service-object Svc1 tcp eq 8080",
                 ZyxelObjectCmd::cmdAddService("Svc1", "tcp", 8080).c_str());
    STRCMP_EQUAL("no service-object Svc1",
                 ZyxelObjectCmd::cmdDeleteService("Svc1").c_str());

    STRCMP_EQUAL("show object-group address",
                 ZyxelObjectCmd::cmdShowAddressGroup("").c_str());
    STRCMP_EQUAL("show object-group address RD",
                 ZyxelObjectCmd::cmdShowAddressGroup("RD").c_str());
    STRCMP_EQUAL("show object-group service",
                 ZyxelObjectCmd::cmdShowServiceGroup("").c_str());
    STRCMP_EQUAL("show object-group service SG1",
                 ZyxelObjectCmd::cmdShowServiceGroup("SG1").c_str());
}

TEST(ZyxelObjectCmdTest, ParseAddressObjectsManualPage461Transcript) {
    // Official transcript from manual page 461
    std::string raw =
        "Router(config)# show address-object\n"
        "Object name                     Type    Address                         Ref.\n"
        "=====================================================================\n"
        "A0                              HOST    192.168.1.1                      0\n"
        "A1                              RANGE   192.168.1.1-192.168.1.20         0\n"
        "A2                              SUBNET  192.168.1.0/24                   1\n";

    std::vector<ZyxelAddressObject> objects;
    CHECK_TRUE(ZyxelObjectCmd::parseAddressObjects(raw, objects));
    LONGS_EQUAL(3, objects.size());

    STRCMP_EQUAL("A0", objects[0].name.c_str());
    STRCMP_EQUAL("HOST", objects[0].type.c_str());
    STRCMP_EQUAL("192.168.1.1", objects[0].ip.c_str());
    LONGS_EQUAL(0, objects[0].refCount);

    STRCMP_EQUAL("A1", objects[1].name.c_str());
    STRCMP_EQUAL("RANGE", objects[1].type.c_str());
    STRCMP_EQUAL("192.168.1.1", objects[1].ip.c_str());
    STRCMP_EQUAL("192.168.1.20", objects[1].secondaryIpOrMask.c_str());

    STRCMP_EQUAL("A2", objects[2].name.c_str());
    STRCMP_EQUAL("SUBNET", objects[2].type.c_str());
    STRCMP_EQUAL("192.168.1.0", objects[2].ip.c_str());
    STRCMP_EQUAL("24", objects[2].secondaryIpOrMask.c_str());
    LONGS_EQUAL(1, objects[2].refCount);
}

TEST(ZyxelObjectCmdTest, ParseAddressGroupsTableFormat) {
    std::string raw =
        "Address Group                   Members\n"
        "=====================================================================\n"
        "Blocked_Hosts                   A0, A1, NETMON_BLK_192_168_8_50\n";

    std::vector<ZyxelAddressGroup> groups;
    CHECK_TRUE(ZyxelObjectCmd::parseAddressGroups(raw, groups));
    LONGS_EQUAL(1, groups.size());
    STRCMP_EQUAL("Blocked_Hosts", groups[0].name.c_str());
    LONGS_EQUAL(3, groups[0].members.size());
    STRCMP_EQUAL("A0", groups[0].members[0].c_str());
    STRCMP_EQUAL("A1", groups[0].members[1].c_str());
    STRCMP_EQUAL("NETMON_BLK_192_168_8_50", groups[0].members[2].c_str());
}

TEST(ZyxelObjectCmdTest, ParseServiceObjectsTableFormat) {
    std::string raw =
        "Service name                    Protocol  Port                           Ref.\n"
        "=====================================================================\n"
        "HTTP                            TCP       80                             5\n"
        "CustomWeb                       TCP       8000-8080                      2\n";

    std::vector<ZyxelServiceObject> svcs;
    CHECK_TRUE(ZyxelObjectCmd::parseServiceObjects(raw, svcs));
    LONGS_EQUAL(2, svcs.size());

    STRCMP_EQUAL("HTTP", svcs[0].name.c_str());
    STRCMP_EQUAL("TCP", svcs[0].protocol.c_str());
    LONGS_EQUAL(80, svcs[0].portStart);
    LONGS_EQUAL(80, svcs[0].portEnd);
    LONGS_EQUAL(5, svcs[0].refCount);

    STRCMP_EQUAL("CustomWeb", svcs[1].name.c_str());
    LONGS_EQUAL(8000, svcs[1].portStart);
    LONGS_EQUAL(8080, svcs[1].portEnd);
    LONGS_EQUAL(2, svcs[1].refCount);
}

TEST(ZyxelObjectCmdTest, Envelope4CommandGeneratorsReturnExpectedStrings) {
    STRCMP_EQUAL("show schedule-object",
                 ZyxelObjectCmd::cmdShowScheduleObjects().c_str());
    STRCMP_EQUAL("show schedule-object S1",
                 ZyxelObjectCmd::cmdShowScheduleObjects("S1").c_str());
    STRCMP_EQUAL("show schedule-object recurring",
                 ZyxelObjectCmd::cmdShowScheduleRecurring().c_str());
    STRCMP_EQUAL("show schedule-object one-time",
                 ZyxelObjectCmd::cmdShowScheduleOneTime().c_str());
    STRCMP_EQUAL("schedule-object S1 2026-12-31 00:00 2026-12-31 23:59",
                 ZyxelObjectCmd::cmdAddScheduleOneTime("S1", "2026-12-31", "00:00", "2026-12-31", "23:59").c_str());
    STRCMP_EQUAL("no schedule-object S1",
                 ZyxelObjectCmd::cmdDeleteSchedule("S1").c_str());
    STRCMP_EQUAL("object-group schedule SG1",
                 ZyxelObjectCmd::cmdAddScheduleGroup("SG1").c_str());
    STRCMP_EQUAL("no object-group schedule SG1",
                 ZyxelObjectCmd::cmdDeleteScheduleGroup("SG1").c_str());

    STRCMP_EQUAL("show address6-object",
                 ZyxelObjectCmd::cmdShowAddress6Objects().c_str());
    STRCMP_EQUAL("show address6-object A6",
                 ZyxelObjectCmd::cmdShowAddress6Objects("A6").c_str());
    STRCMP_EQUAL("address6-object A6 2001:db8::1",
                 ZyxelObjectCmd::cmdAddAddress6Host("A6", "2001:db8::1").c_str());
    STRCMP_EQUAL("address6-object S6 2001:db8:1::/64",
                 ZyxelObjectCmd::cmdAddAddress6Subnet("S6", "2001:db8:1::", 64).c_str());
    STRCMP_EQUAL("address6-object R6 2001:db8::1-2001:db8::10",
                 ZyxelObjectCmd::cmdAddAddress6Range("R6", "2001:db8::1", "2001:db8::10").c_str());
    STRCMP_EQUAL("no address6-object A6",
                 ZyxelObjectCmd::cmdDeleteAddress6("A6").c_str());
    STRCMP_EQUAL("show object-group address6",
                 ZyxelObjectCmd::cmdShowAddress6Group().c_str());
    STRCMP_EQUAL("show object-group address6 G6",
                 ZyxelObjectCmd::cmdShowAddress6Group("G6").c_str());
    STRCMP_EQUAL("object-group address6 G6",
                 ZyxelObjectCmd::cmdAddAddress6Group("G6").c_str());
    STRCMP_EQUAL("no object-group address6 G6",
                 ZyxelObjectCmd::cmdDeleteAddress6Group("G6").c_str());

    STRCMP_EQUAL("show fqdn",
                 ZyxelObjectCmd::cmdShowFqdn().c_str());
    STRCMP_EQUAL("show fqdn-object all",
                 ZyxelObjectCmd::cmdShowFqdnObjects().c_str());
    STRCMP_EQUAL("show fqdn-object query-period",
                 ZyxelObjectCmd::cmdShowFqdnQueryPeriod().c_str());
    STRCMP_EQUAL("show fqdn-object sync-period",
                 ZyxelObjectCmd::cmdShowFqdnSyncPeriod().c_str());
    STRCMP_EQUAL("show geo-ip country-code",
                 ZyxelObjectCmd::cmdShowGeoIpCountryCode().c_str());
    STRCMP_EQUAL("show geo-ip database update",
                 ZyxelObjectCmd::cmdShowGeoIpDatabaseUpdate().c_str());
    STRCMP_EQUAL("show geo-ip database version",
                 ZyxelObjectCmd::cmdShowGeoIpDatabaseVersion().c_str());

    STRCMP_EQUAL("object-group service SG1",
                 ZyxelObjectCmd::cmdAddServiceGroup("SG1").c_str());
    STRCMP_EQUAL("no object-group service SG1",
                 ZyxelObjectCmd::cmdDeleteServiceGroup("SG1").c_str());

    STRCMP_EQUAL("show account pppoe",
                 ZyxelObjectCmd::cmdShowAccountPppoe().c_str());
    STRCMP_EQUAL("show account pptp",
                 ZyxelObjectCmd::cmdShowAccountPptp().c_str());
    STRCMP_EQUAL("show account cellular",
                 ZyxelObjectCmd::cmdShowAccountCellular().c_str());

    STRCMP_EQUAL("show sslvpn application",
                 ZyxelObjectCmd::cmdShowSslvpnApplication().c_str());

    STRCMP_EQUAL("show dhcp6 interface",
                 ZyxelObjectCmd::cmdShowDhcp6Interface().c_str());
    STRCMP_EQUAL("show dhcp6 lease-object",
                 ZyxelObjectCmd::cmdShowDhcp6LeaseObjects().c_str());
    STRCMP_EQUAL("show dhcp6 request-object",
                 ZyxelObjectCmd::cmdShowDhcp6RequestObjects().c_str());
    STRCMP_EQUAL("show ipv6 dhcp6 binding",
                 ZyxelObjectCmd::cmdShowIpv6Dhcp6Bindings().c_str());

    STRCMP_EQUAL("show address-object NETMON_NONEXISTENT_ADDR_999",
                 ZyxelObjectCmd::cmdInvalidAddressObjectDryFire().c_str());
    STRCMP_EQUAL("show address6-object NETMON_NONEXISTENT_ADDR6_999",
                 ZyxelObjectCmd::cmdInvalidAddress6ObjectDryFire().c_str());
    STRCMP_EQUAL("show service-object NETMON_NONEXISTENT_SVC_999",
                 ZyxelObjectCmd::cmdInvalidServiceObjectDryFire().c_str());
    STRCMP_EQUAL("show schedule-object NETMON_NONEXISTENT_SCHED_999",
                 ZyxelObjectCmd::cmdInvalidScheduleObjectDryFire().c_str());
}

TEST(ZyxelObjectCmdTest, ParseScheduleObjectsTableFormat) {
    std::string raw =
        "Object name                     Type      Start/End                         Ref.\n"
        "===============================================================================\n"
        "NETMON_QA_SCHED                 Once      2026-12-31 00:00/2026-12-31 23:59 0\n";

    std::vector<ZyxelScheduleObject> schedules;
    CHECK_TRUE(ZyxelObjectCmd::parseScheduleObjects(raw, schedules));
    LONGS_EQUAL(1, schedules.size());
    STRCMP_EQUAL("NETMON_QA_SCHED", schedules[0].name.c_str());
    STRCMP_EQUAL("Once", schedules[0].type.c_str());
    STRCMP_EQUAL("2026-12-31 00:00/2026-12-31 23:59", schedules[0].startEnd.c_str());
    LONGS_EQUAL(0, schedules[0].refCount);

    // Empty table is also valid (returns true, size 0)
    std::string emptyRaw =
        "Object name                     Type      Start/End                         Ref.\n"
        "===============================================================================\n";
    std::vector<ZyxelScheduleObject> emptySchedules;
    CHECK_TRUE(ZyxelObjectCmd::parseScheduleObjects(emptyRaw, emptySchedules));
    LONGS_EQUAL(0, emptySchedules.size());
}

TEST(ZyxelObjectCmdTest, ParseAddress6ObjectsTableFormat) {
    std::string raw =
        "Object name                     Type               Address Type     Index\n"
        "Address                                                                          \n"
        "Note            Ref.\n"
        "===============================================================================\n"
        "LAN1_SUBNET_STATIC              INTERFACE SUBNET   STATIC           1     \n"
        "::/0                                                                            \n"
        "lan1            0       \n"
        "NETMON_QA_ADDR6                 HOST               \n"
        "2001:db8::1\n"
        "                0\n";

    std::vector<ZyxelAddress6Object> objects;
    CHECK_TRUE(ZyxelObjectCmd::parseAddress6Objects(raw, objects));
    LONGS_EQUAL(2, objects.size());

    STRCMP_EQUAL("LAN1_SUBNET_STATIC", objects[0].name.c_str());
    STRCMP_EQUAL("INTERFACE SUBNET", objects[0].type.c_str());
    STRCMP_EQUAL("::/0", objects[0].address.c_str());
    LONGS_EQUAL(0, objects[0].refCount);

    STRCMP_EQUAL("NETMON_QA_ADDR6", objects[1].name.c_str());
    STRCMP_EQUAL("HOST", objects[1].type.c_str());
    STRCMP_EQUAL("2001:db8::1", objects[1].address.c_str());
    LONGS_EQUAL(0, objects[1].refCount);
}

TEST(ZyxelObjectCmdTest, ParseServiceGroupsTableFormat) {
    std::string rawList =
        "Group name                      Reference            Family              \n"
        "Description\n"
        "===============================================================================\n"
        "CU-SEEME                        0                    Common              \n"
        "DNS                             4                    Common              \n";

    std::vector<ZyxelServiceGroup> groups;
    CHECK_TRUE(ZyxelObjectCmd::parseServiceGroups(rawList, groups));
    LONGS_EQUAL(2, groups.size());
    STRCMP_EQUAL("CU-SEEME", groups[0].name.c_str());
    STRCMP_EQUAL("DNS", groups[1].name.c_str());

    std::string rawDetail =
        "Object/Group name               Type   Reference\n"
        "===============================================================================\n"
        "DNS_TCP                         Object 2         \n"
        "DNS_UDP                         Object 2         \n";

    std::vector<ZyxelServiceGroup> detail;
    CHECK_TRUE(ZyxelObjectCmd::parseServiceGroups(rawDetail, detail));
    LONGS_EQUAL(1, detail.size());
    LONGS_EQUAL(2, detail[0].members.size());
    STRCMP_EQUAL("DNS_TCP", detail[0].members[0].c_str());
    STRCMP_EQUAL("DNS_UDP", detail[0].members[1].c_str());
}

TEST(ZyxelObjectCmdTest, ParseAccountPppoeTranscript) {
    std::string raw =
        "PPPoE account:\n"
        "  profile name: WAN1_PPPoE_ACCOUNT\n"
        "    protocol: pppoe\n"
        "    username: user@example.com\n"
        "    authentication type: pap\n"
        "    password: dummy_password\n"
        "    service name: ExampleISP\n"
        "    compression: no\n"
        "    idle timeout: 0\n"
        "  profile name: WAN2_PPPoE_ACCOUNT\n"
        "    protocol: pppoe\n"
        "    username: user2@example.com\n"
        "    authentication type: pap\n"
        "    password: dummy_password\n"
        "    service name: ExampleISP\n"
        "    compression: yes\n"
        "    idle timeout: 100\n"
        "total pppoe account file: 2\n";

    std::vector<ZyxelAccountPppoeEntry> entries;
    CHECK_TRUE(ZyxelObjectCmd::parseAccountPppoe(raw, entries));
    LONGS_EQUAL(2, entries.size());

    STRCMP_EQUAL("WAN1_PPPoE_ACCOUNT", entries[0].profileName.c_str());
    STRCMP_EQUAL("pppoe", entries[0].protocol.c_str());
    STRCMP_EQUAL("user@example.com", entries[0].username.c_str());
    STRCMP_EQUAL("pap", entries[0].authType.c_str());
    STRCMP_EQUAL("ExampleISP", entries[0].serviceName.c_str());
    CHECK_FALSE(entries[0].compression);
    LONGS_EQUAL(0, entries[0].idleTimeout);

    STRCMP_EQUAL("WAN2_PPPoE_ACCOUNT", entries[1].profileName.c_str());
    CHECK_TRUE(entries[1].compression);
    LONGS_EQUAL(100, entries[1].idleTimeout);
}

TEST(ZyxelObjectCmdTest, ParseErrorsOnCorruptedBuffers) {
    std::vector<ZyxelAddressObject> objects;
    CHECK_FALSE(ZyxelObjectCmd::parseAddressObjects("Random garbage % syntax error", objects));

    std::vector<ZyxelAddressGroup> groups;
    CHECK_FALSE(ZyxelObjectCmd::parseAddressGroups("No group entries", groups));

    std::vector<ZyxelServiceObject> svcs;
    CHECK_FALSE(ZyxelObjectCmd::parseServiceObjects("No service entries", svcs));

    std::vector<ZyxelScheduleObject> schedules;
    CHECK_FALSE(ZyxelObjectCmd::parseScheduleObjects("ERROR: The object does not exist.\nretval = -43000", schedules));

    std::vector<ZyxelAddress6Object> addr6;
    CHECK_FALSE(ZyxelObjectCmd::parseAddress6Objects("ERROR: The object does not exist.\nretval = -43000", addr6));

    std::vector<ZyxelAccountPppoeEntry> pppoe;
    CHECK_FALSE(ZyxelObjectCmd::parseAccountPppoe("ERROR: The object does not exist.\nretval = -43000", pppoe));
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
