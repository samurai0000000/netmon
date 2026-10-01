/*
 * ZyxelScanner.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "zyxel/ZyxelScanner.hxx"

#include <iostream>
#include <algorithm>
#include <cctype>

namespace ZyxelScanner {

std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

std::vector<std::string> splitLines(const std::string &raw) {
    std::vector<std::string> lines;
    std::istringstream stream(raw);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        // Replace non-printable and invalid high-byte characters with ASCII safe chars
        for (char &c : line) {
            unsigned char uc = static_cast<unsigned char>(c);
            if (uc < 32 && uc != '\t') {
                c = ' ';
            } else if (uc > 126) {
                c = '?';
            }
        }
        lines.push_back(line);
    }
    return lines;
}

std::vector<std::string> splitTokens(const std::string &line) {
    std::vector<std::string> tokens;
    std::istringstream stream(line);
    std::string token;
    while (stream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

bool parseKeyValue(const std::string &line, std::string &key, std::string &value, char delim) {
    size_t pos = line.find(delim);
    if (pos == std::string::npos) {
        return false;
    }
    key = trim(line.substr(0, pos));
    value = trim(line.substr(pos + 1));
    return !key.empty();
}

int findDividerLine(const std::vector<std::string> &lines) {
    static const char *headerKeywords[] = {
        "index", "rule", "name", "from", "to", "status", "interface",
        "ip", "address", "netmask", "mask", "mac", "mtu", "slot",
        "model", "firmware", "version", "service", "action", "source",
        "destination", "type", "port", "zone", "metric", "gateway",
        "route", "prefix", "hops"
    };

    for (size_t i = 1; i < lines.size(); ++i) {
        std::string trimmed = trim(lines[i]);
        if (trimmed.length() >= 4 &&
            (trimmed.find("====") == 0 || trimmed.find("----") == 0)) {
            // Check if divider line has segmented columns: e.g. "=====  ===="
            size_t firstSpace = trimmed.find_first_of(" \t");
            if (firstSpace != std::string::npos &&
                trimmed.find_first_not_of(" \t", firstSpace) != std::string::npos) {
                return static_cast<int>(i);
            }

            // Reject if line i-2 was also a divider line (indicates a banner box)
            if (i >= 2) {
                std::string prev2 = trim(lines[i - 2]);
                if (prev2.length() >= 4 &&
                    (prev2.find("====") == 0 || prev2.find("----") == 0)) {
                    continue;
                }
            }

            // For continuous divider line, header line must contain at least one gap (2+ spaces)
            const std::string &prev = lines[i - 1];
            if (prev.find("  ") == std::string::npos) {
                continue;
            }

            // Verify preceding line contains recognizable table keywords
            std::string lowerPrev = prev;
            std::transform(lowerPrev.begin(), lowerPrev.end(), lowerPrev.begin(), ::tolower);
            for (const char *kw : headerKeywords) {
                if (lowerPrev.find(kw) != std::string::npos) {
                    return static_cast<int>(i);
                }
            }
        }
    }
    return -1;
}

std::vector<TableColumn> parseTableColumns(const std::string &headerLine, const std::string &dividerLine) {
    std::vector<TableColumn> columns;
    if (dividerLine.empty()) {
        return columns;
    }

    // Check if divider line has segmented runs (i.e. spaces separating divider dashes/equals)
    bool hasDividerSegments = false;
    size_t firstNonSpace = dividerLine.find_first_not_of(" \t");
    if (firstNonSpace != std::string::npos) {
        size_t firstSpace = dividerLine.find_first_of(" \t", firstNonSpace);
        if (firstSpace != std::string::npos &&
            dividerLine.find_first_not_of(" \t", firstSpace) != std::string::npos) {
            hasDividerSegments = true;
        }
    }

    if (hasDividerSegments) {
        // Divider has explicit runs of '=' or '-' separated by spaces:
        // e.g. "=====  ====================  ======  ======"
        struct Span {
            size_t start;
            size_t end;
        };
        std::vector<Span> spans;
        size_t idx = 0;
        while (idx < dividerLine.length()) {
            if (dividerLine[idx] != ' ' && dividerLine[idx] != '\t') {
                size_t spanStart = idx;
                while (idx < dividerLine.length() && dividerLine[idx] != ' ' && dividerLine[idx] != '\t') {
                    idx++;
                }
                spans.push_back({spanStart, idx});
            } else {
                idx++;
            }
        }

        for (size_t i = 0; i < spans.size(); ++i) {
            TableColumn col;
            col.startCol = spans[i].start;
            col.endCol = (i + 1 < spans.size()) ? spans[i + 1].start : std::string::npos;
            if (col.startCol < headerLine.length()) {
                size_t count = (col.endCol != std::string::npos && col.endCol <= headerLine.length()) ?
                                (col.endCol - col.startCol) : std::string::npos;
                col.name = trim(headerLine.substr(col.startCol, count));
            }
            columns.push_back(col);
        }
    } else {
        // Continuous divider line (e.g. "====================================")
        // Detect columns from headerLine using gaps of >= 2 consecutive spaces
        size_t pos = headerLine.find_first_not_of(" \t");
        while (pos != std::string::npos && pos < headerLine.length()) {
            size_t nextGap = headerLine.find("  ", pos);
            if (nextGap == std::string::npos) {
                TableColumn col;
                col.startCol = pos;
                col.endCol = std::string::npos;
                col.name = trim(headerLine.substr(pos));
                if (!col.name.empty()) {
                    columns.push_back(col);
                }
                break;
            }

            size_t nextColStart = headerLine.find_first_not_of(" \t", nextGap);
            if (nextColStart == std::string::npos) {
                TableColumn col;
                col.startCol = pos;
                col.endCol = std::string::npos;
                col.name = trim(headerLine.substr(pos));
                if (!col.name.empty()) {
                    columns.push_back(col);
                }
                break;
            }

            TableColumn col;
            col.startCol = pos;
            col.endCol = nextColStart;
            col.name = trim(headerLine.substr(pos, nextColStart - pos));
            if (!col.name.empty()) {
                columns.push_back(col);
            }
            pos = nextColStart;
        }
    }

    if (!columns.empty()) {
        columns[0].startCol = 0;
    }

    return columns;
}

int findColumn(const std::vector<TableColumn> &cols, const std::string &keyword) {
    if (keyword.empty()) {
        return -1;
    }
    std::string lowerKw = keyword;
    std::transform(lowerKw.begin(), lowerKw.end(), lowerKw.begin(), ::tolower);
    for (size_t i = 0; i < cols.size(); ++i) {
        std::string lowerName = cols[i].name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
        if (lowerName.find(lowerKw) != std::string::npos) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::string extractCell(const std::string &row, const TableColumn &col) {
    if (col.startCol >= row.length()) {
        return "";
    }
    size_t count = std::string::npos;
    if (col.endCol != std::string::npos && col.endCol > col.startCol) {
        count = col.endCol - col.startCol;
    }
    return trim(row.substr(col.startCol, count));
}

bool extractPercentage(const std::string &text, double &valOut) {
    size_t pctPos = text.find('%');
    if (pctPos == std::string::npos) {
        return false;
    }

    // Walk back over any spaces or tabs immediately before '%'
    size_t idx = pctPos;
    while (idx > 0 && (text[idx - 1] == ' ' || text[idx - 1] == '\t')) {
        idx--;
    }
    if (idx == 0) {
        return false;
    }

    size_t numEnd = idx;
    bool hasDot = false;
    while (idx > 0) {
        char c = text[idx - 1];
        if (std::isdigit(static_cast<unsigned char>(c))) {
            idx--;
        } else if (c == '.' && !hasDot) {
            hasDot = true;
            idx--;
        } else {
            break;
        }
    }
    if (idx == numEnd) {
        return false;
    }

    std::string numStr = text.substr(idx, numEnd - idx);
    if (numStr == ".") {
        return false;
    }

    try {
        valOut = std::stod(numStr);
        return true;
    } catch (...) {
        return false;
    }
}

bool extractIntegerAfter(const std::string &text, const std::string &keyword, int &valOut) {
    if (text.empty() || keyword.empty()) {
        return false;
    }

    std::string lowerText = text;
    std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::tolower);
    std::string lowerKw = keyword;
    std::transform(lowerKw.begin(), lowerKw.end(), lowerKw.begin(), ::tolower);

    size_t kwPos = lowerText.find(lowerKw);
    if (kwPos == std::string::npos) {
        return false;
    }

    size_t idx = kwPos + lowerKw.length();
    // Scan forward up to 40 characters or boundary (',', ';', '\n') to find a digit or sign
    size_t limit = std::min(text.length(), idx + 40);
    while (idx < limit && text[idx] != ',' && text[idx] != ';' && text[idx] != '\n') {
        char c = text[idx];
        if (std::isdigit(static_cast<unsigned char>(c)) ||
            ((c == '-' || c == '+') && idx + 1 < text.length() &&
             std::isdigit(static_cast<unsigned char>(text[idx + 1])))) {
            break;
        }
        // If keyword is not "/", stop if we hit a slash
        if (lowerKw != "/" && c == '/') {
            return false;
        }
        idx++;
    }

    if (idx >= limit || text[idx] == ',' || text[idx] == ';' || text[idx] == '\n') {
        return false;
    }

    size_t start = idx;
    size_t end = start;
    if (text[end] == '-' || text[end] == '+') {
        end++;
    }
    if (end >= text.length() || !std::isdigit(static_cast<unsigned char>(text[end]))) {
        return false;
    }
    while (end < text.length() && std::isdigit(static_cast<unsigned char>(text[end]))) {
        end++;
    }

    try {
        valOut = std::stoi(text.substr(start, end - start));
        return true;
    } catch (...) {
        return false;
    }
}


std::string sanitizeSnippet(const std::string &line, size_t maxLen) {
    std::string sanitized = line;

    // Redact sensitive patterns without std::regex (to eliminate heap leak flags in CppUTest)
    static const char *sensitiveKeys[] = {"password", "secret", "key", "token", "community"};
    for (const char *kw : sensitiveKeys) {
        size_t pos = 0;
        std::string lower = sanitized;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        while ((pos = lower.find(kw, pos)) != std::string::npos) {
            size_t delimPos = sanitized.find_first_of(":= \t", pos + std::string(kw).length());
            if (delimPos != std::string::npos && delimPos < pos + std::string(kw).length() + 3) {
                size_t valStart = sanitized.find_first_not_of(":= \t", delimPos);
                if (valStart != std::string::npos) {
                    size_t valEnd = sanitized.find_first_of(" \t\r\n,;", valStart);
                    if (valEnd == std::string::npos) valEnd = sanitized.length();
                    sanitized.replace(valStart, valEnd - valStart, "[REDACTED]");
                    lower = sanitized;
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                }
            }
            pos += std::string(kw).length();
        }
    }

    // Strip carriage returns and control characters
    sanitized.erase(std::remove_if(sanitized.begin(), sanitized.end(), [](char c) {
        return (c == '\r' || (static_cast<unsigned char>(c) < 32 && c != '\t'));
    }), sanitized.end());

    if (sanitized.length() > maxLen) {
        sanitized = sanitized.substr(0, maxLen - 12) + " [TRUNCATED]";
    }
    return sanitized;
}

static bool s_quietLogging = false;

void setQuietLogging(bool quiet) {
    s_quietLogging = quiet;
}

void logParseError(const std::string &className, const std::string &cmd,
                   size_t lineNum, const std::string &reason,
                   const std::string &snippet) {
    if (s_quietLogging) {
        return;
    }
    std::cerr << "[ZYXEL_PARSE_ERROR] class=" << className
              << " cmd=\"" << cmd << "\""
              << " line=" << lineNum
              << " reason=\"" << reason << "\""
              << " snippet=\"" << sanitizeSnippet(snippet, 80) << "\""
              << std::endl;
}

} // namespace ZyxelScanner

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
