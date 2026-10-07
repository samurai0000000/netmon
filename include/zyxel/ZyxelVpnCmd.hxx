/*
 * ZyxelVpnCmd.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXEL_VPN_CMD_HXX
#define NETMON_ZYXEL_VPN_CMD_HXX

#include "zyxel/ZyxelTypes.hxx"
#include <string>
#include <vector>

class ZyxelVpnCmd {
public:
    // Envelope 8: IPSec VPN (Ch 33)
    static std::string cmdShowCryptoBoostTcp();
    static std::string cmdShowCryptoIgnoreDfBit();
    static std::string cmdShowCryptoMap(const std::string &mapName = "");
    static std::string cmdShowCryptoMap6(const std::string &mapName = "");
    static std::string cmdShowCryptoMapConnCheck();
    static std::string cmdShowIkev2Policy(const std::string &policyName = "");
    static std::string cmdShowIkev2Policy6(const std::string &policyName = "");
    static std::string cmdShowIsakmpKeepalive();
    static std::string cmdShowIsakmpPolicy(const std::string &policyName = "");
    static std::string cmdShowIsakmpSa();
    static std::string cmdShowSaCounter();
    static std::string cmdShowSaMonitor();
    static std::string cmdShowVcpAllowedCryptoMap();
    static std::string cmdShowVcpAllowedCryptoMap6();
    static std::string cmdShowVcpAllowedUsers();
    static std::string cmdShowVpnConcentrator(const std::string &profileName = "");
    static std::string cmdShowVpnConcentrator6(const std::string &profileName = "");
    static std::string cmdShowVpnConfigurationProvisionActivation();
    static std::string cmdShowVpnConfigurationProvisionAuthentication();
    static std::string cmdShowVpnConfigurationProvisionIosfilter();
    static std::string cmdShowVpnConfigurationProvisionPort();
    static std::string cmdShowVpnConfigurationProvisionRules();
    static std::string cmdShowVpnCounters();
    static std::string cmdShowVpnServiceStatus();

    // IPSec Mutations
    static std::string cmdIsakmpPolicy(const std::string &name);
    static std::string cmdNoIsakmpPolicy(const std::string &name);
    static std::string cmdCryptoMap(const std::string &name);
    static std::string cmdNoCryptoMap(const std::string &name);

    // Envelope 8: SSL VPN (Ch 34)
    static std::string cmdShowSslvpnLoginPort();
    static std::string cmdShowSslvpnPolicy(const std::string &profileName = "");
    static std::string cmdShowSslvpnApplication();
    static std::string cmdShowSslvpnMonitor();
    static std::string cmdShowWorkspaceApplication();
    static std::string cmdShowWorkspaceCifs();
    static std::string cmdShowSslVpnNetworkExtensionLocalIp();

    // SSL VPN Mutations
    static std::string cmdSslvpnApplication(const std::string &name);
    static std::string cmdNoSslvpnApplication(const std::string &name);
    static std::string cmdSslvpnPolicy(const std::string &name);
    static std::string cmdNoSslvpnPolicy(const std::string &name);

    // Envelope 8: L2TP VPN (Ch 35)
    static std::string cmdShowAccountL2tp(const std::string &profileName = "");
    static std::string cmdShowInterfacePpp();
    static std::string cmdShowL2tpOverIpsec();
    static std::string cmdShowL2tpOverIpsecSession();

    // L2TP Mutations
    static std::string cmdAccountL2tp(const std::string &name);
    static std::string cmdNoAccountL2tp(const std::string &name);

    // Dry-fire and negative testing helpers
    static std::string cmdInvalidIpsecDryFire();
    static std::string cmdInvalidSslvpnDryFire();
    static std::string cmdInvalidL2tpDryFire();

    // Envelope 8 Parsers
    static bool parseCryptoBoostTcp(const std::string &raw, bool &boostTcp);
    static bool parseVpnCounters(const std::string &raw, ZyxelVpnCounters &out);
    static bool parseVpnServiceStatus(const std::string &raw, bool &active, bool &autoDisable);
    static bool parseVpnConfigurationProvisionActivation(const std::string &raw, bool &active);
    static bool parseVpnConfigurationProvisionPort(const std::string &raw, int &port);
    static bool parseSaCounter(const std::string &raw, int &status);
    static bool parseSslvpnLoginPort(const std::string &raw, int &port);
    static bool parseL2tpOverIpsec(const std::string &raw, ZyxelL2tpStatus &out);
    static bool parseVcpAllowedUsers(const std::string &raw, std::vector<ZyxelVcpUser> &out);
};

#endif /* NETMON_ZYXEL_VPN_CMD_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
