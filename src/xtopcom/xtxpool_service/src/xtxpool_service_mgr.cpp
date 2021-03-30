// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_service/xtxpool_service_mgr.h"

#include "xblockstore/xblockstore_face.h"
#include "xstore/xstore.h"
#include "xtxpool/xtxpool_error.h"
#include "xtxpool_service/xcons_utl.h"
#include "xtxpool_service/xtxpool_proxy.h"
#include "xtxpool_service/xtxpool_service.h"

#include <cinttypes>

NS_BEG2(top, xtxpool_service)

xtxpool_service_mgr::xtxpool_service_mgr(const observer_ptr<store::xstore_face_t> & store, const xobject_ptr_t<base::xvblockstore_t> & blockstore, const observer_ptr<xtxpool::xtxpool_face_t> & txpool)
  : m_store(store), m_blockstore(blockstore), m_txpool(txpool) {
}

// create txpool proxy by networkdriver
xtxpool_proxy_face_ptr xtxpool_service_mgr::create(const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver, const observer_ptr<router::xrouter_face_t> & router) {
    auto xip = xcons_utl::to_xip2(vnet_driver->address(), true);
    auto key = xcons_utl::erase_version(xip);
    xinfo("xtxpool_service_mgr::create network proxy %s addr:%" PRIx64 " ", vnet_driver->address().to_string().c_str(), xip.low_addr);

    auto service = find(key);
    if (service != nullptr) {
        return std::make_shared<xtxpool_proxy>(xip, vnet_driver, this->shared_from_this(), service);
    }

    std::shared_ptr<xtxpool_service_face> txpool_service = std::make_shared<xtxpool_service>(router, m_store, m_blockstore, m_txpool);
    txpool_service->set_params(xip, vnet_driver);
    {
        // store to map
        std::lock_guard<std::mutex> lock(m_mutex);
        m_service_map[key] = txpool_service;
    }
    base::enum_xchain_zone_index zone_id;
    uint32_t                     fount_table_id;
    uint32_t                     back_table_id;
    txpool_service->get_service_table_boundary(zone_id, fount_table_id, back_table_id);
    m_txpool->subscribe_tables(zone_id, fount_table_id, back_table_id);

    return std::make_shared<xtxpool_proxy>(xip, vnet_driver, this->shared_from_this(), txpool_service);
}

// destroy useless txpool services by networkdriver, call by vnode manager while detemine some service useless
// must call uninit before
bool xtxpool_service_mgr::destroy(const xvip2_t & xip) {
    auto key = xcons_utl::erase_version(xip);
    xinfo("xtxpool_service_mgr::destroy network proxy %" PRIx64 " ", xip.low_addr);
        // erase useless txpool service
    bool need_cleanup = false;
    base::enum_xchain_zone_index zone_id;
    uint32_t                     fount_table_id;
    uint32_t                     back_table_id;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto                        iter = m_service_map.find(key);
        if (iter != m_service_map.end()) {
            auto txpool_service = iter->second;
            txpool_service->get_service_table_boundary(zone_id, fount_table_id, back_table_id);
            need_cleanup = true;
            m_service_map.erase(iter);
        }
    }

    if (need_cleanup) {
        // clear txpool tables corresponding to this service
        m_txpool->unsubscribe_tables(zone_id, fount_table_id, back_table_id);
    }

    return true;
}

std::shared_ptr<xtxpool_service_face> xtxpool_service_mgr::find(const xvip2_t & xip) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto                        iter = m_service_map.find(xip);
        if (iter != m_service_map.end()) {
            return iter->second;
        }
    }
    return nullptr;
}

// init txpool service
bool xtxpool_service_mgr::start(const xvip2_t & xip, const std::shared_ptr<vnetwork::xvnetwork_driver_face_t> & vnet_driver) {
    auto key = xcons_utl::erase_version(xip);
    xinfo("xtxpool_service_mgr::start network proxy %" PRIx64 " ", xip.low_addr);
    std::shared_ptr<xtxpool_service_face> service = find(key);
    if (service != nullptr) {
        service->set_params(xip, vnet_driver);
        service->start(xip);
        return true;
    }
    return false;
}

// uninit data
bool xtxpool_service_mgr::fade(const xvip2_t & xip) {
    auto key = xcons_utl::erase_version(xip);
    xinfo("xtxpool_service_mgr::fade network proxy %" PRIx64 " ", xip.low_addr);
    std::shared_ptr<xtxpool_service_face> service = find(key);
    if (service != nullptr) {
        service->fade(xip);
        return true;
    }
    return false;
}

void xtxpool_service_mgr::send_receipt(const xcons_transaction_ptr_t & receipt, uint32_t resend_time) {
    std::string source_addr;
    if (receipt->is_recv_tx()) {
        source_addr = receipt->get_transaction()->get_source_addr();
    } else {
        source_addr = receipt->get_transaction()->get_target_addr();
    }
    auto                                  source_tableid = data::account_map_to_table_id(common::xaccount_address_t{source_addr});
    std::shared_ptr<xtxpool_service_face> service = find_receipt_sender(source_tableid, receipt->get_transaction()->digest());
    if (service != nullptr) {
        xdbg("xtxpool_service_mgr::send_receipt service found,zone:%d table:%d tx:%s", source_tableid.get_zone_index(), source_tableid.get_subaddr(), receipt->dump().c_str());
        service->send_receipt(receipt, resend_time);
    } else {
        xdbg("xtxpool_service_mgr::send_receipt no service found,zone:%d table:%d tx:%s", source_tableid.get_zone_index(), source_tableid.get_subaddr(), receipt->dump().c_str());
    }
}

std::shared_ptr<xtxpool_service_face> xtxpool_service_mgr::find_receipt_sender(const xtable_id_t & tableid, const uint256_t & hash) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto & iter : m_service_map) {
            auto service = iter.second;
            if (service->is_receipt_sender(tableid, hash)) {
                return service;
            }
        }
    }
    return nullptr;
}

xcons_transaction_ptr_t xtxpool_service_mgr::query_tx(const std::string & account, const uint256_t & hash) const {
     return m_txpool->query_tx(account, hash);
}

std::shared_ptr<xtxpool_service_mgr_face> xtxpool_service_mgr_instance::create_xtxpool_service_mgr_inst(const observer_ptr<store::xstore_face_t> &    store,
                                                                                                        const xobject_ptr_t<base::xvblockstore_t> &    blockstore,
                                                                                                        const observer_ptr<xtxpool::xtxpool_face_t> & txpool) {
    auto txpool_service_mgr = std::make_shared<xtxpool_service_mgr>(store, blockstore, txpool);
    txpool->set_receipt_tranceiver(txpool_service_mgr);
    return txpool_service_mgr;
}

NS_END2
