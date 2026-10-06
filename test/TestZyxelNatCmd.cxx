/*
 * TestZyxelNatCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelNatCmd.hxx"
#include <CppUTest/TestHarness.h>

TEST_GROUP(ZyxelNatCmdTest) {
    void setup() {}
    void teardown() {}
};

TEST(ZyxelNatCmdTest, CommandGeneratorsReturnExpectedStrings) {
    STRCMP_EQUAL("show ip virtual-server", ZyxelNatCmd::cmdShowVirtualServers().c_str());
    STRCMP_EQUAL("show ip virtual-server WebServer",
                 ZyxelNatCmd::cmdShowVirtualServers("WebServer").c_str());
    STRCMP_EQUAL("no ip virtual-server WebServer",
                 ZyxelNatCmd::cmdDeleteVirtualServer("WebServer").c_str());

    ZyxelVirtualServerRule rule;
    rule.name = "HTTP_Rule";
    rule.interface = "wan1";
    rule.originalIp = "1.1.1.2";
    rule.mapToIp = "192.168.3.7";
    rule.originalService = "HTTP";
    rule.mappedService = "HTTP";
    rule.active = true;

    STRCMP_EQUAL("ip virtual-server HTTP_Rule interface wan1 original-ip 1.1.1.2 map-to 192.168.3.7 map-type original-service HTTP mapped-service HTTP",
                 ZyxelNatCmd::cmdAddVirtualServer(rule).c_str());
}

TEST(ZyxelNatCmdTest, ParseVirtualServersManualPage192Transcript) {
    // Official transcript from manual page 192
    std::string raw =
        "Router(config)# show ip virtual-server\n"
        "virtual server: WAN-LAN_H323\n"
        "  Index: 1\n"
        "  active: yes\n"
        "  interface: wan1\n"
        "  NAT-loopback active: yes\n"
        "  NAT 1-1: no\n"
        "  original IP: 10.0.0.8\n"
        "  mapped IP: 192.168.1.56\n"
        "  mapping type: port\n"
        "  protocol type: tcp\n"
        "  original service: H323\n"
        "  mapped service: H323\n";

    std::vector<ZyxelVirtualServerRule> rules;
    CHECK_TRUE(ZyxelNatCmd::parseVirtualServers(raw, rules));
    LONGS_EQUAL(1, rules.size());

    STRCMP_EQUAL("WAN-LAN_H323", rules[0].name.c_str());
    LONGS_EQUAL(1, rules[0].index);
    CHECK_TRUE(rules[0].active);
    STRCMP_EQUAL("wan1", rules[0].interface.c_str());
    STRCMP_EQUAL("10.0.0.8", rules[0].originalIp.c_str());
    STRCMP_EQUAL("192.168.1.56", rules[0].mapToIp.c_str());
    STRCMP_EQUAL("H323", rules[0].originalService.c_str());
    STRCMP_EQUAL("H323", rules[0].mappedService.c_str());
}

TEST(ZyxelNatCmdTest, ParseVirtualServersTableFormat) {
    std::string raw =
        "Name            Interface  Original IP     Mapped IP       Status\n"
        "=================================================================\n"
        "HTTP_Fwd        wan1       1.1.1.2         192.168.3.7     yes\n";

    std::vector<ZyxelVirtualServerRule> rules;
    CHECK_TRUE(ZyxelNatCmd::parseVirtualServers(raw, rules));
    LONGS_EQUAL(1, rules.size());

    STRCMP_EQUAL("HTTP_Fwd", rules[0].name.c_str());
    STRCMP_EQUAL("wan1", rules[0].interface.c_str());
    STRCMP_EQUAL("1.1.1.2", rules[0].originalIp.c_str());
    STRCMP_EQUAL("192.168.3.7", rules[0].mapToIp.c_str());
    CHECK_TRUE(rules[0].active);
}

TEST(ZyxelNatCmdTest, ParseErrorsOnCorruptedBuffers) {
    std::vector<ZyxelVirtualServerRule> rules;
    CHECK_FALSE(ZyxelNatCmd::parseVirtualServers("Random garbage % syntax error", rules));
    CHECK_FALSE(ZyxelNatCmd::parseVirtualServers("No virtual servers configured", rules));
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
