// Copyright (c) 2018-2020 Telos Foundation & contributors
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xnode_type.h"
#include "xcommon/xstrategy.hpp"
#include "xvledger/xvblock.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvtxindex.h"
#include "xvledger/xvtxstore.h"

NS_BEG2(top, txstore)

class xtxstoreimpl : public base::xvtxstore_t {
public:
    xtxstoreimpl();
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
    const std::string load_tx_bin(const std::string & raw_tx_hash) override;
    base::xauto_ptr<base::xdataunit_t> load_tx_obj(const std::string & raw_tx_hash) override;

public:  // write interface
    bool store_txs(base::xvblock_t * block_ptr, bool store_raw_tx_bin) override;
    bool store_tx_bin(const std::string & raw_tx_hash, const std::string & raw_tx_bin) override;
    bool store_tx_obj(const std::string & raw_tx_hash, base::xdataunit_t * raw_tx_obj) override;

public:
    void update_node_type(uint32_t combined_node_type) noexcept override;

private:
    bool strategy_permission() const noexcept;

private:
    mutable std::mutex m_node_type_mutex{};
    common::xnode_type_t m_combined_node_type;
    common::xbool_strategy_t m_strategy;
};

NS_END2