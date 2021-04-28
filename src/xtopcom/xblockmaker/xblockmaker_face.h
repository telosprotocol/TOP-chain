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
    xblock_ptr_t                            m_block{nullptr};
    int32_t                                 m_make_block_error_code{0};
    std::vector<xcons_transaction_ptr_t>    m_success_txs;
    std::vector<xcons_transaction_ptr_t>    m_fail_txs;
};

struct xunitmaker_para_t {
    xunitmaker_para_t(const data::xtablestate_ptr_t & tablestate)
    : m_tablestate(tablestate) {}
    xunitmaker_para_t(const data::xtablestate_ptr_t & tablestate, const xunit_proposal_input_t & unit_input)
    : m_tablestate(tablestate), m_unit_input(unit_input) {}

    data::xtablestate_ptr_t                 m_tablestate{nullptr};
    xunit_proposal_input_t                  m_unit_input;
};

struct xtablemaker_result_t {
    xblock_ptr_t                            m_block{nullptr};
    int32_t                                 m_make_block_error_code{0};
    std::vector<xunitmaker_result_t>        m_unit_results;
};

class xtablemaker_para_t {
 public:
    xtablemaker_para_t() {
        m_proposal = make_object_ptr<xtable_proposal_input_t>();
    }
    xtablemaker_para_t(const data::xtablestate_ptr_t & tablestate)
    : m_tablestate(tablestate) {
        m_proposal = make_object_ptr<xtable_proposal_input_t>();
    }
    xtablemaker_para_t(const std::vector<xcons_transaction_ptr_t> & origin_txs)
    : m_origin_txs(origin_txs) {
        m_proposal = make_object_ptr<xtable_proposal_input_t>();
    }

 public:
    void    set_origin_txs(const std::vector<xcons_transaction_ptr_t> & origin_txs) {
        m_origin_txs = origin_txs;
    }
    void    set_other_accounts(const std::vector<std::string> & accounts) {
        m_other_accounts = accounts;
    }
    void    set_table_state(const data::xtablestate_ptr_t & tablestate) const {
        m_tablestate = tablestate;
    }
    void    push_tx_to_proposal(const xcons_transaction_ptr_t & input_tx) const {
        m_proposal->set_input_tx(input_tx);
    }
    bool    delete_fail_tx_from_proposal(const std::vector<xcons_transaction_ptr_t> & fail_txs) const {
        for (auto & tx : fail_txs) {
            if (false == m_proposal->delete_fail_tx(tx)) {
                return false;
            }
        }
        return true;
    }

    const std::vector<xcons_transaction_ptr_t> &    get_origin_txs() const {return m_origin_txs;}
    const std::vector<std::string> &                get_other_accounts() const {return m_other_accounts;}
    const data::xtablestate_ptr_t &                 get_tablestate() const {return m_tablestate;}
    const xtable_proposal_input_ptr_t &             get_proposal() const {return m_proposal;}

 private:
    std::vector<xcons_transaction_ptr_t>    m_origin_txs;
    std::vector<std::string>                m_other_accounts;  // for empty or full unit accounts

    mutable xtable_proposal_input_ptr_t     m_proposal;  // leader should make proposal input; backup should verify proposal input
    mutable data::xtablestate_ptr_t         m_tablestate{nullptr};
};

class xblock_maker_t : public base::xvaccount_t {
 public:
    explicit xblock_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources, uint32_t latest_blocks_max)
    : base::xvaccount_t(account), m_resources(resources), m_keep_latest_blocks_max(latest_blocks_max) {}
    virtual ~xblock_maker_t() {}

 public:
    void                        set_latest_block(const xblock_ptr_t & block);
    bool                        load_and_cache_enough_blocks(const xblock_ptr_t & latest_block);
    bool                        check_latest_blocks() const;

 public:
    store::xstore_face_t*       get_store() const {return m_resources->get_store();}
    base::xvblockstore_t*       get_blockstore() const {return m_resources->get_blockstore();}
    xtxpool_v2::xtxpool_face_t*    get_txpool() const {return m_resources->get_txpool();}
    const xblockmaker_resources_ptr_t & get_resources() const {return m_resources;}

    bool                        has_uncommitted_blocks() const;

    uint64_t                    get_keep_latest_blocks_max() const {return m_keep_latest_blocks_max;}
    const xblock_ptr_t &        get_latest_committed_block() const {return m_latest_commit_block;}
    const xaccount_ptr_t &      get_latest_committed_state() const {return m_commit_account;}
    std::string                 get_lock_block_sign_hash() const;
    std::string                 get_lock_output_root_hash() const;
    const std::map<uint64_t, xblock_ptr_t> & get_latest_blocks() const {return m_latest_blocks;}
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
    void                        clear_old_blocks();

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
