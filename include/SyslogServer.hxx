/*
 * SyslogServer.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_SYSLOGSERVER_HXX
#define NETMON_SYSLOGSERVER_HXX

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include "SnmpDatabase.hxx"

class SyslogParser {
public:
    static constexpr size_t MAX_DATAGRAM_SIZE = 2048;

    static bool parse(const char *data, size_t len, SyslogEvent &out);
    static bool parse(const std::string &raw, SyslogEvent &out);

private:
    static int64_t parseRfc3164Timestamp(const std::string &tsStr);
    static int64_t parseIsoTimestamp(const std::string &tsStr);
};

class SyslogServer {
public:
    static SyslogServer &getInstance();

    bool start(const std::string &bindAddress = "0.0.0.0", int port = 1514);
    void stop();
    void join();
    bool isRunning() const;

    void setRouterAddress(const std::string &ip);
    std::string getRouterAddress() const;

    int getPort() const;

    void resetForTesting();

private:
    SyslogServer();
    ~SyslogServer();
    SyslogServer(const SyslogServer &) = delete;
    SyslogServer &operator=(const SyslogServer &) = delete;

    void runLoop();

    std::atomic<bool> _running;
    int               _sockfd;
    int               _port;
    std::string       _bindAddress;
    std::string       _routerAddress;
    mutable std::mutex _mutex;
    std::thread       _thread;
};

#endif /* NETMON_SYSLOGSERVER_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
