// Copyright (c) 2018-2020 Telos Foundation & contributors
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxstore/xtxstoreimpl.h"

#include "xmetrics/xmetrics.h"

NS_BEG2(top, txstore)

using common::xdefault_strategy_t;
using common::xnode_type_strategy_t;
using common::xnode_type_t;
using common::xstrategy_priority_enum_t;
using common::xstrategy_value_enum_t;

xtxstoreimpl::xtxstoreimpl(observer_ptr<mbus::xmessage_bus_face_t> const & mbus, observer_ptr<xbase_timer_driver_t> const & timer_driver)
  : base::xvtxstore_t()
  , m_txstore_strategy{common::define_bool_strategy(xdefault_strategy_t{xstrategy_value_enum_t::enable, xstrategy_priority_enum_t::low},
                                                    xnode_type_strategy_t{xnode_type_t::consensus, xstrategy_value_enum_t::enable, xstrategy_priority_enum_t::normal})}
  , m_tx_prepare_mgr{std::make_shared<txexecutor::xtransaction_prepare_mgr>(mbus, timer_driver)}
  , m_tx_cache_strategy{common::define_bool_strategy(xdefault_strategy_t{xstrategy_value_enum_t::disable, xstrategy_priority_enum_t::low},
                                                     xnode_type_strategy_t{xnode_type_t::full_node, xstrategy_value_enum_t::enable, xstrategy_priority_enum_t::normal})} {
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

const std::string xtxstoreimpl::load_tx_bin(const std::string & raw_tx_hash) {
    xassert(raw_tx_hash.empty() == false);
    if (raw_tx_hash.empty())
        return std::string();

    const std::string raw_tx_key = base::xvdbkey_t::create_tx_key(raw_tx_hash);
    xdbg("xvtxstore_t::load_tx_bin, to load raw tx bin for (%s)", base::xstring_utl::to_hex(raw_tx_key).c_str());
    return base::xvchain_t::instance().get_xdbstore()->get_value(raw_tx_key);
}

base::xauto_ptr<base::xdataunit_t> xtxstoreimpl::load_tx_obj(const std::string & raw_tx_hash) {
    const std::string raw_tx_bin = load_tx_bin(raw_tx_hash);
    if (raw_tx_bin.empty()) {
        xwarn("xvtxstore_t::load_tx_obj,fail to load raw tx bin for hash(%s)", base::xstring_utl::to_hex(raw_tx_hash).c_str());
        return nullptr;
    }
    return base::xdataunit_t::read_from(raw_tx_bin);
}

bool xtxstoreimpl::store_txs(base::xvblock_t * block_ptr) {
    if (!strategy_permission(m_txstore_strategy)) {
        return false;
    }

    xassert(block_ptr != NULL);
    if (NULL == block_ptr)
        return false;

    if (block_ptr->get_block_class() == base::enum_xvblock_class_nil)  // nothing to store
        return true;

    std::vector<xobject_ptr_t<base::xvtxindex_t>> sub_txs;
    if (block_ptr->extract_sub_txs(sub_txs)) {
        bool has_error = false;
        for (auto & v : sub_txs) {
            base::enum_txindex_type txindex_type = base::xvtxkey_t::transaction_subtype_to_txindex_type(v->get_tx_phase_type());
            const std::string tx_key = base::xvdbkey_t::create_tx_index_key(v->get_tx_hash(), txindex_type);
            std::string tx_bin;
            v->serialize_to_string(tx_bin);
            xassert(!tx_bin.empty());

            if (v->get_tx_phase_type() == base::enum_transaction_subtype_send) {
                XMETRICS_GAUGE(metrics::store_tx_index_send, 1);
            } else if (v->get_tx_phase_type() == base::enum_transaction_subtype_recv) {
                XMETRICS_GAUGE(metrics::store_tx_index_recv, 1);
            } else if (v->get_tx_phase_type() == base::enum_transaction_subtype_self) {
                XMETRICS_GAUGE(metrics::store_tx_index_self, 1);
            } else if (v->get_tx_phase_type() == base::enum_transaction_subtype_confirm) {
                XMETRICS_GAUGE(metrics::store_tx_index_confirm, 1);
            }

            if (base::xvchain_t::instance().get_xdbstore()->set_value(tx_key, tx_bin) == false) {
                xerror("xvtxstore_t::store_txs_index,fail to store tx for block(%s)", block_ptr->dump().c_str());
                has_error = false;  // mark it but let do rest work
            } else {
                xinfo("xvtxstore_t::store_txs_index,store tx to DB for block=%s,tx=%s",
                      block_ptr->dump().c_str(),
                      base::xvtxkey_t::transaction_hash_subtype_to_string(v->get_tx_hash(), v->get_tx_phase_type()).c_str());
            }

#ifdef LONG_CONFIRM_CHECK
            if (v->get_tx_phase_type() == base::enum_transaction_subtype_confirm) {
                base::xauto_ptr<base::xvtxindex_t> send_txindex = base::xvchain_t::instance().get_xtxstore()->load_tx_idx(v->get_tx_hash(), base::enum_transaction_subtype_send);
                if (send_txindex == nullptr) {
                    xwarn("xvtxstore_t::store_txs,fail find sendtx index. tx=%s", base::xstring_utl::to_hex(v->get_tx_hash()).c_str());
                } else {
                    uint64_t confirmtx_clock = block_ptr->get_clock();
                    uint64_t sendtx_clock = send_txindex->get_block_clock();
                    uint64_t delay_time = confirmtx_clock > sendtx_clock ? confirmtx_clock - sendtx_clock : 0;
                    static std::atomic<uint64_t> max_time{0};
                    if (max_time < delay_time) {
                        max_time = delay_time;
                    }

                    if (delay_time >= 6)  // 6 clock
                    {
                        xwarn("xvtxstore_t::store_txs,confirm tx time long.max_time=%ld,time=%ld,tx=%s",
                              (uint64_t)max_time,
                              delay_time,
                              base::xstring_utl::to_hex(v->get_tx_hash()).c_str());
                    }
                }
            }
#endif
        }
        if (has_error)
            return false;
        return true;
    } else {
        xerror("xvtxstore_t::store_txs_index,fail to extract subtxs for block(%s)", block_ptr->dump().c_str());
        return false;
    }
}

bool xtxstoreimpl::store_tx_bin(const std::string & raw_tx_hash, const std::string & raw_tx_bin) {
    xassert(raw_tx_hash.empty() == false);
    xassert(raw_tx_bin.empty() == false);
    if (raw_tx_hash.empty() || raw_tx_bin.empty())
        return false;

    const std::string raw_tx_key = base::xvdbkey_t::create_tx_key(raw_tx_hash);
    const std::string existing_value = base::xvchain_t::instance().get_xdbstore()->get_value(raw_tx_key);
    if (existing_value == raw_tx_bin)  // has stored already
        return true;

    // replace by new one
    XMETRICS_GAUGE(metrics::store_tx_origin, 1);
    return base::xvchain_t::instance().get_xdbstore()->set_value(raw_tx_key, raw_tx_bin);
}

bool xtxstoreimpl::store_tx_obj(const std::string & raw_tx_hash, base::xdataunit_t * raw_tx_obj) {
    xassert(raw_tx_hash.empty() == false);
    if (raw_tx_hash.empty() || (raw_tx_obj == NULL)) {
        xdbg("xvtxstore_t::store_tx_obj, null tx hash or tx obj %s", base::xstring_utl::to_hex(raw_tx_hash).c_str());
        return false;
    }

    // check whether has stored
    const std::string raw_tx_key = base::xvdbkey_t::create_tx_key(raw_tx_hash);
    std::string raw_tx_bin;
    raw_tx_obj->serialize_to_string(raw_tx_bin);

    const std::string existing_value = base::xvchain_t::instance().get_xdbstore()->get_value(raw_tx_key);
    if (existing_value == raw_tx_bin)  // nothing changed
        return true;

    xdbg("xvtxstore_t::store_tx_obj,%s", base::xstring_utl::to_hex(raw_tx_key).c_str());
    // replace by new one
    XMETRICS_GAUGE(metrics::store_tx_origin, 1);
    return base::xvchain_t::instance().get_xdbstore()->set_value(raw_tx_key, raw_tx_bin);
}

bool xtxstoreimpl::tx_cache_add(std::string const & tx_hash, data::xtransaction_ptr_t tx_ptr) {
    if (strategy_permission(m_tx_cache_strategy)) {
        return m_tx_prepare_mgr->transaction_cache()->tx_add(tx_hash, tx_ptr);
    }
    return false;
}

bool xtxstoreimpl::tx_cache_get(std::string const & tx_hash, std::shared_ptr<data::xtransaction_cache_data_t> tx_cache_data_ptr) {
    if (strategy_permission(m_tx_cache_strategy)) {
        return m_tx_prepare_mgr->transaction_cache()->tx_get(tx_hash, *tx_cache_data_ptr.get()); // todo change tx_get interface.
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

NS_END2
