// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cassert>
#include <string>
#include <list>
#include <map>

#include "xbase/xlog.h"
#include "xbase/xlock.h"
#include "xconfig/xconfig_face.h"
#include "xconfig/xchain_names.h"

NS_BEG2(top, config)

class xconfig_register_t
{
    xconfig_register_t() = default;
public:
    void add_listener(xconfig_register_listener_ptr_t l);
    void remove_listener(xconfig_register_listener_ptr_t l);
    void clear_listeners();

    void add_loader(xconfig_loader_ptr_t loader);
    void remove_loader(xconfig_loader_ptr_t loader);
    void clear_loaders();

    void init_static_config();

    bool load();
    void dump();
    void log_dump() const;
    size_t size();
    /**
     * Update parameter. Please note that add a new parameter is possible.
     *
     * @param key
     * @param value
     */

    template<typename T, typename std::enable_if<std::is_same<T, std::string>::value
                                            || std::is_same<T, bool>::value
                                            || std::is_same<T, int16_t>::value
                                            || std::is_same<T, uint16_t>::value
                                            || std::is_same<T, int32_t>::value
                                            || std::is_same<T, uint32_t>::value
                                            || std::is_same<T, int64_t>::value
                                            || std::is_same<T, uint64_t>::value
                                            || std::is_same<T, float>::value
                                            || std::is_same<T, double>::value
                                            >::type * = nullptr>
    bool set(const std::string& key, T value);

    template<typename T, typename std::enable_if<std::is_same<T, std::string>::value
                                            || std::is_same<T, bool>::value
                                            || std::is_same<T, int16_t>::value
                                            || std::is_same<T, uint16_t>::value
                                            || std::is_same<T, int32_t>::value
                                            || std::is_same<T, uint32_t>::value
                                            || std::is_same<T, int64_t>::value
                                            || std::is_same<T, uint64_t>::value
                                            || std::is_same<T, float>::value
                                            || std::is_same<T, double>::value
                                            >::type * = nullptr>
    bool get(const std::string& key, T& value) const;

    template <typename T, typename std::enable_if<std::is_same<T, std::string>::value   ||
                                                  std::is_same<T, bool>::value          ||
                                                  std::is_same<T, char>::value          ||
                                                  std::is_same<T, float>::value         ||
                                                  std::is_same<T, double>::value        ||
                                                  std::is_same<T, std::int8_t>::value   ||
                                                  std::is_same<T, std::int16_t>::value  ||
                                                  std::is_same<T, std::int32_t>::value  ||
                                                  std::is_same<T, std::int64_t>::value  ||
                                                  std::is_same<T, std::uint8_t>::value  ||
                                                  std::is_same<T, std::uint16_t>::value ||
                                                  std::is_same<T, std::uint32_t>::value ||
                                                  std::is_same<T, std::uint64_t>::value>::type * = nullptr>
    T value_or(T const default_value, std::string const & key) const {
        T value;
        if (get(key, value)) {
            return value;
        }
        xwarn("[config register] %s read fail, use default value", key.c_str());
        return default_value;
    }

    template <typename T, typename std::enable_if<std::is_same<T, char const *>::value>::type * = nullptr>
    std::string value_or(T const default_value, std::string const & key) const {
        std::string value;
        if (get(key, value)) {
            return value;
        }
        xwarn("[config register] %s read fail, use default value", key.c_str());
        return default_value;
    }

    template <typename ConfigType, typename std::enable_if<std::is_same<typename ConfigType::type, std::string>::value   ||
                                                           std::is_same<typename ConfigType::type, bool>::value          ||
                                                           std::is_same<typename ConfigType::type, char>::value          ||
                                                           std::is_same<typename ConfigType::type, float>::value         ||
                                                           std::is_same<typename ConfigType::type, double>::value        ||
                                                           std::is_same<typename ConfigType::type, std::int8_t>::value   ||
                                                           std::is_same<typename ConfigType::type, std::int16_t>::value  ||
                                                           std::is_same<typename ConfigType::type, std::int32_t>::value  ||
                                                           std::is_same<typename ConfigType::type, std::int64_t>::value  ||
                                                           std::is_same<typename ConfigType::type, std::uint8_t>::value  ||
                                                           std::is_same<typename ConfigType::type, std::uint16_t>::value ||
                                                           std::is_same<typename ConfigType::type, std::uint32_t>::value ||
                                                           std::is_same<typename ConfigType::type, std::uint64_t>::value>::type * = nullptr>
    typename ConfigType::type
    get() const {
        return value_or<typename ConfigType::type>(ConfigType::value, ConfigType::name);
    }

    template <typename ConfigType, typename std::enable_if<std::is_same<typename ConfigType::type, char const *>::value>::type * = nullptr>
    std::string
    get() const {
        return value_or<typename ConfigType::type>(ConfigType::value, ConfigType::name);
    }

    void update_params(const std::map<std::string, std::string>& map);
    void add_delete_params(const std::map<std::string, std::string>& content_map, bool add = true); // default add param
    void update_cache_and_persist(const std::map<std::string, std::string>& filterd_map);

    static xconfig_register_t& get_instance();

private:
    void filter_changes(const std::map<std::string, std::string>& map,
            std::map<std::string, std::string>& filterd_map);
    bool is_param_changed(const std::string& key, const std::string& value);
private:
    std::mutex m_listener_lock {};
    std::list<xconfig_register_listener_ptr_t> m_listeners {};

    std::mutex m_loader_lock {};
    std::list<xconfig_loader_ptr_t> m_loaders {};

    mutable base::xrwlock_t m_param_lock {};
    std::map<std::string, std::string> m_params_map {};
};

static xconfig_register_t& config_register = config::xconfig_register_t::get_instance();

NS_END2

#define XGET_CONFIG(NAME) static_cast<top::config::xconfig_register_t const &>(top::config::xconfig_register_t::get_instance()).get<top::config::x ## NAME ## _configuration_t>()
#define XGET_ONCHAIN_GOVERNANCE_PARAMETER(NAME) static_cast<top::config::xconfig_register_t const &>(top::config::xconfig_register_t::get_instance()).get<top::config::x ## NAME ## _onchain_goverance_parameter_t>()
