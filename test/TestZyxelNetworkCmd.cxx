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
    STRCMP_EQUAL("show ip route-settings", ZyxelNetworkCmd::cmdShowIpRoute().c_str());
    STRCMP_EQUAL("ip route 10.10.10.0 255.255.255.0 192.0.2.254 1",
                 ZyxelNetworkCmd::cmdAddRoute("10.10.10.0", "255.255.255.0", "192.0.2.254", 1).c_str());
    STRCMP_EQUAL("no ip route 10.10.10.0 255.255.255.0 192.0.2.254",
                 ZyxelNetworkCmd::cmdDeleteRoute("10.10.10.0", "255.255.255.0", "192.0.2.254").c_str());
    STRCMP_EQUAL("show zone", ZyxelNetworkCmd::cmdShowZones().c_str());
    STRCMP_EQUAL("show arp-table", ZyxelNetworkCmd::cmdShowArp().c_str());
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
