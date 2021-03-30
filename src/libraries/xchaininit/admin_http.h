#pragma once

#include <memory>
#include <functional>
#include <iostream>

#include "simplewebserver/server_http.hpp"
#include "simplewebserver/client_http.hpp"

// nlohmann_json
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace  top {

using HttpServer    = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpServerPtr = std::shared_ptr<HttpServer>;
using ResponsePtr   = std::shared_ptr<HttpServer::Response>;
using RequestPtr    = std::shared_ptr<HttpServer::Request>;

namespace admin {

// check string endswith substring(like python str.endswith)
bool endswith(std::string const &fullString, std::string const &ending);
//use to check if a file exists
bool isFileExist (const std::string& name);



class HttpBaseHandler {
public:
    virtual bool index(ResponsePtr res, RequestPtr req)              = 0;
    virtual bool headers(ResponsePtr res, RequestPtr req)            = 0;
    virtual bool get(ResponsePtr res, RequestPtr req)                = 0;
    virtual bool post(ResponsePtr res, RequestPtr req)               = 0;
    virtual bool response_headers(ResponsePtr res, RequestPtr req)   = 0;
    virtual bool cookies(ResponsePtr res, RequestPtr req)            = 0;
    virtual bool default_not_found(ResponsePtr res, RequestPtr req)  = 0;
    virtual bool default_error(ResponsePtr res, RequestPtr req, const SimpleWeb::error_code & ec)      = 0;
    virtual bool webroot(ResponsePtr res, RequestPtr req)            = 0;
};

class HttpHandler : public HttpBaseHandler {
public:
    HttpHandler(HttpHandler const &)              = delete;
    HttpHandler& operator=(HttpHandler const &)   = delete;
    HttpHandler(HttpHandler &&)                   = delete;
    HttpHandler& operator=(HttpHandler &&)        = delete;
    virtual ~HttpHandler()                        = default;

    HttpHandler();
    explicit HttpHandler(const std::string& webroot);

public:
    bool index(ResponsePtr res, RequestPtr req)              override;
    bool headers(ResponsePtr res, RequestPtr req)            override;
    bool get(ResponsePtr res, RequestPtr req)                override;
    bool post(ResponsePtr res, RequestPtr req)               override;
    bool response_headers(ResponsePtr res, RequestPtr req)   override;
    bool cookies(ResponsePtr res, RequestPtr req)            override;
    bool default_not_found(ResponsePtr res, RequestPtr req)  override;
    bool default_error(ResponsePtr res, RequestPtr req, const SimpleWeb::error_code & ec)      override;
    bool webroot(ResponsePtr res, RequestPtr req)            override;

    bool verify_token(ResponsePtr res, RequestPtr req);
    bool handle_command(ResponsePtr res, RequestPtr req);

private:
    std::string webroot_ {"./"};
    std::string token_ { "" };
};


class AdminHttpServer : public std::enable_shared_from_this<AdminHttpServer> {
public:
    AdminHttpServer(AdminHttpServer const &)              = delete;
    AdminHttpServer & operator=(AdminHttpServer const &)  = delete;
    AdminHttpServer(AdminHttpServer &&)                   = delete;
    AdminHttpServer & operator=(AdminHttpServer &&)       = delete;

public:
    AdminHttpServer();
    explicit AdminHttpServer(uint16_t port);
    AdminHttpServer(const std::string& local_ip, uint16_t port);
    AdminHttpServer(const std::string& local_ip, uint16_t port, const std::string& webroot);
    virtual  ~AdminHttpServer();

    void Start();

protected:
    void bind_route_callback();
    void bind_route_callback_for_command();

private:
    HttpServerPtr                      svr_          { nullptr };
    std::string                        local_ip_     { "127.0.0.1" };
    uint16_t                           listen_port_  { 8000 };
    //std::shared_ptr<HttpBaseHandler>   http_handler_ { nullptr };
    std::shared_ptr<HttpHandler>       http_handler_ { nullptr };
    std::thread                        svr_th_;
    std::string                        webroot_      { "./" };
};

// end of http server

} // namespace admin

} // namespace top
