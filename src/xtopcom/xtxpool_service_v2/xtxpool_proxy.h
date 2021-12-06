// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xtxpool_service_v2/xtxpool_service_face.h"

#include <vector>

NS_BEG2(top, xtxpool_service_v2)

// default block service entry
class xtxpool_proxy : public xtxpool_proxy_face {
public:
    explicit xtxpool_proxy(const xvip2_t & xip, const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver, const std::shared_ptr<xtxpool_service_mgr_face> & txpool_mgr, const std::shared_ptr<xtxpool_service_face> & txpool_service);

public:
    bool    start() override;
    bool    unreg() override;
    bool    fade() override;
    int32_t request_transaction_consensus(const data::xtransaction_ptr_t & trans, bool local) override;
    xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const override;

private:
    xvip2_t                                   m_xip;
    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> m_vnet_driver;
    std::shared_ptr<xtxpool_service_mgr_face> m_txpool_mgr;
    std::shared_ptr<xtxpool_service_face>     m_txpool_service;
};

NS_END2
