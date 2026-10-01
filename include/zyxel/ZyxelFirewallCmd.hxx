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
    // Command generators
    static std::string cmdShowSecurePolicy(const std::string &nameOrNum = "");
    static std::vector<std::string> cmdInsertRule(int position, const ZyxelFirewallRule &rule);
    static std::vector<std::string> cmdInsertFastDeny(int position,
                                                     const std::string &ruleName,
                                                     const std::string &srcObjName,
                                                     const std::string &reason);
    static std::string cmdDeleteRule(const std::string &nameOrNum);

    // Parsers
    static bool parseSecurePolicy(const std::string &raw, std::vector<ZyxelFirewallRule> &out);
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
