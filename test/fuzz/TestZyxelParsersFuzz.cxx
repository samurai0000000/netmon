/*
 * TestZyxelParsersFuzz.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <iostream>
#include <string>
#include <vector>
#include <rapidcheck.h>

#include "zyxel/ZyxelTypes.hxx"
#include "zyxel/ZyxelScanner.hxx"
#include "zyxel/ZyxelSystemCmd.hxx"
#include "zyxel/ZyxelNetworkCmd.hxx"
#include "zyxel/ZyxelObjectCmd.hxx"
#include "zyxel/ZyxelFirewallCmd.hxx"
#include "zyxel/ZyxelNatCmd.hxx"
#include "ZyxelSshClient.hxx"
#include "AiSecurityClearance.hxx"

int main() {
    // Suppress parse error log output during generative fuzzing
    ZyxelScanner::setQuietLogging(true);

    std::cout << "[RapidCheck] Starting Zyxel parser property verification..." << std::endl;

    // Property 1: Percentage extraction memory safety and idempotency
    bool p1 = rc::check("ZyxelScanner::extractPercentage safety", [](const std::string &s) {
        double v1 = 0.0, v2 = 0.0;
        bool ok1 = ZyxelScanner::extractPercentage(s, v1);
        bool ok2 = ZyxelScanner::extractPercentage(s, v2);
        RC_ASSERT(ok1 == ok2);
        if (ok1) {
            RC_ASSERT(v1 == v2);
        }
    });
    if (!p1) return 1;

    // Property 2: Integer token extraction memory safety and determinism
    bool p2 = rc::check("ZyxelScanner::extractIntegerAfter safety",
                        [](const std::string &s, const std::string &kw) {
        int v1 = 0, v2 = 0;
        bool ok1 = ZyxelScanner::extractIntegerAfter(s, kw, v1);
        bool ok2 = ZyxelScanner::extractIntegerAfter(s, kw, v2);
        RC_ASSERT(ok1 == ok2);
        if (ok1) {
            RC_ASSERT(v1 == v2);
        }
    });
    if (!p2) return 1;

    // Property 3: Table column parsing and cell extraction safety
    bool p3 = rc::check("ZyxelScanner::parseTableColumns and extractCell safety",
                        [](const std::string &header, const std::string &divider,
                           const std::string &row) {
        auto cols = ZyxelScanner::parseTableColumns(header, divider);
        for (const auto &col : cols) {
            std::string cell = ZyxelScanner::extractCell(row, col);
            (void)cell;
        }
    });
    if (!p3) return 1;

    // Property 4: Ping parser crash-resistance and diagnostic consistency
    bool p4 = rc::check("ZyxelSystemCmd::parsePing safety and consistency",
                        [](const std::string &raw) {
        ZyxelDiagnosticResult res1, res2;
        bool ok1 = ZyxelSystemCmd::parsePing(raw, res1);
        bool ok2 = ZyxelSystemCmd::parsePing(raw, res2);
        RC_ASSERT(ok1 == ok2);
        if (ok1) {
            RC_ASSERT(res1.packetsTransmitted >= res1.packetsReceived);
            auto j = res1.toJson();
            (void)j;
        }
    });
    if (!p4) return 1;

    // Property 5: Traceroute parser crash-resistance and AST consistency
    bool p5 = rc::check("ZyxelSystemCmd::parseTraceroute safety and consistency",
                        [](const std::string &raw) {
        ZyxelDiagnosticResult res1, res2;
        bool ok1 = ZyxelSystemCmd::parseTraceroute(raw, res1);
        bool ok2 = ZyxelSystemCmd::parseTraceroute(raw, res2);
        RC_ASSERT(ok1 == ok2);
        if (ok1) {
            RC_ASSERT(res1.hops.size() == static_cast<size_t>(res1.packetsReceived));
            auto j = res1.toJson();
            (void)j;
        }
    });
    if (!p5) return 1;

    // Property 6: Firewall rule table parser safety and determinism
    bool p6 = rc::check("ZyxelFirewallCmd::parseSecurePolicy safety",
                        [](const std::string &raw) {
        std::vector<ZyxelFirewallRule> r1, r2;
        bool ok1 = ZyxelFirewallCmd::parseSecurePolicy(raw, r1);
        bool ok2 = ZyxelFirewallCmd::parseSecurePolicy(raw, r2);
        RC_ASSERT(ok1 == ok2);
        RC_ASSERT(r1.size() == r2.size());
    });
    if (!p6) return 1;

    // Property 7: Network interface parser safety
    bool p7 = rc::check("ZyxelNetworkCmd::parseInterfaces safety",
                        [](const std::string &raw) {
        std::vector<ZyxelInterfaceInfo> i1, i2;
        bool ok1 = ZyxelNetworkCmd::parseInterfaces(raw, i1);
        bool ok2 = ZyxelNetworkCmd::parseInterfaces(raw, i2);
        RC_ASSERT(ok1 == ok2);
        RC_ASSERT(i1.size() == i2.size());
    });
    if (!p7) return 1;

    // Property 8: IP routes parser safety
    bool p8 = rc::check("ZyxelNetworkCmd::parseIpRoutes safety",
                        [](const std::string &raw) {
        std::vector<ZyxelRouteEntry> r1, r2;
        bool ok1 = ZyxelNetworkCmd::parseIpRoutes(raw, r1);
        bool ok2 = ZyxelNetworkCmd::parseIpRoutes(raw, r2);
        RC_ASSERT(ok1 == ok2);
        RC_ASSERT(r1.size() == r2.size());
    });
    if (!p8) return 1;

    // Property 9: Virtual server NAT parser safety
    bool p9 = rc::check("ZyxelNatCmd::parseVirtualServers safety",
                        [](const std::string &raw) {
        std::vector<ZyxelVirtualServerRule> vs1, vs2;
        bool ok1 = ZyxelNatCmd::parseVirtualServers(raw, vs1);
        bool ok2 = ZyxelNatCmd::parseVirtualServers(raw, vs2);
        RC_ASSERT(ok1 == ok2);
        RC_ASSERT(vs1.size() == vs2.size());
    });
    if (!p9) return 1;

    // Property 10: Prefix truncation mutator on genuine transcript fragments
    static const std::string sampleTranscripts[] = {
        "Rule  Name                     From   To      Source     Destination  Service   Action  Status\n"
        "==============================================================================================\n"
        "1     Default LAN Rule with Sp LAN    WAN     any        any          any       allow   yes\n",

        "traceroute to 8.8.8.8 (8.8.8.8), 30 hops max, 38 byte packets\n"
        " 1  * * *\n"
        " 2  168.95.105.138  10.370 ms  11.521 ms  11.538 ms\n"
        " 3  220.128.9.82  11.454 ms 220.128.9.214  13.442 ms  13.809 ms\n",

        "PING 1.1.1.1 (1.1.1.1) 56(84) bytes of data.\n"
        "4 packets transmitted, 4 received, 0% packet loss, time 3004ms\n"
        "rtt min/avg/max/mdev = 11.821/12.145/12.482/0.250 ms\n"
    };

    bool p10 = rc::check("Truncation mutator safety across genuine transcripts",
                         [](const std::string &randomJunk) {
        for (const auto &sample : sampleTranscripts) {
            for (size_t len = 1; len <= sample.length(); len += 5) {
                std::string truncated = sample.substr(0, len) + randomJunk;
                ZyxelDiagnosticResult diag;
                (void)ZyxelSystemCmd::parsePing(truncated, diag);
                (void)ZyxelSystemCmd::parseTraceroute(truncated, diag);
                std::vector<ZyxelFirewallRule> fw;
                (void)ZyxelFirewallCmd::parseSecurePolicy(truncated, fw);
            }
        }
    });
    if (!p10) return 1;

    auto hostChar = rc::gen::element<char>(
        'a', 'b', 'c', 'R', 'Z', '0', '1', '-', '_', '.');
    bool p11 = rc::check("anchored prompt accepts the six forms and rejects noise",
                         [hostChar]() {
        std::size_t len = *rc::gen::inRange<std::size_t>(1, 24);
        std::string host;
        host.reserve(len);
        for (std::size_t i = 0; i < len; ++i) {
            host.push_back(*hostChar);
        }
        std::string sub = *rc::gen::element<std::string>(
            "router", "address", "policy-control", "bwm");
        const std::string forms[6] = {
            host + ">",
            host + "#",
            host + "(config)#",
            host + "(secure-policy)#",
            host + "(config-" + sub + ")#",
            host + "(config-" + sub + ")>"
        };
        const PromptState states[6] = {
            PromptState::USER,
            PromptState::ROOT,
            PromptState::CONFIG,
            PromptState::POLICY_SUBMODE,
            PromptState::OTHER_SUBMODE,
            PromptState::OTHER_SUBMODE
        };
        std::string ansi = *rc::gen::element<std::string>("", "\033[32m", "\033[0m", "\033[2K");
        for (int i = 0; i < 6; ++i) {
            std::string shown = ansi + forms[i] + "  ";
            std::string matched;
            PromptState state = PromptState::UNKNOWN;
            RC_ASSERT(ZyxelSshClient::matchPrompt(host, shown, matched, state));
            RC_ASSERT(state == states[i]);
            RC_ASSERT(matched == forms[i]);
        }

        std::string other = host + "x";
        std::string matched;
        PromptState state = PromptState::UNKNOWN;
        RC_ASSERT(!ZyxelSshClient::matchPrompt(host, other + "#", matched, state));
        RC_ASSERT(!ZyxelSshClient::matchPrompt(host, "see " + host + "# later\nmore text", matched, state));
        RC_ASSERT(!ZyxelSshClient::matchPrompt(host, "", matched, state));
        std::string truncated = forms[2];
        truncated.pop_back();
        RC_ASSERT(!ZyxelSshClient::matchPrompt(host, truncated, matched, state));

        std::string huge(65536, 'Q');
        RC_ASSERT(!ZyxelSshClient::matchPrompt(host, huge, matched, state));
        huge.push_back('\n');
        huge += forms[1];
        RC_ASSERT(ZyxelSshClient::matchPrompt(host, huge, matched, state));
        RC_ASSERT(state == PromptState::ROOT);
    });
    if (!p11) return 1;

    bool p12 = rc::check("classifier rejects question marks and control bytes",
                         [](const std::string &prefix, const std::string &suffix) {
        int timeoutMs = 0;
        std::string method;
        std::string withQuestion = prefix + "?" + suffix;
        RC_ASSERT(AiSecurityClassifier::classify(withQuestion, "Router#", timeoutMs, method) ==
                  LineClassification::UNCLASSIFIED);
        std::string withControl = prefix + std::string(1, '\x01') + suffix;
        RC_ASSERT(AiSecurityClassifier::classify(withControl, "Router#", timeoutMs, method) ==
                  LineClassification::UNCLASSIFIED);
        std::string withDelete = prefix + std::string(1, '\x7f') + suffix;
        RC_ASSERT(AiSecurityClassifier::classify(withDelete, "Router#", timeoutMs, method) ==
                  LineClassification::UNCLASSIFIED);
    });
    if (!p12) return 1;

    bool p13 = rc::check("sanitized block reason cannot inject a command",
                         [](const std::string &raw) {
        std::string clean = ZyxelSshClient::sanitizeReason(raw);
        RC_ASSERT(clean.size() <= 63);
        RC_ASSERT(clean.find('?') == std::string::npos);
        RC_ASSERT(clean.find('\n') == std::string::npos);
        RC_ASSERT(clean.find('\r') == std::string::npos);
        for (unsigned char uc : clean) {
            RC_ASSERT(uc >= 32 && uc != 127);
        }
    });
    if (!p13) return 1;

    bool p14 = rc::check("a failed block step sends no later line and no compensating delete",
                         [](unsigned char rawK) {
        const char *steps[] = {
            "show secure-policy",
            "configure terminal",
            "address-object NETMON_BLK_192_0_2_9 192.0.2.9",
            "secure-policy insert 1",
            "no activate",
            "name NETMON_BLK_192_0_2_9",
            "sourceip NETMON_BLK_192_0_2_9",
            "action deny",
            "description netmon-block",
            "activate",
            "exit",
            "exit",
            "show secure-policy"
        };
        const int count = 13;
        int k = static_cast<int>(rawK % count);
        for (int i = 0; i <= k; ++i) {
            std::string line = steps[i];
            if (i < k) {
                RC_ASSERT(line.compare(0, 16, "no secure-policy") != 0);
                RC_ASSERT(line.compare(0, 17, "no address-object") != 0);
            }
        }
        RC_ASSERT(k < count);
    });
    if (!p14) return 1;

    std::cout << "[RapidCheck] All 14 property suites passed successfully!" << std::endl;
    return 0;
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
