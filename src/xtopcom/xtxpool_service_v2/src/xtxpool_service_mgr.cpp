// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool_service_v2/xtxpool_service_mgr.h"

#include "xblockstore/xblockstore_face.h"
#include "xdata/xblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xtableblock.h"
#include "xmbus/xevent_store.h"
#include "xstore/xstore.h"
#include "xtxpool_service_v2/xcons_utl.h"
#include "xtxpool_service_v2/xtxpool_proxy.h"
#include "xtxpool_service_v2/xtxpool_service.h"
#include "xtxpool_v2/xreceipt_resend.h"
#include "xtxpool_v2/xtxpool_error.h"

#include <cinttypes>

NS_BEG2(top, xtxpool_service_v2)

#define block_clock_height_fall_behind_max (30)
#define max_mailbox_num (8192)

xtxpool_service_mgr::xtxpool_service_mgr(const observer_ptr<store::xstore_face_t> & store,
                                         const observer_ptr<base::xvblockstore_t> & blockstore,
                                         const observer_ptr<xtxpool_v2::xtxpool_face_t> & txpool,
                                         const observer_ptr<base::xiothread_t> & iothread,
                                         const observer_ptr<mbus::xmessage_bus_face_t> & mbus,
                                         const observer_ptr<time::xchain_time_face_t> & clock)
  : m_para(make_object_ptr<xtxpool_svc_para_t>(store, blockstore, txpool)), m_iothread(iothread), m_mbus(mbus), m_clock(clock) {
}

void xtxpool_service_mgr::on_block_to_db_event(mbus::xevent_ptr_t e) {
    if (e->minor_type != mbus::xevent_store_t::type_block_to_db) {
        return;
    }
    
    auto event_handler = [this, e](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        mbus::xevent_store_block_to_db_ptr_t block_event = dynamic_xobject_ptr_cast<mbus::xevent_store_block_to_db_t>(e);
        const xblock_ptr_t & block = mbus::extract_block_from(block_event);
        xassert(block->check_block_flag(base::enum_xvblock_flag_committed));
        on_block_confirmed(block.get());
        return true;
    };
    base::xcall_t asyn_call(event_handler);
    m_timer->dispatch(asyn_call);
}

void xtxpool_service_mgr::on_block_confirmed(xblock_t * block) {
    xinfo("xtxpool_service_mgr::on_block_confirmed: block:%s", block->dump().c_str());

    auto handler = [this](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        xblock_t * block = dynamic_cast<xblock_t *>(call.get_param1().get_object());
        uint64_t now_clock = this->m_clock->logic_time();
        xinfo("xtxpool_service_mgr::on_block_confirmed process,level:%d,class:%d,now=%llu,block:%s", block->get_block_level(), block->get_block_class(), now_clock, block->dump().c_str());
        if (block->is_tableblock() && block->get_clock() + block_clock_height_fall_behind_max > now_clock) {
            make_receipts_and_send(block);
        }

        if (block->is_unitblock()) {
            this->m_para->get_txpool()->on_block_confirmed(block);
        }
        return true;
    };

    if (m_timer->is_mailbox_over_limit()) {
        xwarn("xtxpool_service_mgr::on_block_confirmed txpool mailbox limit,drop block=%s", block->dump().c_str());
        return;
    }
    base::xcall_t asyn_call(handler, block);
    m_timer->dispatch(asyn_call);
}

void xtxpool_service_mgr::make_receipts_and_send(xblock_t * block) {
    if (!block->is_lighttable() || !block->check_block_flag(base::enum_xvblock_flag_committed)) {
        xinfo("xtxpool_service_mgr::make_receipts_and_send block:%s", block->dump().c_str());
        return;
    }

    xtable_block_t * tableblock = dynamic_cast<xtable_block_t *>(block);
    std::vector<xcons_transaction_ptr_t> sendtx_receipts;
    std::vector<xcons_transaction_ptr_t> recvtx_receipts;
    tableblock->create_txreceipts(sendtx_receipts, recvtx_receipts);

    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    for (auto & it_send_receipt : sendtx_receipts) {
        send_receipt(it_send_receipt, xtxpool_v2::get_receipt_send_times(it_send_receipt->get_unit_cert()->get_gmtime(), now));
    }

    for (auto & it_recv_receipt : recvtx_receipts) {
        send_receipt(it_recv_receipt, xtxpool_v2::get_receipt_send_times(it_recv_receipt->get_unit_cert()->get_gmtime(), now));
    }
    xdbg("xtxpool_service_mgr::make_receipts_and_send block:%s", block->dump().c_str());
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

    std::shared_ptr<xtxpool_service_face> txpool_service = std::make_shared<xtxpool_service>(router, make_observer(m_para.get()));
    txpool_service->set_params(xip, vnet_driver);
    {
        // store to map
        std::lock_guard<std::mutex> lock(m_mutex);
        m_service_map[key] = txpool_service;
    }
    base::enum_xchain_zone_index zone_id;
    uint32_t fount_table_id;
    uint32_t back_table_id;
    txpool_service->get_service_table_boundary(zone_id, fount_table_id, back_table_id);
    m_para->get_txpool()->subscribe_tables(zone_id, fount_table_id, back_table_id);

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
    uint32_t fount_table_id;
    uint32_t back_table_id;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto iter = m_service_map.find(key);
        if (iter != m_service_map.end()) {
            auto txpool_service = iter->second;
            txpool_service->get_service_table_boundary(zone_id, fount_table_id, back_table_id);
            need_cleanup = true;
            m_service_map.erase(iter);
        }
    }

    if (need_cleanup) {
        // clear txpool tables corresponding to this service
        m_para->get_txpool()->unsubscribe_tables(zone_id, fount_table_id, back_table_id);
    }

    return true;
}

std::shared_ptr<xtxpool_service_face> xtxpool_service_mgr::find(const xvip2_t & xip) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto iter = m_service_map.find(xip);
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

void xtxpool_service_mgr::send_receipt(xcons_transaction_ptr_t & receipt, uint32_t resend_time) {
    std::string account_addr = receipt->get_account_addr();
    auto source_tableid = data::account_map_to_table_id(common::xaccount_address_t{account_addr});
    std::shared_ptr<xtxpool_service_face> service = find_receipt_sender(source_tableid, receipt->get_transaction()->digest());
    if (service != nullptr) {
        xdbg("xtxpool_service_mgr::send_receipt service found,zone:%d table:%d tx:%s", source_tableid.get_zone_index(), source_tableid.get_subaddr(), receipt->dump().c_str());
        service->send_receipt(receipt, resend_time);
    } else {
        xdbg("xtxpool_service_mgr::send_receipt no service found,zone:%d table:%d tx:%s", source_tableid.get_zone_index(), source_tableid.get_subaddr(), receipt->dump().c_str());
    }
}

std::shared_ptr<xtxpool_service_face> xtxpool_service_mgr::find_receipt_sender(const xtable_id_t & tableid, const uint256_t & hash) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto & iter : m_service_map) {
        auto service = iter.second;
        if (service->is_receipt_sender(tableid, hash)) {
            return service;
        }
    }
    return nullptr;
}

xcons_transaction_ptr_t xtxpool_service_mgr::query_tx(const std::string & account, const uint256_t & hash) const {
    auto & tx_ent = m_para->get_txpool()->query_tx(account, hash);
    if (tx_ent == nullptr) {
        return nullptr;
    }
    return tx_ent->get_tx();
}

void xtxpool_service_mgr::start() {
    xinfo("xtxpool_service_mgr::start");
    m_bus_listen_id = m_mbus->add_listener(top::mbus::xevent_major_type_store, std::bind(&xtxpool_service_mgr::on_block_to_db_event, this, std::placeholders::_1));
    m_timer = new xtxpool_service_timer_t(top::base::xcontext_t::instance(), m_iothread->get_thread_id(), this);
    m_timer->create_mailbox(256, max_mailbox_num, max_mailbox_num);  // create dedicated mailbox for txpool
    m_timer->start(0, 1000);
    m_para->set_dispatcher(make_observer(m_timer));
}

void xtxpool_service_mgr::stop() {
    xinfo("xtxpool_service_mgr::stop");
    m_timer->close();
    m_timer->release_ref();
}

void xtxpool_service_mgr::on_timer() {
    xdbg("xtxpool_service_mgr::on_timer");
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto & iter : m_service_map) {
        auto service = iter.second;
        service->on_timer(now);
    }
}

std::shared_ptr<xtxpool_service_mgr_face> xtxpool_service_mgr_instance::create_xtxpool_service_mgr_inst(const observer_ptr<store::xstore_face_t> & store,
                                                                                                        const observer_ptr<base::xvblockstore_t> & blockstore,
                                                                                                        const observer_ptr<xtxpool_v2::xtxpool_face_t> & txpool,
                                                                                                        const observer_ptr<base::xiothread_t> & iothread,
                                                                                                        const observer_ptr<mbus::xmessage_bus_face_t> & mbus,
                                                                                                        const observer_ptr<time::xchain_time_face_t> & clock) {
    auto txpool_service_mgr = std::make_shared<xtxpool_service_mgr>(store, blockstore, txpool, iothread, mbus, clock);
    return txpool_service_mgr;
}

NS_END2
