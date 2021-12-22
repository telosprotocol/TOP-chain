// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbase/xbase.h"
#include "xbase/xdata.h"
#include "xbase/xthread.h"
#include "xvledger/xvblock.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xbasic/xmemory.hpp"
#include "xchain_timer/xchain_timer_face.h"
#include "xcommon/xfadable.h"
#include "xdata/xblock.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xtransaction.h"
#include "xrouter/xrouter.h"
#include "xstore/xstore_face.h"
#include "xtxpool_service_v2/xrequest_tx_receiver_face.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xvnetwork/xvnetwork_driver_face.h"
#include "xmbus/xmessage_bus.h"

NS_BEG2(top, xtxpool_service_v2)

using base::xvblock_t;

class xtxpool_service_dispatcher_t {
public:
    virtual void dispatch(base::xcall_t & call) = 0;
    virtual bool is_mailbox_over_limit() = 0;
};

class xcovered_tables_t {
public:
    void add_covered_table(uint8_t zoneid, uint16_t tableid) {
        uint32_t id = (zoneid << 16) + tableid;
        m_table_set.insert(id);
    }
    bool is_covered(uint8_t zoneid, uint16_t tableid) const {
        uint32_t id = (zoneid << 16) + tableid;
        auto iter = m_table_set.find(id);
        return (iter != m_table_set.end());
    }
private:
    std::set<uint32_t> m_table_set;
};

// xtxpool_service_face interface of txpool net service, process network event
class xtxpool_service_face : public xrequest_tx_receiver_face {
public:
    virtual bool start(const xvip2_t & xip) = 0;
    virtual bool unreg(const xvip2_t & xip) = 0;
    virtual bool fade(const xvip2_t & xip) = 0;
    virtual void set_params(const xvip2_t & xip, const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver) = 0;
    // virtual bool is_receipt_sender(const base::xtable_index_t & tableid) const = 0;
    virtual bool is_send_receipt_role() const = 0;
    virtual bool table_boundary_equal_to(std::shared_ptr<xtxpool_service_face> & service) const = 0;
    virtual void get_service_table_boundary(base::enum_xchain_zone_index & zone_id, uint32_t & fount_table_id, uint32_t & back_table_id, common::xnode_type_t & node_type) const = 0;
    virtual void pull_lacking_receipts(uint64_t now, xcovered_tables_t & covered_tables) = 0;
    virtual void send_receipt_id_state(uint64_t now) = 0;
    virtual bool is_running()const = 0;
};

class xtxpool_proxy_face : public xrequest_tx_receiver_face {
public:
    virtual bool start() = 0;
    virtual bool unreg() = 0;
    virtual bool fade() = 0;
};

using xtxpool_proxy_face_ptr = std::shared_ptr<xtxpool_proxy_face>;

class xtxpool_service_mgr_face {
public:
    virtual xtxpool_proxy_face_ptr create(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver, const observer_ptr<router::xrouter_face_t> & router) = 0;
    // destroy useless cons services by networkdriver
    virtual bool destroy(const xvip2_t & xip) = 0;
    // init reference data
    virtual bool start(const xvip2_t & xip, const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver) = 0;
    // uninit reference data
    virtual bool unreg(const xvip2_t & xip) = 0;
    virtual bool fade(const xvip2_t & xip) = 0;
    virtual xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
};

class xtxpool_service_mgr_instance {
public:
    static std::shared_ptr<xtxpool_service_mgr_face> create_xtxpool_service_mgr_inst(const observer_ptr<store::xstore_face_t> & store,
                                                                                     const observer_ptr<base::xvblockstore_t> & blockstore,
                                                                                     const observer_ptr<xtxpool_v2::xtxpool_face_t> & txpool,
                                                                                     const std::vector<xobject_ptr_t<base::xiothread_t>> & iothreads,
                                                                                     const observer_ptr<mbus::xmessage_bus_face_t> & mbus,
                                                                                     const observer_ptr<time::xchain_time_face_t> & clock);
};

NS_END2
