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

    static bool isReservedAdminPort(int port);

    bool start(const std::string &bindAddress = "0.0.0.0",
               int port = 3884,
               int adminPort = 3886);
    void stop();
    void join();
    bool isRunning() const;
    int  getPort() const;
    int  getAdminPort() const;

    void setRouterDriver(std::shared_ptr<class RouterDriver> driver);

private:
    WebServer();
    ~WebServer();
    WebServer(const WebServer &) = delete;
    WebServer &operator=(const WebServer &) = delete;

    void setupDashboardRoutes();
    void setupAdminRoutes();
    void runDashboard();
    void runAdmin();

    std::string createUiSession();
    bool isValidUiSession(const httplib::Request &req) const;
    static std::string getMcpHintForPath(const std::string &path, const std::string &body);

    bool isValidAdminSession(const httplib::Request &req, std::string &outToken) const;

    std::string                      _bindAddress;
    int                              _port;
    std::string                      _adminBindAddress;
    int                              _adminPort;
    bool                             _endpointsEnabled;
    std::atomic<bool>                _running;

    std::unique_ptr<httplib::Server> _server;
    std::thread                      _thread;

    std::unique_ptr<httplib::Server> _adminServer;
    std::thread                      _adminThread;

    mutable std::mutex               _sessionMutex;
    std::map<std::string, time_t>    _uiSessions;

    std::shared_ptr<class RouterDriver> _routerDriver;
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
