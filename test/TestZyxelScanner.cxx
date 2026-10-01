/*
 * TestZyxelScanner.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelScanner.hxx"
#include <CppUTest/TestHarness.h>

TEST_GROUP(ZyxelScannerTest) {
    void setup() {}
    void teardown() {}
};

TEST(ZyxelScannerTest, ParseTableColumnsSegmented) {
    std::string header  = "Index  Rule Name             From    To";
    std::string divider = "=====  ====================  ======  ======";

    auto cols = ZyxelScanner::parseTableColumns(header, divider);
    LONGS_EQUAL(4, cols.size());
    STRCMP_EQUAL("Index", cols[0].name.c_str());
    STRCMP_EQUAL("Rule Name", cols[1].name.c_str());
    STRCMP_EQUAL("From", cols[2].name.c_str());
    STRCMP_EQUAL("To", cols[3].name.c_str());

    std::string row = "1      Default LAN Rule      LAN     WAN";
    STRCMP_EQUAL("1", ZyxelScanner::extractCell(row, cols[0]).c_str());
    STRCMP_EQUAL("Default LAN Rule", ZyxelScanner::extractCell(row, cols[1]).c_str());
    STRCMP_EQUAL("LAN", ZyxelScanner::extractCell(row, cols[2]).c_str());
    STRCMP_EQUAL("WAN", ZyxelScanner::extractCell(row, cols[3]).c_str());
}

TEST(ZyxelScannerTest, ParseTableColumnsContinuous) {
    std::string header  = "Interface       Status  IP Address      Netmask";
    std::string divider = "================================================================";

    auto cols = ZyxelScanner::parseTableColumns(header, divider);
    LONGS_EQUAL(4, cols.size());
    STRCMP_EQUAL("Interface", cols[0].name.c_str());
    STRCMP_EQUAL("Status", cols[1].name.c_str());
    STRCMP_EQUAL("IP Address", cols[2].name.c_str());
    STRCMP_EQUAL("Netmask", cols[3].name.c_str());

    std::string row = "wan1            up      192.0.2.1       255.255.255.0";
    STRCMP_EQUAL("wan1", ZyxelScanner::extractCell(row, cols[0]).c_str());
    STRCMP_EQUAL("up", ZyxelScanner::extractCell(row, cols[1]).c_str());
    STRCMP_EQUAL("192.0.2.1", ZyxelScanner::extractCell(row, cols[2]).c_str());
    STRCMP_EQUAL("255.255.255.0", ZyxelScanner::extractCell(row, cols[3]).c_str());
}

TEST(ZyxelScannerTest, ExtractCellTruncatedAndEmpty) {
    std::string header  = "Index  Rule Name             From    To";
    std::string divider = "=====  ====================  ======  ======";
    auto cols = ZyxelScanner::parseTableColumns(header, divider);

    std::string shortRow = "1      Short Name";
    STRCMP_EQUAL("1", ZyxelScanner::extractCell(shortRow, cols[0]).c_str());
    STRCMP_EQUAL("Short Name", ZyxelScanner::extractCell(shortRow, cols[1]).c_str());
    STRCMP_EQUAL("", ZyxelScanner::extractCell(shortRow, cols[2]).c_str());
    STRCMP_EQUAL("", ZyxelScanner::extractCell(shortRow, cols[3]).c_str());

    std::string emptyRow = "";
    STRCMP_EQUAL("", ZyxelScanner::extractCell(emptyRow, cols[0]).c_str());
    STRCMP_EQUAL("", ZyxelScanner::extractCell(emptyRow, cols[1]).c_str());
}

TEST(ZyxelScannerTest, ExtractPercentage) {
    double val = 0.0;

    CHECK_TRUE(ZyxelScanner::extractPercentage("15%", val));
    DOUBLES_EQUAL(15.0, val, 0.001);

    CHECK_TRUE(ZyxelScanner::extractPercentage("25.5%", val));
    DOUBLES_EQUAL(25.5, val, 0.001);

    CHECK_TRUE(ZyxelScanner::extractPercentage("CPU utilization: 25 %", val));
    DOUBLES_EQUAL(25.0, val, 0.001);

    CHECK_TRUE(ZyxelScanner::extractPercentage("Memory: 1024MB / 4096MB (25%)", val));
    DOUBLES_EQUAL(25.0, val, 0.001);

    CHECK_TRUE(ZyxelScanner::extractPercentage("0% packet loss", val));
    DOUBLES_EQUAL(0.0, val, 0.001);

    CHECK_TRUE(ZyxelScanner::extractPercentage("100%", val));
    DOUBLES_EQUAL(100.0, val, 0.001);

    CHECK_FALSE(ZyxelScanner::extractPercentage("no percentage here", val));
    CHECK_FALSE(ZyxelScanner::extractPercentage("%%", val));
    CHECK_FALSE(ZyxelScanner::extractPercentage("", val));
}

TEST(ZyxelScannerTest, ExtractIntegerAfter) {
    int val = 0;

    CHECK_TRUE(ZyxelScanner::extractIntegerAfter("Active sessions: 142", "session", val));
    LONGS_EQUAL(142, val);

    CHECK_TRUE(ZyxelScanner::extractIntegerAfter("Current sessions: 250 / 50000", "session", val));
    LONGS_EQUAL(250, val);

    CHECK_TRUE(ZyxelScanner::extractIntegerAfter("Current sessions: 250 / 50000", "/", val));
    LONGS_EQUAL(50000, val);

    CHECK_TRUE(ZyxelScanner::extractIntegerAfter("Max sessions: 1000000", "max", val));
    LONGS_EQUAL(1000000, val);

    CHECK_TRUE(ZyxelScanner::extractIntegerAfter("count = 4", "count", val));
    LONGS_EQUAL(4, val);

    CHECK_FALSE(ZyxelScanner::extractIntegerAfter("no match here", "sessions", val));
    CHECK_FALSE(ZyxelScanner::extractIntegerAfter("sessions: none", "sessions", val));
}

TEST(ZyxelScannerTest, FindDividerLineRejectsBanners) {
    std::vector<std::string> bannerLines = {
        "=================================================",
        "Welcome to ZyWALL USG FLEX 200",
        "=================================================",
        "Router> "
    };
    LONGS_EQUAL(-1, ZyxelScanner::findDividerLine(bannerLines));

    std::vector<std::string> tableLines = {
        "Interface       Status  IP Address",
        "==================================",
        "wan1            up      1.2.3.4"
    };
    LONGS_EQUAL(1, ZyxelScanner::findDividerLine(tableLines));
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
