/*
 * ZyshSimulator.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_ZYSIMULATOR_HXX
#define NETMON_ZYSIMULATOR_HXX

#include "ZyxelSshClient.hxx"

#include <map>
#include <string>
#include <vector>

class ZyshSimulator {
public:
    ZyshSimulator();

    ZyshTransport transport();

    void setHostKey(const std::string &sha256Hex);
    void setNextHostKey(const std::string &sha256Hex);
    void setStall(bool stall);
    void setEofOnNextRead(bool enable);
    void setReadErrorOnNext(bool enable);
    void setKeepaliveFail(bool fail);
    void armLateOutput(const std::string &marker);
    void failOnOccurrence(const std::string &line, int occurrence, const std::string &errorText);
    void seedRule(const std::string &name, const std::string &source,
                  const std::string &action, bool active);
    void seedAddress(const std::string &name);

    const std::vector<std::string> &lines() const;
    int openCount() const;
    int closeCount() const;
    bool lateDelivered() const;
    const std::string &runningConfig() const;

private:
    enum class Mode {
        USER,
        ROOT,
        CONFIG,
        POLICY,
        CONFIG_ROUTER,
        OBJECT
    };

    struct Rule {
        std::string name;
        std::string action;
        std::string source;
        bool active;
    };

    struct PlannedError {
        int remaining = 0;
        std::string text;
    };

    SshResult onOpen(const std::string &host, int port,
                     const std::string &user, const std::string &password,
                     std::string &hostKeySha256Out);
    ssize_t onRead(char *buf, size_t len, bool &eofOut);
    ssize_t onWrite(const char *buf, size_t len);
    bool onKeepalive();
    void onClose();

    void handleLine(const std::string &line);
    void queuePrompt();
    std::string promptText() const;
    std::string nextAutoName();
    void render();

    std::string              _hostKey;
    std::string              _nextHostKey;
    std::string              _hostname;
    std::string              _rx;
    std::string              _tx;
    std::string              _late;
    std::string              _runningConfig;
    std::vector<std::string> _lines;
    std::vector<Rule>        _rules;
    std::vector<std::string> _addressObjects;
    std::vector<std::string> _profileNames;
    std::map<std::string, PlannedError> _errors;
    std::string              _draftName;
    std::string              _draftSource;
    std::string              _draftAction;
    Mode                     _mode;
    Mode                     _modeBeforePolicy;
    bool                     _draft;
    bool                     _draftInactive;
    bool                     _ospf;
    bool                     _stall;
    bool                     _eofNext;
    bool                     _readErrorNext;
    bool                     _keepaliveFail;
    bool                     _lateDelivered;
    bool                     _open;
    int                      _openCount;
    int                      _closeCount;
    int                      _autoName;
};

#endif /* NETMON_ZYSIMULATOR_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
