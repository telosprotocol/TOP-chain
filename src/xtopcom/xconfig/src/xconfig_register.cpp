// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <memory>
#include <algorithm>
#include <iostream>

#include "xbase/xobject_ptr.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xconfig_to_string_helper.h"
#include "xconfig/xpredefined_configurations.h"

#include "xpbase/base/check_cast.h"

NS_BEG2(top, config)

void xconfig_register_t::add_listener(xconfig_register_listener_ptr_t l) {
    std::lock_guard<std::mutex> lock{m_listener_lock};

    auto it = std::find(m_listeners.begin(), m_listeners.end(), l);
    if (it == m_listeners.end()) {
        m_listeners.push_back(l);
    }
}

void xconfig_register_t::remove_listener(xconfig_register_listener_ptr_t l) {
    std::lock_guard<std::mutex> lock{m_listener_lock};

    auto it = std::find(m_listeners.begin(), m_listeners.end(), l);
    if (it != m_listeners.end()) {
        m_listeners.erase(it);
    }
}

void xconfig_register_t::clear_listeners() {
    std::lock_guard<std::mutex> lock{m_listener_lock};
    m_listeners.clear();
}

void xconfig_register_t::add_loader(xconfig_loader_ptr_t loader) {
    {
        std::lock_guard<std::mutex> lock{m_loader_lock};

        auto it = std::find(m_loaders.begin(), m_loaders.end(), loader);
        if (it == m_loaders.end()) {
            m_loaders.push_back(loader);
        }
    }
    loader->start();
}

void xconfig_register_t::remove_loader(xconfig_loader_ptr_t loader) {
    std::lock_guard<std::mutex> lock{m_loader_lock};

    auto it = std::find(m_loaders.begin(), m_loaders.end(), loader);
    if (it != m_loaders.end()) {
        (*it)->stop();
        m_loaders.erase(it);
    }
}

void xconfig_register_t::clear_loaders() {
    std::lock_guard<std::mutex> lock{m_loader_lock};
    for (const auto& loader : m_loaders) {
        loader->stop();
    }
    m_loaders.clear();
}

#define XADD_OFFCHAIN_PARAMETER(NAME)                                                                                                                                              \
    m_params_map.insert({top::config::x##NAME##_configuration_t::name,                                                                                                             \
                         top::config::xto_string_helper_t<top::config::x##NAME##_configuration_t::type>::to_string(top::config::x##NAME##_configuration_t::value)})

void xconfig_register_t::init_static_config() {
    m_param_lock.lock_write();

    XADD_OFFCHAIN_PARAMETER(zone_count);
    XADD_OFFCHAIN_PARAMETER(cluster_count);

    XADD_OFFCHAIN_PARAMETER(executor_max_total_sessions_service_counts);
    XADD_OFFCHAIN_PARAMETER(executor_max_session_service_counts);
    XADD_OFFCHAIN_PARAMETER(executor_session_time_interval);
    XADD_OFFCHAIN_PARAMETER(executor_max_sessions);

    XADD_OFFCHAIN_PARAMETER(grpc_port);
    XADD_OFFCHAIN_PARAMETER(dht_port);
    XADD_OFFCHAIN_PARAMETER(msg_port);
    XADD_OFFCHAIN_PARAMETER(http_port);
    XADD_OFFCHAIN_PARAMETER(ws_port);
    XADD_OFFCHAIN_PARAMETER(network_id);
    XADD_OFFCHAIN_PARAMETER(log_level);
    XADD_OFFCHAIN_PARAMETER(log_path);
    XADD_OFFCHAIN_PARAMETER(db_path);
    XADD_OFFCHAIN_PARAMETER(ip);
    XADD_OFFCHAIN_PARAMETER(root_hash);

    XADD_OFFCHAIN_PARAMETER(platform_business_port);
    XADD_OFFCHAIN_PARAMETER(platform_public_endpoints);
    XADD_OFFCHAIN_PARAMETER(platform_url_endpoints);
    XADD_OFFCHAIN_PARAMETER(platform_show_cmd);
    XADD_OFFCHAIN_PARAMETER(platform_db_path);

    XADD_OFFCHAIN_PARAMETER(min_account_deposit);
    XADD_OFFCHAIN_PARAMETER(account_send_queue_tx_max_num);
    XADD_OFFCHAIN_PARAMETER(recv_tx_cache_window);
    XADD_OFFCHAIN_PARAMETER(config_property_alias_name_max_len);
    XADD_OFFCHAIN_PARAMETER(edge_max_msg_packet_size);
    XADD_OFFCHAIN_PARAMETER(chain_name);
    XADD_OFFCHAIN_PARAMETER(root_hash);
    XADD_OFFCHAIN_PARAMETER(leader_election_round);
    XADD_OFFCHAIN_PARAMETER(unitblock_confirm_tx_batch_num);
    XADD_OFFCHAIN_PARAMETER(unitblock_recv_transfer_tx_batch_num);
    XADD_OFFCHAIN_PARAMETER(unitblock_send_transfer_tx_batch_num);
    XADD_OFFCHAIN_PARAMETER(tableblock_batch_unitblock_max_num);
    XADD_OFFCHAIN_PARAMETER(tableblock_batch_tx_max_num);
    XADD_OFFCHAIN_PARAMETER(fulltable_interval_block_num);
    XADD_OFFCHAIN_PARAMETER(local_blacklist);
    XADD_OFFCHAIN_PARAMETER(local_whitelist);

    XADD_OFFCHAIN_PARAMETER(slash_fulltable_interval);
    XADD_OFFCHAIN_PARAMETER(slash_table_split_num);

    m_param_lock.release_write();
}

#undef XADD_OFFCHAIN_PARAMETER

bool xconfig_register_t::load() {
    m_param_lock.lock_write();

    for (auto& l : m_loaders) {
        if (!l->fetch_all(m_params_map)) {
            m_param_lock.release_write();
            // not allowed adding new parameter to config center through proposal
            return false;
        }
    }
    m_param_lock.release_write();
    return true;
}

void xconfig_register_t::dump() {
    std::cout << "=======config dump start======" << std::endl;
    m_param_lock.lock_read();
    for (const auto& entry : m_params_map) {
        std::cout << entry.first << " : " << entry.second << std::endl;
    }
    m_param_lock.release_read();
    std::cout << "=======config dump end======" << std::endl;
}

 void xconfig_register_t::log_dump() const {
    xinfo("xconfig_register_t::log_dump");
    xinfo("=======================current load config dump start=============================");
    m_param_lock.lock_read();
    for (const auto& entry : m_params_map) {
        xinfo("%s : %s", entry.first.c_str(), entry.second.c_str());
    }
    m_param_lock.release_read();
    xinfo("=======================current load config dump end=============================");
 }

size_t xconfig_register_t::size() {
    m_param_lock.lock_read();
    size_t s = m_params_map.size();
    m_param_lock.release_read();
    return s;
}

void xconfig_register_t::update_params(const std::map<std::string, std::string>& map) {
    std::map<std::string, std::string> filterd_map;

    filter_changes(map, filterd_map);
    update_cache_and_persist(filterd_map);
}

void xconfig_register_t::add_delete_params(const std::map<std::string, std::string>& content_map, bool add) {

    if (!content_map.empty()) {
        m_param_lock.lock_write();
        if (add) {
            for (auto& entry : content_map) {
                xinfo("[xconfig_register_t::add_delete_params] add target: %s, value: %s", entry.first.c_str(), entry.second.c_str());
                m_params_map[entry.first] = entry.second;
            }
        } else {
            for (auto& entry : content_map) {
                xinfo("[xconfig_register_t::add_delete_params] delete target: %s, value: %s", entry.first.c_str(), entry.second.c_str());
                m_params_map.erase(entry.first);
            }
        }
        m_param_lock.release_write();

        // save changes
        for (auto& l : m_loaders) {
            l->save_conf(m_params_map);
        }
    }

}

template<>
bool xconfig_register_t::set<std::string>(const std::string& key, std::string value) {
    m_param_lock.lock_write();
    auto iter = m_params_map.find(key);
    if (iter == m_params_map.end()) {
        auto ins_iter = m_params_map.insert({key, std::move(value)});
        if (!ins_iter.second) {
            m_param_lock.release_write();
            return false;
        }
    } else {
        iter->second = std::move(value);
    }
    m_param_lock.release_write();
    return true;
}

template<>
bool xconfig_register_t::set(const std::string& key, bool value) {
    std::string val = check_cast<std::string>(value);
    return set<std::string>(key, val);
}

template<>
bool xconfig_register_t::set(const std::string& key, int16_t value) {
    std::string val = check_cast<std::string>(value);
    return set<std::string>(key, val);
}

template<>
bool xconfig_register_t::set(const std::string& key, uint16_t value) {
    std::string val = check_cast<std::string>(value);
    return set<std::string>(key, val);
}

template<>
bool xconfig_register_t::set(const std::string& key, int32_t value) {
    std::string val = check_cast<std::string>(value);
    return set<std::string>(key, val);
}

template<>
bool xconfig_register_t::set(const std::string& key, uint32_t value) {
    std::string val = check_cast<std::string>(value);
    return set<std::string>(key, val);
}

template<>
bool xconfig_register_t::set(const std::string& key, int64_t value) {
    std::string val = check_cast<std::string>(value);
    return set<std::string>(key, val);
}

template<>
bool xconfig_register_t::set(const std::string& key, uint64_t value) {
    std::string val = check_cast<std::string>(value);
    return set<std::string>(key, val);
}

template<>
bool xconfig_register_t::set(const std::string& key, float value) {
    std::string val = check_cast<std::string>(value);
    return set<std::string>(key, val);
}

template<>
bool xconfig_register_t::set(const std::string& key, double value) {
    std::string val = check_cast<std::string>(value);
    return set<std::string>(key, val);
}

template<>
bool xconfig_register_t::get<std::string>(const std::string& key, std::string& value) const {
    m_param_lock.lock_read();
    auto it = m_params_map.find(key);
    if (it != m_params_map.end()) {
        value = it->second;
        m_param_lock.release_read();
        return true;
    }
    m_param_lock.release_read();
    return false;
}

template<>
bool xconfig_register_t::get<bool>(const std::string& key, bool& value)  const {
    std::string str_value;
    if (!get<std::string>(key, str_value)) {
        return false;
    }

    if (str_value == "1" || str_value == "true") {
        value = true;
        return true;
    }

    if (str_value == "0" || str_value == "false") {
        value = false;
        return true;
    }

    return false;
}

template<>
bool xconfig_register_t::get(const std::string& key, int16_t& value) const {
    std::string str_value;
    if (!get<std::string>(key, str_value)) {
        return false;
    }

    try {
        value = check_cast<int16_t>(str_value.c_str());
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

template<>
bool xconfig_register_t::get(const std::string& key, uint16_t& value) const {
    std::string str_value;
    if (!get<std::string>(key, str_value)) {
        return false;
    }

    try {
        value = check_cast<uint16_t>(str_value.c_str());
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

template<>
bool xconfig_register_t::get(const std::string& key, int32_t& value) const {
    std::string str_value;
    if (!get<std::string>(key, str_value)) {
        return false;
    }

    try {
        value = check_cast<int32_t>(str_value.c_str());
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

template<>
bool xconfig_register_t::get(const std::string& key, uint32_t& value) const {
    std::string str_value;
    if (!get<std::string>(key, str_value)) {
        return false;
    }

    try {
        value = check_cast<uint32_t>(str_value.c_str());
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

template<>
bool xconfig_register_t::get(const std::string& key, int64_t& value) const {
    std::string str_value;
    if (!get<std::string>(key, str_value)) {
        return false;
    }

    try {
        value = check_cast<int64_t>(str_value.c_str());
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

template<>
bool xconfig_register_t::get(const std::string& key, uint64_t& value) const {
    std::string str_value;
    if (!get<std::string>(key, str_value)) {
        return false;
    }

    try {
        value = check_cast<uint64_t>(str_value.c_str());
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

template<>
bool xconfig_register_t::get(const std::string& key, float& value) const {
    std::string str_value;
    if (!get<std::string>(key, str_value)) {
        return false;
    }

    try {
        value = check_cast<float>(str_value.c_str());
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

template<>
bool xconfig_register_t::get(const std::string& key, double& value) const {
    std::string str_value;
    if (!get<std::string>(key, str_value)) {
        return false;
    }

    try {
        value = check_cast<double>(str_value.c_str());
        return true;
    } catch (...) {
        return false;
    }
    return false;
}

void xconfig_register_t::filter_changes(const std::map<std::string, std::string>& map,
        std::map<std::string, std::string>& filterd_map) {
    for (auto& entry : map) {
        if (!entry.first.empty() && !entry.second.empty()) {
            if (is_param_changed(entry.first, entry.second)) {
                filterd_map[entry.first] = entry.second;
            }
        }
    }
}

void xconfig_register_t::update_cache_and_persist(const std::map<std::string, std::string>& filterd_map) {
    if (!filterd_map.empty()) {
        m_param_lock.lock_write();
        for (auto& entry : filterd_map) {
            xinfo("[CONFIG] in update_cache_and_persist, first: %s, second: %s, original second: %s", entry.first.c_str(), entry.second.c_str(), m_params_map[entry.first].c_str());
            m_params_map[entry.first] = entry.second;
        }
        m_param_lock.release_write();

        // save changes
        for (auto& l : m_loaders) {
            l->save_conf(m_params_map);
        }
    }
}

bool xconfig_register_t::is_param_changed(const std::string& key, const std::string& value) {
    m_param_lock.lock_read();
    auto it = m_params_map.find(key);
    if (it == m_params_map.end()) {
        m_param_lock.release_read();
        return true;
    }
    m_param_lock.release_read();
    return it->second != value;
}

xconfig_register_t& xconfig_register_t::get_instance() {
    static xconfig_register_t inst;
    return inst;
}

// uint32_t get_receive_tx_cache_time() {
//     uint32_t receive_tx_cache_time = xreceive_tx_cache_time_s_configuration_t::value;
//     if (!config_register.get<uint32_t>(xreceive_tx_cache_time_s_configuration_t::name, receive_tx_cache_time)) {
//         assert(0);
//     }

//     return receive_tx_cache_time;

// }

NS_END2
