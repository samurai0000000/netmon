/*
 * TestZyxelVpnCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelVpnCmd.hxx"
#include <CppUTest/TestHarness.h>

TEST_GROUP(ZyxelVpnCmdTest) {
    void setup() {}
    void teardown() {}
};

TEST(ZyxelVpnCmdTest, CommandGeneratorsReturnExpectedStrings) {
    // Ch 33: IPSec VPN
    STRCMP_EQUAL("show crypto boost-tcp", ZyxelVpnCmd::cmdShowCryptoBoostTcp().c_str());
    STRCMP_EQUAL("show crypto ignore-df-bit", ZyxelVpnCmd::cmdShowCryptoIgnoreDfBit().c_str());
    STRCMP_EQUAL("show crypto map", ZyxelVpnCmd::cmdShowCryptoMap().c_str());
    STRCMP_EQUAL("show crypto map test_map", ZyxelVpnCmd::cmdShowCryptoMap("test_map").c_str());
    STRCMP_EQUAL("show crypto map6", ZyxelVpnCmd::cmdShowCryptoMap6().c_str());
    STRCMP_EQUAL("show crypto map6 test_map6", ZyxelVpnCmd::cmdShowCryptoMap6("test_map6").c_str());
    STRCMP_EQUAL("show crypto map conn-check", ZyxelVpnCmd::cmdShowCryptoMapConnCheck().c_str());
    STRCMP_EQUAL("show ikev2 policy", ZyxelVpnCmd::cmdShowIkev2Policy().c_str());
    STRCMP_EQUAL("show ikev2 policy p1", ZyxelVpnCmd::cmdShowIkev2Policy("p1").c_str());
    STRCMP_EQUAL("show ikev2 policy6", ZyxelVpnCmd::cmdShowIkev2Policy6().c_str());
    STRCMP_EQUAL("show ikev2 policy6 p1_6", ZyxelVpnCmd::cmdShowIkev2Policy6("p1_6").c_str());
    STRCMP_EQUAL("show isakmp keepalive", ZyxelVpnCmd::cmdShowIsakmpKeepalive().c_str());
    STRCMP_EQUAL("show isakmp policy", ZyxelVpnCmd::cmdShowIsakmpPolicy().c_str());
    STRCMP_EQUAL("show isakmp policy p_ike", ZyxelVpnCmd::cmdShowIsakmpPolicy("p_ike").c_str());
    STRCMP_EQUAL("show isakmp sa", ZyxelVpnCmd::cmdShowIsakmpSa().c_str());
    STRCMP_EQUAL("show sa counter", ZyxelVpnCmd::cmdShowSaCounter().c_str());
    STRCMP_EQUAL("show sa monitor", ZyxelVpnCmd::cmdShowSaMonitor().c_str());
    STRCMP_EQUAL("show vcp allowed crypto map", ZyxelVpnCmd::cmdShowVcpAllowedCryptoMap().c_str());
    STRCMP_EQUAL("show vcp allowed crypto map6", ZyxelVpnCmd::cmdShowVcpAllowedCryptoMap6().c_str());
    STRCMP_EQUAL("show vcp allowed users", ZyxelVpnCmd::cmdShowVcpAllowedUsers().c_str());
    STRCMP_EQUAL("show vpn-concentrator", ZyxelVpnCmd::cmdShowVpnConcentrator().c_str());
    STRCMP_EQUAL("show vpn-concentrator conc1", ZyxelVpnCmd::cmdShowVpnConcentrator("conc1").c_str());
    STRCMP_EQUAL("show vpn-concentrator6", ZyxelVpnCmd::cmdShowVpnConcentrator6().c_str());
    STRCMP_EQUAL("show vpn-concentrator6 conc6", ZyxelVpnCmd::cmdShowVpnConcentrator6("conc6").c_str());
    STRCMP_EQUAL("show vpn-configuration-provision activation", ZyxelVpnCmd::cmdShowVpnConfigurationProvisionActivation().c_str());
    STRCMP_EQUAL("show vpn-configuration-provision authentication", ZyxelVpnCmd::cmdShowVpnConfigurationProvisionAuthentication().c_str());
    STRCMP_EQUAL("show vpn-configuration-provision iosfilter", ZyxelVpnCmd::cmdShowVpnConfigurationProvisionIosfilter().c_str());
    STRCMP_EQUAL("show vpn-configuration-provision port", ZyxelVpnCmd::cmdShowVpnConfigurationProvisionPort().c_str());
    STRCMP_EQUAL("show vpn-configuration-provision rules", ZyxelVpnCmd::cmdShowVpnConfigurationProvisionRules().c_str());
    STRCMP_EQUAL("show vpn-counters", ZyxelVpnCmd::cmdShowVpnCounters().c_str());
    STRCMP_EQUAL("show vpn-service status", ZyxelVpnCmd::cmdShowVpnServiceStatus().c_str());

    STRCMP_EQUAL("isakmp policy my_ike", ZyxelVpnCmd::cmdIsakmpPolicy("my_ike").c_str());
    STRCMP_EQUAL("no isakmp policy my_ike", ZyxelVpnCmd::cmdNoIsakmpPolicy("my_ike").c_str());
    STRCMP_EQUAL("crypto map my_vpn", ZyxelVpnCmd::cmdCryptoMap("my_vpn").c_str());
    STRCMP_EQUAL("no crypto map my_vpn", ZyxelVpnCmd::cmdNoCryptoMap("my_vpn").c_str());

    // Ch 34: SSL VPN
    STRCMP_EQUAL("show sslvpn login-port", ZyxelVpnCmd::cmdShowSslvpnLoginPort().c_str());
    STRCMP_EQUAL("show sslvpn policy", ZyxelVpnCmd::cmdShowSslvpnPolicy().c_str());
    STRCMP_EQUAL("show sslvpn policy ssl_pol", ZyxelVpnCmd::cmdShowSslvpnPolicy("ssl_pol").c_str());
    STRCMP_EQUAL("show sslvpn application", ZyxelVpnCmd::cmdShowSslvpnApplication().c_str());
    STRCMP_EQUAL("show sslvpn monitor", ZyxelVpnCmd::cmdShowSslvpnMonitor().c_str());
    STRCMP_EQUAL("show workspace application", ZyxelVpnCmd::cmdShowWorkspaceApplication().c_str());
    STRCMP_EQUAL("show workspace cifs", ZyxelVpnCmd::cmdShowWorkspaceCifs().c_str());
    STRCMP_EQUAL("show ssl-vpn network-extension local-ip", ZyxelVpnCmd::cmdShowSslVpnNetworkExtensionLocalIp().c_str());

    STRCMP_EQUAL("sslvpn application app1", ZyxelVpnCmd::cmdSslvpnApplication("app1").c_str());
    STRCMP_EQUAL("no sslvpn application app1", ZyxelVpnCmd::cmdNoSslvpnApplication("app1").c_str());
    STRCMP_EQUAL("sslvpn policy pol1", ZyxelVpnCmd::cmdSslvpnPolicy("pol1").c_str());
    STRCMP_EQUAL("no sslvpn policy pol1", ZyxelVpnCmd::cmdNoSslvpnPolicy("pol1").c_str());

    // Ch 35: L2TP VPN
    STRCMP_EQUAL("show account l2tp", ZyxelVpnCmd::cmdShowAccountL2tp().c_str());
    STRCMP_EQUAL("show account l2tp l2_prof", ZyxelVpnCmd::cmdShowAccountL2tp("l2_prof").c_str());
    STRCMP_EQUAL("show interface ppp", ZyxelVpnCmd::cmdShowInterfacePpp().c_str());
    STRCMP_EQUAL("show l2tp-over-ipsec", ZyxelVpnCmd::cmdShowL2tpOverIpsec().c_str());
    STRCMP_EQUAL("show l2tp-over-ipsec session", ZyxelVpnCmd::cmdShowL2tpOverIpsecSession().c_str());

    STRCMP_EQUAL("account l2tp l2tp_acc", ZyxelVpnCmd::cmdAccountL2tp("l2tp_acc").c_str());
    STRCMP_EQUAL("no account l2tp l2tp_acc", ZyxelVpnCmd::cmdNoAccountL2tp("l2tp_acc").c_str());

    // Dry-fire helpers
    STRCMP_EQUAL("show ikev2 policy6", ZyxelVpnCmd::cmdInvalidIpsecDryFire().c_str());
    STRCMP_EQUAL("show ssl-vpn network-extension local-ip", ZyxelVpnCmd::cmdInvalidSslvpnDryFire().c_str());
    STRCMP_EQUAL("no l2tp-over-ipsec session tunnel-id 99999", ZyxelVpnCmd::cmdInvalidL2tpDryFire().c_str());
}

TEST(ZyxelVpnCmdTest, ParseCryptoBoostTcp) {
    bool boostTcp = false;
    const std::string rawDeactive =
        "ipsec boost tcp: deactivate\n"
        "ipsec boost tcp disperse-on : No\n";
    CHECK_TRUE(ZyxelVpnCmd::parseCryptoBoostTcp(rawDeactive, boostTcp));
    CHECK_FALSE(boostTcp);

    const std::string rawActive =
        "ipsec boost tcp: activate\n"
        "ipsec boost tcp disperse-on : Yes\n";
    CHECK_TRUE(ZyxelVpnCmd::parseCryptoBoostTcp(rawActive, boostTcp));
    CHECK_TRUE(boostTcp);

    CHECK_FALSE(ZyxelVpnCmd::parseCryptoBoostTcp("", boostTcp));
}

TEST(ZyxelVpnCmdTest, ParseVpnCounters) {
    ZyxelVpnCounters counters;
    const std::string raw =
        "inbpkt count      : 123456\n"
        "outbpkt count     : 654321\n"
        "inbpkt throughput : 42\n"
        "outbpkt throughput: 88\n";
    CHECK_TRUE(ZyxelVpnCmd::parseVpnCounters(raw, counters));
    LONGS_EQUAL(123456, counters.inbpktCount);
    LONGS_EQUAL(654321, counters.outbpktCount);
    LONGS_EQUAL(42, counters.inbpktThroughput);
    LONGS_EQUAL(88, counters.outbpktThroughput);

    CHECK_FALSE(ZyxelVpnCmd::parseVpnCounters("invalid data\n", counters));
}

TEST(ZyxelVpnCmdTest, ParseVpnServiceStatus) {
    bool active = false;
    bool autoDisable = false;
    const std::string raw =
        "status: deactivate\n"
        "auto disable status: activate\n";
    CHECK_TRUE(ZyxelVpnCmd::parseVpnServiceStatus(raw, active, autoDisable));
    CHECK_FALSE(active);
    CHECK_TRUE(autoDisable);

    CHECK_FALSE(ZyxelVpnCmd::parseVpnServiceStatus("", active, autoDisable));
}

TEST(ZyxelVpnCmdTest, ParseVpnConfigurationProvision) {
    bool active = false;
    const std::string rawAct = "VPN configuration provision activation: no\n";
    CHECK_TRUE(ZyxelVpnCmd::parseVpnConfigurationProvisionActivation(rawAct, active));
    CHECK_FALSE(active);

    int port = 0;
    const std::string rawPort = "VPN configuration provision port: 443\n";
    CHECK_TRUE(ZyxelVpnCmd::parseVpnConfigurationProvisionPort(rawPort, port));
    LONGS_EQUAL(443, port);
}

TEST(ZyxelVpnCmdTest, ParseSaCounter) {
    int status = -1;
    const std::string raw = "vpn status: 0\n";
    CHECK_TRUE(ZyxelVpnCmd::parseSaCounter(raw, status));
    LONGS_EQUAL(0, status);
}

TEST(ZyxelVpnCmdTest, ParseSslvpnLoginPort) {
    int port = 0;
    const std::string raw = "SSL VPN Login Port: 443\n";
    CHECK_TRUE(ZyxelVpnCmd::parseSslvpnLoginPort(raw, port));
    LONGS_EQUAL(443, port);
}

TEST(ZyxelVpnCmdTest, ParseL2tpOverIpsec) {
    ZyxelL2tpStatus l2tp;
    const std::string raw =
        "L2TP over IPSec:\n"
        "activate          : no\n"
        "crypto            : default_crypto\n"
        "address pool      : POOL_1\n"
        "authentication    : default\n"
        "certificate       : default\n"
        "user              : any\n"
        "keepalive timer   : 60\n"
        "first dns server  : 1.1.1.1\n";
    CHECK_TRUE(ZyxelVpnCmd::parseL2tpOverIpsec(raw, l2tp));
    CHECK_FALSE(l2tp.activate);
    STRCMP_EQUAL("default_crypto", l2tp.crypto.c_str());
    STRCMP_EQUAL("POOL_1", l2tp.addressPool.c_str());
    STRCMP_EQUAL("default", l2tp.authentication.c_str());
    STRCMP_EQUAL("default", l2tp.certificate.c_str());
    STRCMP_EQUAL("any", l2tp.user.c_str());
    LONGS_EQUAL(60, l2tp.keepaliveTimer);
    STRCMP_EQUAL("1.1.1.1", l2tp.firstDnsServer.c_str());
}

TEST(ZyxelVpnCmdTest, ParseVcpAllowedUsers) {
    std::vector<ZyxelVcpUser> users;
    const std::string raw =
        "No.  Username                         User Type      Description\n"
        "===============================================================================\n"
        "1    ldap-users                       ext-user       External LDAP Users\n"
        "2    radius-users                     ext-user       External RADIUS Users\n"
        "3    ad-users                         ext-user       External AD Users\n";
    CHECK_TRUE(ZyxelVpnCmd::parseVcpAllowedUsers(raw, users));
    LONGS_EQUAL(3, users.size());
    LONGS_EQUAL(1, users[0].index);
    STRCMP_EQUAL("ldap-users", users[0].username.c_str());
    STRCMP_EQUAL("ext-user", users[0].userType.c_str());
    STRCMP_EQUAL("External LDAP Users", users[0].description.c_str());
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
