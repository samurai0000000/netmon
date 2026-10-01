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

TEST(ZyxelObjectCmdTest, ParseErrorsOnCorruptedBuffers) {
    std::vector<ZyxelAddressObject> objects;
    CHECK_FALSE(ZyxelObjectCmd::parseAddressObjects("Random garbage % syntax error", objects));

    std::vector<ZyxelAddressGroup> groups;
    CHECK_FALSE(ZyxelObjectCmd::parseAddressGroups("No group entries", groups));

    std::vector<ZyxelServiceObject> svcs;
    CHECK_FALSE(ZyxelObjectCmd::parseServiceObjects("No service entries", svcs));
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
