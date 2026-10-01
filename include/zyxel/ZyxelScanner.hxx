/*
 * ZyxelScanner.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYXEL_SCANNER_HXX
#define NETMON_ZYXEL_SCANNER_HXX

#include <string>
#include <vector>
#include <sstream>

namespace ZyxelScanner {

struct TableColumn {
    std::string name;
    size_t      startCol = 0;
    size_t      endCol = std::string::npos;
};

std::string trim(const std::string &str);
std::vector<std::string> splitLines(const std::string &raw);
std::vector<std::string> splitTokens(const std::string &line);
bool parseKeyValue(const std::string &line, std::string &key, std::string &value, char delim = ':');
int findDividerLine(const std::vector<std::string> &lines);
std::vector<TableColumn> parseTableColumns(const std::string &headerLine, const std::string &dividerLine);
int findColumn(const std::vector<TableColumn> &cols, const std::string &keyword);
std::string extractCell(const std::string &row, const TableColumn &col);
bool extractPercentage(const std::string &text, double &valOut);
bool extractIntegerAfter(const std::string &text, const std::string &keyword, int &valOut);
std::string sanitizeSnippet(const std::string &line, size_t maxLen = 80);
void setQuietLogging(bool quiet);
void logParseError(const std::string &className, const std::string &cmd,
                   size_t lineNum, const std::string &reason,
                   const std::string &snippet);

} // namespace ZyxelScanner

#endif /* NETMON_ZYXEL_SCANNER_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
