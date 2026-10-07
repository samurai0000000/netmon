/*
 * ZyxelAuthCmd.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXEL_AUTH_CMD_HXX
#define NETMON_ZYXEL_AUTH_CMD_HXX

#include "zyxel/ZyxelTypes.hxx"
#include <string>
#include <vector>

class ZyxelAuthCmd {
public:
    // Ch 30: Cloud CNM
    static std::string cmdShowCnmAgentConfiguration();
    static std::string cmdShowMonitorMode();
    static std::string cmdShowSecuReporterStatus();
    static std::string cmdShowSecumanagerStatus();

    // Ch 31: Web Authentication
    static std::string cmdShowWebAuthActivation();
    static std::string cmdShowWebAuthDefaultRule();
    static std::string cmdShowWebAuthExceptionalService();
    static std::string cmdShowWebAuthMethod();
    static std::string cmdShowWebAuthPolicy(const std::string &policy = "all");
    static std::string cmdShowWebAuthPortalStatus();
    static std::string cmdShowWebAuthRedirectFqdn();
    static std::string cmdShowWebAuthStatus();
    static std::string cmdShowSsoAgent();
    static std::string cmdShowSsoPort();

    // Ch 32: Hotspot
    static std::string cmdShowAdvertisementActivation();
    static std::string cmdShowBillingStatus();
    static std::string cmdShowFreeTimeStatus();
    static std::string cmdShowIpIpnpActivation();
    static std::string cmdShowPaymentServiceActivation();

    // Ch 50: User/Group
    static std::string cmdShowUsername(const std::string &name = "");
    static std::string cmdShowGroupname(const std::string &name = "");
    static std::string cmdShowLockoutUsers();
    static std::string cmdShowPasswordComplexityVerifyStatus();
    static std::string cmdShowPwdExpiry(const std::string &type = "all");
    static std::string cmdShowUsersDefaultSetting(const std::string &type = "all");
    static std::string cmdShowUsersIdleDetectionSettings();
    static std::string cmdShowUsersRetrySettings();
    static std::string cmdShowUsersSimultaneousLogonSettings();
    static std::string cmdShowUsersUpdateLeaseSettings();
    static std::string cmdShowUsers(const std::string &target = "all");

    // Ch 55: AAA Server
    static std::string cmdShowAaaGroupServerRadius(const std::string &name);
    static std::string cmdShowAaaGroupServerLdap(const std::string &name);
    static std::string cmdShowAaaGroupServerAd(const std::string &name);

    // Ch 56: Authentication Objects
    static std::string cmdShowAaaAuthentication(const std::string &name = "default");

    // Ch 58: Certificates
    static std::string cmdShowCaCategoryLocal();
    static std::string cmdShowCaCategoryRemote();
    static std::string cmdShowCaSpaceusage();

    // Safe Bounded Mutations
    static std::string cmdGroupname(const std::string &name);
    static std::string cmdNoGroupname(const std::string &name);
    static std::string cmdAaaGroupServerRadius(const std::string &name);
    static std::string cmdNoAaaGroupServerRadius(const std::string &name);

    // Dry-fire and negative testing helpers
    static std::string cmdInvalidDynamicGuestDryFire();
    static std::string cmdInvalidAaaServerDryFire();
    static std::string cmdInvalidAuthMethodDryFire();

    // Parsers
    static bool parseCnmConfiguration(const std::string &raw, bool &active);
    static bool parseMonitorMode(const std::string &raw, ZyxelCnmStatus &out);
    static bool parseSecuReporterStatus(const std::string &raw, ZyxelSecuReporterStatus &out);
    static bool parseWebAuthActivation(const std::string &raw, bool &active);
    static bool parseWebAuthPortalStatus(const std::string &raw, std::string &logoutIp, bool &sessionPage);
    static bool parseSsoPort(const std::string &raw, int &port);
    static bool parseBillingStatus(const std::string &raw, std::string &acctMethod, int &accumTimeout);
    static bool parseFreeTimeStatus(const std::string &raw, bool &active, std::string &period);
    static bool parseUsers(const std::string &raw, std::vector<ZyxelUserEntry> &out);
    static bool parseUsersRetrySettings(const std::string &raw, ZyxelUserRetrySettings &out);
    static bool parseCaSpaceUsage(const std::string &raw, ZyxelCaSpaceUsage &out);
    static bool parseCaCertInfo(const std::string &raw, std::vector<ZyxelCaCertInfo> &out);
};

#endif /* NETMON_ZYXEL_AUTH_CMD_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
