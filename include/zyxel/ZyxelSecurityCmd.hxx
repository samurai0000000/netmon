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
    // Command generators
    static std::string cmdShowAppStatisticsSummary();
    static std::string cmdShowIdpStatisticsSummary();
    static std::string cmdShowConnStatus();

    // Parsers
    static bool parseConnStatus(const std::string &raw, ZyxelSessionSummary &out);
    static bool parseAppStatisticsSummary(const std::string &raw, ZyxelAppPatrolSummary &out);
    static bool parseIdpStatisticsSummary(const std::string &raw, ZyxelIdpSummary &out);
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
