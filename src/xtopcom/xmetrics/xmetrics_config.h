// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbasic/xthreading/xutility.h"
#include "xmetrics/Variant.h"

#include <chrono>
#include <map>
#include <mutex>
#include <sstream>

NS_BEG2(top, metrics)

class e_metrics_config {
public:
    using config_value = Variant<std::chrono::microseconds, std::size_t, bool>;

    static e_metrics_config & get_instance() {
        static e_metrics_config instance;
        return instance;
    }

    template <typename R>
    void get(std::string config_name, R & value) {
        XLOCK_GUARD(m_config_mutex) {
            if (config_hub.find(config_name) != config_hub.end()) {
                value = config_hub.at(config_name).GetConstRef<R>();
                return;
            } else {
                xkinfo("[METRICS_CONFIG] NOT find config : %s", config_name.c_str());
            }
        }
    }

    template <typename R>
    void set(std::string config_name, R & value) {
        XLOCK_GUARD(m_config_mutex) {
            if (config_hub.find(config_name) != config_hub.end()) {
                config_hub.at(config_name).GetRef<R>() = value;
                return;
            } else {
                xkinfo("[METRICS_CONFIG] NOT find config : %s", config_name.c_str());
            }
        }
    }

private:
    e_metrics_config() {
        config_hub.insert({"dump_interval", (std::size_t)300});
        // config_hub.insert({"default_time_out", std::chrono::seconds{5}}); // todo how to make const variable configuare
        config_hub.insert({"queue_procss_behind_sleep_time", std::chrono::seconds{1}});
        config_hub.insert({"dump_full_unit", (bool)true});
        config_hub.insert({"dump_json_format", (bool)true});
    }
    mutable std::mutex m_config_mutex;
    std::map<std::string, config_value> config_hub;
};
#define XMETRICS_CONFIG_GET(config_name, value) top::metrics::e_metrics_config::get_instance().get(config_name, value);
#define XMETRICS_CONFIG_SET(config_name, value) top::metrics::e_metrics_config::get_instance().set(config_name, value);

NS_END2