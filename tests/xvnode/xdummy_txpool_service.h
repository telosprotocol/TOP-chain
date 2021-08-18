// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xtxpool_service_v2/xtxpool_service_face.h"

NS_BEG3(top, tests, vnode)
class xtop_dummy_txpool_proxy_face : public top::xtxpool_service_v2::xtxpool_proxy_face {
public:
    bool start() override { return false; }
    bool unreg() override { return false; }
    int32_t request_transaction_consensus(const top::data::xtransaction_ptr_t & trans, bool local) override { return -1; }
    xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const override {return nullptr;}
};

using xdummy_txpool_proxy_face_t = xtop_dummy_txpool_proxy_face;

class xtop_dummy_txpool_service_mgr_face : public top::xtxpool_service_v2::xtxpool_service_mgr_face {
public:
    top::xtxpool_service_v2::xtxpool_proxy_face_ptr create(const std::shared_ptr<top::vnetwork::xvnetwork_driver_face_t> & network,
                                                        const observer_ptr<top::router::xrouter_face_t> & router) override {
        return std::make_shared<xdummy_txpool_proxy_face_t>();
    }

    bool destroy(const xvip2_t & xip) override { return false; }
    bool start(const xvip2_t & xip, const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver) override { return false; }
    bool unreg(const xvip2_t & xip) override { return false; }
    xcons_transaction_ptr_t query_tx(const std::string & account, const uint256_t & hash) const override {return nullptr;}
    void start() {}
    void stop() {}
};
using xdummy_txpool_service_mgr_t = xtop_dummy_txpool_service_mgr_face;

extern xdummy_txpool_service_mgr_t dummy_txpool_service_mgr;

NS_END3
