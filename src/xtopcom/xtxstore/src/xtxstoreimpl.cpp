// Copyright (c) 2018-2020 Telos Foundation & contributors
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxstore/xtxstoreimpl.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvtxindex.h"
#include "xmetrics/xmetrics.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xblockextract.h"
#include "xpbase/base/top_utils.h"

NS_BEG2(top, txstore)

using common::xdefault_strategy_t;
using common::xnode_type_strategy_t;
using common::xnode_type_t;
using common::xstrategy_priority_enum_t;
using common::xstrategy_value_enum_t;

xtxstoreimpl::xtxstoreimpl(observer_ptr<mbus::xmessage_bus_face_t> const & mbus, observer_ptr<xbase_timer_driver_t> const & timer_driver)
  : base::xvtxstore_t()
  , m_txstore_strategy{common::define_bool_strategy(xdefault_strategy_t{xstrategy_value_enum_t::disable, xstrategy_priority_enum_t::low},
                                                    xnode_type_strategy_t{xnode_type_t::storage, xstrategy_value_enum_t::enable, xstrategy_priority_enum_t::normal})}
  , m_tx_prepare_mgr{std::make_shared<txexecutor::xtransaction_prepare_mgr>(mbus, timer_driver)}
  , m_tx_cache_strategy{common::define_bool_strategy(xdefault_strategy_t{xstrategy_value_enum_t::disable, xstrategy_priority_enum_t::low},
                                                     xnode_type_strategy_t{xnode_type_t::storage_exchange, xstrategy_value_enum_t::enable, xstrategy_priority_enum_t::normal})} {
}

xtxstoreimpl::~xtxstoreimpl() {
}

bool xtxstoreimpl::close(bool force_async)  // must call close before release object,otherwise object never be cleanup
{
    base::xobject_t::close(force_async);  // since mutiple base class has close(),we need call seperately

    xkinfo("xtxstoreimpl::close");
    return true;
}

void * xtxstoreimpl::query_interface(const int32_t _enum_xobject_type_) {
    if (_enum_xobject_type_ == base::enum_xobject_type_vtxstore)
        return this;

    return xobject_t::query_interface(_enum_xobject_type_);
}

base::xauto_ptr<base::xvtxindex_t> xtxstoreimpl::load_tx_idx(const std::string & raw_tx_hash, base::enum_transaction_subtype type) {
    base::enum_txindex_type txindex_type = base::xvtxkey_t::transaction_subtype_to_txindex_type(type);
    const std::string tx_idx_key = base::xvdbkey_t::create_tx_index_key(raw_tx_hash, txindex_type);
    const std::string tx_idx_bin = base::xvchain_t::instance().get_xdbstore()->get_value(tx_idx_key);
    if (tx_idx_bin.empty()) {
        xwarn("xvtxstore_t::load_tx_idx,index not find for hahs_tx=%s", base::xstring_utl::to_hex(raw_tx_hash).c_str());
        return nullptr;
    }
    base::xauto_ptr<base::xvtxindex_t> txindex(new base::xvtxindex_t());
    if (txindex->serialize_from_string(tx_idx_bin) <= 0) {
        xerror("xvtxstore_t::load_tx_idx,found bad index for hahs_tx=%s", base::xstring_utl::to_hex(raw_tx_hash).c_str());
        return nullptr;
    }
    txindex->set_tx_hash(raw_tx_hash);
    return txindex;
}


base::xauto_ptr<base::xvtxindex_t> xtxstoreimpl::load_relay_tx_idx(const std::string & raw_tx_hash, base::enum_transaction_subtype type) {
    base::enum_txindex_type txindex_type = base::xvtxkey_t::transaction_subtype_to_txindex_type(type);
    const std::string tx_idx_key = base::xvdbkey_t::create_prunable_relay_tx_index_key(raw_tx_hash, txindex_type);
    const std::string tx_idx_bin = base::xvchain_t::instance().get_xdbstore()->get_value(tx_idx_key);
    if (tx_idx_bin.empty()) {
        xwarn("xvtxstore_t::load_relay_tx_idx, index not find for hahs_tx=%s", base::xstring_utl::to_hex(raw_tx_hash).c_str());
        return nullptr;
    }
    base::xauto_ptr<base::xvtxindex_t> txindex(new base::xvtxindex_t());
    if (txindex->serialize_from_string(tx_idx_bin) <= 0) {
        xerror("xvtxstore_t::load_tx_idx,found bad index for hahs_tx=%s", base::xstring_utl::to_hex(raw_tx_hash).c_str());
        return nullptr;
    }
    txindex->set_tx_hash(raw_tx_hash);
    return txindex;
}
bool xtxstoreimpl::store_blockhash_index(base::xvbindex_t * this_index) {
    // XTODO only evm-table and relay need store blockhash now
    std::string block_hash;
    if (this_index->get_account() == sys_contract_eth_table_block_addr_with_suffix)
        block_hash = this_index->get_block_hash();
    else if (this_index->get_account() == sys_contract_relay_block_addr)
        block_hash = this_index->get_extend_data();
    else
        return false;

    if (!strategy_permission(m_txstore_strategy)) {
        xdbg("xtxstoreimpl::store_blockhash_index strategy_permission failed . don't store");
        return false;
    }

    if (block_hash.empty()) {
        xerror("xtxstoreimpl::store_blockhash_index index=%s",this_index->dump().c_str());
    }

    xassert(!block_hash.empty());
    std::string key_path2 = base::xvdbkey_t::create_prunable_blockhash_key(block_hash);
    base::xvchain_t::instance().get_xdbstore()->set_value(key_path2, this_index->get_account() + "/" + base::xstring_utl::uint642hex(this_index->get_height()));
    xdbg("xtxstoreimpl::store_blockhash_index, %s,blockhash=%s",
         this_index->dump().c_str(), base::xstring_utl::to_hex(block_hash).c_str());
    return true;
}
bool xtxstoreimpl::store_relay_txs(base::xvblock_t * block_ptr) {
    xassert(block_ptr != NULL);
    if (NULL == block_ptr)
        return false;

    if (block_ptr->get_height() == 0) {
        xdbg("xtxstoreimpl::store_relay_txs,genesis.block=%s", block_ptr->dump().c_str());
        return true;
    }

    if (!strategy_permission(m_txstore_strategy)) {
        xdbg("xtxstoreimpl::store_relay_txs strategy_permission failed . don't store");
        return false;
    }

    std::error_code ec;
    top::data::xrelay_block extra_relay_block;
    data::xblockextract_t::unpack_relayblock_from_wrapblock(block_ptr, extra_relay_block, ec);
    if (ec) {
        xerror("xtxstoreimpl::store_relay_txs fail-decodeBytes.error:%s", ec.message().c_str());
        return false;
    }

    if (extra_relay_block.get_all_transactions().empty()) {
        xinfo("xtxstoreimpl::store_relay_txs,no tx block.block=%s", extra_relay_block.dump().c_str());
        return true;
    }


    // get tx hash from txs
    bool has_error = false;
    for (auto & tx : extra_relay_block.get_all_transactions()) {
        auto tx_hash_u256 = tx.get_tx_hash();
        std::string tx_hash = std::string(reinterpret_cast<char *>(tx_hash_u256.data()), tx_hash_u256.size());
        base::xvtxindex_ptr tx_index = make_object_ptr<base::xvtxindex_t>(*block_ptr, tx_hash, base::enum_transaction_subtype_send);

        base::enum_txindex_type txindex_type = base::xvtxkey_t::transaction_subtype_to_txindex_type(tx_index->get_tx_phase_type());
        const std::string tx_key = base::xvdbkey_t::create_prunable_relay_tx_index_key(tx_index->get_tx_hash(), txindex_type);
        std::string tx_bin;
        tx_index->serialize_to_string(tx_bin);

        if (base::xvchain_t::instance().get_xdbstore()->set_value(tx_key, tx_bin) == false) {
            xerror("txstoreimpl::store_relay_txs,fail to store tx for block(%s)", block_ptr->dump().c_str());
            has_error = true;  // mark it but let do rest work
        } else {
            xinfo("txstoreimpl::store_relay_txs,succ store tx:%s,block=%s",
                base::xvtxkey_t::transaction_hash_subtype_to_string(tx_index->get_tx_hash(), tx_index->get_tx_phase_type()).c_str(), extra_relay_block.dump().c_str());
        }        
    }

    if (has_error)
        return false;
    return true;
}

bool xtxstoreimpl::store_txs(base::xvblock_t * block_ptr) {
    if (!strategy_permission(m_txstore_strategy)) {
        xdbg("xtxstoreimpl::store_txs strategy_permission failed . don't store txs");
        return false;
    }

    xassert(block_ptr != NULL);
    if (NULL == block_ptr)
        return false;

    if (block_ptr->get_block_class() == base::enum_xvblock_class_nil)  // nothing to store
        return true;

    std::vector<xobject_ptr_t<base::xvtxindex_t>> sub_txs;
    if (block_ptr->extract_sub_txs(sub_txs)) {
        // xassert(!sub_txs.empty()); //XTODO for old version(eg. 0x100), the table block may only has empty units and has none of sub_txs
        std::map<std::string, std::string> kvs;
        for (auto & v : sub_txs) {
            base::enum_txindex_type txindex_type = base::xvtxkey_t::transaction_subtype_to_txindex_type(v->get_tx_phase_type());
            const std::string tx_key = base::xvdbkey_t::create_tx_index_key(v->get_tx_hash(), txindex_type);
            std::string tx_bin;
            v->serialize_to_string(tx_bin);
            xassert(!tx_bin.empty());

#ifdef ENABLE_METRICS
            if (v->get_tx_phase_type() == base::enum_transaction_subtype_send) {
                XMETRICS_GAUGE(metrics::store_tx_index_send, 1);
            } else if (v->get_tx_phase_type() == base::enum_transaction_subtype_recv) {
                XMETRICS_GAUGE(metrics::store_tx_index_recv, 1);
            } else if (v->get_tx_phase_type() == base::enum_transaction_subtype_self) {
                XMETRICS_GAUGE(metrics::store_tx_index_self, 1);
            } else if (v->get_tx_phase_type() == base::enum_transaction_subtype_confirm) {
                XMETRICS_GAUGE(metrics::store_tx_index_confirm, 1);
            }
#endif

            kvs[tx_key] = tx_bin;
            xdbg_info("xvtxstore_t::store_txs_index,store tx to DB for block=%s,tx=%s",
                                block_ptr->dump().c_str(),
                                base::xvtxkey_t::transaction_hash_subtype_to_string(v->get_tx_hash(), v->get_tx_phase_type()).c_str());
        }

        if (sub_txs.size() > 0) {
            if (sub_txs.size() != kvs.size()) {
                xerror("xvtxstore_t::store_txs_index,repeats txs for block(%s),size=%zu,%zu", block_ptr->dump().c_str(),sub_txs.size(),kvs.size());
                return false;
            }
            bool has_error = base::xvchain_t::instance().get_xdbstore()->set_values(kvs);
            if (!has_error) {
                xerror("xvtxstore_t::store_txs_index,fail to store txs for block(%s)", block_ptr->dump().c_str());
                return false;
            }           
        }
        xinfo("xvtxstore_t::store_txs_index,tps_key succ store txs for block(%s), txs_size=%zu", block_ptr->dump().c_str(),sub_txs.size());
        return true;        
    } else {
        xerror("xvtxstore_t::store_txs_index,fail to extract subtxs for block(%s)", block_ptr->dump().c_str());
        return false;
    }
}

bool xtxstoreimpl::tx_cache_add(std::string const & tx_hash, data::xtransaction_ptr_t tx_ptr) {
    if (strategy_permission(m_tx_cache_strategy)) {
        return m_tx_prepare_mgr->transaction_cache()->tx_add(tx_hash, tx_ptr);
    }
    return false;
}

bool xtxstoreimpl::tx_cache_get(std::string const & tx_hash, std::shared_ptr<data::xtransaction_cache_data_t> tx_cache_data_ptr) {
    if (strategy_permission(m_tx_cache_strategy)) {
        return m_tx_prepare_mgr->transaction_cache()->tx_get(tx_hash, *tx_cache_data_ptr.get());  // todo change tx_get interface.
    }
    return false;
}

void xtxstoreimpl::update_node_type(uint32_t combined_node_type) noexcept {
    XLOCK_GUARD(m_node_type_mutex) {
        m_combined_node_type = static_cast<common::xnode_type_t>(combined_node_type);
        xdbg("xtxstoreimpl::update_node_type update to %s", common::to_string(m_combined_node_type).c_str());
    }
    if (strategy_permission(m_tx_cache_strategy)) {
        m_tx_prepare_mgr->start();
    } else {
        m_tx_prepare_mgr->stop();
    }
}

bool xtxstoreimpl::strategy_permission(common::xbool_strategy_t const & strategy) const noexcept {
    XLOCK_GUARD(m_node_type_mutex) {
        return strategy.allow(m_combined_node_type);
    }
    return false;
}

std::vector<base::xvblock_ptr_t> xtxstoreimpl::load_block_objects(const std::string & tx_hash, const base::enum_transaction_subtype type) {
    auto blocks = top::base::xvchain_t::instance().get_xblockstore()->load_block_object(tx_hash, type, metrics::blockstore_access_from_sync_load_tx);
    for (auto & block : blocks) {
        if (false == top::base::xvchain_t::instance().get_xblockstore()->load_block_output(base::xvaccount_t(block->get_account()), block.get(), metrics::blockstore_access_from_sync_load_tx_output)
            || false == top::base::xvchain_t::instance().get_xblockstore()->load_block_input(base::xvaccount_t(block->get_account()), block.get(), metrics::blockstore_access_from_sync_load_tx_input)
            || false == top::base::xvchain_t::instance().get_xblockstore()->load_block_output_offdata(base::xvaccount_t(block->get_account()), block.get(), metrics::blockstore_access_from_sync_load_tx_output)) {
            xerror("xtxstoreimpl::load_block_objects for txhash fail-load block input or output. block=%s", block->dump().c_str());
            return {};
        }
    }
    return blocks;
}

int xtxstoreimpl::load_block_by_hash(const std::string& hash, std::vector<base::xvblock_ptr_t>& blocks) {
    base::xauto_ptr<base::xvtxindex_t> index = load_tx_idx(hash, base::enum_transaction_subtype_send);
    if ((nullptr != index) && index->is_self_tx()) {
        auto xvblocks = load_block_objects(hash, (base::enum_transaction_subtype)base::enum_transaction_subtype_self);
        xdbg("self load_block_objects: %d", xvblocks.size());
        for (uint32_t i = 0; i < xvblocks.size(); i++) {
            blocks.push_back(xvblocks[i]);
        }
        return 0;
    }
    
    for(int index = base::enum_transaction_subtype_send; index <= base::enum_transaction_subtype_confirm; index++) {
        auto xvblocks = load_block_objects(hash, (base::enum_transaction_subtype)index);
        xdbg("load_block_objects: %d", xvblocks.size());
        for (uint32_t i = 0; i < xvblocks.size(); i++) {
            blocks.push_back(xvblocks[i]);
        }
    }
    return 0;
}

NS_END2
