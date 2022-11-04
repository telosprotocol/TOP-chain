// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xconfig/xconfig_face.h"

#include "xmbus/xmessage_bus.h"

#include "xchain_timer/xchain_timer.h"
#include "xdata/xproposal_data.h"
#include "xmbus/xbase_sync_event_monitor.hpp"


NS_BEG2(top, loader)

class xconfig_onchain_loader_t : public config::xconfig_loader_face_t
                               , public std::enable_shared_from_this<xconfig_onchain_loader_t> {
public:
    xconfig_onchain_loader_t(observer_ptr<mbus::xmessage_bus_face_t> const & bus,
                             observer_ptr<time::xchain_time_face_t> const & logic_timer);

    void start() override;
    void stop() override;

    virtual bool save_conf(const std::map<std::string, std::string>& map) override;
    virtual bool fetch_all(std::map<std::string, std::string>& map) override;

private:
    void chain_timer(common::xlogic_time_t time);
    void update_onchain_param(common::xlogic_time_t time);
    void filter_changes(const std::map<std::string, std::string>& map,
            std::map<std::string, std::string>& filterd_map) const;
    void get_deleted_params(std::map<std::string, std::string> const& map, std::map<std::string, std::string>& deleted_map) const;
    bool is_param_changed(const std::string& key, const std::string& value) const;
    bool onchain_param_changed(std::map<std::string, std::string> const& params);
    config::xconfig_update_action_ptr_t find(const std::string& type);

private:
    std::mutex m_action_param_mutex;
    std::map<std::string, config::xconfig_update_action_ptr_t> m_action_map{};

    observer_ptr<mbus::xmessage_bus_face_t> m_bus{nullptr};
    observer_ptr<time::xchain_time_face_t> m_logic_timer{nullptr};
    uint64_t m_last_update_height{0};
    std::map<std::string, std::string> m_last_param_map;

    std::multimap<uint64_t, tcc::proposal_info> m_pending_proposed_parameters{};

};

NS_END2
