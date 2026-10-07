/*
 * ZyxelSecurityCmd.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXEL_SECURITY_CMD_HXX
#define NETMON_ZYXEL_SECURITY_CMD_HXX

#include "zyxel/ZyxelTypes.hxx"
#include <string>

class ZyxelSecurityCmd {
public:
    // Existing Command generators
    static std::string cmdShowAppStatisticsSummary();
    static std::string cmdShowIdpStatisticsSummary();
    static std::string cmdShowConnStatus();

    // Existing Parsers
    static bool parseConnStatus(const std::string &raw, ZyxelSessionSummary &out);
    static bool parseAppStatisticsSummary(const std::string &raw, ZyxelAppPatrolSummary &out);
    static bool parseIdpStatisticsSummary(const std::string &raw, ZyxelIdpSummary &out);

    // Envelope 7: Bandwidth Management (Ch 36)
    static std::string cmdShowBwmActivation();
    static std::string cmdShowBwmAll();
    static std::string cmdShowBwm(const std::string &rule = "");
    static std::string cmdShowBwmControlTcpAck();
    static std::string cmdShowBwmDefault();
    static std::string cmdShowBwmApplicationsList();
    static std::string cmdBwmAppend();
    static std::string cmdBwmDelete(int index);
    static std::string cmdBwmActivate(bool enable);
    static std::string cmdBwmControlTcpAckActivate(bool enable);
    static std::string cmdBwmDefaultInboundPriority(int prio);
    static std::string cmdBwmDefaultOutboundPriority(int prio);

    // Envelope 7: Application Patrol (Ch 37)
    static std::string cmdShowAppProfiles();
    static std::string cmdAppProfile(const std::string &name);
    static std::string cmdNoAppProfile(const std::string &name);

    // Envelope 7: Anti-Virus (Ch 38)
    static std::string cmdShowAntiVirusProfile();
    static std::string cmdShowAntiVirusUpdate();
    static std::string cmdShowAntiVirusStatistics();
    static std::string cmdAntiVirusProfile(const std::string &name);
    static std::string cmdNoAntiVirusProfile(const std::string &name);

    // Envelope 7: RTLS (Ch 39)
    static std::string cmdShowRtls();

    // Envelope 7: Reputation Filter (Ch 40)
    static std::string cmdShowReputationFilterStatus();
    static std::string cmdShowReputationFilterIpReputationStatus();
    static std::string cmdShowReputationFilterDnsThreatFilterStatus();
    static std::string cmdShowReputationFilterUrlThreatFilterStatus();

    // Envelope 7: Sandboxing (Ch 41)
    static std::string cmdShowSandboxingStatus();

    // Envelope 7: IDP (Ch 42)
    static std::string cmdShowIdpEngine();
    static std::string cmdShowIdpSignature();

    // Envelope 7: Content Filtering (Ch 43)
    static std::string cmdShowContentFilterProfile();
    static std::string cmdShowContentFilterSettings();
    static std::string cmdShowContentFilterStatistics();
    static std::string cmdContentFilterCfQueueFlush();

    // Envelope 7: Anti-Spam (Ch 44)
    static std::string cmdShowAntiSpamProfile();
    static std::string cmdShowAntiSpamStatistics();
    static std::string cmdAntiSpamProfileAppend();
    static std::string cmdAntiSpamProfileDelete(int index);

    // Envelope 7: CDR (Ch 45)
    static std::string cmdShowCdrStatus();
    static std::string cmdShowCdrBlockList();
    static std::string cmdShowCdrRules();
    static std::string cmdCdrActivate(bool enable);

    // Envelope 7: SSL Inspection (Ch 46)
    static std::string cmdShowSslInspectionStatus();
    static std::string cmdShowSslInspectionProfile();
    static std::string cmdShowSslInspectionStatistics();
    static std::string cmdSslInspectionProfile(const std::string &name);
    static std::string cmdNoSslInspectionProfile(const std::string &name);

    // Dry-fire and negative testing helpers
    static std::string cmdInvalidBwmDryFire();
    static std::string cmdInvalidAppDryFire();
    static std::string cmdInvalidAntiVirusDryFire();
    static std::string cmdInvalidSslInspectionDryFire();

    // Envelope 7 Parsers
    static bool parseBwmActivation(const std::string &raw, bool &active);
    static bool parseBwmControlTcpAck(const std::string &raw, bool &active);
    static bool parseBwmAll(const std::string &raw, std::vector<ZyxelBwmRule> &rules);
    static bool parseCdrStatus(const std::string &raw, ZyxelCdrStatus &status);
    static bool parseCdrRules(const std::string &raw, std::vector<ZyxelCdrRule> &rules);
    static bool parseSslInspectionStatus(const std::string &raw, ZyxelSslInspectionStatus &status);
    static bool parseContentFilterSettings(const std::string &raw, ZyxelContentFilterSettings &settings);
};

#endif /* NETMON_ZYXEL_SECURITY_CMD_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
