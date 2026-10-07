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

    std::vector<ZyxelDdnsStatusEntry> ddns;
    CHECK_FALSE(ZyxelNatCmd::parseDdnsStatus("Random garbage", ddns));

    ZyxelAlgStatus alg;
    CHECK_FALSE(ZyxelNatCmd::parseAlgStatus("", "ftp", alg));
}

TEST(ZyxelNatCmdTest, Envelope5CommandGenerators) {
    STRCMP_EQUAL("show ip virtual-server status",
                 ZyxelNatCmd::cmdShowVirtualServerStatus().c_str());
    STRCMP_EQUAL("show ip virtual-server load-balancer",
                 ZyxelNatCmd::cmdShowVirtualServerLoadBalancer().c_str());
    STRCMP_EQUAL("show ip virtual-server load-balancer lb1",
                 ZyxelNatCmd::cmdShowVirtualServerLoadBalancer("lb1").c_str());
    STRCMP_EQUAL("show ip virtual-server load-balancer NETMON_NONEXISTENT_VS_999",
                 ZyxelNatCmd::cmdInvalidVirtualServerDryFire().c_str());

    STRCMP_EQUAL("show ddns", ZyxelNatCmd::cmdShowDdns().c_str());
    STRCMP_EQUAL("show ddns my_profile", ZyxelNatCmd::cmdShowDdns("my_profile").c_str());
    STRCMP_EQUAL("show ddns-status", ZyxelNatCmd::cmdShowDdnsStatus().c_str());
    STRCMP_EQUAL("ip ddns profile test_ddns", ZyxelNatCmd::cmdAddDdnsProfile("test_ddns").c_str());
    STRCMP_EQUAL("no ip ddns profile test_ddns", ZyxelNatCmd::cmdDeleteDdnsProfile("test_ddns").c_str());
    STRCMP_EQUAL("show ddns NETMON_NONEXISTENT_DDNS_999", ZyxelNatCmd::cmdInvalidDdnsDryFire().c_str());

    STRCMP_EQUAL("show ip http-redirect", ZyxelNatCmd::cmdShowHttpRedirect().c_str());
    STRCMP_EQUAL("show ip http-redirect redir1", ZyxelNatCmd::cmdShowHttpRedirect("redir1").c_str());
    STRCMP_EQUAL("ip http-redirect redir1", ZyxelNatCmd::cmdAddHttpRedirect("redir1").c_str());
    STRCMP_EQUAL("no ip http-redirect redir1", ZyxelNatCmd::cmdDeleteHttpRedirect("redir1").c_str());
    STRCMP_EQUAL("show ip http-redirect NETMON_NONEXISTENT_HTTP_999",
                 ZyxelNatCmd::cmdInvalidHttpRedirectDryFire().c_str());

    STRCMP_EQUAL("show redirect-service", ZyxelNatCmd::cmdShowRedirectService().c_str());
    STRCMP_EQUAL("show redirect-service 1", ZyxelNatCmd::cmdShowRedirectService(1).c_str());
    STRCMP_EQUAL("show redirect-service 99", ZyxelNatCmd::cmdInvalidRedirectServiceDryFire().c_str());

    STRCMP_EQUAL("show alg ftp", ZyxelNatCmd::cmdShowAlgFtp().c_str());
    STRCMP_EQUAL("show alg sip", ZyxelNatCmd::cmdShowAlgSip().c_str());
    STRCMP_EQUAL("show alg h323", ZyxelNatCmd::cmdShowAlgH323().c_str());
    STRCMP_EQUAL("show alg invalid_proto_999", ZyxelNatCmd::cmdInvalidAlgDryFire().c_str());

    STRCMP_EQUAL("show upnp", ZyxelNatCmd::cmdShowUpnp().c_str());
    STRCMP_EQUAL("show upnp-igd", ZyxelNatCmd::cmdShowUpnpIgd().c_str());
    STRCMP_EQUAL("show nat-pmp", ZyxelNatCmd::cmdShowNatPmp().c_str());
    STRCMP_EQUAL("show upnp invalid_probe_999", ZyxelNatCmd::cmdInvalidUpnpDryFire().c_str());
}

TEST(ZyxelNatCmdTest, ParseDdnsStatusTranscript) {
    std::string raw =
        "No. Profile_Name                     Domain_Name\n"
        "    Effective_ip         Status      Update_time\n"
        "===============================================================================\n"
        "1   test_ddns1                       test1.example.org\n"
        "    192.0.2.10           Success     2026-09-28 02:39:17\n"
        "2   test_ddns2                       test2.example.org\n"
        "    192.0.2.20           Success     2026-10-04 04:41:05\n";

    std::vector<ZyxelDdnsStatusEntry> ddns;
    CHECK_TRUE(ZyxelNatCmd::parseDdnsStatus(raw, ddns));
    LONGS_EQUAL(2, ddns.size());

    LONGS_EQUAL(1, ddns[0].index);
    STRCMP_EQUAL("test_ddns1", ddns[0].profileName.c_str());
    STRCMP_EQUAL("test1.example.org", ddns[0].domainName.c_str());
    STRCMP_EQUAL("192.0.2.10", ddns[0].effectiveIp.c_str());
    STRCMP_EQUAL("Success", ddns[0].status.c_str());
    STRCMP_EQUAL("2026-09-28 02:39:17", ddns[0].updateTime.c_str());

    LONGS_EQUAL(2, ddns[1].index);
    STRCMP_EQUAL("test_ddns2", ddns[1].profileName.c_str());
    STRCMP_EQUAL("test2.example.org", ddns[1].domainName.c_str());
    STRCMP_EQUAL("192.0.2.20", ddns[1].effectiveIp.c_str());
    STRCMP_EQUAL("Success", ddns[1].status.c_str());
    STRCMP_EQUAL("2026-10-04 04:41:05", ddns[1].updateTime.c_str());
}

TEST(ZyxelNatCmdTest, ParseAlgStatusTranscript) {
    std::string ftpRaw =
        "active: yes\n"
        "transformation: yes\n"
        "signaling port: 21\n"
        "additional signaling port: 0\n";

    ZyxelAlgStatus ftpStatus;
    CHECK_TRUE(ZyxelNatCmd::parseAlgStatus(ftpRaw, "ftp", ftpStatus));
    STRCMP_EQUAL("ftp", ftpStatus.protocol.c_str());
    CHECK_TRUE(ftpStatus.active);
    CHECK_TRUE(ftpStatus.transformation);
    LONGS_EQUAL(21, ftpStatus.signalingPort);

    std::string sipRaw =
        "active: no\n"
        "transformation: no\n"
        "private-ip-only: no\n"
        "inactivity-timeout: no\n"
        "media inactivity timeout: 120\n"
        "signaling inactivity timeout: 1800\n"
        "direct media: yes\n"
        "direct signalling: yes\n"
        "log usb-storage: disable\n"
        "No.   Port\n"
        "===============================================================================\n"
        "1     5060\n";

    ZyxelAlgStatus sipStatus;
    CHECK_TRUE(ZyxelNatCmd::parseAlgStatus(sipRaw, "sip", sipStatus));
    STRCMP_EQUAL("sip", sipStatus.protocol.c_str());
    CHECK_FALSE(sipStatus.active);
    CHECK_FALSE(sipStatus.transformation);
    LONGS_EQUAL(5060, sipStatus.signalingPort);

    std::string h323Raw =
        "active: no\n"
        "transformation: no\n"
        "signaling port: 1720\n"
        "additional signaling port: 0\n";

    ZyxelAlgStatus h323Status;
    CHECK_TRUE(ZyxelNatCmd::parseAlgStatus(h323Raw, "h323", h323Status));
    STRCMP_EQUAL("h323", h323Status.protocol.c_str());
    CHECK_FALSE(h323Status.active);
    CHECK_FALSE(h323Status.transformation);
    LONGS_EQUAL(1720, h323Status.signalingPort);
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
