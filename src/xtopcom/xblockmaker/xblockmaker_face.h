// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include "xdata/xblock.h"
#include "xdata/xtablestate.h"
#include "xstore/xstore_face.h"
#include "xblockstore/xblockstore_face.h"
#include "xindexstore/xindexstore_face.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xblockmaker/xblock_maker_para.h"

NS_BEG2(top, blockmaker)

using data::xblock_ptr_t;
using data::xaccount_ptr_t;

class xblockmaker_resources_t {
 public:
    virtual store::xstore_face_t*       get_store() const = 0;
    virtual base::xvblockstore_t*       get_blockstore() const = 0;
    virtual xtxpool_v2::xtxpool_face_t* get_txpool() const = 0;
    virtual store::xindexstorehub_t*    get_indexstorehub() const = 0;
};
using xblockmaker_resources_ptr_t = std::shared_ptr<xblockmaker_resources_t>;

class xblockmaker_resources_impl_t : public xblockmaker_resources_t {
 public:
    xblockmaker_resources_impl_t(const observer_ptr<store::xstore_face_t> & store,
                                 const observer_ptr<base::xvblockstore_t> & blockstore,
                                 const observer_ptr<xtxpool_v2::xtxpool_face_t> & txpool,
                                 const observer_ptr<store::xindexstorehub_t> & indexstorehub)
    : m_store(store), m_blockstore(blockstore), m_txpool(txpool), m_indexstorehub(indexstorehub) {}

    virtual store::xstore_face_t*       get_store() const {return m_store.get();}
    virtual base::xvblockstore_t*       get_blockstore() const {return m_blockstore.get();}
    virtual xtxpool_v2::xtxpool_face_t* get_txpool() const {return m_txpool.get();}
    virtual store::xindexstorehub_t*    get_indexstorehub() const {return m_indexstorehub.get();}

 private:
    observer_ptr<store::xstore_face_t>          m_store{nullptr};
    observer_ptr<base::xvblockstore_t>          m_blockstore{nullptr};
    observer_ptr<xtxpool_v2::xtxpool_face_t>    m_txpool{nullptr};
    observer_ptr<store::xindexstorehub_t>       m_indexstorehub{nullptr};
};

struct xunitmaker_result_t {
    data::xtablestate_ptr_t                 m_tablestate{nullptr};
    xblock_ptr_t                            m_block{nullptr};
    int32_t                                 m_make_block_error_code{0};
    std::vector<xcons_transaction_ptr_t>    m_success_txs;
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
    xtablemaker_para_t() = default;

    void set_unitmaker_info(const std::string & account, uint64_t last_block_height, const std::string & last_block_hash, const std::vector<xcons_transaction_ptr_t> & input_txs) {
        xunit_proposal_input_t unit_input(account, last_block_height, last_block_hash, input_txs);
        m_proposal_input.add_unit_input(unit_input);
    }
    void set_unitmaker_txs(const std::string & account, const std::vector<xcons_transaction_ptr_t> & input_txs) {
        xunit_proposal_input_t unit_input(account, 0, {}, input_txs);
        m_proposal_input.add_unit_input(unit_input);
    }
    void set_batch_txs(const std::map<std::string, std::vector<xcons_transaction_ptr_t>> & batch_txs) {
        for (auto & v : batch_txs) {
            set_unitmaker_txs(v.first, v.second);
        }
    }
    xtableblock_proposal_input_t            m_proposal_input;
    data::xtablestate_ptr_t                 m_tablestate{nullptr};
};

class xblock_maker_t : public base::xvaccount_t {
 public:
    explicit xblock_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources, uint32_t latest_blocks_max)
    : base::xvaccount_t(account), m_resources(resources), m_keep_latest_blocks_max(latest_blocks_max) {}
    virtual ~xblock_maker_t() {}

 public:
    xblock_ptr_t                set_latest_block(const xblock_ptr_t & block);  // return deleted
    void                        clear_block(const xblock_ptr_t & block);
    bool                        load_latest_blocks(const xblock_ptr_t & latest_block, std::map<uint64_t, xblock_ptr_t> & latest_blocks);  // load latest blocks from db for updating cache
    bool                        load_and_cache_enough_blocks();
    bool                        check_latest_blocks() const;

 public:
    store::xstore_face_t*       get_store() const {return m_resources->get_store();}
    base::xvblockstore_t*       get_blockstore() const {return m_resources->get_blockstore();}
    xtxpool_v2::xtxpool_face_t*    get_txpool() const {return m_resources->get_txpool();}
    const xblockmaker_resources_ptr_t & get_resources() const {return m_resources;}

    bool                        has_uncommitted_blocks() const;
    uint32_t                    get_latest_consecutive_empty_block_num() const;
    uint64_t                    get_keep_latest_blocks_max() const {return m_keep_latest_blocks_max;}
    const xblock_ptr_t &        get_latest_committed_block() const {return m_latest_commit_block;}
    const xaccount_ptr_t &      get_latest_committed_state() const {return m_commit_account;}
    std::string                 get_lock_block_sign_hash() const;
    std::string                 get_lock_output_root_hash() const;
    const std::map<uint64_t, xblock_ptr_t> & get_latest_blocks() const {return m_latest_blocks;}
    xblock_ptr_t                get_latest_block(uint64_t height) const;
    const xblock_ptr_t &        get_highest_height_block() const;
    const xblock_ptr_t &        get_lowest_height_block() const;
    xblock_ptr_t                get_highest_non_empty_block() const;
    std::vector<xblock_ptr_t>   get_uncommit_blocks() const;
    xblock_ptr_t                get_highest_lock_block() const;
    xblock_ptr_t                get_highest_commit_block() const;
    xblock_ptr_t                get_prev_block(const xblock_ptr_t & current) const;
    xaccount_ptr_t              clone_latest_committed_state() const;
    bool                        verify_latest_blocks(base::xvblock_t* latest_cert_block, base::xvblock_t* lock_block, base::xvblock_t* commited_block);

 protected:
    void                        set_latest_committed_block(const xblock_ptr_t & latest_committed_block);
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

class xblock_rules_face_t {
 public:
    virtual bool        unit_rules_filter(const xblock_ptr_t & rules_end_block,
                                            const xaccount_ptr_t & rules_end_state,
                                            const std::vector<xcons_transaction_ptr_t> & origin_txs,
                                            std::vector<xcons_transaction_ptr_t> & valid_txs,
                                            std::vector<xcons_transaction_ptr_t> & pop_txs) = 0;
};
using xblock_rules_face_ptr_t = std::shared_ptr<xblock_rules_face_t>;

class xblock_builder_para_face_t {
 public:
    xblock_builder_para_face_t() = default;
    xblock_builder_para_face_t(const xblockmaker_resources_ptr_t & resources)
    : m_resources(resources) {}

 public:
    virtual store::xstore_face_t*       get_store() const {return m_resources->get_store();}
    virtual base::xvblockstore_t*       get_blockstore() const {return m_resources->get_blockstore();}
    virtual xtxpool_v2::xtxpool_face_t* get_txpool() const {return m_resources->get_txpool();}
    virtual int32_t                     get_error_code() const {return m_error_code;}
    virtual void                        set_error_code(int32_t error_code) {m_error_code = error_code;}

 private:
    xblockmaker_resources_ptr_t m_resources{nullptr};
    int32_t                     m_error_code{0};
};
using xblock_builder_para_ptr_t = std::shared_ptr<xblock_builder_para_face_t>;

class xblock_builder_face_t {
 public:
    virtual data::xblock_ptr_t          build_block(const data::xblock_ptr_t & prev_block,
                                                  const data::xaccount_ptr_t & prev_state,
                                                  const data::xblock_consensus_para_t & cs_para,
                                                  xblock_builder_para_ptr_t & build_para) = 0;
    virtual xblock_ptr_t                build_empty_block(const xblock_ptr_t & prev_block,
                                                        const data::xblock_consensus_para_t & cs_para);

    xblock_ptr_t                        build_genesis_block(const std::string & account, int64_t top_balance = 0);
 protected:
    base::xvblock_t*                    build_genesis_unit(const std::string & account, int64_t top_balance);
};

using xblock_builder_face_ptr_t = std::shared_ptr<xblock_builder_face_t>;

NS_END2
