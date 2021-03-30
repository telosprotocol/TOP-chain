// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xns_macro.h"
#include "xdata/xblocktool.h"
#include "xdata/xtableblock.h"
#include "xtxpool/xtxpool.h"
#include "xtxpool/xtxpool_error.h"
#include "xtxpool/xtxpool_log.h"
#include "xverifier/xtx_verifier.h"
#include "xverifier/xverifier_utl.h"
#include "xverifier/xwhitelist_verifier.h"
#include "xconfig/xconfig_register.h"

#include <cinttypes>
#include <iostream>

NS_BEG2(top, xtxpool)

xtxpool_table_t::xtxpool_table_t(uint8_t zone_id,
                                 uint16_t table_id,
                                 const std::shared_ptr<xtxpool_resources_face> &para)
  : m_table_account(data::xblocktool_t::make_address_table_account((base::enum_xchain_zone_index)zone_id, table_id)), m_para(para), m_consensused_recvtx_cache(para, m_table_account), m_unconfirm_sendtx_cache(para, m_table_account) {
    xdbg("xtxpool_table_t::xtxpool_table_t create table=%s", m_table_account.c_str());
}

xtxpool_table_t::~xtxpool_table_t() {
    xdbg("xtxpool_table_t::~xtxpool_table_t destroy table=%s", m_table_account.c_str());
}

int32_t xtxpool_table_t::push_send_tx(const xcons_transaction_ptr_t & cons_tx) {
    if (!data::is_sys_contract_address(common::xaccount_address_t{cons_tx->get_source_addr()})) {
        if (m_send_tx_num >= enum_max_table_send_tx_num) {
            xwarn("xtxpool_table_t::push_send_tx table %s send tx number reached upper limit, tx:%s", m_table_account.c_str(), cons_tx->dump().c_str());
            return xtxpool_error_send_tx_reach_upper_limit;
        }

        int32_t unconfirm_tx_num = m_unconfirm_sendtx_cache.get_unconfirm_tx_num();
        if (unconfirm_tx_num >= enum_max_table_unconfirm_tx_num) {
            xwarn("xtxpool_table_t::push_send_tx table %s unconfirm tx number reached upper limit, tx:%s", m_table_account.c_str(), cons_tx->dump().c_str());
            return xtxpool_error_send_tx_reach_upper_limit;
        }
    }

    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    int32_t ret = verify_send_tx(cons_tx, now);
    if (ret) {
        xwarn("xtxpool_table_t::push_send_tx table %s verify send tx fail, tx:%s", m_table_account.c_str(), cons_tx->dump().c_str());
        XMETRICS_COUNTER_INCREMENT("txpool_push_tx_fail", 1);
        return ret;
    }

    // push txpool fire expire check
    ret = xverifier::xtx_verifier::verify_tx_fire_expiration(cons_tx->get_transaction(), now);
    if (ret) {
        xwarn("xtxpool_table_t::push_send_tx table %s tx fire expired, tx:%s", m_table_account.c_str(), cons_tx->dump().c_str());
        XMETRICS_COUNTER_INCREMENT("txpool_push_tx_fail", 1);
        return ret;
    }

    xtransaction_t * tx = cons_tx->get_transaction();
    tx->set_push_pool_timestamp(now);

    {
        std::lock_guard<std::mutex> lck(m_table_mutex);
        xaccountobj_t * _account_obj = find_account_and_create_new(tx->get_source_addr(), tx->get_push_pool_timestamp());
        ret = _account_obj->push_send_tx(cons_tx);
    }
    return ret;
}

int32_t xtxpool_table_t::push_recv_tx(const xcons_transaction_ptr_t & cons_tx) {
    int32_t ret = verify_receipt_tx(cons_tx);
    if (ret) {
        XMETRICS_COUNTER_INCREMENT("txpool_push_tx_fail", 1);
        return ret;
    }
    xtransaction_t * tx = cons_tx->get_transaction();
    uint64_t tx_timer_height = cons_tx->get_clock();
    std::vector<std::pair<std::string, uint256_t>> committed_recv_txs;
    ret = m_consensused_recvtx_cache.is_receipt_duplicated(cons_tx->get_clock(), tx, committed_recv_txs);
    delete_committed_recv_txs(committed_recv_txs);
    if (ret != xsuccess) {
        xwarn("xtxpool_table_t::tx_push fail. table=%s,timer_height:%ld,tx=%s,fail-%s", m_table_account.c_str(), tx_timer_height, cons_tx->dump().c_str(), get_error_str(ret).c_str());
        return ret;
    }
    tx->set_push_pool_timestamp(xverifier::xtx_utl::get_gmttime_s());

    {
        std::lock_guard<std::mutex> lck(m_table_mutex);
        xaccountobj_t * _account_obj = find_account_and_create_new(tx->get_target_addr(), tx->get_push_pool_timestamp());
        ret = _account_obj->push_recv_tx(cons_tx);
    }
    return ret;
}

int32_t xtxpool_table_t::push_recv_ack_tx(const xcons_transaction_ptr_t & cons_tx) {
    int32_t ret = verify_receipt_tx(cons_tx);
    if (ret) {
        XMETRICS_COUNTER_INCREMENT("txpool_push_tx_fail", 1);
        return ret;
    }
    xtransaction_t * tx = cons_tx->get_transaction();
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    ret = m_unconfirm_sendtx_cache.has_txhash_receipt(tx->get_source_addr(), tx->digest(), now);
    if (ret != xtxpool_error_unconfirm_sendtx_exist) {
        xwarn("xtxpool_table_t::tx_push fail. table=%s,tx=%s,fail-%s", m_table_account.c_str(), cons_tx->dump().c_str(), get_error_str(ret).c_str());
        return ret;
    }
    tx->set_push_pool_timestamp(now);

    {
        std::lock_guard<std::mutex> lck(m_table_mutex);
        xaccountobj_t * _account_obj = find_account_and_create_new(tx->get_source_addr(), tx->get_push_pool_timestamp());
        ret = _account_obj->push_recv_ack_tx(cons_tx);
    }
    return ret;
}

std::map<std::string, std::vector<xcons_transaction_ptr_t>> xtxpool_table_t::get_txs(uint64_t commit_height, uint32_t max_account_num) {
    uint32_t account_num = 0;
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    // for accountobj sort
    std::set<xaccountobj_t *, accountobj_default_comp> accountobj_set;
    std::map<std::string, std::vector<xcons_transaction_ptr_t>> txs_map;

    xdbg("xtxpool_table_t::get_txs enter,table=%s,height=%" PRIu64 ",max_num=%d", m_table_account.c_str(), commit_height, max_account_num);
    std::lock_guard<std::mutex> lck(m_table_mutex);
    for (auto iter = m_waiting_accounts.begin(); iter != m_waiting_accounts.end();) {
        const std::string & account = iter->first;
        if (m_accounts_lock_mgr.is_account_lock(account)) {
            xdbg("xtxpool_table_t::get_txs,account is locked. table=%s,account=%s", m_table_account.c_str(), account.c_str());
            iter++;
            continue;
        }

        xaccountobj_t * _account_obj = iter->second;
        if (_account_obj->empty()) {
            _account_obj->release_ref();
            iter = m_waiting_accounts.erase(iter);
        } else {
            accountobj_set.insert(_account_obj);
            iter++;
        }
    }

    int32_t tableblock_batch_tx_num_residue = XGET_CONFIG(tableblock_batch_tx_max_num);

    for (auto & iter1 : accountobj_set) {
        xaccountobj_t * _account_obj = iter1;
        if (account_num >= max_account_num && tableblock_batch_tx_num_residue > 0) {
            _account_obj->clear_selected_time();
            continue;
        }
        const std::string & account = _account_obj->get_account_address();
        // unit has no lock and highqc block
        auto latest_commit_block = m_para->get_vblockstore()->get_latest_committed_block(account);
        // the latest block must connected and executed
        if (!xblocktool_t::is_connect_and_executed_block(latest_commit_block.get())) {
            xwarn("xtxpool_table_t::get_txs,account behind. table=%s,latest_commit_block=%s",
                m_table_account.c_str(), latest_commit_block->dump().c_str());
            //invoke_sync(account, "unit leader sync");
            continue;
        }

        // TODO(jimmy) cold account should delay xcold_account_unknown_sync_time_configuration_t second for consensus
        // if (now < info.m_update_timestamp) {
        //     xinfo("xtxpool_table_t::get_txs,leader account usable timestamp can't use, table=%s,account=%s,unit_height:%ld update_stamp:%ld now:%ld",
        //           m_table_account.c_str(),
        //           account.c_str(),
        //           info.m_unit_height,
        //           info.m_update_timestamp,
        //           now);
        //     continue;
        // }

        uint64_t latest_send_trans_number = 0;
        uint256_t latest_send_trans_hash;
        if (!latest_commit_block->is_genesis_block()) {
            base::xauto_ptr<xblockchain2_t> blockchain{m_para->get_store()->clone_account(account)};
            if (blockchain == nullptr) {
                xwarn("xtxpool_table_t::get_txs,blockchain not find. table=%s,latest_commit_block=%s",
                    m_table_account.c_str(), latest_commit_block->dump().c_str());
                continue;
            }
            latest_send_trans_number = blockchain->account_send_trans_number();
            latest_send_trans_hash = blockchain->account_send_trans_hash();
        }

        _account_obj->clear_invalid_send_tx(now);
        std::vector<xcons_transaction_ptr_t> txs = _account_obj->get_cons_txs(latest_send_trans_number, latest_send_trans_hash);
        txs_validity_check_leader(_account_obj, txs, commit_height, latest_commit_block->get_height(), now);
        if (txs.empty()) {
            xwarn("xtxpool_table_t::get_txs,txs empty after duplicate check,table=%s,account=%s", m_table_account.c_str(), account.c_str());
            continue;
        }
        tableblock_batch_tx_num_residue -= txs.size();
        txs_map[account] = txs;
        for (auto & iter : txs) {
            xinfo("xtxpool_table_t::get_txs succ.table=%s,account=%s,height=%llu,tx=%s", m_table_account.c_str(), account.c_str(), latest_commit_block->get_height(), iter->dump().c_str());
        }
        account_num++;
    }

    return txs_map;
}

std::vector<std::string> xtxpool_table_t::get_accounts() {
    auto max_account_num = XGET_CONFIG(tableblock_batch_unitblock_max_num);
    return get_accounts(max_account_num);
}

std::vector<std::string> xtxpool_table_t::get_accounts(uint32_t max_account_num) {
    uint32_t account_num = 0;
    // for accountobj sort
    std::set<xaccountobj_t *, accountobj_default_comp> accountobj_set;
    std::vector<std::string> accounts;

    xdbg("xtxpool_table_t::get_accounts enter,table=%s,max_num=%d", m_table_account.c_str(), max_account_num);
    std::lock_guard<std::mutex> lck(m_table_mutex);
    for (auto iter = m_waiting_accounts.begin(); iter != m_waiting_accounts.end();) {
        const std::string & account = iter->first;
        if (m_accounts_lock_mgr.is_account_lock(account)) {
            xdbg("xtxpool_table_t::get_accounts,account is locked. table=%s,account=%s", m_table_account.c_str(), account.c_str());
            iter++;
            continue;
        }

        xaccountobj_t * _account_obj = iter->second;
        if (_account_obj->empty()) {
            _account_obj->release_ref();
            iter = m_waiting_accounts.erase(iter);
        } else {
            accountobj_set.insert(_account_obj);
            iter++;
        }
    }

    for (auto & iter1 : accountobj_set) {
        xaccountobj_t * _account_obj = iter1;
        if (account_num >= max_account_num) {
            _account_obj->clear_selected_time();
            continue;
        }
        accounts.push_back(_account_obj->get_account_address());
        xdbg("xtxpool_table_t::get_accounts, table=%s,account=%s", m_table_account.c_str(), _account_obj->get_account_address().c_str());
    }
    return accounts;
}

std::vector<xcons_transaction_ptr_t> xtxpool_table_t::get_account_txs(const std::string & account, uint64_t commit_height, uint64_t unit_height, uint64_t last_nonce, const uint256_t & last_hash, uint64_t now) {
    std::lock_guard<std::mutex> lck(m_table_mutex);
    xaccountobj_t * _account_obj = find_account(account);
    if (_account_obj == nullptr) {
        return {};
    }

    _account_obj->clear_invalid_send_tx(now);
    std::vector<xcons_transaction_ptr_t> txs = _account_obj->get_cons_txs(last_nonce, last_hash);
    txs_validity_check_leader(_account_obj, txs, commit_height, unit_height, now);
    if (txs.empty()) {
        xwarn("xtxpool_table_t::get_account_txs,txs empty after duplicate check,table=%s,account=%s,height=%llu,%llu",
            m_table_account.c_str(), account.c_str(), commit_height, unit_height);
        return {};
    }
    for (auto & iter : txs) {
        xinfo("xtxpool_table_t::get_account_txs succ.table=%s,account=%s,height=%llu,%llu,last_nonce=%llu,tx=%s",
            m_table_account.c_str(), account.c_str(), commit_height, unit_height, last_nonce, iter->dump(false).c_str());
    }
    return txs;
}

int32_t xtxpool_table_t::verify_txs(const std::string & account, uint64_t commit_height, uint64_t unit_height, std::vector<xcons_transaction_ptr_t> txs,
                                    uint64_t last_nonce, const uint256_t & last_hash) {
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    return txs_validity_check_backup(txs, commit_height, unit_height, now, last_nonce, last_hash);
}

void xtxpool_table_t::on_block_confirmed(xblock_t * block) {
    if (block->is_unitblock()) {
        base::xauto_ptr<xblockchain2_t> blockchain(m_para->get_store()->clone_account(block->get_account()));
        xassert(blockchain != nullptr);
        update_committed_unit_block(block, blockchain->account_send_trans_number(), blockchain->account_send_trans_hash());
    } else if (block->is_tableblock()) {
        update_committed_table_block(block);
    }
}

void xtxpool_table_t::update_committed_table_block(xblock_t * block) {
    auto committed_recv_txs = m_consensused_recvtx_cache.update_tx_cache(block);
    delete_committed_recv_txs(committed_recv_txs);
}

void xtxpool_table_t::delete_committed_recv_txs(std::vector<std::pair<std::string, uint256_t>> & committed_recv_txs) {
    if (committed_recv_txs.empty()) {
        return;
    }

    std::lock_guard<std::mutex> lck(m_table_mutex);
    for (auto iter : committed_recv_txs) {
        xdbg("update_committed_table_block will delete duplicated tx: account=%s tx=%s",iter.first.c_str(), to_hex_str(iter.second).c_str());
        xaccountobj_t * _account_obj = find_account(iter.first);
        if (_account_obj != nullptr) {
            _account_obj->pop_tx_by_hash(iter.second, data::enum_transaction_subtype_recv, 0);
        }
    }
}

void xtxpool_table_t::update_committed_unit_block(xblock_t * block, uint64_t account_tx_nonce, const uint256_t & account_tx_hash) {
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    m_unconfirm_sendtx_cache.on_unit_update(block->get_account(), now);
    {
        std::lock_guard<std::mutex> lck(m_table_mutex);
        xaccountobj_t * _account_obj = find_account(block->get_account());
        if (_account_obj != nullptr) {
            _account_obj->on_unit_to_db(block, account_tx_nonce, account_tx_hash);
        }
    }
}

void xtxpool_table_t::on_timer_check_cache(xreceipt_tranceiver_face_t & tranceiver, xtxpool_receipt_receiver_counter & counter) {
    std::vector<xcons_transaction_ptr_t> receipts;
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    receipts = m_unconfirm_sendtx_cache.on_timer_check_cache(now, counter);
    if (receipts.empty()) {
        return;
    }
    for (auto & receipt : receipts) {
        if (!receipt->is_commit_prove_cert_set()) {
            xassert(receipt->is_recv_tx());
            std::string table_account = account_address_to_block_address(common::xaccount_address_t(receipt->get_source_addr()));
            uint64_t justify_table_height = receipt->get_unit_cert()->get_parent_block_height() + 2;
            base::xauto_ptr<base::xvblock_t> justify_tableblock = xblocktool_t::load_justify_block(m_para->get_vblockstore(), table_account, justify_table_height);
            if (justify_tableblock == nullptr) {
                xwarn("xtxpool_table_t::on_timer_check_cache can not load justify tableblock. tx=%s,account=%s,height=%ld",
                    receipt->dump().c_str(), table_account.c_str(), justify_table_height);
                continue;
            }
            receipt->set_commit_prove_with_parent_cert(justify_tableblock->get_cert());
        } else {
            xdbg("xtxpool_table_t::on_timer_check_cache tx receipt already set commit prove. tx=%s", receipt->dump().c_str());
        }

        tranceiver.send_receipt(receipt, get_receipt_resend_time(receipt->get_unit_cert()->get_gmtime(), now));
    }
}

void xtxpool_table_t::init() {
    base::xauto_ptr<xblockchain2_t> blockchain{m_para->get_store()->clone_account(m_table_account)};
    if (blockchain != nullptr) {
        std::set<std::string> accounts = blockchain->get_unconfirmed_accounts();
        if (!accounts.empty()) {
            xdbg("xtxpool_table_t::init unconfirmed accounts not empty size:%u", accounts.size());
            uint64_t now = xverifier::xtx_utl::get_gmttime_s();
            m_unconfirm_sendtx_cache.accounts_update(accounts, now);
        }
    }
}

void xtxpool_table_t::clear() {
    // not clear unconfirm cache
    // m_unconfirm_sendtx_cache.clear();

    std::lock_guard<std::mutex> lck(m_table_mutex);
    for (auto iter = m_waiting_accounts.begin(); iter != m_waiting_accounts.end();) {
        xdbg("xtxpool_table_t::clear,clear waiting account. table=%s,account=%s", m_table_account.c_str(), iter->first.c_str());
        xaccountobj_t * _account_obj = iter->second;
        _account_obj->release_ref();
        iter = m_waiting_accounts.erase(iter);
    }
}

xaccountobj_t * xtxpool_table_t::find_account_and_create_new(const std::string & address, uint64_t now) {
    xaccountobj_t * _accountobj = find_account(address);
    if (_accountobj == nullptr) {
        uint64_t latest_send_trans_number = 0;
        uint256_t latest_send_trans_hash;
        base::xauto_ptr<xblockchain2_t> blockchain{m_para->get_store()->clone_account(address)};
        if (blockchain != nullptr) {
            latest_send_trans_number = blockchain->account_send_trans_number();
            latest_send_trans_hash = blockchain->account_send_trans_hash();
        }
        _accountobj = new xaccountobj_t(address, latest_send_trans_number, latest_send_trans_hash, this);
        m_waiting_accounts[address] = _accountobj;
    }
    return _accountobj;
}

xaccountobj_t * xtxpool_table_t::find_account(const std::string & address) const {
    auto iter = m_waiting_accounts.find(address);
    if (iter != m_waiting_accounts.end()) {
        return iter->second;
    }
    return nullptr;
}

xcons_transaction_ptr_t xtxpool_table_t::pop_tx_by_hash(const std::string & address, const uint256_t & hash, uint8_t subtype, int32_t result) {
    std::lock_guard<std::mutex> lck(m_table_mutex);
    xaccountobj_t * _accountobj = find_account(address);
    if (_accountobj != nullptr) {
        return _accountobj->pop_tx_by_hash(hash, subtype, result);
    }
    return nullptr;
}

int32_t xtxpool_table_t::verify_cons_tx(const xcons_transaction_ptr_t & tx, uint64_t now) {
    int32_t ret;
    if (tx->is_send_tx() || tx->is_self_tx()) {
        ret = verify_send_tx(tx, now);
    } else if (tx->is_recv_tx()) {
        ret = verify_receipt_tx(tx);
    } else if (tx->is_confirm_tx()) {
        ret = verify_receipt_tx(tx);
    } else {
        ret = xtxpool_error_tx_invalid_type;
    }
    return ret;
}

bool xtxpool_table_t::nonce_continous(uint64_t & last_nonce, uint256_t & last_hash, const xcons_transaction_ptr_t & tx) {
    if (last_nonce == tx->get_transaction()->get_last_nonce() && tx->get_transaction()->check_last_trans_hash(last_hash)) {
        last_nonce = tx->get_transaction()->get_tx_nonce();
        last_hash = tx->get_transaction()->digest();
        return true;
    } else {
        return false;
    }
}

int32_t xtxpool_table_t::txs_validity_check_leader(xaccountobj_t * account_obj,
                                                   std::vector<xcons_transaction_ptr_t> & txs,
                                                   uint64_t commit_height,
                                                   uint64_t unit_height,
                                                   uint64_t now) {
    int32_t ret;
    uint64_t last_nonce = (uint64_t)-1;

    for (auto iter = txs.begin(); iter != txs.end();) {
        const auto & tx = *iter;
        if (tx->is_confirm_tx()) {
            ret = m_unconfirm_sendtx_cache.has_txhash_receipt(tx->get_transaction()->get_source_addr(), tx->get_transaction()->digest(), now, unit_height);
        } else if (tx->is_recv_tx()) {
            std::vector<std::pair<std::string, uint256_t>> committed_recv_txs;
            ret = m_consensused_recvtx_cache.is_receipt_duplicated(tx->get_clock(), tx->get_transaction(), committed_recv_txs, commit_height);
            delete_committed_recv_txs(committed_recv_txs);
        } else {
            ret = xsuccess;
        }

        if (ret == xtxpool_error_unconfirm_sendtx_not_exist || ret == xtxpool_error_sendtx_receipt_duplicate || ret == xtxpool_error_sendtx_receipt_expired) {
            xinfo("xtxpool_table_t::txs_validity_check_leader,delete duplicate receipt.table=%s,tx=%s,error:%s",
                    m_table_account.c_str(),
                    tx->dump().c_str(),
                    get_error_str(ret).c_str());
            account_obj->pop_tx_by_hash(tx->get_transaction()->digest(), tx->get_tx_subtype(), ret);
            iter = txs.erase(iter);
            continue;
        } else if (ret != xtxpool_error_unconfirm_sendtx_exist && ret != xsuccess) {
            xwarn("xtxpool_table_t::txs_validity_check_leader,receipt can't check duplicate.table=%s,tx=%s,error:%s",
                    m_table_account.c_str(),
                    tx->dump().c_str(),
                    get_error_str(ret).c_str());
            txs.clear();
            return ret;
        }
        iter++;
    }
    return xsuccess;
}

int32_t xtxpool_table_t::txs_validity_check_backup(std::vector<xcons_transaction_ptr_t> & txs,
                                                   uint64_t commit_height,
                                                   uint64_t unit_height,
                                                   uint64_t now,
                                                   uint64_t last_nonce,
                                                   const uint256_t & last_hash) {
    int32_t ret;
    uint64_t last_nonce_tmp = last_nonce;
    uint256_t last_hash_tmp = last_hash;

    for (auto iter = txs.begin(); iter != txs.end();) {
        const auto & tx = *iter;
        ret = verify_cons_tx(tx, now);
        if (ret) {
            return ret;
        }
        if ((tx->is_self_tx() || tx->is_send_tx())) {
            if (!nonce_continous(last_nonce_tmp, last_hash_tmp, tx)) {
                ret = xtxpool_error_tx_nonce_incontinuity;
                xwarn("xtxpool_table_t::txs_validity_check_backup,nonce incontinuity.table=%s,tx=%s,last_nonce=%llu,error:%s",
                    m_table_account.c_str(),
                    tx->dump().c_str(),
                    last_nonce_tmp,
                    get_error_str(ret).c_str());
                return ret;
            }
        } else if (tx->is_confirm_tx()) {
            ret = m_unconfirm_sendtx_cache.has_txhash_receipt(tx->get_transaction()->get_source_addr(), tx->get_transaction()->digest(), now, unit_height);
        } else if (tx->is_recv_tx()) {
            std::vector<std::pair<std::string, uint256_t>> committed_recv_txs;
            ret = m_consensused_recvtx_cache.is_receipt_duplicated(tx->get_clock(), tx->get_transaction(), committed_recv_txs, commit_height);
            delete_committed_recv_txs(committed_recv_txs);
        } else {
            ret = xtxpool_error_tx_invalid_type;
        }

        if (ret != xtxpool_error_unconfirm_sendtx_exist && ret != xsuccess) {
            xwarn("xtxpool_table_t::txs_validity_check_backup,receipt can't check duplicate.table=%s,tx=%s,error:%s",
                    m_table_account.c_str(),
                    tx->dump().c_str(),
                    get_error_str(ret).c_str());
            return ret;
        }
        iter++;
    }
    return xsuccess;
}

void xtxpool_table_t::update_lock_blocks(const base::xblock_mptrs & latest_blocks) {
    m_accounts_lock_mgr.update_blocks(latest_blocks);
}

int32_t xtxpool_table_t::verify_send_tx(const xcons_transaction_ptr_t & tx, uint64_t now) {
    // 1. validation check
    int32_t ret = xverifier::xtx_verifier::verify_send_tx_validation(tx->get_transaction());
    if (ret) {
        return ret;
    }
    // 2. legal check, include hash/signature check and white/black check
    ret = xverifier::xtx_verifier::verify_send_tx_legitimacy(tx->get_transaction(), make_observer(m_para->get_store()));
    if (ret) {
        return ret;
    }
    // 3. tx duration expire check
    ret = xverifier::xtx_verifier::verify_tx_duration_expiration(tx->get_transaction(), now);
    if (ret) {
        return ret;
    }
    return xsuccess;
}

int32_t xtxpool_table_t::verify_receipt_tx(const xcons_transaction_ptr_t & tx) {
    // only check digest here for process too long zec_workload contract transaction receipt
    // should recover length check at later version
    if (!tx->get_transaction()->digest_check()) {
        xwarn("xtxpool_table_t::verify_receipt_tx digest check fail, tx:%s", tx->dump().c_str());
        return xtxpool_error_receipt_invalid;
    }

    if (!tx->verify_cons_transaction()) {
        return xtxpool_error_receipt_invalid;
    }

    base::xvqcert_t* prove_cert;
    std::string prove_account;
    if (!tx->get_commit_prove_cert_and_account(prove_cert, prove_account)) {
        return xtxpool_error_tx_multi_sign_error;
    }

    // verify_muti_sign
    base::enum_vcert_auth_result auth_result = m_para->get_certauth()->verify_muti_sign(prove_cert, prove_account);
    if (auth_result != base::enum_vcert_auth_result::enum_successful) {
        int32_t ret = xtxpool_error_tx_multi_sign_error;
        xwarn("xtxpool_table_t::verify_receipt_tx fail. account=%s,tx=%s,auth_result:%d,fail-%s",
            prove_account.c_str(), tx->dump().c_str(), auth_result, get_error_str(ret).c_str());
        return ret;
    }
    return xsuccess;
}

void xtxpool_table_t::send_tx_increment(int64_t inc_num) {
    XMETRICS_COUNTER_INCREMENT("txpool_push_tx_send_cur", inc_num);
    XMETRICS_COUNTER_INCREMENT("table_send_tx_cur" + m_table_account, inc_num);
    m_send_tx_num += inc_num;
    xassert(m_send_tx_num >= 0);
}

void xtxpool_table_t::recv_tx_increment(int64_t inc_num) const {
    XMETRICS_COUNTER_INCREMENT("txpool_push_tx_recv_cur", inc_num);
    XMETRICS_COUNTER_INCREMENT("table_recv_tx_cur" + m_table_account, inc_num);
}

void xtxpool_table_t::conf_tx_increment(int64_t inc_num) const {
    XMETRICS_COUNTER_INCREMENT("txpool_push_tx_confirm_cur", inc_num);
    XMETRICS_COUNTER_INCREMENT("table_confirm_tx_cur" + m_table_account, inc_num);
}

const std::string & xtxpool_table_t::get_table_name() const {
    return m_table_account;
}

xcons_transaction_ptr_t xtxpool_table_t::query_tx(const std::string & account, const uint256_t & hash) const {
    std::lock_guard<std::mutex> lck(m_table_mutex);
    xaccountobj_t * _accountobj = find_account(account);
    if (_accountobj != nullptr) {
        return _accountobj->query_tx(hash);
    }
    return nullptr;
}

xcons_transaction_ptr_t xtxpool_table_t::get_unconfirm_tx(const std::string source_addr, const uint256_t & hash) {
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    return m_unconfirm_sendtx_cache.get_unconfirm_tx(source_addr, hash, now);
}

NS_END2
