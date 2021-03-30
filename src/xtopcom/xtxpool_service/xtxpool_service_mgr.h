// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbase/xbase.h"
#include "xbase/xthread.h"
#include "xbasic/xns_macro.h"
#include "xtxpool/xtxpool_face.h"
#include "xtxpool_service/xcons_utl.h"
#include "xtxpool_service/xtxpool_service_face.h"

#include <map>

NS_BEG2(top, xtxpool_service)

// txpool proxy, bridge for txpool and txpool service
class xtxpool_service_mgr
  : public xtxpool_service_mgr_face
  , public xtxpool::xreceipt_tranceiver_face_t
  , public std::enable_shared_from_this<xtxpool_service_mgr> {
public:
    explicit xtxpool_service_mgr(const observer_ptr<store::xstore_face_t> & store, const xobject_ptr_t<base::xvblockstore_t> & blockstore, const observer_ptr<xtxpool::xtxpool_face_t> & txpool);

    xtxpool_proxy_face_ptr create(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver, const observer_ptr<router::xrouter_face_t> & router) override;
    bool                   destroy(const xvip2_t & xip) override;
    bool                   start(const xvip2_t & xip, const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver) override;
    bool                   fade(const xvip2_t & xip) override;
    void                   send_receipt(const xcons_transaction_ptr_t & receipt, uint32_t resend_time) override;
    xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const override;

private:
    std::shared_ptr<xtxpool_service_face> find(const xvip2_t & xip);
    std::shared_ptr<xtxpool_service_face> find_receipt_sender(const xtable_id_t & tableid, const uint256_t & hash);

private:
    observer_ptr<store::xstore_face_t>                                      m_store;
    xobject_ptr_t<base::xvblockstore_t>                                     m_blockstore;
    observer_ptr<xtxpool::xtxpool_face_t>                                   m_txpool;
    std::map<xvip2_t, std::shared_ptr<xtxpool_service_face>, xvip2_compare> m_service_map;
    std::mutex                                                              m_mutex;
};
NS_END2
