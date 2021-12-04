// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include "xvledger/xvstate.h"
#include "xvledger/xvstatestore.h"
#include "xvledger/xvledger.h"
#include "xdata/xblock.h"
#include "xdata/xtable_bstate.h"
#include "xstore/xstore_face.h"
#include "xblockstore/xblockstore_face.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xblockmaker/xblock_maker_para.h"

NS_BEG2(top, blockmaker)

using data::xblock_ptr_t;
using data::xaccount_ptr_t;

class xblockmaker_resources_t {
 public:
    virtual base::xvblockstore_t*       get_blockstore() const = 0;
    virtual xtxpool_v2::xtxpool_face_t* get_txpool() const = 0;
    virtual mbus::xmessage_bus_face_t*  get_bus() const = 0;
    virtual base::xvblkstatestore_t*    get_xblkstatestore() const = 0;
};
using xblockmaker_resources_ptr_t = std::shared_ptr<xblockmaker_resources_t>;

class xblockmaker_resources_impl_t : public xblockmaker_resources_t {
 public:
    xblockmaker_resources_impl_t(const observer_ptr<store::xstore_face_t> & store,
                                 const observer_ptr<base::xvblockstore_t> & blockstore,
                                 const observer_ptr<xtxpool_v2::xtxpool_face_t> & txpool,
                                 const observer_ptr<mbus::xmessage_bus_face_t> & bus)
    : m_blockstore(blockstore), m_txpool(txpool), m_bus(bus) {}

    virtual base::xvblockstore_t*       get_blockstore() const {return m_blockstore.get();}
    virtual xtxpool_v2::xtxpool_face_t* get_txpool() const {return m_txpool.get();}
    virtual mbus::xmessage_bus_face_t*  get_bus() const {return m_bus.get();}
    virtual base::xvblkstatestore_t*    get_xblkstatestore() const {return base::xvchain_t::instance().get_xstatestore()->get_blkstate_store();}

 private:
    observer_ptr<base::xvblockstore_t>          m_blockstore{nullptr};
    observer_ptr<xtxpool_v2::xtxpool_face_t>    m_txpool{nullptr};
    observer_ptr<mbus::xmessage_bus_face_t>     m_bus{nullptr};
};

struct xunitmaker_result_t {
 public:
    void        add_pack_txs(const std::vector<xcons_transaction_ptr_t> & txs) {
        for (auto & tx : txs) {
            if (tx->is_self_tx()) {
                m_self_tx_num++;
            } else if (tx->is_send_tx()) {
                m_send_tx_num++;
            } else if (tx->is_recv_tx()) {
                m_recv_tx_num++;
            } else if (tx->is_confirm_tx()) {
                m_confirm_tx_num++;
            }
        }
        m_pack_txs = txs;
    }

    xblock_ptr_t                            m_block{nullptr};
    int32_t                                 m_make_block_error_code{0};
    std::vector<xcons_transaction_ptr_t>    m_pack_txs;
    std::vector<xcons_transaction_ptr_t>    m_fail_txs;
    std::vector<xcons_transaction_ptr_t>    m_unchange_txs;
    int64_t                                 m_tgas_balance_change{0};
    uint32_t                                m_self_tx_num{0};
    uint32_t                                m_send_tx_num{0};
    uint32_t                                m_recv_tx_num{0};
    uint32_t                                m_confirm_tx_num{0};
};

struct xunitmaker_para_t {
    xunitmaker_para_t(const data::xtablestate_ptr_t & tablestate, bool is_leader)
    : m_tablestate(tablestate), m_is_leader(is_leader) {}
    xunitmaker_para_t(const data::xtablestate_ptr_t & tablestate, const xunit_proposal_input_t & unit_input)
    : m_tablestate(tablestate), m_unit_input(unit_input) {}

    data::xtablestate_ptr_t                 m_tablestate{nullptr};
    xunit_proposal_input_t                  m_unit_input;
    bool                                    m_is_leader{false};
};

class xtablemaker_result_t {
 public:
    void add_unit_result(const xunitmaker_result_t & unit_result);
 public:
    xblock_ptr_t                            m_block{nullptr};
    int32_t                                 m_make_block_error_code{0};
    std::vector<xunitmaker_result_t>        m_unit_results;

    uint32_t                                m_total_tx_num{0};
    uint32_t                                m_self_tx_num{0};
    uint32_t                                m_send_tx_num{0};
    uint32_t                                m_recv_tx_num{0};
    uint32_t                                m_confirm_tx_num{0};

    uint32_t                                m_total_unit_num{0};
    uint32_t                                m_fail_unit_num{0};
    uint32_t                                m_succ_unit_num{0};
    uint32_t                                m_empty_unit_num{0};
    uint32_t                                m_light_unit_num{0};
    uint32_t                                m_full_unit_num{0};
    std::vector<xcons_transaction_ptr_t>    m_unchange_txs;
};

class xtablemaker_para_t {
 public:
    xtablemaker_para_t() {
        m_proposal = make_object_ptr<xtable_proposal_input_t>();
    }
    xtablemaker_para_t(const data::xtablestate_ptr_t & tablestate, const data::xtablestate_ptr_t & commit_tablestate)
      : m_tablestate(tablestate), m_commit_tablestate(commit_tablestate) {
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
    const data::xtablestate_ptr_t &                 get_commit_tablestate() const {return m_commit_tablestate;}
    const xtable_proposal_input_ptr_t &             get_proposal() const {return m_proposal;}

 private:
    std::vector<xcons_transaction_ptr_t>    m_origin_txs;
    std::vector<std::string>                m_other_accounts;  // for empty or full unit accounts

    mutable xtable_proposal_input_ptr_t     m_proposal;  // leader should make proposal input; backup should verify proposal input
    mutable data::xtablestate_ptr_t         m_tablestate{nullptr};
    mutable data::xtablestate_ptr_t         m_commit_tablestate{nullptr};
};

class xblock_maker_t : public base::xvaccount_t {
 public:
    explicit xblock_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources, uint32_t latest_blocks_max);
    virtual ~xblock_maker_t();

 public:
    void                        set_latest_block(const xblock_ptr_t & block);
    void                        reset_latest_cert_block(const xblock_ptr_t & block);
    bool                        load_and_cache_enough_blocks(const xblock_ptr_t & latest_block, uint64_t & lacked_height_from, uint64_t & lacked_height_to);
    bool                        load_and_cache_enough_blocks(const xblock_ptr_t & latest_block);
    bool                        check_latest_blocks(const xblock_ptr_t & latest_block) const;

 public:
    base::xvblockstore_t*       get_blockstore() const {return m_resources->get_blockstore();}
    xtxpool_v2::xtxpool_face_t*    get_txpool() const {return m_resources->get_txpool();}
    mbus::xmessage_bus_face_t*  get_bus() const {return m_resources->get_bus();}
    const xblockmaker_resources_ptr_t & get_resources() const {return m_resources;}

    const xaccount_ptr_t &      get_latest_bstate() const {return m_latest_bstate;}
    const std::map<uint64_t, xblock_ptr_t> & get_latest_blocks() const {return m_latest_blocks;}
    const xblock_ptr_t &        get_highest_height_block() const;
    xblock_ptr_t                get_prev_block_from_cache(const xblock_ptr_t & current) const;
    void                        set_keep_latest_blocks_max(uint32_t keep_latest_blocks_max) {m_keep_latest_blocks_max = keep_latest_blocks_max;}

 protected:
    bool                        update_account_state(const xblock_ptr_t & latest_block);
    void                        clear_old_blocks();

 private:
    xblockmaker_resources_ptr_t             m_resources{nullptr};
    std::map<uint64_t, xblock_ptr_t>        m_latest_blocks;
    uint32_t                                m_keep_latest_blocks_max{0};
    xaccount_ptr_t                          m_latest_bstate{nullptr};
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
    xblock_builder_para_face_t(const xblockmaker_resources_ptr_t & resources, const std::vector<xlightunit_tx_info_ptr_t> & txs_info = std::vector<xlightunit_tx_info_ptr_t>{})
    : m_resources(resources), m_txs_info(txs_info) {}

 public:
    virtual base::xvblockstore_t*       get_blockstore() const {return m_resources->get_blockstore();}
    virtual xtxpool_v2::xtxpool_face_t* get_txpool() const {return m_resources->get_txpool();}
    virtual int32_t                     get_error_code() const {return m_error_code;}
    virtual void                        set_error_code(int32_t error_code) {m_error_code = error_code;}
    int64_t get_tgas_balance_change() const { return m_tgas_balance_change; }
    void set_tgas_balance_change(const int64_t amount) { m_tgas_balance_change = amount; }
    const std::vector<xlightunit_tx_info_ptr_t> & get_txs() const {return m_txs_info;}

 private:
    xblockmaker_resources_ptr_t m_resources{nullptr};
    int32_t                     m_error_code{0};
    int64_t                     m_tgas_balance_change{0};
    std::vector<xlightunit_tx_info_ptr_t> m_txs_info;
};
using xblock_builder_para_ptr_t = std::shared_ptr<xblock_builder_para_face_t>;

class xblock_builder_face_t {
 public:
    virtual data::xblock_ptr_t          build_block(const data::xblock_ptr_t & prev_block,
                                                  const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                                  const data::xblock_consensus_para_t & cs_para,
                                                  xblock_builder_para_ptr_t & build_para) = 0;
    void                                alloc_tx_receiptid(const std::vector<xcons_transaction_ptr_t> & input_txs,
                                                           const base::xreceiptid_state_ptr_t & receiptid_state);
};

using xblock_builder_face_ptr_t = std::shared_ptr<xblock_builder_face_t>;

NS_END2
