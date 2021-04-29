// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xcontext.h"
#include "xbase/xns_macro.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvblockstore.h"

#include <cstdint>
#include <string>

NS_BEG3(top, tests, vledger)

class xtop_dummy_blockstore : public base::xvblockstore_t {
public:
    xtop_dummy_blockstore(xtop_dummy_blockstore const &) = delete;
    xtop_dummy_blockstore & operator=(xtop_dummy_blockstore const &) = delete;

    xtop_dummy_blockstore(int32_t const target_thread_id) : base::xvblockstore_t(base::xcontext_t::instance(), target_thread_id) {
    }

protected:
    ~xtop_dummy_blockstore() override = default;

public:
    std::string get_store_path() const {
        return {};
    }

    base::xauto_ptr<base::xvblock_t> get_genesis_block(base::xvaccount_t const & account) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> get_latest_cert_block(base::xvaccount_t const &) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> get_latest_locked_block(base::xvaccount_t const &) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> get_latest_committed_block(base::xvaccount_t const &) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> get_latest_executed_block(base::xvaccount_t const &)  override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> get_latest_connected_block(base::xvaccount_t const &) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> get_latest_genesis_connected_block(const base::xvaccount_t & account) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> get_latest_committed_full_block(const base::xvaccount_t & account) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> get_latest_full_block(base::xvaccount_t const &) override {
        return nullptr;
    }

    base::xblock_mptrs get_latest_blocks(const base::xvaccount_t &) override {
        return {};
    }

    base::xblock_vector query_block(const base::xvaccount_t & account, const uint64_t height) override {
        return {};
    }

    base::xauto_ptr<base::xvblock_t> query_block(const base::xvaccount_t &, const uint64_t, const uint64_t) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> query_block(const base::xvaccount_t &, const uint64_t, const std::string &) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> query_block(const base::xvaccount_t &, const uint64_t, base::enum_xvblock_flag) override {
        return nullptr;
    }

    base::xblock_vector load_block_object(const base::xvaccount_t &, const uint64_t) override {
        return {};
    }

    base::xauto_ptr<base::xvblock_t> load_block_object(const base::xvaccount_t &, const uint64_t, const uint64_t, bool) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> load_block_object(const base::xvaccount_t &, const uint64_t, const std::string &, bool) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> load_block_object(const base::xvaccount_t &, const uint64_t, base::enum_xvblock_flag, bool) override {
        return nullptr;
    }

    bool load_block_input(const base::xvaccount_t &, base::xvblock_t *) override {
        return false;
    }

    bool load_block_output(const base::xvaccount_t &, base::xvblock_t *) override {
        return false;
    }

    bool load_block_offdata(const base::xvaccount_t & account, base::xvblock_t * block) override {
        return false;
    }

    bool store_block(const base::xvaccount_t & account, base::xvblock_t * block) override {
        return false;
    }

    bool delete_block(const base::xvaccount_t & account, base::xvblock_t * block) override {
        return false;
    }

    bool store_blocks(const base::xvaccount_t &, std::vector<base::xvblock_t *> &) override {
        return false;
    }

    base::xvbindex_vector load_block_index(const base::xvaccount_t &, const uint64_t) override {
        return {};
    }

    base::xauto_ptr<base::xvbindex_t> load_block_index(const base::xvaccount_t &, const uint64_t, const uint64_t) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvbindex_t> load_block_index(const base::xvaccount_t &, const uint64_t, const std::string &) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvbindex_t> load_block_index(const base::xvaccount_t &, const uint64_t, base::enum_xvblock_flag) override {
        return nullptr;
    }

    bool clean_caches(const base::xvaccount_t & account) override {
        return false;
    }

    bool reset_cache_timeout(const base::xvaccount_t &, const uint32_t) override {
        return false;
    }

    bool execute_block(const base::xvaccount_t &, base::xvblock_t *) override {
        return false;
    }

    base::xvtransaction_store_ptr_t  query_tx(const std::string &, base::enum_transaction_subtype) override {
        return nullptr;
    }
    
    bool exist_genesis_block(base::xvaccount_t const & account) override {
        return false;
    }
};
using xdummy_block_store_t = xtop_dummy_blockstore;

NS_END3
