/*
 * ZyxelFirewallCmd.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXEL_FIREWALL_CMD_HXX
#define NETMON_ZYXEL_FIREWALL_CMD_HXX

#include "zyxel/ZyxelTypes.hxx"
#include <string>
#include <vector>

class ZyxelFirewallCmd {
public:
    // Command generators - Secure Policy Operational Reads
    static std::string cmdShowSecurePolicy(const std::string &nameOrNum = "");
    static std::string cmdShowSecurePolicyStatus();
    static std::string cmdShowSecurePolicyBlockRules();
    static std::string cmdShowSecurePolicy6(const std::string &nameOrNum = "");

    // Command generators - Rule Insertion & Appending
    static std::vector<std::string> cmdInsertRule(int position, const ZyxelFirewallRule &rule);
    static std::vector<std::string> cmdInsertFastDeny(int position,
                                                     const std::string &ruleName,
                                                     const std::string &srcObjName,
                                                     const std::string &reason);
    static std::vector<std::string> cmdAppendRule(const ZyxelFirewallRule &rule);
    static std::vector<std::string> cmdAppendFastDeny(const std::string &ruleName,
                                                     const std::string &srcObjName,
                                                     const std::string &reason);
    static std::string cmdDeleteRule(const std::string &nameOrNum);
    static std::string cmdDeleteRuleByName(const std::string &name);
    static std::string cmdDeleteRuleByNumber(int num);

    // Command generators - Policy Control & Configuration
    static std::string cmdActivateRule(const std::string &nameOrNum, bool activate);
    static std::string cmdActivateSecurePolicy(bool activate);
    static std::string cmdActivateSecurePolicy6(bool activate);
    static std::string cmdSetAsymmetricalRoute(bool enable);

    // Command generators - Device HA (Ch 48)
    static std::string cmdShowDeviceHa();
    static std::string cmdShowDeviceHaStatus();
    static std::string cmdShowDeviceHaMode();
    static std::string cmdShowDeviceHa2();
    static std::string cmdShowDeviceHa2Interfaces();
    static std::string cmdShowDeviceHa2DeviceStatus();

    // Command generators - Dry-Fire Rejections
    static std::string cmdInvalidSecurePolicyDryFire();
    static std::string cmdInvalidSecurePolicy6DryFire();
    static std::string cmdInvalidDeviceHaDryFire();
    static std::string cmdInvalidDeviceHa2DryFire();

    // Parsers
    static bool parseSecurePolicy(const std::string &raw, std::vector<ZyxelFirewallRule> &out);
    static bool parseSecurePolicyStatus(const std::string &raw, ZyxelSecurePolicyStatus &out);
};

#endif /* NETMON_ZYXEL_FIREWALL_CMD_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
