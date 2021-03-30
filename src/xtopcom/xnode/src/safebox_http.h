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

namespace safebox {

static const uint32_t kExpirePeriod = 30 * 60 * 1000; // expire  after 30 * 60 s)
const std::string safebox_default_addr = "127.0.0.1";
const uint16_t    safebox_default_port = 7000;

// check string endswith substring(like python str.endswith)
bool endswith(std::string const &fullString, std::string const &ending);
//use to check if a file exists
bool isFileExist (const std::string& name);

class SafeBox {
public:
    SafeBox()                            = default;
    ~SafeBox()                           = default;
    SafeBox(const SafeBox&)              = delete;
    SafeBox & operator=(const SafeBox &) = delete;
    SafeBox(SafeBox &&)                  = delete;
    SafeBox & operator=(SafeBox &&)      = delete;

    struct pw_box {
        std::string account;
        std::string private_key;
        std::chrono::steady_clock::time_point t;
        uint32_t expired_time {kExpirePeriod};
    };

public:
    inline bool getAccount(std::string& account, std::string &private_key) {
        if (pw_sort_.size() == 0) {
            return false;
        }
        const std::string last_account = pw_sort_[pw_sort_.size() - 1];
        account = last_account;
        return getAccount(last_account, private_key);
    }

    inline bool getAccount(const std::string &account, std::string &private_key) {
        std::unique_lock<std::mutex> lock(pw_map_mutex_);
        auto ifind = pw_map_.find(account);
        if (ifind == pw_map_.end()) {
            return false;
        }
        auto box = ifind->second;
        // 0 meaning never expired
        if (box.expired_time != 0) {
            auto now = std::chrono::steady_clock::now();
            if (box.t + std::chrono::milliseconds(kExpirePeriod) < now) {
                // expire
                return false;
            }
        }
        private_key = box.private_key;
        return true;
    }

    inline bool setAccount(const std::string &account, const std::string &private_key, uint32_t expired_time) {
        if (account.empty() || private_key.empty()) {
            return false;
        }
        std::unique_lock<std::mutex> lock(pw_map_mutex_);
        if (pw_map_.size() > 500) {
            pw_map_.clear();
            pw_sort_.clear();
        }
        pw_box box;
        box.account = account;
        box.private_key = private_key;
        box.expired_time = expired_time;
        box.t = std::chrono::steady_clock::now();
        pw_map_[account] = box;
        if (expired_time != 0) {
            // for client
            pw_sort_.push_back(account);
        }
        return true;
    }

private:
    // key is account, value is pw_box
    std::map<std::string, pw_box> pw_map_;
    std::mutex                    pw_map_mutex_;
    std::vector<std::string>      pw_sort_;
};





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
    SafeBox                            safebox_;
};


// cache password of keystore
class SafeboxHttpServer : public std::enable_shared_from_this<SafeboxHttpServer> {
public:
    SafeboxHttpServer(SafeboxHttpServer const &)              = delete;
    SafeboxHttpServer & operator=(SafeboxHttpServer const &)  = delete;
    SafeboxHttpServer(SafeboxHttpServer &&)                   = delete;
    SafeboxHttpServer & operator=(SafeboxHttpServer &&)       = delete;

public:
    SafeboxHttpServer();
    explicit SafeboxHttpServer(uint16_t port);
    SafeboxHttpServer(const std::string& local_ip, uint16_t port);
    SafeboxHttpServer(const std::string& local_ip, uint16_t port, const std::string& webroot);
    virtual  ~SafeboxHttpServer();

    void Start();

protected:
    void bind_route_callback();
    void bind_route_callback_for_command();

private:
    HttpServerPtr                      svr_          { nullptr };
    std::string                        local_ip_     { safebox_default_addr };
    uint16_t                           listen_port_  { safebox_default_port };
    //std::shared_ptr<HttpBaseHandler>   http_handler_ { nullptr };
    std::shared_ptr<HttpHandler>       http_handler_ { nullptr };
    std::thread                        svr_th_;
    std::string                        webroot_      { "./" };
};

// end of http server

} // namespace safebox 

} // namespace top
