/*
 * NcursesConsole.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "NcursesConsole.hxx"
#include "Version.hxx"
#include "Config.hxx"
#include "LanSniffer.hxx"
#include "ZyxelDriver.hxx"
#include "AimonGatewayClient.hxx"
#include "DeviceRegistry.hxx"
#include "NetMonShell.hxx"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <cctype>
#include <clocale>

// --- NcursesStreamBuf Implementation ---

NcursesStreamBuf::NcursesStreamBuf(NcursesConsole &console, bool toServerLog, int colorPair)
    : _console(console),
      _toServerLog(toServerLog),
      _colorPair(colorPair) {
}

NcursesStreamBuf::int_type NcursesStreamBuf::overflow(int_type c) {
    if (c != EOF) {
        std::string line;
        bool hasLine = false;
        {
            std::lock_guard<std::mutex> lock(_bufferMutex);
            if (c == '\n') {
                line = _lineBuffer;
                _lineBuffer.clear();
                hasLine = true;
            } else if (c != '\r') {
                _lineBuffer += static_cast<char>(c);
            }
        }
        if (hasLine) {
            if (_toServerLog) {
                _console.logServer(line, _colorPair);
            } else {
                _console.logOutput(line, _colorPair, false);
            }
        }
    }
    return c;
}

int NcursesStreamBuf::sync() {
    std::string line;
    bool hasLine = false;
    {
        std::lock_guard<std::mutex> lock(_bufferMutex);
        if (!_lineBuffer.empty()) {
            line = _lineBuffer;
            _lineBuffer.clear();
            hasLine = true;
        }
    }
    if (hasLine) {
        if (_toServerLog) {
            _console.logServer(line, _colorPair);
        } else {
            _console.logOutput(line, _colorPair, false);
        }
    }
    return 0;
}

// --- NcursesConsole Implementation ---

NcursesConsole &NcursesConsole::getInstance() {
    static NcursesConsole instance;
    return instance;
}

NcursesConsole::NcursesConsole()
    : _coutBuf(*this, true, PAIR_MUTED),
      _cerrBuf(*this, true, PAIR_ERROR),
      _cmdOutputBuf(*this, false, PAIR_TEXT) {
}

NcursesConsole::~NcursesConsole() {
    shutdown();
}

bool NcursesConsole::isRunning() const {
    return _running.load();
}

NcursesConsole::InputState NcursesConsole::getInputState() const {
    return _inputState.load();
}

std::vector<std::string> NcursesConsole::wrapText(const std::string &text, int maxCols) {
    std::vector<std::string> result;
    if (maxCols <= 4) {
        result.push_back(text);
        return result;
    }

    std::istringstream iss(text);
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if ((int)line.size() <= maxCols) {
            result.push_back(line);
            continue;
        }

        // Line exceeds maxCols -> wrap on word boundary
        std::string remaining = line;
        bool isContinuation = false;
        while ((int)remaining.size() > maxCols) {
            int limit = maxCols;
            int breakPos = -1;
            int minBreak = isContinuation ? 4 : 1;
            for (int i = limit; i >= minBreak; --i) {
                if (remaining[i] == ' ' || remaining[i] == '\t') {
                    breakPos = i;
                    break;
                }
            }

            if (breakPos > 0) {
                result.push_back(remaining.substr(0, breakPos));
                size_t nextStart = remaining.find_first_not_of(" \t", breakPos);
                if (nextStart != std::string::npos) {
                    remaining = "  " + remaining.substr(nextStart);
                } else {
                    remaining.clear();
                    break;
                }
            } else {
                // No whitespace delimiter found (unbroken token)
                result.push_back(remaining.substr(0, limit));
                remaining = "  " + remaining.substr(limit);
            }
            isContinuation = true;
        }
        if (!remaining.empty()) {
            result.push_back(remaining);
        }
    }
    return result;
}

NcursesConsole::LayoutBudget NcursesConsole::calculateLayout(int termRows) {
    if (termRows < 12) termRows = 12;
    LayoutBudget b;
    b.headerRows = 1;
    b.logSepRows = 1;
    b.logRows = (termRows >= 24) ? 4 : 2;
    b.midSepRows = 1;
    b.bottomSepRows = 1;
    b.inputRows = 1;
    b.cmdHeight = termRows - b.headerRows - b.logSepRows - b.logRows - b.midSepRows - b.bottomSepRows - b.inputRows;
    if (b.cmdHeight < 3) b.cmdHeight = 3;
    return b;
}

bool NcursesConsole::init() {
    std::lock_guard<std::recursive_mutex> lock(_uiMutex);
    if (_initialized.load()) {
        return true;
    }

    setlocale(LC_ALL, "");

    initscr();
    cbreak();
    noecho();
    nonl();
    set_escdelay(25);

    if (has_colors()) {
        start_color();
        use_default_colors();
        init_pair(PAIR_HEADER, COLOR_WHITE, COLOR_BLUE);
        init_pair(PAIR_PROMPT, COLOR_GREEN, -1);
        init_pair(PAIR_INFO, COLOR_CYAN, -1);
        init_pair(PAIR_WARN, COLOR_YELLOW, -1);
        init_pair(PAIR_ERROR, COLOR_RED, -1);
        init_pair(PAIR_TEXT, COLOR_WHITE, -1);
        init_pair(PAIR_MUTED, COLOR_WHITE, -1);
    }

    setupWindows();
    _initialized = true;
    _running = true;

    // Redirect stdout and stderr so daemon server logs go strictly to the top log pane
    _oldCoutBuf = std::cout.rdbuf(&_coutBuf);
    _oldCerrBuf = std::cerr.rdbuf(&_cerrBuf);

    return true;
}

void NcursesConsole::setupWindows() {
    getmaxyx(stdscr, _termRows, _termCols);

    if (_termRows < 12) _termRows = 12;
    if (_termCols < 40) _termCols = 40;

    auto b = calculateLayout(_termRows);
    _logHeight = b.logRows;
    _cmdHeight = b.cmdHeight;

    // 1. Header window (Row 0)
    _headerWin = newwin(b.headerRows, _termCols, 0, 0);

    // 2. Server log separator (Row 1)
    _logSepWin = newwin(b.logSepRows, _termCols, 1, 0);

    // 3. Server logs window (Rows 2 to 2 + _logHeight - 1)
    _logWin = newwin(b.logRows, _termCols, 2, 0);
    scrollok(_logWin, FALSE);

    // 4. Middle divider with title & scroll badge (Row 2 + _logHeight)
    int midSepRow = 2 + _logHeight;
    _midSepWin = newwin(b.midSepRows, _termCols, midSepRow, 0);

    // 5. Command outputs panel (Rows midSepRow + 1 to midSepRow + _cmdHeight)
    int cmdRow = midSepRow + 1;
    _cmdWin = newwin(_cmdHeight, _termCols, cmdRow, 0);
    scrollok(_cmdWin, FALSE);

    // 6. Bottom divider (Row _termRows - 2)
    _bottomSepWin = newwin(b.bottomSepRows, _termCols, _termRows - 2, 0);
    std::string botSep(_termCols > 1 ? _termCols - 1 : 1, '-');
    mvwprintw(_bottomSepWin, 0, 0, "%s", botSep.c_str());
    wrefresh(_bottomSepWin);

    // 7. Input line (Row _termRows - 1)
    _inputWin = newwin(b.inputRows, _termCols, _termRows - 1, 0);
    scrollok(_inputWin, FALSE);
    idlok(_inputWin, FALSE);
    wtimeout(_inputWin, 100);
    keypad(_inputWin, TRUE);

    // Initial banner output
    if (_cmdHistory.empty()) {
        addOutputLine("--- Welcome to NetMon Console ---", PAIR_INFO, true);
        addOutputLine("Type 'help' for commands, 'status' for daemon metrics, 'quit' to exit.", PAIR_INFO, false);
        addOutputLine("", 0, false);
    }

    updateHeader();
    renderLogPanel();
    renderMiddlePanel();
    redrawInputLine();
}

void NcursesConsole::destroyWindows() {
    if (_headerWin) { delwin(_headerWin); _headerWin = nullptr; }
    if (_logSepWin) { delwin(_logSepWin); _logSepWin = nullptr; }
    if (_logWin) { delwin(_logWin); _logWin = nullptr; }
    if (_midSepWin) { delwin(_midSepWin); _midSepWin = nullptr; }
    if (_cmdWin) { delwin(_cmdWin); _cmdWin = nullptr; }
    if (_bottomSepWin) { delwin(_bottomSepWin); _bottomSepWin = nullptr; }
    if (_inputWin) { delwin(_inputWin); _inputWin = nullptr; }
}

void NcursesConsole::shutdown() {
    _running = false;
    if (_inputState.load() == InputState::EXECUTING) {
        NetMonShell::getInstance().cancelCurrentCommand();
    }
    if (_cmdWorker && _cmdWorker->joinable()) {
        _cmdWorker->join();
    }
    _cmdWorker.reset();
    _inputState.store(InputState::READY);

    std::lock_guard<std::recursive_mutex> lock(_uiMutex);
    if (!_initialized.load()) {
        return;
    }

    if (_oldCoutBuf) {
        std::cout.rdbuf(_oldCoutBuf);
        _oldCoutBuf = nullptr;
    }
    if (_oldCerrBuf) {
        std::cerr.rdbuf(_oldCerrBuf);
        _oldCerrBuf = nullptr;
    }

    destroyWindows();
    endwin();
    _initialized = false;
}

void NcursesConsole::handleResize() {
    destroyWindows();
    endwin();
    refresh();
    setupWindows();
}

void NcursesConsole::setShutdownCallback(ShutdownCallback cb) {
    _shutdownCb = cb;
}

void NcursesConsole::updateHeader() {
    if (!_headerWin) return;

    std::ostringstream ss;
    ss << " NetMon v" << NETMON_VERSION_STRING << " | ";

    if (LanSniffer::getInstance().isPcapActive()) {
        ss << "PCAP: [ACTIVE: " << Config::getInstance().getInterface() << "] | ";
    } else {
        ss << "PCAP: [INACTIVE] | ";
    }

    if (ZyxelDriver::getInstance().isConnected()) {
        ss << "Zyxel: [CONNECTED] | ";
    } else {
        ss << "Zyxel: [DISCONNECTED] | ";
    }

    ss << "Devs: " << DeviceRegistry::getInstance().getDeviceCount() << " | ";

    if (AimonGatewayClient::getInstance().isConnected()) {
        ss << "aimon: [OK]";
    } else {
        ss << "aimon: [OFF]";
    }

    std::string text = ss.str();
    if ((int)text.size() < _termCols) {
        text.append(_termCols - text.size(), ' ');
    } else {
        text = text.substr(0, _termCols);
    }

    wattron(_headerWin, COLOR_PAIR(PAIR_HEADER) | A_BOLD);
    mvwprintw(_headerWin, 0, 0, "%s", text.c_str());
    wattroff(_headerWin, COLOR_PAIR(PAIR_HEADER) | A_BOLD);
    wrefresh(_headerWin);
}

void NcursesConsole::addOutputLine(const std::string &line, int colorPair, bool isBold) {
    OutputLine item;
    item.text = line;
    item.colorPair = colorPair;
    item.isBold = isBold;

    _cmdHistory.push_back(item);
    while (_cmdHistory.size() > MAX_HISTORY_LINES) {
        _cmdHistory.pop_front();
    }

    int maxScroll = std::max(0, static_cast<int>(_cmdHistory.size()) - _cmdHeight);
    if (_scrollOffset > maxScroll) {
        _scrollOffset = maxScroll;
    }
}

void NcursesConsole::logOutput(const std::string &text, int colorPair, bool isBold) {
    std::lock_guard<std::recursive_mutex> lock(_uiMutex);

    int maxCols = (_termCols > 1) ? (_termCols - 1) : 79;
    auto wrapped = wrapText(text, maxCols);
    for (const auto &l : wrapped) {
        addOutputLine(l, colorPair, isBold);
    }

    renderMiddlePanel();
    redrawInputLine();
}

void NcursesConsole::logServer(const std::string &text, int colorPair) {
    std::lock_guard<std::recursive_mutex> lock(_uiMutex);
    if (!_logWin) return;

    int maxCols = (_termCols > 2) ? (_termCols - 2) : 78;
    auto wrapped = wrapText(text, maxCols);
    for (const auto &l : wrapped) {
        LogLine item;
        item.text = l;
        item.colorPair = colorPair;
        _serverLogHistory.push_back(item);
        while (_serverLogHistory.size() > MAX_SERVER_LOG_LINES) {
            _serverLogHistory.pop_front();
        }
    }

    renderLogPanel();
    redrawInputLine();
}

void NcursesConsole::renderLogPanel() {
    if (!_logWin || !_logSepWin) return;

    // 1. Render separator header
    std::string sepLine(_termCols > 1 ? _termCols - 1 : 1, '-');
    std::string title = "--- Daemon Logs & Syslog Events ---";
    if (title.size() < sepLine.size()) {
        sepLine.replace(0, title.size(), title);
    } else {
        sepLine = title.substr(0, _termCols - 1);
    }
    werase(_logSepWin);
    mvwprintw(_logSepWin, 0, 0, "%s", sepLine.c_str());
    wrefresh(_logSepWin);

    // 2. Render visible log lines
    werase(_logWin);
    int totalLogs = static_cast<int>(_serverLogHistory.size());
    if (totalLogs == 0) {
        wattron(_logWin, COLOR_PAIR(PAIR_MUTED));
        mvwprintw(_logWin, 0, 0, "No daemon events or syslog entries recorded yet.");
        wattroff(_logWin, COLOR_PAIR(PAIR_MUTED));
    } else {
        int startIdx = std::max(0, totalLogs - _logHeight);
        int row = 0;
        for (int i = startIdx; i < totalLogs && row < _logHeight; ++i, ++row) {
            const auto &line = _serverLogHistory[i];
            int attrs = (line.colorPair > 0) ? COLOR_PAIR(line.colorPair) : 0;
            if (attrs) wattron(_logWin, attrs);
            mvwprintw(_logWin, row, 0, "%s", line.text.c_str());
            wclrtoeol(_logWin);
            if (attrs) wattroff(_logWin, attrs);
        }
    }
    wrefresh(_logWin);
}

void NcursesConsole::renderMiddlePanel() {
    if (!_cmdWin || !_midSepWin) return;

    // 1. Render middle separator line with title & scroll badge using standard ASCII dashes
    std::string midLine(_termCols > 1 ? _termCols - 1 : 1, '-');
    std::string title = "--- Command Outputs [PgUp/PgDn to scroll]";
    if (_scrollOffset > 0) {
        title += " [^ " + std::to_string(_scrollOffset) + " lines scrolled]";
    }
    title += " ---";

    if (title.size() < midLine.size()) {
        midLine.replace(0, title.size(), title);
    } else {
        midLine = title.substr(0, _termCols - 1);
    }
    werase(_midSepWin);
    mvwprintw(_midSepWin, 0, 0, "%s", midLine.c_str());
    wrefresh(_midSepWin);

    // 2. Render visible history slice in _cmdWin
    werase(_cmdWin);

    int totalLines = static_cast<int>(_cmdHistory.size());
    int endIdx = totalLines - _scrollOffset;
    int startIdx = std::max(0, endIdx - _cmdHeight);

    int row = 0;
    if (totalLines > 0 && _cmdHeight > 0) {
        for (int i = startIdx; i < endIdx && row < _cmdHeight; ++i, ++row) {
            const auto &line = _cmdHistory[i];
            int attrs = 0;
            if (line.colorPair > 0) {
                attrs |= COLOR_PAIR(line.colorPair);
            }
            if (line.isBold) {
                attrs |= A_BOLD;
            }

            if (attrs != 0) wattron(_cmdWin, attrs);
            mvwprintw(_cmdWin, row, 0, "%s", line.text.c_str());
            wclrtoeol(_cmdWin);
            if (attrs != 0) wattroff(_cmdWin, attrs);
        }
    }

    for (; row < _cmdHeight; ++row) {
        wmove(_cmdWin, row, 0);
        wclrtoeol(_cmdWin);
    }

    wrefresh(_cmdWin);
}

void NcursesConsole::redrawInputLine() {
    if (!_inputWin) return;

    werase(_inputWin);

    if (_inputState.load() == InputState::EXECUTING) {
        std::string busyMsg = "netmon [busy: " + _activeCommandName + " | Ctrl+C to abort]> ";
        if ((int)busyMsg.size() > _termCols - 2) {
            busyMsg = "netmon [busy | Ctrl+C to abort]> ";
        }
        wattron(_inputWin, COLOR_PAIR(PAIR_WARN) | A_BOLD);
        mvwprintw(_inputWin, 0, 0, "%s", busyMsg.c_str());
        wattroff(_inputWin, COLOR_PAIR(PAIR_WARN) | A_BOLD);
        wclrtoeol(_inputWin);
        wmove(_inputWin, 0, std::min(_termCols - 1, (int)busyMsg.size()));
        wrefresh(_inputWin);
        return;
    }

    wattron(_inputWin, COLOR_PAIR(PAIR_PROMPT) | A_BOLD);
    mvwprintw(_inputWin, 0, 0, "netmon> ");
    wattroff(_inputWin, COLOR_PAIR(PAIR_PROMPT) | A_BOLD);

    wattron(_inputWin, COLOR_PAIR(PAIR_TEXT));
    int promptLen = 8; // "netmon> "
    int maxInputDisplay = _termCols - promptLen - 2;
    if (maxInputDisplay <= 0) maxInputDisplay = 1;

    int cursorCol = promptLen;
    if ((int)_inputBuffer.size() > maxInputDisplay) {
        std::string visible = _inputBuffer.substr(_inputBuffer.size() - maxInputDisplay);
        mvwprintw(_inputWin, 0, promptLen, "%s", visible.c_str());
        cursorCol = promptLen + (int)visible.size();
    } else {
        mvwprintw(_inputWin, 0, promptLen, "%s", _inputBuffer.c_str());
        cursorCol = promptLen + (int)_inputBuffer.size();
    }
    wclrtoeol(_inputWin);
    wattroff(_inputWin, COLOR_PAIR(PAIR_TEXT));

    // Explicitly lock hardware cursor at the prompt insertion point
    wmove(_inputWin, 0, cursorCol);
    wrefresh(_inputWin);
}

void NcursesConsole::processCommand(const std::string &line) {
    if (line.empty()) {
        return;
    }

    if (_commandHistoryRing.empty() || _commandHistoryRing.back() != line) {
        _commandHistoryRing.push_back(line);
    }
    _historyRingIdx = -1;

    addOutputLine("netmon> " + line, PAIR_PROMPT, false);

    std::string trimmed = line;
    size_t first = trimmed.find_first_not_of(" \t");
    if (first != std::string::npos) {
        trimmed = trimmed.substr(first);
    }
    size_t last = trimmed.find_last_not_of(" \t");
    if (last != std::string::npos) {
        trimmed = trimmed.substr(0, last + 1);
    }

    if (trimmed == "clear") {
        _cmdHistory.clear();
        _scrollOffset = 0;
        addOutputLine("History cleared.", PAIR_INFO, false);
        addOutputLine("", 0, false);
        renderMiddlePanel();
        redrawInputLine();
        return;
    } else if (trimmed == "quit" || trimmed == "exit") {
        addOutputLine("Shutting down NetMon daemon...", PAIR_INFO, true);
        renderMiddlePanel();
        _running = false;
        if (_shutdownCb) {
            _shutdownCb();
        }
        return;
    }

    // Set state to EXECUTING and spawn background worker thread
    _inputState.store(InputState::EXECUTING);
    _activeCommandName = trimmed;
    _workerFinished.store(false);
    _scrollOffset = 0;
    renderMiddlePanel();
    redrawInputLine();

    if (_cmdWorker && _cmdWorker->joinable()) {
        _cmdWorker->join();
    }
    _cmdWorker = std::make_unique<std::thread>([this, line]() {
        auto *prevCout = std::cout.rdbuf(&_cmdOutputBuf);
        auto *prevCerr = std::cerr.rdbuf(&_cmdOutputBuf);
        int rc = NetMonShell::getInstance().executeCommand(line);
        std::cout << std::flush;
        std::cerr << std::flush;
        std::cout.rdbuf(prevCout);
        std::cerr.rdbuf(prevCerr);
        _workerExitCode = rc;
        _workerFinished.store(true);
    });
}

void NcursesConsole::run() {
    int headerTick = 0;

    while (_running.load()) {
        if (_workerFinished.load()) {
            if (_cmdWorker && _cmdWorker->joinable()) {
                _cmdWorker->join();
            }
            _cmdWorker.reset();
            _workerFinished.store(false);
            _inputState.store(InputState::READY);
            _activeCommandName.clear();

            flushinp();
            _inputBuffer.clear();

            if (_workerExitCode < 0) {
                _running.store(false);
                if (_shutdownCb) {
                    _shutdownCb();
                }
                break;
            }

            std::lock_guard<std::recursive_mutex> lock(_uiMutex);
            _scrollOffset = 0;
            renderMiddlePanel();
            updateHeader();
            redrawInputLine();
        }

        bool wasExecuting = (_inputState.load() == InputState::EXECUTING);
        int ch = wgetch(_inputWin);

        if (_workerFinished.load()) {
            if (_cmdWorker && _cmdWorker->joinable()) {
                _cmdWorker->join();
            }
            _cmdWorker.reset();
            _workerFinished.store(false);
            _inputState.store(InputState::READY);
            _activeCommandName.clear();

            flushinp();
            _inputBuffer.clear();

            if (_workerExitCode < 0) {
                _running.store(false);
                if (_shutdownCb) {
                    _shutdownCb();
                }
                break;
            }

            std::lock_guard<std::recursive_mutex> lock(_uiMutex);
            _scrollOffset = 0;
            renderMiddlePanel();
            updateHeader();
            redrawInputLine();
        }

        if (ch == ERR) {
            if (++headerTick >= 20) { // Update header every ~2s
                headerTick = 0;
                std::lock_guard<std::recursive_mutex> lock(_uiMutex);
                updateHeader();
                redrawInputLine();
            }
            continue;
        }

        if (ch == KEY_RESIZE) {
            std::lock_guard<std::recursive_mutex> lock(_uiMutex);
            handleResize();
            continue;
        }

        std::lock_guard<std::recursive_mutex> lock(_uiMutex);

        // If the console was executing when this event was received:
        if (wasExecuting) {
            // Ctrl+C (ASCII 3): cancel active command immediately
            if (ch == 3) {
                NetMonShell::getInstance().cancelCurrentCommand();
                addOutputLine("^C [Canceling active command...]", PAIR_WARN, false);
                renderMiddlePanel();
                redrawInputLine();
                continue;
            }

            // Scrolling middle panel during execution is allowed
            if (ch == KEY_PPAGE) {
                int maxScroll = std::max(0, static_cast<int>(_cmdHistory.size()) - _cmdHeight);
                _scrollOffset = std::min(maxScroll, _scrollOffset + std::max(1, _cmdHeight - 2));
                renderMiddlePanel();
                redrawInputLine();
                continue;
            }
            if (ch == KEY_NPAGE) {
                _scrollOffset = std::max(0, _scrollOffset - std::max(1, _cmdHeight - 2));
                renderMiddlePanel();
                redrawInputLine();
                continue;
            }
            if (ch == 27) { // ESC sequence for PgUp/PgDn
                wtimeout(_inputWin, 25);
                int c2 = wgetch(_inputWin);
                if (c2 == '[' || c2 == 'O') {
                    int c3 = wgetch(_inputWin);
                    if (c3 == '5') {
                        int c4 = wgetch(_inputWin); (void)c4;
                        int maxScroll = std::max(0, static_cast<int>(_cmdHistory.size()) - _cmdHeight);
                        _scrollOffset = std::min(maxScroll, _scrollOffset + std::max(1, _cmdHeight - 2));
                        renderMiddlePanel();
                        redrawInputLine();
                    } else if (c3 == '6') {
                        int c4 = wgetch(_inputWin); (void)c4;
                        _scrollOffset = std::max(0, _scrollOffset - std::max(1, _cmdHeight - 2));
                        renderMiddlePanel();
                        redrawInputLine();
                    }
                }
                wtimeout(_inputWin, 100);
                continue;
            }
            if (ch == 12) {
                handleResize();
                continue;
            }

            // Drop any other characters typed during execution!
            continue;
        }

        // --- NORMAL READY STATE INPUT HANDLING ---
        // Ctrl+D (ASCII 4)
        if (ch == 4) {
            if (_inputBuffer.empty()) {
                addOutputLine("Shutting down NetMon daemon...", PAIR_INFO, true);
                renderMiddlePanel();
                _running = false;
                if (_shutdownCb) {
                    _shutdownCb();
                }
                break;
            }
            continue;
        }

        // Ctrl+C (ASCII 3) in READY mode: clear current input line
        if (ch == 3) {
            _inputBuffer.clear();
            _historyRingIdx = -1;
            redrawInputLine();
            continue;
        }

        // ESC / Escape sequences (Up, Down, PgUp, PgDn from screen/SSH)
        if (ch == 27) {
            wtimeout(_inputWin, 25);
            int c2 = wgetch(_inputWin);
            if (c2 == '[' || c2 == 'O') {
                int c3 = wgetch(_inputWin);
                if (c3 == 'A') { // Up arrow: command history recall
                    if (!_commandHistoryRing.empty()) {
                        if (_historyRingIdx == -1) {
                            _historyRingIdx = static_cast<int>(_commandHistoryRing.size()) - 1;
                        } else if (_historyRingIdx > 0) {
                            _historyRingIdx--;
                        }
                        if (_historyRingIdx >= 0 && _historyRingIdx < (int)_commandHistoryRing.size()) {
                            _inputBuffer = _commandHistoryRing[_historyRingIdx];
                            redrawInputLine();
                        }
                    }
                } else if (c3 == 'B') { // Down arrow: command history recall
                    if (_historyRingIdx != -1) {
                        _historyRingIdx++;
                        if (_historyRingIdx < (int)_commandHistoryRing.size()) {
                            _inputBuffer = _commandHistoryRing[_historyRingIdx];
                        } else {
                            _historyRingIdx = -1;
                            _inputBuffer.clear();
                        }
                        redrawInputLine();
                    }
                } else if (c3 == '5') { // PgUp (\033[5~)
                    int c4 = wgetch(_inputWin);
                    (void)c4;
                    int maxScroll = std::max(0, static_cast<int>(_cmdHistory.size()) - _cmdHeight);
                    _scrollOffset = std::min(maxScroll, _scrollOffset + std::max(1, _cmdHeight - 2));
                    renderMiddlePanel();
                    redrawInputLine();
                } else if (c3 == '6') { // PgDn (\033[6~)
                    int c4 = wgetch(_inputWin);
                    (void)c4;
                    _scrollOffset = std::max(0, _scrollOffset - std::max(1, _cmdHeight - 2));
                    renderMiddlePanel();
                    redrawInputLine();
                }
            }
            wtimeout(_inputWin, 100);
            continue;
        }

        // Up arrow or Ctrl+P: Command history recall previous
        if (ch == KEY_UP || ch == 16) {
            if (!_commandHistoryRing.empty()) {
                if (_historyRingIdx == -1) {
                    _historyRingIdx = static_cast<int>(_commandHistoryRing.size()) - 1;
                } else if (_historyRingIdx > 0) {
                    _historyRingIdx--;
                }
                if (_historyRingIdx >= 0 && _historyRingIdx < (int)_commandHistoryRing.size()) {
                    _inputBuffer = _commandHistoryRing[_historyRingIdx];
                    redrawInputLine();
                }
            }
            continue;
        }

        // Down arrow or Ctrl+N: Command history recall next
        if (ch == KEY_DOWN || ch == 14) {
            if (_historyRingIdx != -1) {
                _historyRingIdx++;
                if (_historyRingIdx < (int)_commandHistoryRing.size()) {
                    _inputBuffer = _commandHistoryRing[_historyRingIdx];
                } else {
                    _historyRingIdx = -1;
                    _inputBuffer.clear();
                }
                redrawInputLine();
            }
            continue;
        }

        // Page Up: scroll middle panel up by page
        if (ch == KEY_PPAGE) {
            int maxScroll = std::max(0, static_cast<int>(_cmdHistory.size()) - _cmdHeight);
            _scrollOffset = std::min(maxScroll, _scrollOffset + std::max(1, _cmdHeight - 2));
            renderMiddlePanel();
            redrawInputLine();
            continue;
        }

        // Page Down: scroll middle panel down by page
        if (ch == KEY_NPAGE) {
            _scrollOffset = std::max(0, _scrollOffset - std::max(1, _cmdHeight - 2));
            renderMiddlePanel();
            redrawInputLine();
            continue;
        }

        // Ctrl+L (ASCII 12): Repaint and clear screen
        if (ch == 12) {
            handleResize();
            continue;
        }

        // Enter key: dispatch command
        if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
            std::string cmd = _inputBuffer;
            _inputBuffer.clear();
            _historyRingIdx = -1;
            if (_scrollOffset > 0) {
                _scrollOffset = 0;
                renderMiddlePanel();
            }
            redrawInputLine();
            if (!cmd.empty()) {
                processCommand(cmd);
            }
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8 || ch == '\b') {
            if (!_inputBuffer.empty()) {
                _inputBuffer.pop_back();
                redrawInputLine();
            }
        } else if (ch >= 32 && ch <= 126) {
            _inputBuffer += static_cast<char>(ch);
            redrawInputLine();
        }
    }
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
