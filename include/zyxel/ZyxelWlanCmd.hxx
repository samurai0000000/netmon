/*
 * ZyxelWlanCmd.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXEL_WLAN_CMD_HXX
#define NETMON_ZYXEL_WLAN_CMD_HXX

#include <string>
#include <vector>
#include "ZyxelTypes.hxx"

class ZyxelWlanCmd {
public:
    /* Command Generators - Ch 6: AP Management & General WLAN */
    static std::string cmdShowCapwapApAll();
    static std::string cmdShowCapwapApConfigStatus();
    static std::string cmdShowCapwapApStatistics();
    static std::string cmdShowCapwapApFallback();
    static std::string cmdShowCapwapApFallbackInterval();
    static std::string cmdShowCapwapApIdleTimeout();
    static std::string cmdShowCapwapApWaitList();
    static std::string cmdShowCapwapManualAdd();
    static std::string cmdShowCapwapStationAll();
    static std::string cmdShowCountryCodeList();
    static std::string cmdShowDefaultCountryCode();
    static std::string cmdShowVpnPolicyPool();

    /* Command Generators - Ch 8: AP Group */
    static std::string cmdShowApGroupFirstPriority();
    static std::string cmdShowApGroupProfileAll();
    static std::string cmdShowApGroupProfileRuleCount();

    /* Command Generators - Ch 9: WLAN Profiles */
    static std::string cmdShowWlanMacfilterProfileAll();
    static std::string cmdShowWlanMonitorProfileAll();
    static std::string cmdShowWlanRadioProfileAll();
    static std::string cmdShowWlanSecurityProfileAll();
    static std::string cmdShowWlanSsidProfileAll();
    static std::string cmdShowZymeshApInfo();
    static std::string cmdShowZymeshProvisionGroup();
    static std::string cmdShowZymeshProfileAll();

    /* Command Generators - Ch 10: Rogue AP */
    static std::string cmdShowRogueApContainmentConfig();
    static std::string cmdShowRogueApContainmentList();
    static std::string cmdShowRogueApDetectionInfo();
    static std::string cmdShowRogueApDetectionListAll();
    static std::string cmdShowRogueApDetectionMonitoring();
    static std::string cmdShowRogueApDetectionStatus();

    /* Command Generators - Ch 11: Wireless Health */
    static std::string cmdShowWirelessHealthAction();
    static std::string cmdShowApInfoTopAlert(const std::string &band = "all");
    static std::string cmdShowStaInfoTopAlert(const std::string &band = "all");

    /* Command Generators - Ch 12: Wireless Frame Capture */
    static std::string cmdShowFrameCaptureConfig();
    static std::string cmdShowFrameCaptureStatus();

    /* Command Generators - Ch 14: Auto-Healing */
    static std::string cmdShowAutoHealingConfig();

    /* Command Generators - Ch 73: Managed AP Commands */
    static std::string cmdShowCapwapApAcIp();
    static std::string cmdShowCapwapApDiscoveryType();
    static std::string cmdShowCapwapApInfo();

    /* Reversible Tier 2 Mutations */
    static std::string cmdWlanSsidProfile(const std::string &name);
    static std::string cmdNoWlanSsidProfile(const std::string &name);
    static std::string cmdWlanSecurityProfile(const std::string &name);
    static std::string cmdNoWlanSecurityProfile(const std::string &name);

    /* Output Parsers */
    static bool parseCapwapApInfo(const std::string &output,
                                 ZyxelCapwapApInfo &info);
    static bool parseCapwapFallback(const std::string &output,
                                   ZyxelCapwapFallback &fallback);
    static bool parseRogueApInfo(const std::string &output,
                                ZyxelRogueApInfo &info);
    static bool parseRogueApStatus(const std::string &output,
                                  ZyxelRogueApStatus &status);
    static bool parseAutoHealingConfig(const std::string &output,
                                      ZyxelAutoHealingConfig &config);
    static bool parseFrameCaptureConfig(const std::string &output,
                                       ZyxelFrameCaptureConfig &config);
    static bool parseZyMeshInfo(const std::string &output,
                               ZyxelZyMeshInfo &info);
};

#endif /* NETMON_ZYXEL_WLAN_CMD_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
