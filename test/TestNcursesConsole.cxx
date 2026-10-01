/*
 * TestNcursesConsole.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include <iostream>
#include <string>
#include <vector>
#include "NcursesConsole.hxx"
#include <CppUTest/TestHarness.h>

TEST_GROUP(NcursesWordWrap) {
    void setup() {}
    void teardown() {}
};

TEST(NcursesWordWrap, ShortTextFitsInSingleLine) {
    std::string text = "NetMon v1.0.5 is running nominal";
    auto lines = NcursesConsole::wrapText(text, 79);
    LONGS_EQUAL(1, lines.size());
    STRCMP_EQUAL(text.c_str(), lines[0].c_str());
}

TEST(NcursesWordWrap, ExactLengthBoundary) {
    std::string text(79, 'A');
    auto lines = NcursesConsole::wrapText(text, 79);
    LONGS_EQUAL(1, lines.size());
    STRCMP_EQUAL(text.c_str(), lines[0].c_str());
}

TEST(NcursesWordWrap, WordBoundaryWrapAndIndentation) {
    // 85 character sentence with spaces
    std::string text = "LanSniffer: Live streaming capture active on network bridge interface br0 with zero drops";
    auto lines = NcursesConsole::wrapText(text, 60);

    // Must split into 2 lines
    LONGS_EQUAL(2, lines.size());
    // First line must be <= 60 characters
    CHECK_TRUE((int)lines[0].size() <= 60);
    // Continuation line must start with two spaces indentation
    CHECK_TRUE(lines[1].rfind("  ", 0) == 0);
    // Entire text content must be preserved across split
    std::string combined = lines[0] + " " + lines[1].substr(2);
    STRCMP_EQUAL(text.c_str(), combined.c_str());
}

TEST(NcursesWordWrap, UnbrokenLongTokenSplitsGracefully) {
    // 90 character unbroken string (e.g. SHA-256 hash or token)
    std::string unbroken(90, 'X');
    auto lines = NcursesConsole::wrapText(unbroken, 50);

    LONGS_EQUAL(2, lines.size());
    LONGS_EQUAL(50, lines[0].size());
    // Continuation starts with two spaces
    CHECK_TRUE(lines[1].rfind("  ", 0) == 0);
    LONGS_EQUAL(42, lines[1].size()); // 2 space indent + 40 chars remaining
}

TEST(NcursesWordWrap, MultilinePreservesEmptyLinesAndCarriageReturns) {
    std::string multi = "Header Line\r\n\r\nSecond Line\nThird Line";
    auto lines = NcursesConsole::wrapText(multi, 79);

    LONGS_EQUAL(4, lines.size());
    STRCMP_EQUAL("Header Line", lines[0].c_str());
    STRCMP_EQUAL("", lines[1].c_str());
    STRCMP_EQUAL("Second Line", lines[2].c_str());
    STRCMP_EQUAL("Third Line", lines[3].c_str());
}

TEST(NcursesWordWrap, MinimumBoundaryHandling) {
    std::string text = "hello";
    auto lines = NcursesConsole::wrapText(text, 3);
    LONGS_EQUAL(1, lines.size());
    STRCMP_EQUAL("hello", lines[0].c_str());
}

TEST_GROUP(NcursesConsoleGeometry) {
    void setup() {}
    void teardown() {}
};

TEST(NcursesConsoleGeometry, Standard80x24BudgetingCalculation) {
    auto b = NcursesConsole::calculateLayout(24);
    LONGS_EQUAL(1, b.headerRows);
    LONGS_EQUAL(1, b.logSepRows);
    LONGS_EQUAL(4, b.logRows);
    LONGS_EQUAL(1, b.midSepRows);
    LONGS_EQUAL(15, b.cmdHeight);
    LONGS_EQUAL(1, b.bottomSepRows);
    LONGS_EQUAL(1, b.inputRows);

    int total = b.headerRows + b.logSepRows + b.logRows + b.midSepRows + b.cmdHeight + b.bottomSepRows + b.inputRows;
    LONGS_EQUAL(24, total);
}

TEST(NcursesConsoleGeometry, Short19RowBudgetingCalculation) {
    auto b = NcursesConsole::calculateLayout(19);
    LONGS_EQUAL(1, b.headerRows);
    LONGS_EQUAL(1, b.logSepRows);
    LONGS_EQUAL(2, b.logRows);
    LONGS_EQUAL(1, b.midSepRows);
    LONGS_EQUAL(12, b.cmdHeight);
    LONGS_EQUAL(1, b.bottomSepRows);
    LONGS_EQUAL(1, b.inputRows);

    int total = b.headerRows + b.logSepRows + b.logRows + b.midSepRows + b.cmdHeight + b.bottomSepRows + b.inputRows;
    LONGS_EQUAL(19, total);
}

TEST_GROUP(NcursesStreamBufTest) {
    void setup() {}
    void teardown() {}
};

TEST(NcursesStreamBufTest, ColorPairConstantsDefined) {
    CHECK_TRUE(NcursesConsole::PAIR_HEADER == 1);
    CHECK_TRUE(NcursesConsole::PAIR_PROMPT == 2);
    CHECK_TRUE(NcursesConsole::PAIR_INFO == 3);
    CHECK_TRUE(NcursesConsole::PAIR_WARN == 4);
    CHECK_TRUE(NcursesConsole::PAIR_ERROR == 5);
    CHECK_TRUE(NcursesConsole::PAIR_TEXT == 6);
    CHECK_TRUE(NcursesConsole::PAIR_MUTED == 7);
}

TEST(NcursesStreamBufTest, InitialInputStateIsReady) {
    auto &console = NcursesConsole::getInstance();
    CHECK_TRUE(console.getInputState() == NcursesConsole::InputState::READY);
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
