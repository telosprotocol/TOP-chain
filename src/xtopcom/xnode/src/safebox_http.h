#pragma once

#include <memory>
#include <functional>
#include <iostream>

#include "simplewebserver/server_http.hpp"
#include "simplewebserver/client_http.hpp"

#include "xbasic/xtimer_driver_fwd.h"

// nlohmann_json
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace  top {

using HttpServer    = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpServerPtr = std::shared_ptr<HttpServer>;
using ResponsePtr   = std::shared_ptr<HttpServer::Response>;
using RequestPtr    = std::shared_ptr<HttpServer::Request>;

namespace safebox {

static const uint32_t DefaultExpirePeriod = 30 * 60 * 1000; // expire  after 30 * 60 s)
const std::string safebox_default_addr = "127.0.0.1";
const uint16_t    safebox_default_port = 7000;

// check string endswith substring(like python str.endswith)
bool endswith(std::string const &fullString, std::string const &ending);
//use to check if a file exists
bool isFileExist (const std::string& name);

class SafeBox {
public:
    SafeBox() = default;
    ~SafeBox() = default;
    SafeBox(const SafeBox &) = delete;
    SafeBox & operator=(const SafeBox &) = delete;
    SafeBox(SafeBox &&) = delete;
    SafeBox & operator=(SafeBox &&) = delete;

    struct pw_box {
        std::string account;
        std::string private_key;
        std::chrono::steady_clock::time_point t;
        std::chrono::milliseconds expired_time{DefaultExpirePeriod};
    };

public:
    void expire_account() {
        std::unique_lock<std::mutex> locak(pw_map_mutex_);
        for (auto iter = pw_map_.begin(); iter != pw_map_.end();) {
            auto const & account = iter->first;
            auto const & box = iter->second;
            auto now = std::chrono::steady_clock::now();
            if (box.expired_time != std::chrono::milliseconds::zero() && now - box.t > box.expired_time) {
                iter = pw_map_.erase(iter);
            } else {
                iter++;
            }
        }
    }

    inline bool getLastestAccount(std::string & account, std::string & private_key) {
        if (lastest_account.empty()) {
            return false;
        }
        account = lastest_account;
        return getAccount(lastest_account, private_key);
    }

    inline bool getAccount(const std::string & account, std::string & private_key) {
        std::unique_lock<std::mutex> lock(pw_map_mutex_);
        auto ifind = pw_map_.find(account);
        if (ifind == pw_map_.end()) {
            return false;
        }
        auto box = ifind->second;
        private_key = box.private_key;
        return true;
    }

    inline bool setAccount(const std::string & account, const std::string & private_key, std::chrono::milliseconds expired_time) {
        if (account.empty() || private_key.empty()) {
            return false;
        }
        std::unique_lock<std::mutex> lock(pw_map_mutex_);
        pw_box box;
        box.account = account;
        box.private_key = private_key;
        box.expired_time = expired_time;
        box.t = std::chrono::steady_clock::now();
        pw_map_[account] = box;
        if (expired_time != std::chrono::milliseconds::zero()) {
            // for client
            lastest_account = account;
        }
        return true;
    }

private:
    // key is account, value is pw_box
    std::map<std::string, pw_box> pw_map_;
    std::mutex pw_map_mutex_;

    std::string lastest_account;
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

    explicit HttpHandler();

    void expire_safebox() {
        safebox_.expire_account();
    }

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
    SafeBox safebox_;
};


// cache password of keystore
class SafeboxHttpServer : public std::enable_shared_from_this<SafeboxHttpServer> {
public:
    SafeboxHttpServer(SafeboxHttpServer const &)              = delete;
    SafeboxHttpServer & operator=(SafeboxHttpServer const &)  = delete;
    SafeboxHttpServer(SafeboxHttpServer &&)                   = delete;
    SafeboxHttpServer & operator=(SafeboxHttpServer &&)       = delete;

public:
    SafeboxHttpServer(std::string const & local_ip, uint16_t port, std::shared_ptr<top::xbase_timer_driver_t> timer_driver);

    virtual  ~SafeboxHttpServer();

    void Start();

protected:
    void bind_route_callback();
    void bind_route_callback_for_command();
    void check_expire_safebox();

private:
    HttpServerPtr                      svr_          { nullptr };
    std::string                        local_ip_     { safebox_default_addr };
    uint16_t                           listen_port_  { safebox_default_port };
    //std::shared_ptr<HttpBaseHandler>   http_handler_ { nullptr };
    std::shared_ptr<HttpHandler>       http_handler_ { nullptr };
    std::thread                        svr_th_;
    std::string                        webroot_      { "./" };
    std::shared_ptr<top::xbase_timer_driver_t> m_timer_driver;
};

// end of http server

} // namespace safebox 

} // namespace top
