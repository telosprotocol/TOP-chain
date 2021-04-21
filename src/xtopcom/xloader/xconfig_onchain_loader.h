// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xconfig/xconfig_face.h"
#include "xstore/xstore_face.h"
#include "xmbus/xmessage_bus.h"

#include "xchain_timer/xchain_timer.h"
#include "xdata/xproposal_data.h"
#include "xmbus/xbase_sync_event_monitor.hpp"


NS_BEG2(top, loader)

class xconfig_onchain_loader_t : public config::xconfig_loader_face_t
                               , public std::enable_shared_from_this<xconfig_onchain_loader_t> {
public:
    xconfig_onchain_loader_t(observer_ptr<store::xstore_face_t> const & store_ptr,
                             observer_ptr<mbus::xmessage_bus_face_t> const & bus,
                             observer_ptr<time::xchain_time_face_t> const & logic_timer);

    void start() override;
    void stop() override;

    virtual bool save_conf(const std::map<std::string, std::string>& map) override;
    virtual bool fetch_all(std::map<std::string, std::string>& map) override;

private:
    void update(mbus::xevent_ptr_t e);
    void chain_timer(common::xlogic_time_t time);
    config::xconfig_update_action_ptr_t find(const std::string& type);

private:
    class xconfig_bus_monitor : public mbus::xbase_sync_event_monitor_t {
    public:
        xconfig_bus_monitor(xconfig_onchain_loader_t* parent);
        virtual ~xconfig_bus_monitor();
        void init();
        void uninit();
        bool filter_event(const mbus::xevent_ptr_t & e) override;
        void process_event(const mbus::xevent_ptr_t & e) override;
    private:
        xconfig_onchain_loader_t * m_parent;
    };

private:
    std::mutex m_action_param_mutex;
    std::map<std::string, config::xconfig_update_action_ptr_t> m_action_map{};

    observer_ptr<store::xstore_face_t> m_store_ptr{nullptr};
    observer_ptr<mbus::xmessage_bus_face_t> m_bus{nullptr};
    observer_ptr<time::xchain_time_face_t> m_logic_timer{nullptr};

    std::multimap<uint64_t, tcc::proposal_info> m_pending_proposed_parameters{};

    uint32_t m_db_id;
    xconfig_bus_monitor * m_monitor;
};

NS_END2
