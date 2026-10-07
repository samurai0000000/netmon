/*
 * ZyxelNetworkCmd.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXEL_NETWORK_CMD_HXX
#define NETMON_ZYXEL_NETWORK_CMD_HXX

#include "zyxel/ZyxelTypes.hxx"
#include <string>
#include <vector>

class ZyxelNetworkCmd {
public:
    // Command generators
    static std::string cmdShowInterfaces();
    static std::string cmdShowInterface(const std::string &name);
    static std::string cmdShowInterfaceBase();
    static std::string cmdShowIpRoute();
    static std::string cmdAddRoute(const std::string &dest, const std::string &mask,
                                  const std::string &gw, int metric = 1);
    static std::string cmdDeleteRoute(const std::string &dest, const std::string &mask,
                                     const std::string &gw);
    static std::string cmdShowZones();
    static std::string cmdShowZone(const std::string &name);
    static std::string cmdShowZoneDefaultBinding();
    static std::string cmdShowZoneBindingIface();
    static std::string cmdShowArp();
    static std::string cmdShowArpGratuitous();
    static std::string cmdShowIpDhcpBinding();
    static std::string cmdShowL2Isolation();
    static std::string cmdShowL2IsolationActivation();
    static std::string cmdShowL2IsolationWhitelist();
    static std::string cmdInvalidInterfaceDryFire();

    // Chapter 17, 20, 27, 28, 47, 49 Configuration Generators (Hermetic / Tier 1)
    static std::string cmdTrunk(const std::string &name);
    static std::string cmdNoTrunk(const std::string &name);
    static std::string cmdZone(const std::string &name);
    static std::string cmdNoZone(const std::string &name);
    static std::string cmdIpMacBinding(bool activate = true);
    static std::string cmdL2IsolationActivation(bool activate = true);
    static std::string cmdIpException(const std::string &name);
    static std::string cmdNoIpException(const std::string &name);
    static std::string cmdDeviceInsightActivation(bool activate = true);

    // Chapter 18 & 19 Routing & Routing Protocol Operational Generators (Envelope 3)
    static std::string cmdShowIpRouteFilter(const std::string &proto);
    static std::string cmdShowIpRouteSettings();
    static std::string cmdShowIpRouteControlVirtualServer();
    static std::string cmdShowPolicyRoute();
    static std::string cmdShowPolicyRouteRuleCount();
    static std::string cmdShowPolicyRouteConnCheck();
    static std::string cmdShowPolicyRouteConnCheckStatus();
    static std::string cmdShowPolicyRouteOverrideDirectRoute();
    static std::string cmdShowPolicyRouteUnderlayerRules();
    static std::string cmdShowPolicyRouteControlVirtualServer();
    static std::string cmdShowPolicyRouteControlIpsecDynamic();
    static std::string cmdShowPolicyRouteBeginEnd(int begin, int end);
    static std::string cmdShowPolicyRoute6();
    static std::string cmdShowPolicyRoute6RuleCount();
    static std::string cmdShowPolicyRoute6OverrideDirectRoute();
    static std::string cmdShowPolicyRoute6ControlIpsecDynamic();
    static std::string cmdShowBwmActivation();
    static std::string cmdShowOspfGlobal();
    static std::string cmdShowOspfDatabase();
    static std::string cmdShowOspfNeighbor();
    static std::string cmdShowOspfAreaVirtualLink(const std::string &areaIp);
    static std::string cmdShowRipGlobal();
    static std::string cmdShowBgpGlobal();
    static std::string cmdShowBgpSummary();
    static std::string cmdShowBgpRoute();
    static std::string cmdShowBgpMem();
    static std::string cmdShowBgpNeighbor();

    // Chapter 18 & 19 Routing Configuration Generators (Hermetic / Tier 1)
    static std::string cmdIpRouteControlVirtualServer(bool activate = true);
    static std::string cmdPolicyAppend();
    static std::string cmdPolicyDelete(int num);
    static std::string cmdPolicyDefaultRoute();
    static std::string cmdPolicyFlush();
    static std::string cmdPolicyControlVirtualServer(bool activate = true);
    static std::string cmdPolicyControlIpsecDynamic(bool activate = true);
    static std::string cmdBwmActivation(bool activate = true);
    static std::string cmdRouterOspf(bool enable = true);
    static std::string cmdRouterBgp(int asNumber, bool enable = true);
    static std::string cmdRouterRip(bool enable = true);

    // Chapter 18 & 19 Dry-fire / Invalid Commands
    static std::string cmdInvalidRouteDryFire();
    static std::string cmdInvalidPolicyRouteDryFire();
    static std::string cmdInvalidOspfAreaDryFire();

    // Parsers
    static bool parseInterfaces(const std::string &raw, std::vector<ZyxelInterfaceInfo> &out);
    static bool parseIpRoutes(const std::string &raw, std::vector<ZyxelRouteEntry> &out);
    static bool parseZones(const std::string &raw, std::vector<ZyxelZoneInfo> &out);
    static bool parseArp(const std::string &raw, std::vector<ZyxelArpEntry> &out);
    static bool parseDhcpBindings(const std::string &raw, std::vector<ZyxelDhcpBindingEntry> &out);
    static bool parseArpGratuitous(const std::string &raw, ZyxelArpGratuitousInfo &out);
    static bool parseZoneBindings(const std::string &raw, std::vector<ZyxelZoneBindingEntry> &out);
    static bool parseL2IsolationActivation(const std::string &raw, bool &active);
    static bool parseRouteSettings(const std::string &raw, std::vector<ZyxelRouteSettingsEntry> &out);
    static bool parsePolicyRouteRuleCount(const std::string &raw, int &count);
    static bool parsePolicyRouteOverrideDirectRoute(const std::string &raw, bool &active);
    static bool parsePolicyRouteControlVirtualServer(const std::string &raw, bool &active);
    static bool parseBwmActivation(const std::string &raw, bool &active);
    static bool parseOspfGlobal(const std::string &raw, ZyxelOspfGlobalInfo &out);
    static bool parseRipGlobal(const std::string &raw, ZyxelRipGlobalInfo &out);
    static bool parseBgpGlobal(const std::string &raw, ZyxelBgpGlobalInfo &out);
};

#endif /* NETMON_ZYXEL_NETWORK_CMD_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
