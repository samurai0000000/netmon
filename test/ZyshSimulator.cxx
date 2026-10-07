/*
 * ZyshSimulator.cxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#include "ZyshSimulator.hxx"

#include <cstring>
#include <string>

ZyshSimulator::ZyshSimulator()
    : _hostKey("00112233445566778899aabbccddeeff00112233445566778899aabbccddeeff"),
      _hostname("Router"),
      _mode(Mode::ROOT),
      _modeBeforePolicy(Mode::CONFIG),
      _draft(false),
      _draftInactive(false),
      _ospf(false),
      _stall(false),
      _eofNext(false),
      _readErrorNext(false),
      _keepaliveFail(false),
      _lateDelivered(false),
      _open(false),
      _openCount(0),
      _closeCount(0),
      _autoName(0) {
}

ZyshTransport ZyshSimulator::transport() {
    ZyshTransport t;
    t.open = [this](const std::string &host, int port,
                    const std::string &user, const std::string &password,
                    std::string &hostKeySha256Out) {
        return onOpen(host, port, user, password, hostKeySha256Out);
    };
    t.read = [this](char *buf, size_t len, bool &eofOut) {
        return onRead(buf, len, eofOut);
    };
    t.write = [this](const char *buf, size_t len) {
        return onWrite(buf, len);
    };
    t.keepalive = [this]() {
        return onKeepalive();
    };
    t.close = [this]() {
        onClose();
    };
    return t;
}

void ZyshSimulator::setHostKey(const std::string &sha256Hex) {
    _hostKey = sha256Hex;
}

void ZyshSimulator::setNextHostKey(const std::string &sha256Hex) {
    _nextHostKey = sha256Hex;
}

void ZyshSimulator::setStall(bool stall) {
    _stall = stall;
}

void ZyshSimulator::setEofOnNextRead(bool enable) {
    _eofNext = enable;
}

void ZyshSimulator::setReadErrorOnNext(bool enable) {
    _readErrorNext = enable;
}

void ZyshSimulator::setKeepaliveFail(bool fail) {
    _keepaliveFail = fail;
}

void ZyshSimulator::armLateOutput(const std::string &marker) {
    _late = marker;
    _stall = true;
    _lateDelivered = false;
}

const std::vector<std::string> &ZyshSimulator::lines() const {
    return _lines;
}

int ZyshSimulator::openCount() const {
    return _openCount;
}

int ZyshSimulator::closeCount() const {
    return _closeCount;
}

bool ZyshSimulator::lateDelivered() const {
    return _lateDelivered;
}

const std::string &ZyshSimulator::runningConfig() const {
    return _runningConfig;
}

SshResult ZyshSimulator::onOpen(const std::string &host, int port,
                               const std::string &user, const std::string &password,
                               std::string &hostKeySha256Out) {
    (void)host;
    (void)port;
    (void)user;
    (void)password;
    if (!_nextHostKey.empty()) {
        hostKeySha256Out = _nextHostKey;
        _nextHostKey.clear();
    } else {
        hostKeySha256Out = _hostKey;
    }
    _open = true;
    _openCount++;
    _mode = Mode::ROOT;
    _draft = false;
    _draftInactive = false;
    _rx = promptText();
    return SshResult::SUCCESS;
}

ssize_t ZyshSimulator::onRead(char *buf, size_t len, bool &eofOut) {
    eofOut = false;
    if (_readErrorNext) {
        _readErrorNext = false;
        return -1;
    }
    if (_eofNext) {
        _eofNext = false;
        eofOut = true;
        return 0;
    }
    if (_stall) {
        return 0;
    }
    if (_rx.empty()) {
        return 0;
    }
    size_t n = _rx.size() < len ? _rx.size() : len;
    std::memcpy(buf, _rx.data(), n);
    if (_rx.find("LATE_MARKER") != std::string::npos) {
        _lateDelivered = true;
    }
    _rx.erase(0, n);
    return static_cast<ssize_t>(n);
}

ssize_t ZyshSimulator::onWrite(const char *buf, size_t len) {
    if (buf == nullptr || len == 0) {
        return 0;
    }
    if (!_open) {
        return -1;
    }
    _tx.append(buf, len);
    std::string::size_type nl;
    while ((nl = _tx.find('\n')) != std::string::npos) {
        std::string line = _tx.substr(0, nl);
        _tx.erase(0, nl + 1);
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
            line.pop_back();
        }
        handleLine(line);
    }
    return static_cast<ssize_t>(len);
}

bool ZyshSimulator::onKeepalive() {
    return !_keepaliveFail;
}

void ZyshSimulator::onClose() {
    _closeCount++;
    _open = false;
    _rx.clear();
    _tx.clear();
    _late.clear();
    _stall = false;
    _eofNext = false;
    _readErrorNext = false;
    _draft = false;
    _draftInactive = false;
    _mode = Mode::ROOT;
}

void ZyshSimulator::failOnOccurrence(const std::string &line, int occurrence, const std::string &errorText) {
    PlannedError planned;
    planned.remaining = occurrence < 1 ? 1 : occurrence;
    planned.text = errorText;
    _errors[line] = planned;
}

void ZyshSimulator::seedRule(const std::string &name, const std::string &source,
                             const std::string &action, bool active) {
    Rule rule;
    rule.name = name;
    rule.source = source;
    rule.action = action;
    rule.active = active;
    _rules.push_back(rule);
    render();
}

void ZyshSimulator::seedAddress(const std::string &name) {
    _addressObjects.push_back(name);
}

void ZyshSimulator::handleLine(const std::string &line) {
    _lines.push_back(line);

    auto err = _errors.find(line);
    if (err != _errors.end()) {
        err->second.remaining--;
        if (err->second.remaining <= 0) {
            std::string text = err->second.text;
            _errors.erase(err);
            _rx.append(text);
            _rx.push_back('\n');
            queuePrompt();
            return;
        }
    }

    if (!line.empty() && line.back() == '?') {
        if (line.find("router ospf") != std::string::npos) {
            _ospf = true;
            _mode = Mode::CONFIG_ROUTER;
        }
        render();
        queuePrompt();
        return;
    }

    if (line == "terminal length 0") {
        queuePrompt();
        return;
    }
    if (line == "configure terminal") {
        _mode = Mode::CONFIG;
        queuePrompt();
        return;
    }
    if (line == "secure-policy insert 1" ||
        (line.compare(0, 21, "secure-policy insert ") == 0)) {
        _modeBeforePolicy = _mode;
        _mode = Mode::POLICY;
        _draft = true;
        _draftInactive = false;
        _draftName.clear();
        _draftSource.clear();
        _draftAction.clear();
        queuePrompt();
        return;
    }
    if (_mode == Mode::POLICY && line.compare(0, 5, "name ") == 0) {
        _draftName = line.substr(5);
        queuePrompt();
        return;
    }
    if (_mode == Mode::POLICY && line.compare(0, 9, "sourceip ") == 0) {
        _draftSource = line.substr(9);
        queuePrompt();
        return;
    }
    if (_mode == Mode::POLICY && line.compare(0, 7, "action ") == 0) {
        _draftAction = line.substr(7);
        queuePrompt();
        return;
    }
    if (_mode == Mode::POLICY && line == "activate") {
        _draftInactive = false;
        queuePrompt();
        return;
    }
    if (_mode == Mode::POLICY && line == "no activate") {
        _draftInactive = true;
        queuePrompt();
        return;
    }
    if (_mode == Mode::POLICY && line == "exit") {
        Rule rule;
        if (!_draftName.empty()) {
            rule.name = _draftName;
        } else if (!_draftInactive) {
            rule.name = nextAutoName();
        } else {
            rule.name = "draft";
        }
        rule.action = _draftAction.empty() ? (_draftInactive ? "deny" : "allow") : _draftAction;
        rule.source = _draftSource.empty() ? "any" : _draftSource;
        rule.active = !_draftInactive;
        _rules.push_back(rule);
        _draft = false;
        _draftInactive = false;
        _mode = (_modeBeforePolicy == Mode::CONFIG) ? Mode::CONFIG : Mode::ROOT;
        render();
        queuePrompt();
        return;
    }
    if (line == "exit") {
        if (_mode == Mode::OBJECT || _mode == Mode::CONFIG_ROUTER) {
            _mode = Mode::CONFIG;
        } else if (_mode == Mode::CONFIG) {
            _mode = Mode::ROOT;
        } else if (_mode == Mode::ROOT) {
            _mode = Mode::USER;
        }
        queuePrompt();
        return;
    }
    if (line.compare(0, 16, "ip ddns profile ") == 0) {
        if (_mode == Mode::CONFIG) {
            std::string name = line.substr(16);
            bool exists = false;
            for (const auto &got : _profileNames) {
                if (got == name) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                _profileNames.push_back(name);
            }
            _mode = Mode::OBJECT;
        }
        queuePrompt();
        return;
    }
    if (line.compare(0, 19, "no ip ddns profile ") == 0) {
        if (_mode == Mode::CONFIG) {
            std::string name = line.substr(19);
            std::vector<std::string> kept;
            for (const auto &got : _profileNames) {
                if (got != name) {
                    kept.push_back(got);
                }
            }
            _profileNames.swap(kept);
        }
        queuePrompt();
        return;
    }
    if (line == "show ddns") {
        for (const auto &name : _profileNames) {
            _rx.append("ddns profile: ");
            _rx.append(name);
            _rx.push_back('\n');
        }
        queuePrompt();
        return;
    }
    if (line == "show secure-policy" || line == "show running-config") {
        render();
        _rx.append(_runningConfig);
        _rx.push_back('\n');
        queuePrompt();
        return;
    }
    if (line == "show version") {
        _rx.append("model ZyWALL\n");
        queuePrompt();
        return;
    }
    if (line.compare(0, 15, "address-object ") == 0) {
        std::string rest = line.substr(15);
        std::string name = rest;
        std::string::size_type sp = rest.find(' ');
        if (sp != std::string::npos) {
            name = rest.substr(0, sp);
        }
        bool exists = false;
        for (const auto &obj : _addressObjects) {
            if (obj == name) {
                exists = true;
                break;
            }
        }
        if (exists) {
            _rx.append("already exists\n");
        } else {
            _addressObjects.push_back(name);
        }
        queuePrompt();
        return;
    }
    if (line.compare(0, 18, "no address-object ") == 0) {
        std::string name = line.substr(18);
        std::vector<std::string> kept;
        for (const auto &obj : _addressObjects) {
            if (obj != name) {
                kept.push_back(obj);
            }
        }
        _addressObjects.swap(kept);
        queuePrompt();
        return;
    }
    if (line.compare(0, 17, "no secure-policy ") == 0 && line.compare(0, 22, "no secure-policy name ") != 0) {
        int pos = 0;
        try {
            pos = std::stoi(line.substr(17));
        } catch (...) {
            pos = 0;
        }
        if (pos >= 1 && static_cast<size_t>(pos) <= _rules.size()) {
            _rules.erase(_rules.begin() + (pos - 1));
        }
        render();
        queuePrompt();
        return;
    }
    if (line == "show address-object") {
        for (const auto &obj : _addressObjects) {
            _rx.append("address-object: ");
            _rx.append(obj);
            _rx.push_back('\n');
        }
        queuePrompt();
        return;
    }

    queuePrompt();
}

void ZyshSimulator::queuePrompt() {
    _rx.append(promptText());
}

std::string ZyshSimulator::promptText() const {
    std::string text = _hostname;
    if (_mode == Mode::CONFIG) {
        text += "(config)";
    } else if (_mode == Mode::POLICY) {
        text += "(secure-policy)";
    } else if (_mode == Mode::CONFIG_ROUTER) {
        text += "(config-router)";
    } else if (_mode == Mode::OBJECT) {
        text += "(config-object)";
    }
    text.push_back(_mode == Mode::USER ? '>' : '#');
    return text;
}

std::string ZyshSimulator::nextAutoName() {
    int n = _autoName++;
    char name[4];
    name[0] = static_cast<char>('A' + ((n / 676) % 26));
    name[1] = static_cast<char>('A' + ((n / 26) % 26));
    name[2] = static_cast<char>('A' + (n % 26));
    name[3] = '\0';
    return std::string("Policy-Control_") + name;
}

void ZyshSimulator::render() {
    std::string out;
    for (size_t i = 0; i < _rules.size(); ++i) {
        out += "secure-policy rule: " + std::to_string(i + 1) + "\n";
        out += "name: " + _rules[i].name + "\n";
        out += "source ip: " + _rules[i].source + "\n";
        out += "action: " + _rules[i].action + "\n";
        out += std::string("status: ") + (_rules[i].active ? "yes" : "no") + "\n";
    }
    if (_ospf) {
        out += "router ospf\n";
    }
    _runningConfig = out;
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
