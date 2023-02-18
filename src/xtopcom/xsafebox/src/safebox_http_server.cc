// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsafebox/safebox_http_server.h"

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xbasic/xtimer_driver.h"  // xbase/xvevent.h:107:6 `extra ;`

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

// nlohmann_json
#include <nlohmann/json.hpp>
using json = nlohmann::json;

NS_BEG2(top, safebox)

// SafeBox:
void SafeBox::expire_account() {
    std::unique_lock<std::mutex> locak(pw_map_mutex_);
    for (auto iter = pw_map_.begin(); iter != pw_map_.end();) {
        // auto const & account = iter->first;
        auto const & box = iter->second;
        auto now = std::chrono::steady_clock::now();
        if (box.expired_time != std::chrono::milliseconds::zero() && now - box.t > box.expired_time) {
            iter = pw_map_.erase(iter);
        } else {
            iter++;
        }
    }
}

inline bool SafeBox::getLastestAccount(std::string & account, std::string & private_key) {
    if (lastest_account.empty()) {
        return false;
    }
    account = lastest_account;
    return getAccount(lastest_account, private_key);
}

inline bool SafeBox::getAccount(const std::string & account, std::string & private_key) {
    std::unique_lock<std::mutex> lock(pw_map_mutex_);
    auto ifind = pw_map_.find(account);
    if (ifind == pw_map_.end()) {
        return false;
    }
    auto box = ifind->second;
    private_key = box.private_key;
    return true;
}

inline bool SafeBox::setAccount(const std::string & account, const std::string & private_key, std::chrono::milliseconds expired_time) {
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

// SafeBoxHttpServer:
SafeBoxHttpServer::SafeBoxHttpServer(std::string const & server_ip, uint16_t server_port, std::shared_ptr<top::xbase_timer_driver_t> timer_driver)
  : base_t{server_ip, server_port}, m_local_ip{server_ip}, m_listen_port{server_port} {
}

void SafeBoxHttpServer::Start() {
    register_json_request_callback("/api/safebox", xhttp::xhttp_server_method_type_t::POST, [&](std::string const & json_req_content, std::string & json_resp_content) -> bool {
        json req_json;
        try {
            req_json = json::parse(json_req_content);

            json res_content;

            auto method = req_json["method"].get<std::string>();
            std::string private_key;
            if (method == "get") {
                std::string account;
                bool gstatus = false;
                if (req_json.find("account") != req_json.end()) {
                    // there is an entry with key "account"
                    const std::string tmp_account = req_json["account"].get<std::string>();
                    gstatus = safebox_.getAccount(tmp_account, private_key);
                    account = tmp_account;
                } else {
                    gstatus = safebox_.getLastestAccount(account, private_key);
                }

                if (gstatus) {
                    // success
                    res_content["status"] = "ok";
                    res_content["account"] = account;
                    res_content["private_key"] = private_key;
                    res_content["error"] = "";
                } else {
                    // error
                    res_content["status"] = "fail";
                    res_content["error"] = "not found account in cache or account expired";
                }
            } else if (method == "set") {
                auto account = req_json["account"].get<std::string>();
                private_key = req_json["private_key"].get<std::string>();
                uint32_t expired_time = req_json["expired_time"].get<uint32_t>();
                if (safebox_.setAccount(account, private_key, std::chrono::milliseconds(expired_time))) {
                    res_content["status"] = "ok";
                } else {
                    res_content["status"] = "fail";
                    res_content["error"] = "invalid account or private_key";
                }
            } else {
                // not support
                res_content["status"] = "fail";
                res_content["error"] = "not support method";
            }

            json_resp_content = res_content.dump();  // dump(4)
            return true;

        } catch (...) {
            json res_content;
            res_content["status"] = "fail";
            res_content["error"] = "catch error";

            json_resp_content = res_content.dump();  // dump(4)
            return false;
        }
        return true;
    });

    check_expire_safebox();

    start_server(true);  // blocking here.
}

void SafeBoxHttpServer::check_expire_safebox() {
    if (m_timer_driver == nullptr) {
        return;
    }
    auto self = shared_from_this();
    // every 5 second, try expire safebox.
    m_timer_driver->schedule(std::chrono::seconds(5), [this](std::chrono::milliseconds) {
        safebox_.expire_account();
        check_expire_safebox();
    });
}

NS_END2