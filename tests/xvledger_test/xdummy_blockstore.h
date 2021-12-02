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

    base::xauto_ptr<base::xvblock_t> get_genesis_block(base::xvaccount_t const & account,const int atag = 0) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> get_latest_cert_block(base::xvaccount_t const &,const int atag = 0) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> get_latest_locked_block(base::xvaccount_t const &,const int atag = 0) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> get_latest_committed_block(base::xvaccount_t const &,const int atag = 0) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> get_latest_connected_block(base::xvaccount_t const &,const int atag = 0) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> get_latest_genesis_connected_block(const base::xvaccount_t & account, bool ask_full_search,const int atag = 0) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvbindex_t> get_latest_genesis_connected_index(const base::xvaccount_t & account,bool ask_full_search,const int atag = 0) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> get_latest_committed_full_block(const base::xvaccount_t & account,const int atag = 0) override {
        return nullptr;
    }

    uint64_t get_latest_committed_block_height(const base::xvaccount_t & account,const int atag = 0) override {
        return 0;
    }

    uint64_t get_latest_connected_block_height(const base::xvaccount_t & account,const int atag = 0) override {
        return 0;
    }

    uint64_t get_latest_genesis_connected_block_height(const base::xvaccount_t & account,const int atag = 0) override {
        return 0;
    }
    
    uint64_t get_latest_executed_block_height(const base::xvaccount_t & account,const int atag = 0) override {
        return 0;
    }

    bool set_latest_executed_info(const base::xvaccount_t & account,uint64_t height,const std::string & blockhash,const int atag = 0) override {
        return false;
    }

    base::xblock_mptrs get_latest_blocks(const base::xvaccount_t &,const int atag = 0) override {
        return {};
    }

    base::xblock_vector query_block(const base::xvaccount_t & account, const uint64_t height,const int atag = 0) override {
        return {};
    }

    base::xauto_ptr<base::xvblock_t> query_block(const base::xvaccount_t &, const uint64_t, const uint64_t,const int atag = 0) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> query_block(const base::xvaccount_t &, const uint64_t, const std::string &,const int atag = 0) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> query_block(const base::xvaccount_t &, const uint64_t, base::enum_xvblock_flag,const int atag = 0) override {
        return nullptr;
    }

    base::xblock_vector load_block_object(const base::xvaccount_t &, const uint64_t,const int atag = 0) override {
        return {};
    }

    base::xauto_ptr<base::xvblock_t> load_block_object(const base::xvaccount_t &, const uint64_t, const uint64_t, bool,const int atag = 0) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> load_block_object(const base::xvaccount_t &, const uint64_t, const std::string &, bool,const int atag = 0) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> load_block_object(const base::xvaccount_t &, const uint64_t, base::enum_xvblock_flag, bool,const int atag = 0) override {
        return nullptr;
    }

    std::vector<base::xvblock_ptr_t> load_block_object(const std::string & tx_hash,const base::enum_transaction_subtype type,const int atag = 0) override {
        return {};
    }

    bool load_block_input(const base::xvaccount_t &, base::xvblock_t *,const int atag = 0) override {
        return false;
    }

    bool load_block_output(const base::xvaccount_t &, base::xvblock_t *,const int atag = 0) override {
        return false;
    }

    bool store_block(const base::xvaccount_t & account, base::xvblock_t * block,const int atag = 0) override {
        return false;
    }

    bool delete_block(const base::xvaccount_t & account, base::xvblock_t * block,const int atag = 0) override {
        return false;
    }

    bool store_blocks(const base::xvaccount_t &, std::vector<base::xvblock_t *> &,const int atag = 0) override {
        return false;
    }

    bool try_update_account_index(const base::xvaccount_t & account, uint64_t height, uint64_t viewid, bool update_pre_block) override {
        return false;
    }

    bool store_committed_unit_block(const base::xvaccount_t & account, base::xvblock_t * container_block) override {
        return false;
    }

    base::xvbindex_vector load_block_index(const base::xvaccount_t &, const uint64_t,const int atag = 0) override {
        return {};
    }

    base::xauto_ptr<base::xvbindex_t> load_block_index(const base::xvaccount_t &, const uint64_t, const uint64_t,const int atag = 0) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvbindex_t> load_block_index(const base::xvaccount_t &, const uint64_t, const std::string &,const int atag = 0) override {
        return nullptr;
    }

    base::xauto_ptr<base::xvbindex_t> load_block_index(const base::xvaccount_t &, const uint64_t, base::enum_xvblock_flag,const int atag = 0) override {
        return nullptr;
    }

    bool clean_caches(const base::xvaccount_t & account,const int atag = 0) override {
        return false;
    }

    base::xvtransaction_store_ptr_t  query_tx(const std::string &, base::enum_transaction_subtype,const int atag = 0) override {
        return nullptr;
    }

    bool exist_genesis_block(base::xvaccount_t const & account,const int atag = 0) override {
        return false;
    }
    
    // genesis connected  blocks
    bool set_genesis_height(const base::xvaccount_t & account, const std::string &height) override {
        return true;
    }

    const std::string    get_genesis_height(const base::xvaccount_t & account) override {
        return "";
    }

    bool        set_block_span(const base::xvaccount_t & account, const uint64_t height,  const std::string &span) override {
        return true;
    }

    bool        delete_block_span(const base::xvaccount_t & account, const uint64_t height) override {
        return true;
    }

    const std::string get_block_span(const base::xvaccount_t & account, const uint64_t height) override {
        return "";
    }

    bool set_unit_proof(const base::xvaccount_t & account, const std::string & unit_proof, const uint64_t height) override {
        return true;
    }

    const std::string get_unit_proof(const base::xvaccount_t & account, const uint64_t height) override {
        return "";
    }
};
using xdummy_block_store_t = xtop_dummy_blockstore;

NS_END3
