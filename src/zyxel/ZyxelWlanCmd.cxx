/*
 * ZyxelWlanCmd.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelWlanCmd.hxx"
#include <sstream>
#include <regex>
#include <iostream>

/* Command Generators - Ch 6: AP Management & General WLAN */

std::string ZyxelWlanCmd::cmdShowCapwapApAll()
{
    return "show capwap ap all";
}

std::string ZyxelWlanCmd::cmdShowCapwapApConfigStatus()
{
    return "show capwap ap all config status";
}

std::string ZyxelWlanCmd::cmdShowCapwapApStatistics()
{
    return "show capwap ap all statistics";
}

std::string ZyxelWlanCmd::cmdShowCapwapApFallback()
{
    return "show capwap ap fallback";
}

std::string ZyxelWlanCmd::cmdShowCapwapApFallbackInterval()
{
    return "show capwap ap fallback interval";
}

std::string ZyxelWlanCmd::cmdShowCapwapApIdleTimeout()
{
    return "show capwap ap idle timeout";
}

std::string ZyxelWlanCmd::cmdShowCapwapApWaitList()
{
    return "show capwap ap wait-list";
}

std::string ZyxelWlanCmd::cmdShowCapwapManualAdd()
{
    return "show capwap manual-add";
}

std::string ZyxelWlanCmd::cmdShowCapwapStationAll()
{
    return "show capwap station all";
}

std::string ZyxelWlanCmd::cmdShowCountryCodeList()
{
    return "show country-code list";
}

std::string ZyxelWlanCmd::cmdShowDefaultCountryCode()
{
    return "show default country-code";
}

std::string ZyxelWlanCmd::cmdShowVpnPolicyPool()
{
    return "show vpn-policy-pool";
}

/* Command Generators - Ch 8: AP Group */

std::string ZyxelWlanCmd::cmdShowApGroupFirstPriority()
{
    return "show ap-group first-priority";
}

std::string ZyxelWlanCmd::cmdShowApGroupProfileAll()
{
    return "show ap-group-profile all";
}

std::string ZyxelWlanCmd::cmdShowApGroupProfileRuleCount()
{
    return "show ap-group-profile rule_count";
}

/* Command Generators - Ch 9: WLAN Profiles */

std::string ZyxelWlanCmd::cmdShowWlanMacfilterProfileAll()
{
    return "show wlan-macfilter-profile all";
}

std::string ZyxelWlanCmd::cmdShowWlanMonitorProfileAll()
{
    return "show wlan-monitor-profile all";
}

std::string ZyxelWlanCmd::cmdShowWlanRadioProfileAll()
{
    return "show wlan-radio-profile all";
}

std::string ZyxelWlanCmd::cmdShowWlanSecurityProfileAll()
{
    return "show wlan-security-profile all";
}

std::string ZyxelWlanCmd::cmdShowWlanSsidProfileAll()
{
    return "show wlan-ssid-profile all";
}

std::string ZyxelWlanCmd::cmdShowZymeshApInfo()
{
    return "show zymesh ap info";
}

std::string ZyxelWlanCmd::cmdShowZymeshProvisionGroup()
{
    return "show zymesh provision-group";
}

std::string ZyxelWlanCmd::cmdShowZymeshProfileAll()
{
    return "show zymesh-profile all";
}

/* Command Generators - Ch 10: Rogue AP */

std::string ZyxelWlanCmd::cmdShowRogueApContainmentConfig()
{
    return "show rogue-ap containment config";
}

std::string ZyxelWlanCmd::cmdShowRogueApContainmentList()
{
    return "show rogue-ap containment list";
}

std::string ZyxelWlanCmd::cmdShowRogueApDetectionInfo()
{
    return "show rogue-ap detection info";
}

std::string ZyxelWlanCmd::cmdShowRogueApDetectionListAll()
{
    return "show rogue-ap detection list all";
}

std::string ZyxelWlanCmd::cmdShowRogueApDetectionMonitoring()
{
    return "show rogue-ap detection monitoring";
}

std::string ZyxelWlanCmd::cmdShowRogueApDetectionStatus()
{
    return "show rogue-ap detection status";
}

/* Command Generators - Ch 11: Wireless Health */

std::string ZyxelWlanCmd::cmdShowWirelessHealthAction()
{
    return "show wireless-health-action";
}

std::string ZyxelWlanCmd::cmdShowApInfoTopAlert(const std::string &band)
{
    return "show ap-info top 10 alert " + band;
}

std::string ZyxelWlanCmd::cmdShowStaInfoTopAlert(const std::string &band)
{
    return "show sta-info top 10 alert " + band;
}

/* Command Generators - Ch 12: Wireless Frame Capture */

std::string ZyxelWlanCmd::cmdShowFrameCaptureConfig()
{
    return "show frame-capture config";
}

std::string ZyxelWlanCmd::cmdShowFrameCaptureStatus()
{
    return "show frame-capture status";
}

/* Command Generators - Ch 14: Auto-Healing */

std::string ZyxelWlanCmd::cmdShowAutoHealingConfig()
{
    return "show auto-healing config";
}

/* Command Generators - Ch 73: Managed AP Commands */

std::string ZyxelWlanCmd::cmdShowCapwapApAcIp()
{
    return "show capwap ap ac-ip";
}

std::string ZyxelWlanCmd::cmdShowCapwapApDiscoveryType()
{
    return "show capwap ap discovery-type";
}

std::string ZyxelWlanCmd::cmdShowCapwapApInfo()
{
    return "show capwap ap info";
}

/* Reversible Tier 2 Mutations */

std::string ZyxelWlanCmd::cmdWlanSsidProfile(const std::string &name)
{
    return "wlan-ssid-profile " + name;
}

std::string ZyxelWlanCmd::cmdNoWlanSsidProfile(const std::string &name)
{
    return "no wlan-ssid-profile " + name;
}

std::string ZyxelWlanCmd::cmdWlanSecurityProfile(const std::string &name)
{
    return "wlan-security-profile " + name;
}

std::string ZyxelWlanCmd::cmdNoWlanSecurityProfile(const std::string &name)
{
    return "no wlan-security-profile " + name;
}

/* Output Parsers */

bool ZyxelWlanCmd::parseCapwapApInfo(const std::string &output,
                                     ZyxelCapwapApInfo &info)
{
    info = ZyxelCapwapApInfo();
    bool found = false;

    std::regex reOnline(R"(Online mgnt ap\s*:\s*(\d+))", std::regex::icase);
    std::regex reOffline(R"(Offline mgnt ap\s*:\s*(\d+))", std::regex::icase);
    std::regex reUnmgnt(R"(Un-mgnt ap\s*:\s*(\d+))", std::regex::icase);
    std::regex reStation(R"(Station\s*:\s*(\d+))", std::regex::icase);
    std::regex reRemote(R"(Remote AP\s*:\s*(\d+))", std::regex::icase);
    std::regex reUpdate(R"(Update available\s*:\s*(Yes|No))", std::regex::icase);

    std::smatch match;
    if (std::regex_search(output, match, reOnline)) {
        info.onlineMgntAp = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reOffline)) {
        info.offlineMgntAp = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reUnmgnt)) {
        info.unMgntAp = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reStation)) {
        info.stationCount = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reRemote)) {
        info.remoteApCount = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reUpdate)) {
        info.updateAvailable = (match[1] == "Yes" || match[1] == "yes");
        found = true;
    }

    if (!found) {
        std::cerr << "[ZYXEL_PARSE_ERROR] class=ZyxelWlanCmd cmd=\"show capwap ap info\" "
                  << "reason=\"No CAPWAP AP info fields matched\" snippet=\""
                  << output.substr(0, 60) << "\"\n";
    }
    return found;
}

bool ZyxelWlanCmd::parseCapwapFallback(const std::string &output,
                                       ZyxelCapwapFallback &fallback)
{
    fallback = ZyxelCapwapFallback();
    bool found = false;

    std::regex reFallback(R"(Fallback\s*:\s*(enable|disable))", std::regex::icase);
    std::regex reInterval(R"(Fallback Interval\s*:\s*(\d+))", std::regex::icase);
    std::regex reIdle(R"(Idle timeout\s*:\s*([^\r\n]+))", std::regex::icase);

    std::smatch match;
    if (std::regex_search(output, match, reFallback)) {
        fallback.fallbackEnabled = (match[1] == "enable" || match[1] == "Enable");
        found = true;
    }
    if (std::regex_search(output, match, reInterval)) {
        fallback.fallbackInterval = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reIdle)) {
        fallback.idleTimeout = match[1];
        found = true;
    }

    if (!found) {
        std::cerr << "[ZYXEL_PARSE_ERROR] class=ZyxelWlanCmd cmd=\"show capwap ap fallback\" "
                  << "reason=\"No fallback fields matched\" snippet=\""
                  << output.substr(0, 60) << "\"\n";
    }
    return found;
}

bool ZyxelWlanCmd::parseRogueApInfo(const std::string &output,
                                    ZyxelRogueApInfo &info)
{
    info = ZyxelRogueApInfo();
    bool found = false;

    std::regex reRogue(R"(rogue ap\s*:\s*(\d+))", std::regex::icase);
    std::regex reFriendly(R"(friendly ap\s*:\s*(\d+))", std::regex::icase);
    std::regex reSuspected(R"(suspected rogue ap\s*:\s*(\d+))", std::regex::icase);
    std::regex reAdhoc(R"(adhoc\s*:\s*(\d+))", std::regex::icase);
    std::regex reUnclass(R"(unclassified ap\s*:\s*(\d+))", std::regex::icase);
    std::regex reTotal(R"(total devices\s*:\s*(\d+))", std::regex::icase);

    std::smatch match;
    if (std::regex_search(output, match, reRogue)) {
        info.rogueApCount = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reFriendly)) {
        info.friendlyApCount = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reSuspected)) {
        info.suspectedRogueApCount = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reAdhoc)) {
        info.adhocCount = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reUnclass)) {
        info.unclassifiedApCount = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reTotal)) {
        info.totalDevices = std::stoi(match[1]);
        found = true;
    }

    if (!found) {
        std::cerr << "[ZYXEL_PARSE_ERROR] class=ZyxelWlanCmd cmd=\"show rogue-ap detection info\" "
                  << "reason=\"No rogue AP detection count fields matched\" snippet=\""
                  << output.substr(0, 60) << "\"\n";
    }
    return found;
}

bool ZyxelWlanCmd::parseRogueApStatus(const std::string &output,
                                      ZyxelRogueApStatus &status)
{
    status = ZyxelRogueApStatus();
    bool found = false;

    std::regex reStatus(R"(rogue-ap detection status\s*:\s*(on|off))", std::regex::icase);
    std::regex reInterval(R"(ap-mode detection interval\s*:\s*(\d+))", std::regex::icase);
    std::regex reWeak(R"(rogue-rule weak-security\s*:\s*(yes|no))", std::regex::icase);
    std::regex reUnmgnt(R"(rogue-rule unmanaged-ap\s*:\s*(yes|no))", std::regex::icase);
    std::regex reHidden(R"(rogue-rule hidden-ssid\s*:\s*(yes|no))", std::regex::icase);
    std::regex reSsid(R"(rogue-rule ssid-keyword\s*:\s*(yes|no))", std::regex::icase);

    std::smatch match;
    if (std::regex_search(output, match, reStatus)) {
        status.detectionStatus = (match[1] == "on" || match[1] == "On");
        found = true;
    }
    if (std::regex_search(output, match, reInterval)) {
        status.detectionInterval = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reWeak)) {
        status.weakSecurity = (match[1] == "yes" || match[1] == "Yes");
        found = true;
    }
    if (std::regex_search(output, match, reUnmgnt)) {
        status.unmanagedAp = (match[1] == "yes" || match[1] == "Yes");
        found = true;
    }
    if (std::regex_search(output, match, reHidden)) {
        status.hiddenSsid = (match[1] == "yes" || match[1] == "Yes");
        found = true;
    }
    if (std::regex_search(output, match, reSsid)) {
        status.ssidKeyword = (match[1] == "yes" || match[1] == "Yes");
        found = true;
    }

    if (!found) {
        std::cerr << "[ZYXEL_PARSE_ERROR] class=ZyxelWlanCmd cmd=\"show rogue-ap detection status\" "
                  << "reason=\"No rogue AP status fields matched\" snippet=\""
                  << output.substr(0, 60) << "\"\n";
    }
    return found;
}

bool ZyxelWlanCmd::parseAutoHealingConfig(const std::string &output,
                                          ZyxelAutoHealingConfig &config)
{
    config = ZyxelAutoHealingConfig();
    bool found = false;

    std::regex reActivate(R"(auto-healing activate\s*:\s*(yes|no))", std::regex::icase);
    std::regex reInterval(R"(auto-healing interval\s*:\s*(\d+))", std::regex::icase);
    std::regex rePower(R"(auto-healing power threshold\s*:\s*(-?\d+)\s*dBm)", std::regex::icase);
    std::regex reHealing(R"(auto-healing healing threshold\s*:\s*(-?\d+)\s*dBm)", std::regex::icase);
    std::regex reMargin(R"(auto-healing margin\s*:\s*(\d+))", std::regex::icase);

    std::smatch match;
    if (std::regex_search(output, match, reActivate)) {
        config.activated = (match[1] == "yes" || match[1] == "Yes");
        found = true;
    }
    if (std::regex_search(output, match, reInterval)) {
        config.interval = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, rePower)) {
        config.powerThresholdDbm = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reHealing)) {
        config.healingThresholdDbm = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reMargin)) {
        config.margin = std::stoi(match[1]);
        found = true;
    }

    if (!found) {
        std::cerr << "[ZYXEL_PARSE_ERROR] class=ZyxelWlanCmd cmd=\"show auto-healing config\" "
                  << "reason=\"No auto-healing fields matched\" snippet=\""
                  << output.substr(0, 60) << "\"\n";
    }
    return found;
}

bool ZyxelWlanCmd::parseFrameCaptureConfig(const std::string &output,
                                           ZyxelFrameCaptureConfig &config)
{
    config = ZyxelFrameCaptureConfig();
    bool found = false;

    std::regex reSource(R"(capture source\s*:\s*([^\r\n]+))", std::regex::icase);
    std::regex rePrefix(R"(file prefix\s*:\s*([^\r\n]+))", std::regex::icase);
    std::regex reSize(R"(file size\s*:\s*(\d+))", std::regex::icase);

    std::smatch match;
    if (std::regex_search(output, match, reSource)) {
        config.captureSource = match[1];
        found = true;
    }
    if (std::regex_search(output, match, rePrefix)) {
        config.filePrefix = match[1];
        found = true;
    }
    if (std::regex_search(output, match, reSize)) {
        config.fileSize = std::stoi(match[1]);
        found = true;
    }

    if (!found) {
        std::cerr << "[ZYXEL_PARSE_ERROR] class=ZyxelWlanCmd cmd=\"show frame-capture config\" "
                  << "reason=\"No frame capture fields matched\" snippet=\""
                  << output.substr(0, 60) << "\"\n";
    }
    return found;
}

bool ZyxelWlanCmd::parseZyMeshInfo(const std::string &output,
                                   ZyxelZyMeshInfo &info)
{
    info = ZyxelZyMeshInfo();
    bool found = false;

    std::regex reOnlineRoot(R"(Online Root AP\s*:\s*(\d+))", std::regex::icase);
    std::regex reOnlineRepeater(R"(Online Repeater AP\s*:\s*(\d+))", std::regex::icase);
    std::regex reOfflineRoot(R"(Offline Root AP\s*:\s*(\d+))", std::regex::icase);
    std::regex reOfflineRepeater(R"(Offline Repeater AP\s*:\s*(\d+))", std::regex::icase);
    std::regex reProvMac(R"(Provision Group MAC\s*:\s*([0-9a-fA-F:]+))", std::regex::icase);

    std::smatch match;
    if (std::regex_search(output, match, reOnlineRoot)) {
        info.onlineRootAp = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reOnlineRepeater)) {
        info.onlineRepeaterAp = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reOfflineRoot)) {
        info.offlineRootAp = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reOfflineRepeater)) {
        info.offlineRepeaterAp = std::stoi(match[1]);
        found = true;
    }
    if (std::regex_search(output, match, reProvMac)) {
        info.provisionGroupMac = match[1];
        found = true;
    }

    if (!found) {
        std::cerr << "[ZYXEL_PARSE_ERROR] class=ZyxelWlanCmd cmd=\"show zymesh ap info\" "
                  << "reason=\"No ZyMesh info fields matched\" snippet=\""
                  << output.substr(0, 60) << "\"\n";
    }
    return found;
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
