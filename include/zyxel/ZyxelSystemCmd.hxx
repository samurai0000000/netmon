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

    // Envelope 1: System Command Generators
    static std::string cmdShowIpDnsServerStatus();
    static std::string cmdShowLoggingStatus();
    static std::string cmdShowDisk();
    static std::string cmdShowMac();
    static std::string cmdShowLedStatus();
    static std::string cmdShowExtensionSlot();
    static std::string cmdShowSerialNumber();
    static std::string cmdShowBootStatus();
    static std::string cmdShowSocketListen();
    static std::string cmdShowSocketOpen();
    static std::string cmdShowRamSize();
    static std::string cmdShowComportStatus();
    static std::string cmdDir();
    static std::string cmdShutdown();
    static std::string cmdAppendDnsZoneForwarder(const std::string &zone, const std::string &serverIp);
    static std::string cmdDeleteDnsZoneForwarder(int index);
    static std::string cmdInvalidSystemDryFire();

    // Parsers
    static bool parseVersion(const std::string &raw, ZyxelVersionInfo &out);
    static bool parseCpuStatus(const std::string &raw, double &cpuPercentOut);
    static bool parseMemStatus(const std::string &raw, double &memPercentOut);
    static bool parseConnStatus(const std::string &raw, ZyxelSessionSummary &out);
    static bool parsePing(const std::string &raw, ZyxelDiagnosticResult &out);
    static bool parseTraceroute(const std::string &raw, ZyxelDiagnosticResult &out);

    // Envelope 1: System Parsers
    static bool parseIpDnsServerStatus(const std::string &raw, bool &activeOut);
    static bool parseLoggingStatus(const std::string &raw, int &eventsLoggedOut, bool &suppressionOut);
    static bool parseDisk(const std::string &raw, std::vector<ZyxelDiskEntry> &out);
    static bool parseMac(const std::string &raw, std::string &macOut);
    static bool parseLedStatus(const std::string &raw, std::string &ledStatusOut);
    static bool parseExtensionSlot(const std::string &raw, std::vector<ZyxelExtensionSlotEntry> &out);
    static bool parseSerialNumber(const std::string &raw, std::string &serialNumberOut);
    static bool parseBootStatus(const std::string &raw, int &statusCodeOut, std::string &statusMsgOut);
    static bool parseSocketList(const std::string &raw, std::vector<ZyxelSocketEntry> &out);
    static bool parseRamSize(const std::string &raw, int &ramSizeMbOut);
    static bool parseComportStatus(const std::string &raw, std::string &comportStatusOut);
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
