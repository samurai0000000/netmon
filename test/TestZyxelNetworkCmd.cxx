/*
 * TestZyxelNetworkCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelNetworkCmd.hxx"
#include <CppUTest/TestHarness.h>

TEST_GROUP(ZyxelNetworkCmdTest) {
    void setup() {}
    void teardown() {}
};

TEST(ZyxelNetworkCmdTest, CommandGeneratorsReturnExpectedStrings) {
    STRCMP_EQUAL("show interface summary all", ZyxelNetworkCmd::cmdShowInterfaces().c_str());
    STRCMP_EQUAL("show interface ge1", ZyxelNetworkCmd::cmdShowInterface("ge1").c_str());
    STRCMP_EQUAL("show interface base", ZyxelNetworkCmd::cmdShowInterfaceBase().c_str());
    STRCMP_EQUAL("show ip route-settings", ZyxelNetworkCmd::cmdShowIpRoute().c_str());
    STRCMP_EQUAL("ip route 10.10.10.0 255.255.255.0 192.0.2.254 1",
                 ZyxelNetworkCmd::cmdAddRoute("10.10.10.0", "255.255.255.0", "192.0.2.254", 1).c_str());
    STRCMP_EQUAL("no ip route 10.10.10.0 255.255.255.0 192.0.2.254",
                 ZyxelNetworkCmd::cmdDeleteRoute("10.10.10.0", "255.255.255.0", "192.0.2.254").c_str());
    STRCMP_EQUAL("show zone", ZyxelNetworkCmd::cmdShowZones().c_str());
    STRCMP_EQUAL("show zone LAN1", ZyxelNetworkCmd::cmdShowZone("LAN1").c_str());
    STRCMP_EQUAL("show zone default-binding", ZyxelNetworkCmd::cmdShowZoneDefaultBinding().c_str());
    STRCMP_EQUAL("show zone binding-iface", ZyxelNetworkCmd::cmdShowZoneBindingIface().c_str());
    STRCMP_EQUAL("show arp-table", ZyxelNetworkCmd::cmdShowArp().c_str());
    STRCMP_EQUAL("show arp gratuitous", ZyxelNetworkCmd::cmdShowArpGratuitous().c_str());
    STRCMP_EQUAL("show ip dhcp binding", ZyxelNetworkCmd::cmdShowIpDhcpBinding().c_str());
    STRCMP_EQUAL("show l2-isolation", ZyxelNetworkCmd::cmdShowL2Isolation().c_str());
    STRCMP_EQUAL("show l2-isolation activation", ZyxelNetworkCmd::cmdShowL2IsolationActivation().c_str());
    STRCMP_EQUAL("show l2-isolation white-list", ZyxelNetworkCmd::cmdShowL2IsolationWhitelist().c_str());
    STRCMP_EQUAL("interface invalid_test_iface_999", ZyxelNetworkCmd::cmdInvalidInterfaceDryFire().c_str());
    STRCMP_EQUAL("trunk trunk1", ZyxelNetworkCmd::cmdTrunk("trunk1").c_str());
    STRCMP_EQUAL("no trunk trunk1", ZyxelNetworkCmd::cmdNoTrunk("trunk1").c_str());
    STRCMP_EQUAL("zone DMZ2", ZyxelNetworkCmd::cmdZone("DMZ2").c_str());
    STRCMP_EQUAL("no zone DMZ2", ZyxelNetworkCmd::cmdNoZone("DMZ2").c_str());
    STRCMP_EQUAL("ip-mac-binding activate", ZyxelNetworkCmd::cmdIpMacBinding(true).c_str());
    STRCMP_EQUAL("no ip-mac-binding activate", ZyxelNetworkCmd::cmdIpMacBinding(false).c_str());
    STRCMP_EQUAL("l2-isolation activation", ZyxelNetworkCmd::cmdL2IsolationActivation(true).c_str());
    STRCMP_EQUAL("no l2-isolation activation", ZyxelNetworkCmd::cmdL2IsolationActivation(false).c_str());
    STRCMP_EQUAL("ip-exception exp1", ZyxelNetworkCmd::cmdIpException("exp1").c_str());
    STRCMP_EQUAL("no ip-exception exp1", ZyxelNetworkCmd::cmdNoIpException("exp1").c_str());
    STRCMP_EQUAL("device-insight activate", ZyxelNetworkCmd::cmdDeviceInsightActivation(true).c_str());
    STRCMP_EQUAL("no device-insight activate", ZyxelNetworkCmd::cmdDeviceInsightActivation(false).c_str());
}

TEST(ZyxelNetworkCmdTest, ParseInterfacesTableFormat) {
    std::string raw =
        "Interface       Status  IP Address      Netmask         MAC Address         MTU\n"
        "===============================================================================\n"
        "wan1            up      192.0.2.1       255.255.255.0   02:00:00:00:00:01   1500\n"
        "lan1            up      198.51.100.1    255.255.255.0   02:00:00:00:00:02   1500\n"
        "ge3             down    0.0.0.0         0.0.0.0         02:00:00:00:00:03   1500\n";

    std::vector<ZyxelInterfaceInfo> ifaces;
    CHECK_TRUE(ZyxelNetworkCmd::parseInterfaces(raw, ifaces));
    LONGS_EQUAL(3, ifaces.size());

    STRCMP_EQUAL("wan1", ifaces[0].name.c_str());
    CHECK_TRUE(ifaces[0].linkStatus);
    STRCMP_EQUAL("192.0.2.1", ifaces[0].ip.c_str());
    STRCMP_EQUAL("255.255.255.0", ifaces[0].netmask.c_str());
    STRCMP_EQUAL("02:00:00:00:00:01", ifaces[0].mac.c_str());
    LONGS_EQUAL(1500, ifaces[0].mtu);

    STRCMP_EQUAL("ge3", ifaces[2].name.c_str());
    CHECK_FALSE(ifaces[2].linkStatus);
}

TEST(ZyxelNetworkCmdTest, ParseInterfacesDetailBlockFormat) {
    std::string raw =
        "interface: lan1\n"
        "  status: up\n"
        "  ip address: 198.51.100.1\n"
        "  subnet mask: 255.255.255.0\n"
        "  mac address: 02:00:00:00:00:02\n"
        "  mtu: 1500\n";

    std::vector<ZyxelInterfaceInfo> ifaces;
    CHECK_TRUE(ZyxelNetworkCmd::parseInterfaces(raw, ifaces));
    LONGS_EQUAL(1, ifaces.size());
    STRCMP_EQUAL("lan1", ifaces[0].name.c_str());
    CHECK_TRUE(ifaces[0].linkStatus);
    STRCMP_EQUAL("198.51.100.1", ifaces[0].ip.c_str());
}

TEST(ZyxelNetworkCmdTest, ParseIpRoutesManualPage175Transcript) {
    // Official transcript from manual page 175
    std::string raw =
        "Router(config)# show ip route-settings\n"
        "Route           Netmask         Nexthop         Metric\n"
        "===========================================================================\n"
        "10.10.10.0      255.255.255.0   ge1             0\n"
        "0.0.0.0         0.0.0.0         192.0.2.254     1\n";

    std::vector<ZyxelRouteEntry> routes;
    CHECK_TRUE(ZyxelNetworkCmd::parseIpRoutes(raw, routes));
    LONGS_EQUAL(2, routes.size());

    STRCMP_EQUAL("10.10.10.0", routes[0].destination.c_str());
    STRCMP_EQUAL("255.255.255.0", routes[0].netmask.c_str());
    STRCMP_EQUAL("ge1", routes[0].gateway.c_str());
    LONGS_EQUAL(0, routes[0].metric);

    STRCMP_EQUAL("0.0.0.0", routes[1].destination.c_str());
    STRCMP_EQUAL("192.0.2.254", routes[1].gateway.c_str());
    LONGS_EQUAL(1, routes[1].metric);
}

TEST(ZyxelNetworkCmdTest, ParseZonesTableFormat) {
    std::string raw =
        "Zone            Member Interfaces\n"
        "===========================================================================\n"
        "WAN             wan1, ge2\n"
        "LAN             lan1, ge3\n"
        "DMZ             dmz\n";

    std::vector<ZyxelZoneInfo> zones;
    CHECK_TRUE(ZyxelNetworkCmd::parseZones(raw, zones));
    LONGS_EQUAL(3, zones.size());

    STRCMP_EQUAL("WAN", zones[0].name.c_str());
    LONGS_EQUAL(2, zones[0].interfaces.size());
    STRCMP_EQUAL("wan1", zones[0].interfaces[0].c_str());
    STRCMP_EQUAL("ge2", zones[0].interfaces[1].c_str());

    STRCMP_EQUAL("LAN", zones[1].name.c_str());
    STRCMP_EQUAL("lan1", zones[1].interfaces[0].c_str());
}

TEST(ZyxelNetworkCmdTest, ParseArpTableFormat) {
    std::string raw =
        "IP Address       MAC Address        Interface\n"
        "===========================================================================\n"
        "192.0.2.50       02:00:00:00:00:50  lan1\n"
        "192.0.2.100      02:00:00:00:01:00  lan1\n";

    std::vector<ZyxelArpEntry> entries;
    CHECK_TRUE(ZyxelNetworkCmd::parseArp(raw, entries));
    LONGS_EQUAL(2, entries.size());

    STRCMP_EQUAL("192.0.2.50", entries[0].ip.c_str());
    STRCMP_EQUAL("02:00:00:00:00:50", entries[0].mac.c_str());
    STRCMP_EQUAL("lan1", entries[0].interface.c_str());
}

TEST(ZyxelNetworkCmdTest, ParseErrorsOnCorruptedBuffers) {
    std::vector<ZyxelInterfaceInfo> ifaces;
    CHECK_FALSE(ZyxelNetworkCmd::parseInterfaces("Garbage output % parse error", ifaces));

    std::vector<ZyxelRouteEntry> routes;
    CHECK_FALSE(ZyxelNetworkCmd::parseIpRoutes("No routes available", routes));

    std::vector<ZyxelZoneInfo> zones;
    CHECK_FALSE(ZyxelNetworkCmd::parseZones("Unknown command", zones));

    std::vector<ZyxelArpEntry> arp;
    CHECK_FALSE(ZyxelNetworkCmd::parseArp("No arp entries", arp));

    std::vector<ZyxelDhcpBindingEntry> dhcp;
    CHECK_FALSE(ZyxelNetworkCmd::parseDhcpBindings("Garbage output", dhcp));

    ZyxelArpGratuitousInfo grat;
    CHECK_FALSE(ZyxelNetworkCmd::parseArpGratuitous("Garbage output", grat));

    std::vector<ZyxelZoneBindingEntry> zbind;
    CHECK_FALSE(ZyxelNetworkCmd::parseZoneBindings("Garbage output", zbind));

    bool l2Active = false;
    CHECK_FALSE(ZyxelNetworkCmd::parseL2IsolationActivation("Garbage output", l2Active));
}

TEST(ZyxelNetworkCmdTest, ParseZonesUsgFlexLiveTranscript) {
    std::string raw =
        "No. Name                             Member\n"
        "Ref\n"
        "===============================================================================\n"
        "1   LAN1                             lan1\n"
        "16\n"
        "2   LAN2                             lan2\n"
        "14\n"
        "3   DMZ                              dmz\n"
        "14\n"
        "4   WAN                              wan1_ppp, wan1\n"
        "18\n";

    std::vector<ZyxelZoneInfo> zones;
    CHECK_TRUE(ZyxelNetworkCmd::parseZones(raw, zones));
    LONGS_EQUAL(4, zones.size());

    STRCMP_EQUAL("LAN1", zones[0].name.c_str());
    LONGS_EQUAL(1, zones[0].interfaces.size());
    STRCMP_EQUAL("lan1", zones[0].interfaces[0].c_str());

    STRCMP_EQUAL("WAN", zones[3].name.c_str());
    LONGS_EQUAL(2, zones[3].interfaces.size());
    STRCMP_EQUAL("wan1_ppp", zones[3].interfaces[0].c_str());
    STRCMP_EQUAL("wan1", zones[3].interfaces[1].c_str());
}

TEST(ZyxelNetworkCmdTest, ParseDhcpBindingsLiveTranscript) {
    std::string raw =
        "No.  Interface       IP Address      MAC Address       Reserved Host Name                       Expiration Time\n"
        "Description\n"
        "===============================================================================\n"
        "1    lan1            192.0.2.100     02:00:00:00:00:10 no       host-alpha                      2026-10-07 12:00:00\n"
        "2    lan1            192.0.2.101     02:00:00:00:00:11 yes      printer-beta                    infinite\n";

    std::vector<ZyxelDhcpBindingEntry> entries;
    CHECK_TRUE(ZyxelNetworkCmd::parseDhcpBindings(raw, entries));
    LONGS_EQUAL(2, entries.size());

    LONGS_EQUAL(1, entries[0].index);
    STRCMP_EQUAL("lan1", entries[0].interfaceName.c_str());
    STRCMP_EQUAL("192.0.2.100", entries[0].ip.c_str());
    STRCMP_EQUAL("02:00:00:00:00:10", entries[0].mac.c_str());
    STRCMP_EQUAL("no", entries[0].reserved.c_str());
    STRCMP_EQUAL("host-alpha", entries[0].hostName.c_str());

    LONGS_EQUAL(2, entries[1].index);
    STRCMP_EQUAL("printer-beta", entries[1].hostName.c_str());
}

TEST(ZyxelNetworkCmdTest, ParseArpGratuitousLiveTranscript) {
    std::string raw =
        "Gratuitous ARP status: no\n"
        "Periodical Intervals: 0 seconds\n";

    ZyxelArpGratuitousInfo info;
    CHECK_TRUE(ZyxelNetworkCmd::parseArpGratuitous(raw, info));
    CHECK_FALSE(info.active);
    LONGS_EQUAL(0, info.intervalSeconds);

    std::string rawActive =
        "Gratuitous ARP status: yes\n"
        "Periodical Intervals: 30 seconds\n";

    CHECK_TRUE(ZyxelNetworkCmd::parseArpGratuitous(rawActive, info));
    CHECK_TRUE(info.active);
    LONGS_EQUAL(30, info.intervalSeconds);
}

TEST(ZyxelNetworkCmdTest, ParseZoneBindingsLiveTranscript) {
    std::string raw =
        "No. Interface                        Default Binding Zone\n"
        "===============================================================================\n"
        "1   lan1                             LAN1\n"
        "2   lan2                             LAN2\n"
        "3   dmz                              DMZ\n"
        "4   wan1_ppp                         WAN\n"
        "5   wan1                             WAN\n";

    std::vector<ZyxelZoneBindingEntry> entries;
    CHECK_TRUE(ZyxelNetworkCmd::parseZoneBindings(raw, entries));
    LONGS_EQUAL(5, entries.size());

    LONGS_EQUAL(1, entries[0].index);
    STRCMP_EQUAL("lan1", entries[0].interfaceName.c_str());
    STRCMP_EQUAL("LAN1", entries[0].zoneName.c_str());

    LONGS_EQUAL(5, entries[4].index);
    STRCMP_EQUAL("wan1", entries[4].interfaceName.c_str());
    STRCMP_EQUAL("WAN", entries[4].zoneName.c_str());
}

TEST(ZyxelNetworkCmdTest, ParseL2IsolationActivationLiveTranscript) {
    std::string rawNo = "Layer 2 Isolation Status: no\n";
    bool active = true;
    CHECK_TRUE(ZyxelNetworkCmd::parseL2IsolationActivation(rawNo, active));
    CHECK_FALSE(active);

    std::string rawYes = "Layer 2 Isolation Status: yes\n";
    CHECK_TRUE(ZyxelNetworkCmd::parseL2IsolationActivation(rawYes, active));
    CHECK_TRUE(active);
}

TEST(ZyxelNetworkCmdTest, Envelope3CommandGenerators) {
    STRCMP_EQUAL("show ip route bgp", ZyxelNetworkCmd::cmdShowIpRouteFilter("bgp").c_str());
    STRCMP_EQUAL("show ip route-settings", ZyxelNetworkCmd::cmdShowIpRouteSettings().c_str());
    STRCMP_EQUAL("show ip route control-virtual-server-rules", ZyxelNetworkCmd::cmdShowIpRouteControlVirtualServer().c_str());
    STRCMP_EQUAL("show policy-route", ZyxelNetworkCmd::cmdShowPolicyRoute().c_str());
    STRCMP_EQUAL("show policy-route rule_count", ZyxelNetworkCmd::cmdShowPolicyRouteRuleCount().c_str());
    STRCMP_EQUAL("show policy-route conn-check", ZyxelNetworkCmd::cmdShowPolicyRouteConnCheck().c_str());
    STRCMP_EQUAL("show policy-route conn-check status", ZyxelNetworkCmd::cmdShowPolicyRouteConnCheckStatus().c_str());
    STRCMP_EQUAL("show policy-route override-direct-route", ZyxelNetworkCmd::cmdShowPolicyRouteOverrideDirectRoute().c_str());
    STRCMP_EQUAL("show policy-route underlayer-rules", ZyxelNetworkCmd::cmdShowPolicyRouteUnderlayerRules().c_str());
    STRCMP_EQUAL("show policy-route controll-virtual-server-rules", ZyxelNetworkCmd::cmdShowPolicyRouteControlVirtualServer().c_str());
    STRCMP_EQUAL("show policy-route controll-ipsec-dynamic-rules", ZyxelNetworkCmd::cmdShowPolicyRouteControlIpsecDynamic().c_str());
    STRCMP_EQUAL("show policy-route begin 1 end 5", ZyxelNetworkCmd::cmdShowPolicyRouteBeginEnd(1, 5).c_str());
    STRCMP_EQUAL("show policy-route6", ZyxelNetworkCmd::cmdShowPolicyRoute6().c_str());
    STRCMP_EQUAL("show policy-route6 rule_count", ZyxelNetworkCmd::cmdShowPolicyRoute6RuleCount().c_str());
    STRCMP_EQUAL("show policy-route6 override-direct-route", ZyxelNetworkCmd::cmdShowPolicyRoute6OverrideDirectRoute().c_str());
    STRCMP_EQUAL("show policy-route6 controll-ipsec-dynamic-rules", ZyxelNetworkCmd::cmdShowPolicyRoute6ControlIpsecDynamic().c_str());
    STRCMP_EQUAL("show bwm activation", ZyxelNetworkCmd::cmdShowBwmActivation().c_str());
    STRCMP_EQUAL("show ospf global", ZyxelNetworkCmd::cmdShowOspfGlobal().c_str());
    STRCMP_EQUAL("show ospf database", ZyxelNetworkCmd::cmdShowOspfDatabase().c_str());
    STRCMP_EQUAL("show ospf neighbor", ZyxelNetworkCmd::cmdShowOspfNeighbor().c_str());
    STRCMP_EQUAL("show ospf area 0.0.0.0 virtual-link", ZyxelNetworkCmd::cmdShowOspfAreaVirtualLink("0.0.0.0").c_str());
    STRCMP_EQUAL("show rip global", ZyxelNetworkCmd::cmdShowRipGlobal().c_str());
    STRCMP_EQUAL("show bgp global", ZyxelNetworkCmd::cmdShowBgpGlobal().c_str());
    STRCMP_EQUAL("show bgp summary", ZyxelNetworkCmd::cmdShowBgpSummary().c_str());
    STRCMP_EQUAL("show bgp route", ZyxelNetworkCmd::cmdShowBgpRoute().c_str());
    STRCMP_EQUAL("show bgp mem", ZyxelNetworkCmd::cmdShowBgpMem().c_str());
    STRCMP_EQUAL("show bgp neighbor", ZyxelNetworkCmd::cmdShowBgpNeighbor().c_str());

    STRCMP_EQUAL("ip route control-virtual-server-rules activate", ZyxelNetworkCmd::cmdIpRouteControlVirtualServer(true).c_str());
    STRCMP_EQUAL("no ip route control-virtual-server-rules activate", ZyxelNetworkCmd::cmdIpRouteControlVirtualServer(false).c_str());
    STRCMP_EQUAL("policy append", ZyxelNetworkCmd::cmdPolicyAppend().c_str());
    STRCMP_EQUAL("policy delete 5", ZyxelNetworkCmd::cmdPolicyDelete(5).c_str());
    STRCMP_EQUAL("policy default-route", ZyxelNetworkCmd::cmdPolicyDefaultRoute().c_str());
    STRCMP_EQUAL("policy flush", ZyxelNetworkCmd::cmdPolicyFlush().c_str());
    STRCMP_EQUAL("policy controll-virtual-server-rules activate", ZyxelNetworkCmd::cmdPolicyControlVirtualServer(true).c_str());
    STRCMP_EQUAL("no policy controll-virtual-server-rules activate", ZyxelNetworkCmd::cmdPolicyControlVirtualServer(false).c_str());
    STRCMP_EQUAL("policy controll-ipsec-dynamic-rules activate", ZyxelNetworkCmd::cmdPolicyControlIpsecDynamic(true).c_str());
    STRCMP_EQUAL("no policy controll-ipsec-dynamic-rules activate", ZyxelNetworkCmd::cmdPolicyControlIpsecDynamic(false).c_str());
    STRCMP_EQUAL("bwm activate", ZyxelNetworkCmd::cmdBwmActivation(true).c_str());
    STRCMP_EQUAL("no bwm activate", ZyxelNetworkCmd::cmdBwmActivation(false).c_str());
    STRCMP_EQUAL("router ospf", ZyxelNetworkCmd::cmdRouterOspf(true).c_str());
    STRCMP_EQUAL("no router ospf", ZyxelNetworkCmd::cmdRouterOspf(false).c_str());
    CHECK_TRUE(ZyxelNetworkCmd::cmdRouterBgp(64512, true).find("as-number 64512") != std::string::npos);
    STRCMP_EQUAL("no router bgp", ZyxelNetworkCmd::cmdRouterBgp(64512, false).c_str());
    STRCMP_EQUAL("router rip", ZyxelNetworkCmd::cmdRouterRip(true).c_str());
    STRCMP_EQUAL("no router rip", ZyxelNetworkCmd::cmdRouterRip(false).c_str());

    STRCMP_EQUAL("show ip route 999.999.999.999", ZyxelNetworkCmd::cmdInvalidRouteDryFire().c_str());
    STRCMP_EQUAL("show policy-route 99999", ZyxelNetworkCmd::cmdInvalidPolicyRouteDryFire().c_str());
    STRCMP_EQUAL("show ospf area 0.0.0.0 virtual-link", ZyxelNetworkCmd::cmdInvalidOspfAreaDryFire().c_str());
}

TEST(ZyxelNetworkCmdTest, ParseRouteSettingsLiveTranscript) {
    std::string raw =
        "Route           Netmask         Nexthop         Metric\n"
        "===============================================================================\n"
        "192.0.2.0       255.255.255.0   192.0.2.1       1\n"
        "198.51.100.0    255.255.255.0   198.51.100.1    2\n";

    std::vector<ZyxelRouteSettingsEntry> entries;
    CHECK_TRUE(ZyxelNetworkCmd::parseRouteSettings(raw, entries));
    LONGS_EQUAL(2, entries.size());
    STRCMP_EQUAL("192.0.2.0", entries[0].route.c_str());
    STRCMP_EQUAL("255.255.255.0", entries[0].netmask.c_str());
    STRCMP_EQUAL("192.0.2.1", entries[0].nexthop.c_str());
    LONGS_EQUAL(1, entries[0].metric);

    STRCMP_EQUAL("198.51.100.0", entries[1].route.c_str());
    LONGS_EQUAL(2, entries[1].metric);

    std::string emptyTable =
        "Route           Netmask         Nexthop         Metric\n"
        "===============================================================================\n";
    std::vector<ZyxelRouteSettingsEntry> emptyEntries;
    CHECK_TRUE(ZyxelNetworkCmd::parseRouteSettings(emptyTable, emptyEntries));
    CHECK_TRUE(emptyEntries.empty());
}

TEST(ZyxelNetworkCmdTest, ParsePolicyRouteRuleCountLiveTranscript) {
    std::string raw =
        "show policy-route rule_count\n"
        "policy count : 42\n"
        "Router>\n";

    int count = 0;
    CHECK_TRUE(ZyxelNetworkCmd::parsePolicyRouteRuleCount(raw, count));
    LONGS_EQUAL(42, count);

    CHECK_FALSE(ZyxelNetworkCmd::parsePolicyRouteRuleCount("Corrupted buffer", count));
}

TEST(ZyxelNetworkCmdTest, ParsePolicyRouteOverrideDirectRouteLiveTranscript) {
    std::string rawOff = "policy route override direct route status: off\n";
    bool active = true;
    CHECK_TRUE(ZyxelNetworkCmd::parsePolicyRouteOverrideDirectRoute(rawOff, active));
    CHECK_FALSE(active);

    std::string rawOn = "policy route override direct route status: on\n";
    CHECK_TRUE(ZyxelNetworkCmd::parsePolicyRouteOverrideDirectRoute(rawOn, active));
    CHECK_TRUE(active);
}

TEST(ZyxelNetworkCmdTest, ParsePolicyRouteControlVirtualServerLiveTranscript) {
    std::string rawOn = "policy route control virtual server status: on\n";
    bool active = false;
    CHECK_TRUE(ZyxelNetworkCmd::parsePolicyRouteControlVirtualServer(rawOn, active));
    CHECK_TRUE(active);

    std::string rawOff = "policy route control dynamic ipsec status: off\n";
    CHECK_TRUE(ZyxelNetworkCmd::parsePolicyRouteControlVirtualServer(rawOff, active));
    CHECK_FALSE(active);
}

TEST(ZyxelNetworkCmdTest, ParseBwmActivationLiveTranscript) {
    std::string rawYes = "bwm activation: yes\n";
    bool active = false;
    CHECK_TRUE(ZyxelNetworkCmd::parseBwmActivation(rawYes, active));
    CHECK_TRUE(active);

    std::string rawNo = "bwm activation: no\n";
    CHECK_TRUE(ZyxelNetworkCmd::parseBwmActivation(rawNo, active));
    CHECK_FALSE(active);
}

TEST(ZyxelNetworkCmdTest, ParseOspfGlobalLiveTranscript) {
    std::string raw =
        "OSPF Global Information\n"
        "  router id: default\n"
        "  redistribute RIP: no\n"
        "  redistribute RIP type: none\n"
        "  redistribute RIP metric: none\n"
        "  redistribute static: no\n"
        "  redistribute static type: none\n"
        "  redistribute static metric: none\n"
        "Router>\n";

    ZyxelOspfGlobalInfo info;
    CHECK_TRUE(ZyxelNetworkCmd::parseOspfGlobal(raw, info));
    STRCMP_EQUAL("default", info.routerId.c_str());
    CHECK_FALSE(info.redistributeRip);
    CHECK_FALSE(info.redistributeStatic);
    STRCMP_EQUAL("none", info.redistributeRipType.c_str());
}

TEST(ZyxelNetworkCmdTest, ParseRipGlobalLiveTranscript) {
    std::string raw =
        "RIP Global Information\n"
        "  authentication type: none\n"
        "  text string: none\n"
        "  MD5 key: none\n"
        "  MD5 string: none\n"
        "  redistribute OSPF: no\n"
        "  redistribute OSPF metric: 1\n"
        "  redistribute static: no\n"
        "  redistribute static metric: 1\n"
        "Router>\n";

    ZyxelRipGlobalInfo info;
    CHECK_TRUE(ZyxelNetworkCmd::parseRipGlobal(raw, info));
    STRCMP_EQUAL("none", info.authType.c_str());
    STRCMP_EQUAL("none", info.textString.c_str());
    CHECK_FALSE(info.redistributeOspf);
    LONGS_EQUAL(1, info.redistributeOspfMetric);
}

TEST(ZyxelNetworkCmdTest, ParseBgpGlobalLiveTranscript) {
    std::string raw =
        "BGP Global Information\n"
        "  router id: \n"
        "  AS number: 64512\n"
        "  redistribute connected: no\n"
        "  maximum paths: \n"
        "  BGP network : \n"
        "Router>\n";

    ZyxelBgpGlobalInfo info;
    CHECK_TRUE(ZyxelNetworkCmd::parseBgpGlobal(raw, info));
    LONGS_EQUAL(64512, info.asNumber);
    CHECK_FALSE(info.redistributeConnected);
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
