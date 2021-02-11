// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbase/xdata.h"
#include "xbase/xvblock.h"
#include "xbasic/xmemory.hpp"
#include "xchain_timer/xchain_timer_face.h"
#include "xcommon/xfadable.h"
#include "xdata/xblock.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xtransaction.h"
#include "xrouter/xrouter.h"
#include "xstore/xstore_face.h"
#include "xbase/xvledger.h"
#include "xtxpool/xtxpool_face.h"
#include "xtxpool_service/xrequest_tx_receiver_face.h"
#include "xunit_service/xcons_face.h"
#include "xvnetwork/xvnetwork_driver_face.h"

NS_BEG2(top, xtxpool_service)

using base::xvblock_t;

// xtxpool_service_face interface of txpool net service, process network event
class xtxpool_service_face
  : public xtxpool::xreceipt_tranceiver_face_t
  , public xrequest_tx_receiver_face {
public:
    virtual bool start(const xvip2_t & xip) = 0;
    virtual bool fade(const xvip2_t & xip) = 0;
    virtual void set_params(const xvip2_t & xip, const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver) = 0;
    virtual bool is_running() = 0;
    virtual bool is_receipt_sender(const xtable_id_t & tableid, const uint256_t & hash) = 0;
    virtual bool table_boundary_equal_to(std::shared_ptr<xtxpool_service_face> & service) = 0;
    virtual void get_service_table_boundary(base::enum_xchain_zone_index & zone_id, uint32_t & fount_table_id, uint32_t & back_table_id) = 0;
};

class xtxpool_proxy_face : public xrequest_tx_receiver_face {
public:
    virtual bool start() = 0;
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
    virtual bool fade(const xvip2_t & xip) = 0;

    virtual xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const = 0;
};

class xtxpool_service_mgr_instance {
public:
    static std::shared_ptr<xtxpool_service_mgr_face> create_xtxpool_service_mgr_inst(const observer_ptr<store::xstore_face_t> &    store,
																					 const xobject_ptr_t<base::xvblockstore_t> &   blockstore,
                                                                                     const observer_ptr<xtxpool::xtxpool_face_t> & txpool);
};

NS_END2
