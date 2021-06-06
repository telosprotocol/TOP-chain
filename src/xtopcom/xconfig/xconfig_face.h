// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "xbase/xns_macro.h"

NS_BEG2(top, config)

/**
 * configure register update listener.
 */
class xconfig_register_listener_face_t
{
    public:
        virtual ~xconfig_register_listener_face_t() {}
        /**
         * Notify config updated. <b>NOTE:</b> consider listener removing
         * request in some cases, we use <b>RETURN</b> value to point out
         * if the listener need remove from xconfig_register_t when it is
         * <b>BE CALLING</b>. since xconfig_register_t.update_params() is
         * locked when it is called, and also xconfig_register_t.remove_listener()
         * also need same lock. so when a listener is called, it shouldn't
         * call remove_listener() to unregister itself. the second reason is
         * that directly remove listener from listener list will also break
         * the iteration used by update_params(). the better way is that
         * if a listener wants to unregister itself when it is called, it
         * should return <b>TRUE</b>, update_params() will record this case,
         * and after all listeners are called, it will remove this listener
         * from list.
         *
         * @param map the map contains all changes. listener should enumerate this
         * map, and pick any item it has interesting
         * @return true if the listener want to remove itself from listener list;
         * otherwise false
         */
        virtual bool config_updated(const std::map<std::string, std::string>& map) = 0;
};

using xconfig_register_listener_ptr_t = xconfig_register_listener_face_t*;

class xconfig_update_action_face_t
{
public:
    xconfig_update_action_face_t() = default;
    xconfig_update_action_face_t(xconfig_update_action_face_t const &) = delete;
    xconfig_update_action_face_t & operator=(xconfig_update_action_face_t const &) = delete;
    xconfig_update_action_face_t(xconfig_update_action_face_t &&) = default;
    xconfig_update_action_face_t & operator=(xconfig_update_action_face_t &&) = delete;
    virtual ~xconfig_update_action_face_t() = default;

    virtual bool do_update(const std::map<std::string, std::string>& params) = 0;
};

using xconfig_update_action_ptr_t = std::shared_ptr<xconfig_update_action_face_t>;

enum xconfig_update_action_type_t {
    update_action_none,      // none
    update_action_parameter, // parameter update
    update_action_plugin,    // plug-in update (to be implemented)
    update_action_global,    // global update (to be implemented)
    update_action_max
};

class xconfig_loader_face_t
{
public:
    virtual ~xconfig_loader_face_t() {}
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual bool save_conf(const std::map<std::string, std::string>& map) = 0;
    virtual bool fetch_all(std::map<std::string, std::string>& map) = 0;
};

using xconfig_loader_ptr_t = std::shared_ptr<xconfig_loader_face_t>;

NS_END2
