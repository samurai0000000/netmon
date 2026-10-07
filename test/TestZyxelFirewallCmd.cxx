/*
 * TestZyxelFirewallCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelFirewallCmd.hxx"
#include <CppUTest/TestHarness.h>

TEST_GROUP(ZyxelFirewallCmdTest) {
    void setup() {}
    void teardown() {}
};

TEST(ZyxelFirewallCmdTest, CommandGeneratorsReturnExpectedStrings) {
    STRCMP_EQUAL("show secure-policy", ZyxelFirewallCmd::cmdShowSecurePolicy().c_str());
    STRCMP_EQUAL("show secure-policy 11", ZyxelFirewallCmd::cmdShowSecurePolicy("11").c_str());
    STRCMP_EQUAL("no secure-policy 1", ZyxelFirewallCmd::cmdDeleteRule("1").c_str());
    STRCMP_EQUAL("no secure-policy name NETMON_RULE_1", ZyxelFirewallCmd::cmdDeleteRule("NETMON_RULE_1").c_str());

    auto fastDeny = ZyxelFirewallCmd::cmdInsertFastDeny(1, "NETMON_RULE_192_0_2_55",
                                                       "NETMON_BLK_192_0_2_55",
                                                       "Excessive WAN requests");
    LONGS_EQUAL(0, fastDeny.size());

    // General rule insertion sequence
    ZyxelFirewallRule rule;
    rule.name = "CustomRule";
    rule.description = "Test description";
    rule.fromZone = "WAN";
    rule.toZone = "LAN";
    rule.sourceIp = "HostA";
    rule.destinationIp = "HostB";
    rule.service = "HTTP";
    rule.action = "allow";
    rule.active = true;

    auto fullSeq = ZyxelFirewallCmd::cmdInsertRule(2, rule);
    LONGS_EQUAL(0, fullSeq.size());
}

TEST(ZyxelFirewallCmdTest, ParseSecurePolicyManualPage225Transcript) {
    // Official transcript from manual page 225
    std::string raw =
        "secure-policy rule: 11\n"
        "  name: WAN_to_Device\n"
        "  description: Default allow rule\n"
        "  user: any, schedule: none\n"
        "  from: WAN, to: ZyWALL\n"
        "  source IP: any, source port: any\n"
        "  destination IP: any, service: Default_Allow_WAN_To_ZyWALL\n"
        "  log: no, action: allow, status: yes\n"
        "  connection match: no\n";

    std::vector<ZyxelFirewallRule> rules;
    CHECK_TRUE(ZyxelFirewallCmd::parseSecurePolicy(raw, rules));
    LONGS_EQUAL(1, rules.size());

    LONGS_EQUAL(11, rules[0].index);
    STRCMP_EQUAL("WAN_to_Device", rules[0].name.c_str());
    STRCMP_EQUAL("Default allow rule", rules[0].description.c_str());
    STRCMP_EQUAL("WAN", rules[0].fromZone.c_str());
    STRCMP_EQUAL("ZyWALL", rules[0].toZone.c_str());
    STRCMP_EQUAL("any", rules[0].sourceIp.c_str());
    STRCMP_EQUAL("any", rules[0].destinationIp.c_str());
    STRCMP_EQUAL("Default_Allow_WAN_To_ZyWALL", rules[0].service.c_str());
    STRCMP_EQUAL("allow", rules[0].action.c_str());
    CHECK_TRUE(rules[0].active);
}

TEST(ZyxelFirewallCmdTest, ParseSecurePolicyTableFormat) {
    std::string raw =
        "Rule  Name             From   To      Source     Destination  Service   Action  Status\n"
        "====================================================================================\n"
        "1     NETMON_RULE_1    any    any     NETMON_1   any          any       deny    yes\n"
        "11    WAN_to_Device    WAN    ZyWALL  any        any          Default   allow   yes\n";

    std::vector<ZyxelFirewallRule> rules;
    CHECK_TRUE(ZyxelFirewallCmd::parseSecurePolicy(raw, rules));
    LONGS_EQUAL(2, rules.size());

    LONGS_EQUAL(1, rules[0].index);
    STRCMP_EQUAL("NETMON_RULE_1", rules[0].name.c_str());
    STRCMP_EQUAL("any", rules[0].fromZone.c_str());
    STRCMP_EQUAL("deny", rules[0].action.c_str());
    CHECK_TRUE(rules[0].active);

    LONGS_EQUAL(11, rules[1].index);
    STRCMP_EQUAL("WAN_to_Device", rules[1].name.c_str());
    STRCMP_EQUAL("allow", rules[1].action.c_str());
}

TEST(ZyxelFirewallCmdTest, ParseSecurePolicyTableWithSpacesInRuleName) {
    std::string raw =
        "Rule  Name                     From   To      Source     Destination  Service   Action  Status\n"
        "==============================================================================================\n"
        "1     Default LAN Rule with Sp LAN    WAN     any        any          any       allow   yes\n"
        "2     Drop Inbound Traffic     WAN    LAN     any        any          any       deny    yes\n";

    std::vector<ZyxelFirewallRule> rules;
    CHECK_TRUE(ZyxelFirewallCmd::parseSecurePolicy(raw, rules));
    LONGS_EQUAL(2, rules.size());

    LONGS_EQUAL(1, rules[0].index);
    STRCMP_EQUAL("Default LAN Rule with Sp", rules[0].name.c_str());
    STRCMP_EQUAL("LAN", rules[0].fromZone.c_str());
    STRCMP_EQUAL("WAN", rules[0].toZone.c_str());
    STRCMP_EQUAL("allow", rules[0].action.c_str());

    LONGS_EQUAL(2, rules[1].index);
    STRCMP_EQUAL("Drop Inbound Traffic", rules[1].name.c_str());
    STRCMP_EQUAL("WAN", rules[1].fromZone.c_str());
    STRCMP_EQUAL("LAN", rules[1].toZone.c_str());
    STRCMP_EQUAL("deny", rules[1].action.c_str());
}

TEST(ZyxelFirewallCmdTest, ParseErrorsOnCorruptedBuffers) {
    std::vector<ZyxelFirewallRule> rules;
    CHECK_FALSE(ZyxelFirewallCmd::parseSecurePolicy("Random garbage % syntax error", rules));
    CHECK_FALSE(ZyxelFirewallCmd::parseSecurePolicy("No policy configured", rules));

    ZyxelSecurePolicyStatus status;
    CHECK_FALSE(ZyxelFirewallCmd::parseSecurePolicyStatus("Random garbage", status));
}

TEST(ZyxelFirewallCmdTest, Envelope6CommandGenerators) {
    STRCMP_EQUAL("show secure-policy status", ZyxelFirewallCmd::cmdShowSecurePolicyStatus().c_str());
    STRCMP_EQUAL("show secure-policy block_rules", ZyxelFirewallCmd::cmdShowSecurePolicyBlockRules().c_str());
    STRCMP_EQUAL("show secure-policy6", ZyxelFirewallCmd::cmdShowSecurePolicy6().c_str());
    STRCMP_EQUAL("show secure-policy6 5", ZyxelFirewallCmd::cmdShowSecurePolicy6("5").c_str());

    ZyxelFirewallRule rule;
    rule.name = "AppendRule";
    rule.description = "Test append";
    rule.fromZone = "LAN";
    rule.toZone = "WAN";
    rule.sourceIp = "HostX";
    rule.destinationIp = "HostY";
    rule.service = "HTTPS";
    rule.action = "deny";
    rule.active = false;

    auto appendSeq = ZyxelFirewallCmd::cmdAppendRule(rule);
    LONGS_EQUAL(0, appendSeq.size());

    auto appendFastDeny = ZyxelFirewallCmd::cmdAppendFastDeny("NETMON_QA_RULE", "NETMON_QA_HOST", "Fast block");
    LONGS_EQUAL(0, appendFastDeny.size());

    STRCMP_EQUAL("no secure-policy name NETMON_QA_RULE",
                 ZyxelFirewallCmd::cmdDeleteRuleByName("NETMON_QA_RULE").c_str());
    STRCMP_EQUAL("no secure-policy 5", ZyxelFirewallCmd::cmdDeleteRuleByNumber(5).c_str());

    STRCMP_EQUAL("secure-policy 1 activate", ZyxelFirewallCmd::cmdActivateRule("1", true).c_str());
    STRCMP_EQUAL("no secure-policy name NETMON_QA_RULE activate",
                 ZyxelFirewallCmd::cmdActivateRule("NETMON_QA_RULE", false).c_str());

    STRCMP_EQUAL("secure-policy activate", ZyxelFirewallCmd::cmdActivateSecurePolicy(true).c_str());
    STRCMP_EQUAL("no secure-policy activate", ZyxelFirewallCmd::cmdActivateSecurePolicy(false).c_str());
    STRCMP_EQUAL("secure-policy6 activate", ZyxelFirewallCmd::cmdActivateSecurePolicy6(true).c_str());
    STRCMP_EQUAL("no secure-policy6 activate", ZyxelFirewallCmd::cmdActivateSecurePolicy6(false).c_str());
    STRCMP_EQUAL("secure-policy asymmetrical-route activate",
                 ZyxelFirewallCmd::cmdSetAsymmetricalRoute(true).c_str());
    STRCMP_EQUAL("no secure-policy asymmetrical-route activate",
                 ZyxelFirewallCmd::cmdSetAsymmetricalRoute(false).c_str());

    STRCMP_EQUAL("show device-ha", ZyxelFirewallCmd::cmdShowDeviceHa().c_str());
    STRCMP_EQUAL("show device-ha status", ZyxelFirewallCmd::cmdShowDeviceHaStatus().c_str());
    STRCMP_EQUAL("show device-ha mode", ZyxelFirewallCmd::cmdShowDeviceHaMode().c_str());
    STRCMP_EQUAL("show device-ha2", ZyxelFirewallCmd::cmdShowDeviceHa2().c_str());
    STRCMP_EQUAL("show device-ha2 interfaces", ZyxelFirewallCmd::cmdShowDeviceHa2Interfaces().c_str());
    STRCMP_EQUAL("show device-ha2 device-status", ZyxelFirewallCmd::cmdShowDeviceHa2DeviceStatus().c_str());

    STRCMP_EQUAL("show secure-policy 999", ZyxelFirewallCmd::cmdInvalidSecurePolicyDryFire().c_str());
    STRCMP_EQUAL("show secure-policy6 999", ZyxelFirewallCmd::cmdInvalidSecurePolicy6DryFire().c_str());
    STRCMP_EQUAL("show device-ha invalid_probe_999", ZyxelFirewallCmd::cmdInvalidDeviceHaDryFire().c_str());
    STRCMP_EQUAL("show device-ha2 invalid_probe_999", ZyxelFirewallCmd::cmdInvalidDeviceHa2DryFire().c_str());
}

TEST(ZyxelFirewallCmdTest, ParseSecurePolicyStatusTranscript) {
    std::string raw =
        "secure-policy status: yes\n"
        "secure-policy asymmetrical route status: no\n"
        "secure-policy default rule: deny, no log\n"
        "secure-policy tcp flag detect: yes\n";

    ZyxelSecurePolicyStatus status;
    CHECK_TRUE(ZyxelFirewallCmd::parseSecurePolicyStatus(raw, status));
    CHECK_TRUE(status.active);
    CHECK_FALSE(status.asymmetricalRoute);
    STRCMP_EQUAL("deny, no log", status.defaultRule.c_str());
    CHECK_TRUE(status.tcpFlagDetect);
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
