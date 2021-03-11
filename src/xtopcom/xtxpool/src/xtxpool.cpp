// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool/xtxpool.h"

#include "xbase/xvblock.h"
#include "xbase/xvledger.h"
#include "xbasic/xns_macro.h"
#include "xdata/xblock.h"
#include "xdata/xtableblock.h"
#include "xmbus/xevent_store.h"
#include "xconfig/xconfig_register.h"
#include "xtxpool/xtxpool_error.h"
#include "xtxpool/xtxpool_log.h"
#include "xtxpool/xtxpool_table.h"
#include "xtxpool/xtxpool_para.h"
#include "xtxpool/xtxpool_receipt_receiver_counter.h"

#include <iostream>

NS_BEG2(top, xtxpool)

REG_XMODULE_LOG(chainbase::enum_xmodule_type::xmodule_type_xtxpool, xtxpool_error_to_string, xtxpool_error_base + 1, xtxpool_error_max);

#define block_clock_height_fall_behind_max (30)

#define traverse_table_num_once_per_time_fire (16)

xtxpool_t::xtxpool_t(const std::shared_ptr<xtxpool_resources_face> & para,
                     const xobject_ptr_t<base::xworkerpool_t> & pwork)
  : base::xxtimer_t(pwork->get_thread(0)->get_context(), pwork->get_thread(0)->get_thread_id()), m_para(para), m_work(pwork) {
    for (int32_t i = 0; i < enum_xtxpool_table_type_max; i++) {
        for (int32_t j = 0; j < enum_vbucket_has_tables_count; j++) {
            m_txpool_tables[i][j] = nullptr;
            m_table_cover_count[i][j] = 0;
        }
    }

    m_bus_listen_id = m_para->get_mbus()->add_listener(top::mbus::xevent_major_type_store, std::bind(&xtxpool_t::on_block_to_db_event, this, std::placeholders::_1));
    // todo(nathan):m_bus->remove_listener
}

xtxpool_table_t * xtxpool_t::get_txpool_table(uint8_t zone, uint16_t table_id) const {
    xassert(zone < enum_xtxpool_table_type_max);
    xassert(table_id < enum_vbucket_has_tables_count);
    if (m_txpool_tables[zone][table_id] == nullptr) {
        m_txpool_tables[zone][table_id] = new xtxpool_table_t(zone, table_id, m_para);
    }
    return m_txpool_tables[zone][table_id];
}

xtxpool_table_t * xtxpool_t::get_txpool_table(const std::string & table_account) {
    return get_txpool_table_by_addr(table_account);
}

int32_t xtxpool_t::push_send_tx(const xtransaction_ptr_t & tx) {
    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
    return push_send_tx(cons_tx);
}

int32_t xtxpool_t::push_send_tx(const xcons_transaction_ptr_t & tx) {
    xtxpool_table_t * xtxpool_table = get_txpool_table_by_addr(tx->get_source_addr());
    return xtxpool_table->push_send_tx(tx);
}

int32_t xtxpool_t::push_recv_tx(const xcons_transaction_ptr_t & tx) {
    xtxpool_table_t * xtxpool_table = get_txpool_table_by_addr(tx->get_target_addr());
    return xtxpool_table->push_recv_tx(tx);
}
int32_t xtxpool_t::push_recv_ack_tx(const xcons_transaction_ptr_t & tx) {
    xtxpool_table_t * xtxpool_table = get_txpool_table_by_addr(tx->get_source_addr());
    return xtxpool_table->push_recv_ack_tx(tx);
}

std::map<std::string, std::vector<xcons_transaction_ptr_t>> xtxpool_t::get_txs(const std::string & tableblock_account, uint64_t commit_height) {
    xtxpool_table_t * xtxpool_table = get_txpool_table_by_addr(tableblock_account);
    auto              max_account_num = XGET_CONFIG(tableblock_batch_unitblock_max_num);
    return xtxpool_table->get_txs(commit_height, max_account_num);
}

std::vector<std::string> xtxpool_t::get_accounts(const std::string & tableblock_account) {
    xtxpool_table_t * xtxpool_table = get_txpool_table_by_addr(tableblock_account);
    auto              max_account_num = XGET_CONFIG(tableblock_batch_unitblock_max_num);
    return xtxpool_table->get_accounts(max_account_num);
}

std::vector<xcons_transaction_ptr_t> xtxpool_t::get_account_txs(const std::string & account, uint64_t commit_height, uint64_t unit_height, uint64_t last_nonce, const uint256_t & last_hash, uint64_t now) {
    xtxpool_table_t * xtxpool_table = get_txpool_table_by_addr(account);
    return xtxpool_table->get_account_txs(account, commit_height, unit_height, last_nonce, last_hash, now);
}

int32_t xtxpool_t::verify_txs(const std::string & account, uint64_t commit_height, uint64_t unit_height, std::vector<xcons_transaction_ptr_t> txs, uint64_t last_nonce, const uint256_t & last_hash) {
    xtxpool_table_t * xtxpool_table = get_txpool_table_by_addr(account);
    return xtxpool_table->verify_txs(account, commit_height, unit_height, txs, last_nonce, last_hash);
}

void xtxpool_t::on_block_confirmed(xblock_t * block) {
    xinfo("xtxpool_t::on_block_confirmed: block:%s", block->dump().c_str());

    auto handler = [this](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        xblock_t * block = dynamic_cast<xblock_t *>(call.get_param1().get_object());
        xinfo("xtxpool_t::on_block_confirmed process, block:%s", block->dump().c_str());
        if (block->is_lighttable() && block->get_clock() + block_clock_height_fall_behind_max > this->m_para->get_chain_timer()->logic_time()) {
            make_receipts_and_send(block);
        }
        xtxpool_table_t * xtxpool_table = this->get_txpool_table_by_addr(block->get_account());
        xtxpool_table->on_block_confirmed(block);
        return true;
    };

    if (is_mailbox_over_limit()) {
        xwarn("xtxpool_t::on_block_confirmed txpool mailbox limit,drop block=%s", block->dump().c_str());
        return;
    }
    base::xcall_t asyn_call(handler, block);
    send_call(asyn_call);
}

void xtxpool_t::dispatch(base::xcall_t & call) {
    send_call(call);
}

int32_t xtxpool_t::on_receipt(const data::xcons_transaction_ptr_t & cons_tx) {
    int32_t ret;
    if (cons_tx->is_recv_tx()) {
        XMETRICS_COUNTER_INCREMENT("txpool_receipt_recv_total", 1);
        return push_recv_tx(cons_tx);
    } else {
        return push_recv_ack_tx(cons_tx);
    }
}
void xtxpool_t::set_receipt_tranceiver(std::shared_ptr<xreceipt_tranceiver_face_t> receipt_tranceiver) {
    m_receipt_tranceiver = receipt_tranceiver;
}

xaccountobj_t * xtxpool_t::find_account(const std::string & address) {
    xtxpool_table_t * xtxpool_table = get_txpool_table_by_addr(address);
    return xtxpool_table->find_account(address);
}

xcons_transaction_ptr_t xtxpool_t::pop_tx_by_hash(const std::string & address, const uint256_t & hash, uint8_t subtype, int32_t result) {
    xtxpool_table_t * xtxpool_table = get_txpool_table_by_addr(address);
    return xtxpool_table->pop_tx_by_hash(address, hash, subtype, result);
}

xtxpool_table_t * xtxpool_t::get_txpool_table_by_addr(const std::string & address) const {
    auto              xid = base::xvaccount_t::get_xid_from_account(address);
    uint8_t           zone = get_vledger_zone_index(xid);
    uint16_t          subaddr = get_vledger_subaddr(xid);
    xtxpool_table_t * xtxpool_table = get_txpool_table(zone, subaddr);
    return xtxpool_table;
}

void xtxpool_t::on_block_to_db_event(mbus::xevent_ptr_t e) {
    if (e->minor_type != mbus::xevent_store_t::type_block_to_db) {
        return;
    }

    mbus::xevent_store_block_to_db_ptr_t block_event = std::static_pointer_cast<mbus::xevent_store_block_to_db_t>(e);
    const xblock_ptr_t &                 block = block_event->block;
    xassert(block->check_block_flag(base::enum_xvblock_flag_committed));
    on_block_confirmed(block.get());
}

bool xtxpool_t::is_mailbox_over_limit() {
    int64_t in, out;
    int32_t queue_size = count_calls(in, out);
    bool discard = queue_size >= enum_max_mailbox_num;
    if (discard) {
        xwarn("xtxpool_t::is_mailbox_over_limit in=%ld,out=%ld", in, out);
        return true;
    }
    return false;
}

void xtxpool_t::make_receipts_and_send(xblock_t * block) {
    if (!block->is_lighttable()) {
        return;
    }
    xassert(block->check_block_flag(base::enum_xvblock_flag_committed));

    if (m_receipt_tranceiver == nullptr) {
        xdbg("m_receipt_tranceiver not set");
        return;
    }

    xtable_block_t *                     tableblock = dynamic_cast<xtable_block_t *>(block);
    std::vector<xcons_transaction_ptr_t> sendtx_receipts;
    std::vector<xcons_transaction_ptr_t> recvtx_receipts;
    tableblock->create_txreceipts(sendtx_receipts, recvtx_receipts);

    if ((!sendtx_receipts.empty()) || (!recvtx_receipts.empty())) {
        uint64_t now = xverifier::xtx_utl::get_gmttime_s();
        base::xauto_ptr<base::xvblock_t> justify_tableblock = xblocktool_t::load_justify_block(m_para->get_vblockstore(), tableblock->get_account(), tableblock->get_height() + 2);
        if (justify_tableblock == nullptr) {
            xwarn("xtxpool_t::make_receipts_and_send can not load justify tableblock. tableblock=%s",
                tableblock->dump().c_str());
            return;
        }
        xassert(justify_tableblock->get_justify_cert_hash() == tableblock->get_output_root_hash());

        for (auto & send_receipt : sendtx_receipts) {
            send_receipt->set_commit_prove_with_parent_cert(justify_tableblock->get_cert());
            m_receipt_tranceiver->send_receipt(send_receipt, get_receipt_resend_time(send_receipt->get_unit_cert()->get_gmtime(), now));
        }

        for (auto & recv_receipt : recvtx_receipts) {
            recv_receipt->set_commit_prove_with_parent_cert(justify_tableblock->get_cert());
            m_receipt_tranceiver->send_receipt(recv_receipt, get_receipt_resend_time(recv_receipt->get_unit_cert()->get_gmtime(), now));
        }
    }

    xdbg("xtxpool_t::make_receipts_and_send block:%s", block->dump().c_str());
}

void xtxpool_t::subscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) {
    if (zone >= enum_xtxpool_table_type_max || back_table_id >= enum_vbucket_has_tables_count || front_table_id > back_table_id) {
        xassert(0);
        return;
    }
    std::lock_guard<std::mutex> lck(m_mutex[zone]);
    for (uint16_t table_id = front_table_id; table_id <= back_table_id; table_id++) {
        m_table_cover_count[zone][table_id]++;
        xdbg("xtxpool_t::subscribe_tables zone:%d,table:%d,count:%d", zone, table_id, m_table_cover_count[zone][table_id]);
        xtxpool_table_t *table = get_txpool_table(zone, table_id);
        table->init();
    }
}

void xtxpool_t::unsubscribe_tables(uint8_t zone, uint16_t front_table_id, uint16_t back_table_id) {
    if (zone >= enum_xtxpool_table_type_max || back_table_id >= enum_vbucket_has_tables_count || front_table_id > back_table_id) {
        xassert(0);
        return;
    }
    std::lock_guard<std::mutex> lck(m_mutex[zone]);
    for (uint16_t table_id = front_table_id; table_id <= back_table_id; table_id++) {
        xassert(m_table_cover_count[zone][table_id] > 0);
        m_table_cover_count[zone][table_id]--;
        xdbg("xtxpool_t::unsubscribe_tables zone:%d,table:%d,count:%d", zone, table_id, m_table_cover_count[zone][table_id]);
        if (m_table_cover_count[zone][table_id] == 0 && m_txpool_tables[zone][table_id] != nullptr) {
            m_txpool_tables[zone][table_id]->clear();
        }
    }
}

bool xtxpool_t::start_timer() {
    base::xxtimer_t::start(0, 1000);
    return true;
}

void xtxpool_t::stop_timer() {
    base::xxtimer_t::stop();
}

bool xtxpool_t::on_timer_fire(const int32_t thread_id, const int64_t timer_id, const int64_t current_time_ms, const int32_t start_timeout_ms, int32_t & in_out_cur_interval_ms) {
    if (m_receipt_tranceiver != nullptr) {
        xtxpool_receipt_receiver_counter counter;
        int32_t begin = (m_timer_fire_time * traverse_table_num_once_per_time_fire)%enum_vbucket_has_tables_count;
        int32_t end = begin + traverse_table_num_once_per_time_fire;
        xassert(enum_vbucket_has_tables_count % traverse_table_num_once_per_time_fire == 0);
        xassert(end <= enum_vbucket_has_tables_count);
        for (int32_t i = 0; i < enum_xtxpool_table_type_max; i++) {
            for (int32_t j = begin; j < end; j++) {
                if (m_txpool_tables[i][j] != nullptr && m_table_cover_count[i][j] != 0) {
                    // xdbg("xtxpool_t::on_timer_fire zone:%d,table:%d", i, j);
                    m_txpool_tables[i][j]->on_timer_check_cache(*m_receipt_tranceiver.get(), counter);
                }
            }
        }
        m_timer_fire_time++;
    }
    return true;
}

xcons_transaction_ptr_t xtxpool_t::query_tx(const std::string & account, const uint256_t & hash) const {
    xtxpool_table_t * xtxpool_table = get_txpool_table_by_addr(account);
     return xtxpool_table->query_tx(account, hash);
}

xcons_transaction_ptr_t xtxpool_t::get_unconfirm_tx(const std::string source_addr, const uint256_t & hash) {
    xtxpool_table_t * xtxpool_table = get_txpool_table_by_addr(source_addr);
    return xtxpool_table->get_unconfirm_tx(source_addr, hash);
}

xobject_ptr_t<xtxpool_face_t> xtxpool_instance::create_xtxpool_inst(const observer_ptr<store::xstore_face_t> &     store,
                                                                    xobject_ptr_t<base::xvblockstore_t> &          blockstore,
                                                                    const observer_ptr<mbus::xmessage_bus_face_t> &bus,
                                                                    const xobject_ptr_t<base::xvcertauth_t> &      certauth,
                                                                    const observer_ptr<time::xchain_time_face_t> & clock,
                                                                    enum_xtxpool_order_strategy                    strategy) {
    auto para = std::make_shared<xtxpool_resources>(store, blockstore, bus, certauth, clock, strategy);
    auto work_pool = make_object_ptr<base::xworkerpool_t_impl<1>>(top::base::xcontext_t::instance());
    auto xtxpool = top::make_object_ptr<xtxpool_t>(para, work_pool);
    xtxpool->create_mailbox(256, xtxpool_t::enum_max_mailbox_num, xtxpool_t::enum_max_mailbox_num);  // create dedicated mailbox for txpool
    xtxpool->start_timer();
    return xtxpool;
}

NS_END2
