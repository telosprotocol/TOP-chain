// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xtimer_driver_fwd.h"
#include "xhttp/xhttp_server_base.h"

#include <chrono>
#include <map>
#include <mutex>
#include <string>

NS_BEG2(top, safebox)

static const uint32_t DefaultExpirePeriod = 30 * 60 * 1000;  // expire  after 30 * 60 s)
static const std::string safebox_default_addr = "127.0.0.1";
static const uint16_t safebox_default_port = 7000;

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
    void expire_account();

    inline bool getLastestAccount(std::string & account, std::string & private_key);

    inline bool getAccount(const std::string & account, std::string & private_key);

    inline bool setAccount(const std::string & account, const std::string & private_key, std::chrono::milliseconds expired_time);

private:
    // key is account, value is pw_box
    std::map<std::string, pw_box> pw_map_;
    std::mutex pw_map_mutex_;

    std::string lastest_account;
};

class SafeBoxHttpServer : public xhttp::xhttp_server_base_t {
private:
    using base_t = xhttp::xhttp_server_base_t;

    std::string m_local_ip{safebox_default_addr};
    uint16_t m_listen_port{safebox_default_port};
    std::shared_ptr<top::xbase_timer_driver_t> m_timer_driver;
    SafeBox safebox_;

public:
    SafeBoxHttpServer(std::string const & server_ip, uint16_t server_port, std::shared_ptr<top::xbase_timer_driver_t> timer_driver);

    void Start();

private:
    void check_expire_safebox();
};

NS_END2