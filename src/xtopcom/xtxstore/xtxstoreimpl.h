// Copyright (c) 2018-2020 Telos Foundation & contributors
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xnode_type.h"
#include "xcommon/xstrategy.hpp"
#include "xvledger/xvblock.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvtxindex.h"
#include "xvledger/xvtxstore.h"

#include "xtxstore/xtransaction_prepare_mgr.h"

NS_BEG2(top, txstore)

class xtxstoreimpl : public base::xvtxstore_t {
public:
    xtxstoreimpl(observer_ptr<mbus::xmessage_bus_face_t> const & mbus, observer_ptr<xbase_timer_driver_t> const & timer_driver);
    xtxstoreimpl(xtxstoreimpl const &) = delete;
    xtxstoreimpl & operator=(xtxstoreimpl const &) = delete;
    xtxstoreimpl(xtxstoreimpl &&) = delete;
    xtxstoreimpl & operator=(xtxstoreimpl &&) = delete;
    ~xtxstoreimpl();

public:
    virtual bool close(bool force_async = true) override;  // must call close before release object,otherwise object never be cleanup

public:
    // caller need to cast (void*) to related ptr
    void * query_interface(const int32_t _enum_xobject_type_) override;

public:  // read & load interface
    base::xauto_ptr<base::xvtxindex_t> load_tx_idx(const std::string & raw_tx_hash, base::enum_transaction_subtype type) override;
    base::xauto_ptr<base::xvtxindex_t> load_relay_tx_idx(const std::string & raw_tx_hash, base::enum_transaction_subtype type) override;
    bool store_blockhash_index(base::xvbindex_t * this_index) override;
public:  // write interface
    bool store_txs(base::xvblock_t * block_ptr) override;
    bool store_relay_txs(base::xvblock_t * block_ptr) override;
public: // tx cache
    bool tx_cache_add(std::string const & tx_hash, data::xtransaction_ptr_t tx_ptr) override;
    bool tx_cache_get(std::string const & tx_hash, std::shared_ptr<data::xtransaction_cache_data_t> tx_cache_data_ptr) override;

public:
    void update_node_type(uint32_t combined_node_type) noexcept override;
    int load_block_by_hash(const std::string& hash, std::vector<base::xvblock_ptr_t>& blocks) override;
private:
    bool strategy_permission(common::xbool_strategy_t const & strategy) const noexcept;
    std::vector<base::xvblock_ptr_t> load_block_objects(const std::string & tx_hash, const base::enum_transaction_subtype type);
private:
    mutable std::mutex m_node_type_mutex{};
    common::xnode_type_t m_combined_node_type;
    common::xbool_strategy_t m_txstore_strategy;
    std::shared_ptr<txexecutor::xtransaction_prepare_mgr> m_tx_prepare_mgr;
    common::xbool_strategy_t m_tx_cache_strategy;
};

NS_END2