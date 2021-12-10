// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_service_v2/xtxpool_proxy.h"

#include <vector>

NS_BEG2(top, xtxpool_service_v2)

// default block service entry
xtxpool_proxy::xtxpool_proxy(const xvip2_t & xip, const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> &vnet_driver, const std::shared_ptr<xtxpool_service_mgr_face> & txpool_mgr, const std::shared_ptr<xtxpool_service_face> & txpool_service)
  : m_xip(xip), m_vnet_driver(vnet_driver), m_txpool_mgr(txpool_mgr), m_txpool_service(txpool_service) {}

// vnode start
bool xtxpool_proxy::start() {
    return m_txpool_mgr->start(m_xip, m_vnet_driver);
}

// vnode fade
bool xtxpool_proxy::unreg() {
    return m_txpool_mgr->unreg(m_xip);
}

bool xtxpool_proxy::fade() {
    return m_txpool_mgr->fade(m_xip);
}

int32_t xtxpool_proxy::request_transaction_consensus(const data::xtransaction_ptr_t & trans, bool local) {
    return m_txpool_service->request_transaction_consensus(trans, local);
}

xcons_transaction_ptr_t xtxpool_proxy::query_tx(const std::string & account, const uint256_t & hash) const {
     return m_txpool_mgr->query_tx(account, hash);
}

NS_END2
