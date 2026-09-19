/*
 * WebServer.hxx
 *
 * Copyright (C) 2026, Charles Chiou
 */

#ifndef NETMON_WEBSERVER_HXX
#define NETMON_WEBSERVER_HXX

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <map>
#include <mutex>

namespace httplib {
class Server;
class Request;
class Response;
}

class WebServer {
public:
    static WebServer &getInstance();

    bool start(const std::string &bindAddress = "0.0.0.0", int port = 3884);
    void stop();
    void join();
    bool isRunning() const;
    int  getPort() const;

private:
    WebServer();
    ~WebServer();
    WebServer(const WebServer &) = delete;
    WebServer &operator=(const WebServer &) = delete;

    void setupRoutes();
    void run();

    std::string createUiSession();
    bool isValidUiSession(const httplib::Request &req) const;
    static std::string getMcpHintForPath(const std::string &path, const std::string &body);

    std::string                      _bindAddress;
    int                              _port;
    bool                             _endpointsEnabled;
    std::atomic<bool>                _running;
    std::unique_ptr<httplib::Server> _server;
    std::thread                      _thread;

    mutable std::mutex               _sessionMutex;
    std::map<std::string, time_t>    _uiSessions;
};

#endif /* NETMON_WEBSERVER_HXX */

/*
 * Local variables:
 * mode: C++
 * c-file-style: "BSD"
 * c-basic-offset: 4
 * tab-width: 4
 * indent-tabs-mode: nil
 * End:
 */
