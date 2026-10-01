/*
 * ZyxelSystemCmd.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXEL_SYSTEM_CMD_HXX
#define NETMON_ZYXEL_SYSTEM_CMD_HXX

#include "zyxel/ZyxelTypes.hxx"
#include <string>

class ZyxelSystemCmd {
public:
    // Command generators
    static std::string cmdShowVersion();
    static std::string cmdShowCpuStatus();
    static std::string cmdShowMemStatus();
    static std::string cmdShowConnStatus();
    static std::string cmdPing(const std::string &ip, int count = 4);
    static std::string cmdTraceroute(const std::string &ip);
    static std::string cmdWrite();
    static std::string cmdReboot();

    // Parsers
    static bool parseVersion(const std::string &raw, ZyxelVersionInfo &out);
    static bool parseCpuStatus(const std::string &raw, double &cpuPercentOut);
    static bool parseMemStatus(const std::string &raw, double &memPercentOut);
    static bool parseConnStatus(const std::string &raw, ZyxelSessionSummary &out);
    static bool parsePing(const std::string &raw, ZyxelDiagnosticResult &out);
    static bool parseTraceroute(const std::string &raw, ZyxelDiagnosticResult &out);
};

#endif /* NETMON_ZYXEL_SYSTEM_CMD_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
