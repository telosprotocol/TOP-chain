// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include "xdata/xblock.h"
#include "xstore/xstore_face.h"
#include "xblockstore/xblockstore_face.h"
#include "xtxpool/xtxpool_face.h"
#include "xtxexecutor/xblock_maker_para.h"

NS_BEG2(top, txexecutor)

using data::xblock_ptr_t;
using data::xaccount_ptr_t;

class xblockmaker_resources_t {
 public:
    virtual store::xstore_face_t*       get_store() const = 0;
    virtual base::xvblockstore_t*       get_blockstore() const = 0;
    virtual xtxpool::xtxpool_face_t*     get_txpool() const = 0;
};
using xblockmaker_resources_ptr_t = std::shared_ptr<xblockmaker_resources_t>;

class xblockmaker_resources_impl_t : public xblockmaker_resources_t {
 public:
    xblockmaker_resources_impl_t(const observer_ptr<store::xstore_face_t> & store, const observer_ptr<base::xvblockstore_t> & blockstore, const observer_ptr<xtxpool::xtxpool_face_t> & txpool)
    : m_store(store), m_blockstore(blockstore), m_txpool(txpool) {}

    virtual store::xstore_face_t*       get_store() const {return m_store.get();}
    virtual base::xvblockstore_t*       get_blockstore() const {return m_blockstore.get();}
    virtual xtxpool::xtxpool_face_t*     get_txpool() const {return m_txpool.get();}

 private:
    observer_ptr<store::xstore_face_t>          m_store{nullptr};
    observer_ptr<base::xvblockstore_t>          m_blockstore{nullptr};
    observer_ptr<xtxpool::xtxpool_face_t>       m_txpool{nullptr};
};

struct xunitmaker_result_t {
    xblock_ptr_t                            m_block{nullptr};
    int32_t                                 m_make_block_error_code{0};
    std::vector<xcons_transaction_ptr_t>    m_success_txs;
    xcons_transaction_ptr_t                 m_fail_tx{nullptr};
    int32_t                                 m_fail_tx_error{0};
};

struct xunitmaker_para_t {
    std::vector<xcons_transaction_ptr_t>    m_account_txs;
};

struct xtablemaker_result_t {
    xblock_ptr_t                            m_block{nullptr};
    int32_t                                 m_make_block_error_code{0};
    std::vector<xunitmaker_result_t>        m_unit_results;
};

struct xtablemaker_para_t {
    explicit xtablemaker_para_t(const base::xblock_mptrs & latest_blocks) {
        m_latest_cert_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_cert_block());
        m_latest_locked_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_locked_block());
        m_latest_committed_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_committed_block());
    }
    xtablemaker_para_t(const xblock_ptr_t & cert_block, const xblock_ptr_t & lock_block, const xblock_ptr_t & committed_block)
    : m_latest_cert_block(cert_block), m_latest_locked_block(lock_block), m_latest_committed_block(committed_block) {
    }
    void set_unitmaker_info(const std::string & account, uint64_t last_block_height, const std::string & last_block_hash, const std::vector<xcons_transaction_ptr_t> & input_txs) {
        xunit_proposal_input_t unit_input(account, last_block_height, last_block_hash, input_txs);
        m_proposal_input.add_unit_input(unit_input);
    }
    void set_unitmaker_txs(const std::string & account, const std::vector<xcons_transaction_ptr_t> & input_txs) {
        xunit_proposal_input_t unit_input(account, 0, {}, input_txs);
        m_proposal_input.add_unit_input(unit_input);
    }

    xblock_ptr_t                            m_latest_cert_block{nullptr};
    xblock_ptr_t                            m_latest_locked_block{nullptr};
    xblock_ptr_t                            m_latest_committed_block{nullptr};
    xtableblock_proposal_input_t            m_proposal_input;
};

class xblock_maker_t : public base::xvaccount_t {
 public:
    explicit xblock_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources, uint32_t latest_blocks_max)
    : base::xvaccount_t(account), m_resources(resources), m_keep_latest_blocks_max(latest_blocks_max) {}
    virtual ~xblock_maker_t() {}

 public:
    virtual xblock_ptr_t        make_next_block(const data::xblock_consensus_para_t & cs_para, int32_t & error_code) = 0;
    virtual xblock_ptr_t        make_next_block(const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result) = 0;
    virtual xblock_ptr_t        make_next_block(const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & result) = 0;
    virtual int32_t             check_latest_state(base::xvblock_t* latest_cert_block) = 0;
    virtual bool                can_make_next_block() const = 0;
    virtual bool                can_make_next_empty_block() const = 0;
    virtual bool                can_make_next_full_block() const = 0;
    virtual bool                can_make_next_light_block() const = 0;

 public:
    xblock_ptr_t                set_latest_block(const xblock_ptr_t & block);  // return deleted
    void                        clear_block(const xblock_ptr_t & block);
    bool                        load_latest_blocks(const xblock_ptr_t & latest_block, std::map<uint64_t, xblock_ptr_t> & latest_blocks);  // load latest blocks from db for updating cache

 public:
    store::xstore_face_t*       get_store() const {return m_resources->get_store();}
    base::xvblockstore_t*       get_blockstore() const {return m_resources->get_blockstore();}
    xtxpool::xtxpool_face_t*    get_txpool() const {return m_resources->get_txpool();}
    const xblockmaker_resources_ptr_t & get_resources() const {return m_resources;}

    bool                        has_uncommitted_blocks() const;
    bool                        is_latest_state_unchanged(base::xvblock_t* latest_block) const;
    uint32_t                    get_latest_consecutive_empty_block_num() const;
    uint64_t                    get_keep_latest_blocks_max() const {return m_keep_latest_blocks_max;}
    const xblock_ptr_t &        get_latest_committed_block() const {return m_latest_commit_block;}
    const xaccount_ptr_t &      get_latest_committed_state() const {return m_commit_account;}
    std::string                 get_lock_block_sign_hash() const;
    std::string                 get_lock_output_root_hash() const;
    const std::map<uint64_t, xblock_ptr_t> & get_latest_blocks() const {return m_latest_blocks;}
    xblock_ptr_t                get_latest_block(uint64_t height) const;
    const xblock_ptr_t &        get_proposal_prev_block() const;
    const xblock_ptr_t &        get_proposal_prev_prev_block() const;
    std::vector<xblock_ptr_t>   get_uncommit_blocks() const;
    xblock_ptr_t                get_lock_block() const;

 protected:
    virtual bool                update_latest_state(const xblock_ptr_t & latest_committed_block);
    bool                        update_account_state(const xblock_ptr_t & latest_committed_block);
    void                        set_latest_blocks(const base::xblock_mptrs & latest_blocks);
    bool                        is_latest_blocks_valid(const base::xblock_mptrs & latest_blocks);

 private:
    xblockmaker_resources_ptr_t             m_resources{nullptr};
    xblock_ptr_t                            m_latest_commit_block{nullptr};
    std::map<uint64_t, xblock_ptr_t>        m_latest_blocks;
    uint32_t                                m_keep_latest_blocks_max{0};
    xaccount_ptr_t                          m_commit_account{nullptr};
};

NS_END2
