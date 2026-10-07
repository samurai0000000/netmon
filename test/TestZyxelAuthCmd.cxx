/*
 * TestZyxelAuthCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelAuthCmd.hxx"
#include <CppUTest/TestHarness.h>

TEST_GROUP(ZyxelAuthCmdTest) {
    void setup() {}
    void teardown() {}
};

TEST(ZyxelAuthCmdTest, CommandGeneratorsReturnExpectedStrings) {
    // Ch 30: Cloud CNM
    STRCMP_EQUAL("show cnm-agent configuration", ZyxelAuthCmd::cmdShowCnmAgentConfiguration().c_str());
    STRCMP_EQUAL("show monitor-mode", ZyxelAuthCmd::cmdShowMonitorMode().c_str());
    STRCMP_EQUAL("show secu-reporter status", ZyxelAuthCmd::cmdShowSecuReporterStatus().c_str());
    STRCMP_EQUAL("show secumanager status", ZyxelAuthCmd::cmdShowSecumanagerStatus().c_str());

    // Ch 31: Web Auth
    STRCMP_EQUAL("show web-auth activation", ZyxelAuthCmd::cmdShowWebAuthActivation().c_str());
    STRCMP_EQUAL("show web-auth default-rule", ZyxelAuthCmd::cmdShowWebAuthDefaultRule().c_str());
    STRCMP_EQUAL("show web-auth exceptional-service", ZyxelAuthCmd::cmdShowWebAuthExceptionalService().c_str());
    STRCMP_EQUAL("show web-auth method", ZyxelAuthCmd::cmdShowWebAuthMethod().c_str());
    STRCMP_EQUAL("show web-auth policy all", ZyxelAuthCmd::cmdShowWebAuthPolicy().c_str());
    STRCMP_EQUAL("show web-auth policy 1", ZyxelAuthCmd::cmdShowWebAuthPolicy("1").c_str());
    STRCMP_EQUAL("show web-auth portal status", ZyxelAuthCmd::cmdShowWebAuthPortalStatus().c_str());
    STRCMP_EQUAL("show web-auth redirect-fqdn", ZyxelAuthCmd::cmdShowWebAuthRedirectFqdn().c_str());
    STRCMP_EQUAL("show web-auth status", ZyxelAuthCmd::cmdShowWebAuthStatus().c_str());
    STRCMP_EQUAL("show sso agent", ZyxelAuthCmd::cmdShowSsoAgent().c_str());
    STRCMP_EQUAL("show sso port", ZyxelAuthCmd::cmdShowSsoPort().c_str());

    // Ch 32: Hotspot
    STRCMP_EQUAL("show advertisement activation", ZyxelAuthCmd::cmdShowAdvertisementActivation().c_str());
    STRCMP_EQUAL("show billing status", ZyxelAuthCmd::cmdShowBillingStatus().c_str());
    STRCMP_EQUAL("show free-time status", ZyxelAuthCmd::cmdShowFreeTimeStatus().c_str());
    STRCMP_EQUAL("show ip ipnp activation", ZyxelAuthCmd::cmdShowIpIpnpActivation().c_str());
    STRCMP_EQUAL("show payment-service activation", ZyxelAuthCmd::cmdShowPaymentServiceActivation().c_str());

    // Ch 50: User/Group
    STRCMP_EQUAL("show username", ZyxelAuthCmd::cmdShowUsername().c_str());
    STRCMP_EQUAL("show username admin", ZyxelAuthCmd::cmdShowUsername("admin").c_str());
    STRCMP_EQUAL("show groupname", ZyxelAuthCmd::cmdShowGroupname().c_str());
    STRCMP_EQUAL("show groupname grp1", ZyxelAuthCmd::cmdShowGroupname("grp1").c_str());
    STRCMP_EQUAL("show lockout-users", ZyxelAuthCmd::cmdShowLockoutUsers().c_str());
    STRCMP_EQUAL("show password complexity-verify status", ZyxelAuthCmd::cmdShowPasswordComplexityVerifyStatus().c_str());
    STRCMP_EQUAL("show pwd-expiry all", ZyxelAuthCmd::cmdShowPwdExpiry().c_str());
    STRCMP_EQUAL("show pwd-expiry expiration", ZyxelAuthCmd::cmdShowPwdExpiry("expiration").c_str());
    STRCMP_EQUAL("show users default-setting all", ZyxelAuthCmd::cmdShowUsersDefaultSetting().c_str());
    STRCMP_EQUAL("show users idle-detection-settings", ZyxelAuthCmd::cmdShowUsersIdleDetectionSettings().c_str());
    STRCMP_EQUAL("show users retry-settings", ZyxelAuthCmd::cmdShowUsersRetrySettings().c_str());
    STRCMP_EQUAL("show users simultaneous-logon-settings", ZyxelAuthCmd::cmdShowUsersSimultaneousLogonSettings().c_str());
    STRCMP_EQUAL("show users update-lease-settings", ZyxelAuthCmd::cmdShowUsersUpdateLeaseSettings().c_str());
    STRCMP_EQUAL("show users all", ZyxelAuthCmd::cmdShowUsers().c_str());

    // Ch 55: AAA Server
    STRCMP_EQUAL("show aaa group server radius rad_grp", ZyxelAuthCmd::cmdShowAaaGroupServerRadius("rad_grp").c_str());
    STRCMP_EQUAL("show aaa group server ldap ldap_grp", ZyxelAuthCmd::cmdShowAaaGroupServerLdap("ldap_grp").c_str());
    STRCMP_EQUAL("show aaa group server ad ad_grp", ZyxelAuthCmd::cmdShowAaaGroupServerAd("ad_grp").c_str());

    // Ch 56: Auth Objects
    STRCMP_EQUAL("show aaa authentication default", ZyxelAuthCmd::cmdShowAaaAuthentication().c_str());
    STRCMP_EQUAL("show aaa authentication my_auth", ZyxelAuthCmd::cmdShowAaaAuthentication("my_auth").c_str());

    // Ch 58: Certificates
    STRCMP_EQUAL("show ca category local", ZyxelAuthCmd::cmdShowCaCategoryLocal().c_str());
    STRCMP_EQUAL("show ca category remote", ZyxelAuthCmd::cmdShowCaCategoryRemote().c_str());
    STRCMP_EQUAL("show ca spaceusage", ZyxelAuthCmd::cmdShowCaSpaceusage().c_str());

    // Safe Bounded Mutations
    STRCMP_EQUAL("groupname test_grp", ZyxelAuthCmd::cmdGroupname("test_grp").c_str());
    STRCMP_EQUAL("no groupname test_grp", ZyxelAuthCmd::cmdNoGroupname("test_grp").c_str());
    STRCMP_EQUAL("aaa group server radius test_rad", ZyxelAuthCmd::cmdAaaGroupServerRadius("test_rad").c_str());
    STRCMP_EQUAL("no aaa group server radius test_rad", ZyxelAuthCmd::cmdNoAaaGroupServerRadius("test_rad").c_str());

    // Dry-fire helpers
    STRCMP_EQUAL("show dynamic-guest status", ZyxelAuthCmd::cmdInvalidDynamicGuestDryFire().c_str());
    STRCMP_EQUAL("show aaa-server", ZyxelAuthCmd::cmdInvalidAaaServerDryFire().c_str());
    STRCMP_EQUAL("show auth-method", ZyxelAuthCmd::cmdInvalidAuthMethodDryFire().c_str());
}

TEST(ZyxelAuthCmdTest, ParseCnmConfiguration) {
    bool active = false;
    const std::string raw =
        "Activate: YES\n"
        "ACS URL:\n"
        "ACS Username:\n";
    CHECK_TRUE(ZyxelAuthCmd::parseCnmConfiguration(raw, active));
    CHECK_TRUE(active);

    CHECK_TRUE(ZyxelAuthCmd::parseCnmConfiguration("Activate: NO\n", active));
    CHECK_FALSE(active);

    CHECK_FALSE(ZyxelAuthCmd::parseCnmConfiguration("Corrupted data\n", active));
}

TEST(ZyxelAuthCmdTest, ParseMonitorMode) {
    ZyxelCnmStatus st;
    const std::string raw =
        "active: no\n"
        "ID:\n"
        "status: N/A\n"
        "is_connected: no\n";
    CHECK_TRUE(ZyxelAuthCmd::parseMonitorMode(raw, st));
    CHECK_FALSE(st.active);
    STRCMP_EQUAL("N/A", st.status.c_str());
    CHECK_FALSE(st.isConnected);
}

TEST(ZyxelAuthCmdTest, ParseSecuReporterStatus) {
    ZyxelSecuReporterStatus st;
    const std::string raw =
        "activate: yes\n"
        "send-reporter: yes\n"
        "upload-interval: 600\n"
        "upload-filesize: 10\n"
        "banner: yes\n";
    CHECK_TRUE(ZyxelAuthCmd::parseSecuReporterStatus(raw, st));
    CHECK_TRUE(st.active);
    CHECK_TRUE(st.sendReporter);
    LONGS_EQUAL(600, st.uploadInterval);
    LONGS_EQUAL(10, st.uploadFilesize);
    CHECK_TRUE(st.banner);
}

TEST(ZyxelAuthCmdTest, ParseWebAuthActivation) {
    bool active = false;
    const std::string raw = "Auth Policy Activation: no\n";
    CHECK_TRUE(ZyxelAuthCmd::parseWebAuthActivation(raw, active));
    CHECK_FALSE(active);
}

TEST(ZyxelAuthCmdTest, ParseWebAuthPortalStatus) {
    std::string logoutIp;
    bool sessionPage = false;
    const std::string raw =
        "Logout IP : 6.6.6.6\n"
        "Session Page : yes\n";
    CHECK_TRUE(ZyxelAuthCmd::parseWebAuthPortalStatus(raw, logoutIp, sessionPage));
    STRCMP_EQUAL("6.6.6.6", logoutIp.c_str());
    CHECK_TRUE(sessionPage);
}

TEST(ZyxelAuthCmdTest, ParseSsoPort) {
    int port = 0;
    const std::string raw = "ZySSO port: 2158\n";
    CHECK_TRUE(ZyxelAuthCmd::parseSsoPort(raw, port));
    LONGS_EQUAL(2158, port);
}

TEST(ZyxelAuthCmdTest, ParseBillingStatus) {
    std::string method;
    int accumTimeout = 0;
    const std::string raw =
        "unused_expire: 24 hour\n"
        "Billing accounting method: time-to-finish\n"
        "Accumulation idle timeout: 3\n"
        "accumulation_expire: 90 day\n"
        "Billing currency type: symbol\n";
    CHECK_TRUE(ZyxelAuthCmd::parseBillingStatus(raw, method, accumTimeout));
    STRCMP_EQUAL("time-to-finish", method.c_str());
    LONGS_EQUAL(3, accumTimeout);
}

TEST(ZyxelAuthCmdTest, ParseFreeTimeStatus) {
    bool active = true;
    std::string period;
    const std::string raw =
        "Activate: no\n"
        "Auto Login: no\n"
        "Time Period: 30 minutes\n";
    CHECK_TRUE(ZyxelAuthCmd::parseFreeTimeStatus(raw, active, period));
    CHECK_FALSE(active);
    STRCMP_EQUAL("30 minutes", period.c_str());
}

TEST(ZyxelAuthCmdTest, ParseUsers) {
    std::vector<ZyxelUserEntry> users;
    const std::string raw =
        "No.  Username                         User Type      Description\n"
        "===============================================================================\n"
        "1    admin                            admin          Administration account\n"
        "2    ldap-users                       ext-user       External LDAP Users\n"
        "3    radius-users                     ext-user       External RADIUS Users\n";
    CHECK_TRUE(ZyxelAuthCmd::parseUsers(raw, users));
    LONGS_EQUAL(3, users.size());
    LONGS_EQUAL(1, users[0].no);
    STRCMP_EQUAL("admin", users[0].username.c_str());
    STRCMP_EQUAL("admin", users[0].userType.c_str());
    STRCMP_EQUAL("Administration account", users[0].description.c_str());
}

TEST(ZyxelAuthCmdTest, ParseUsersRetrySettings) {
    ZyxelUserRetrySettings st;
    const std::string raw =
        "enable logon retry limit: yes\n"
        "maximum retry count     : 5\n"
        "lockout period          : 30\n";
    CHECK_TRUE(ZyxelAuthCmd::parseUsersRetrySettings(raw, st));
    CHECK_TRUE(st.retryLimitActive);
    LONGS_EQUAL(5, st.maxRetryCount);
    LONGS_EQUAL(30, st.lockoutPeriod);
}

TEST(ZyxelAuthCmdTest, ParseCaSpaceUsage) {
    ZyxelCaSpaceUsage st;
    const std::string raw =
        "PKI storage space:\n"
        "total: 262144 bytes\n"
        "available: 251637 bytes (96%)\n"
        "in use: 10507 bytes (4%)\n"
        "my certificate: 5283 bytes (2%)\n";
    CHECK_TRUE(ZyxelAuthCmd::parseCaSpaceUsage(raw, st));
    LONGS_EQUAL(262144, st.total);
    LONGS_EQUAL(251637, st.available);
    LONGS_EQUAL(10507, st.inUse);
    LONGS_EQUAL(5283, st.myCert);
}

TEST(ZyxelAuthCmdTest, ParseCaCertInfo) {
    std::vector<ZyxelCaCertInfo> certs;
    const std::string raw =
        "certificate: edge_certificate\n"
        "type: CERT\n"
        "subject: CN=S232L37100891, O=ZyXEL Communications Corporation, ST=Taiwan, C=TW\n"
        "issuer: CN=Nebula Star CA - VV, O=ZyXEL Communications Corporation, ST=Taiwan, C=TW\n"
        "status: VALID\n";
    CHECK_TRUE(ZyxelAuthCmd::parseCaCertInfo(raw, certs));
    LONGS_EQUAL(1, certs.size());
    STRCMP_EQUAL("edge_certificate", certs[0].certificate.c_str());
    STRCMP_EQUAL("CERT", certs[0].type.c_str());
    STRCMP_EQUAL("VALID", certs[0].status.c_str());
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
