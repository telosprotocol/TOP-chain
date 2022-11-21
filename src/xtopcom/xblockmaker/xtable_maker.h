// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include "xstate_mpt/xstate_mpt.h"
#include "xdata/xethheader.h"
#include "xdata/xtableblock.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xblockbuild.h"
#include "xblockmaker/xblock_maker_para.h"
#include "xblockmaker/xblockmaker_face.h"
#include "xtxexecutor/xatomictx_executor.h"
#include "xtxexecutor/xbatchtx_executor.h"
#include "xstatectx/xstatectx.h"
#include "xunit_service/xcons_face.h"

NS_BEG2(top, blockmaker)

using data::xblock_t;
using data::xblock_consensus_para_t;

class relay_wrap_info_t {
public:
   relay_wrap_info_t() {}
   relay_wrap_info_t(uint64_t evm_height, uint64_t elect_height, uint64_t poly_timestamp) : m_evm_height(evm_height), m_elect_height(elect_height), m_poly_timestamp(poly_timestamp) {}
   uint64_t evm_height() const {return m_evm_height;}
   uint64_t elect_height() const {return m_elect_height;}
   uint64_t poly_timestamp() const {return m_poly_timestamp;}
   void set_evm_height(uint64_t evm_height) {m_evm_height = evm_height;}
   void set_elect_height(uint64_t elect_height) {m_elect_height = elect_height;}
   void set_poly_timestamp(uint64_t poly_timestamp) {m_poly_timestamp = poly_timestamp;}
private:
   uint64_t m_evm_height{0};
   uint64_t m_elect_height{0};
   uint64_t m_poly_timestamp{0};
};

class lack_account_info_t {
public:
    lack_account_info_t() {}
    lack_account_info_t(const std::string & addr, const base::xaccount_index_t & account_index) : m_addr(addr), m_account_index(account_index) {}
    const std::string & get_addr() const {return m_addr;}
    const base::xaccount_index_t & get_account_index() const {return m_account_index;}
private:
    std::string m_addr;
    base::xaccount_index_t m_account_index;
};

typedef bool (*account_index_converter)(const base::xvaccount_t & account, const base::xaccount_index_t & old_account_index, base::xaccount_index_t & new_account_index);

class xaccount_index_upgrade_t {
public:
    void init(const std::string & table_addr, uint32_t accounts_num, uint64_t committed_height);
    void add_old_index(const std::string & addr, const base::xaccount_index_t & account_index);
    bool upgrade(account_index_converter convert_func, uint32_t max_convert_num);
    bool get_new_indexes(std::map<std::string, base::xaccount_index_t> & new_indexes, uint64_t committed_height);
    uint64_t get_fork_height() const;
    void clear();

private:
    std::string m_table_address;
    std::map<std::string, base::xaccount_index_t> m_new_indexes;
    std::vector<lack_account_info_t> m_lack_accounts;
    uint32_t m_lack_accounts_pos{0};
    uint32_t m_accounts_num{0};
    uint64_t m_fork_height{0};
};

class xaccount_index_upgrade_tool_t {
public:
    static bool convert_to_new_account_index(const base::xvaccount_t & account, const base::xaccount_index_t & old_account_index, base::xaccount_index_t & new_account_index);
    static bool update_new_indexes_by_block(std::map<std::string, base::xaccount_index_t> & new_indexes, const xblock_ptr_t & block);
private:
    static base::xvblockstore_t * get_xblockstore();
    static base::xvblkstatestore_t* get_blkstate_store();
    static bool write_unitstate_with_new_dbkey(const base::xvaccount_t & account, data::xunitstate_ptr_t const& unitstate, std::string const& block_hash);
};

class xtable_maker_t : public xblock_maker_t {
 public:
    explicit xtable_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources);
    virtual ~xtable_maker_t();

 public:
    xblock_ptr_t            make_proposal(xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & result);
    int32_t                 verify_proposal(base::xvblock_t* proposal_block, const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para);
    bool                    is_make_relay_chain() const;
    bool                    account_index_upgrade();

 protected:
    int32_t                 check_latest_state(const xblock_ptr_t & latest_block); // check table latest block and state
    bool                    can_make_next_full_block(const data::xblock_consensus_para_t & cs_para) const;

    xblock_ptr_t            leader_make_light_table(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result);
    xblock_ptr_t            backup_make_light_table(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result);
    xblock_ptr_t            make_full_table(const xtablemaker_para_t & table_para, const xblock_consensus_para_t & cs_para, int32_t & error_code);
    xblock_ptr_t            make_empty_table(const xtablemaker_para_t & table_para, const xblock_consensus_para_t & cs_para, int32_t & error_code);

    bool                    verify_proposal_with_local(base::xvblock_t *proposal_block, base::xvblock_t *local_block) const;
    bool                    load_table_blocks_from_last_full(const xblock_ptr_t & prev_block, std::vector<xblock_ptr_t> & blocks);
    xblock_ptr_t            make_light_table_v2(bool is_leader, const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result);
    void                    set_packtx_metrics(const xcons_transaction_ptr_t & tx, bool bsucc) const;
    bool                    can_make_next_empty_block(const data::xblock_consensus_para_t & cs_para) const;

private:
    std::vector<xcons_transaction_ptr_t> check_input_txs(bool is_leader, const data::xblock_consensus_para_t & cs_para, const std::vector<xcons_transaction_ptr_t> & input_table_txs, uint64_t now);
    void execute_txs(bool is_leader, const data::xblock_consensus_para_t & cs_para, statectx::xstatectx_ptr_t const& statectx_ptr, const std::vector<xcons_transaction_ptr_t> & input_txs, txexecutor::xexecute_output_t & execute_output, std::error_code & ec);
    std::vector<std::pair<xblock_ptr_t, base::xaccount_index_t>> make_units(bool is_leader, const data::xblock_consensus_para_t & cs_para, statectx::xstatectx_ptr_t const& statectx_ptr, txexecutor::xexecute_output_t const& execute_output, std::error_code & ec);
    std::vector<std::pair<xblock_ptr_t, base::xaccount_index_t>> make_units_v2(bool is_leader, const data::xblock_consensus_para_t & cs_para, statectx::xstatectx_ptr_t const& statectx_ptr, txexecutor::xexecute_output_t const& execute_output, std::error_code & ec);
    void update_receiptid_state(const xtablemaker_para_t & table_para, statectx::xstatectx_ptr_t const& statectx_ptr);
    void resource_plugin_make_txs(bool is_leader, statectx::xstatectx_ptr_t const& statectx_ptr, const data::xblock_consensus_para_t & cs_para, std::vector<xcons_transaction_ptr_t> & input_txs, std::error_code & ec);
    void rerource_plugin_make_resource(bool is_leader, const data::xblock_consensus_para_t & cs_para, data::xtable_block_para_t & lighttable_para, std::error_code & ec);
    std::shared_ptr<state_mpt::xstate_mpt_t> create_new_mpt(evm_common::xh256_t const & last_mpt_root,
                                                              const data::xblock_consensus_para_t & cs_para,
                                                              const statectx::xstatectx_ptr_t & table_state_ctx,
                                                              const std::vector<std::pair<xblock_ptr_t, base::xaccount_index_t>> & batch_unit_and_index);
    bool get_new_account_indexes(const data::xblock_consensus_para_t & cs_para, std::map<std::string, base::xaccount_index_t> & new_indexes);

    xblock_resource_plugin_face_ptr_t           m_resource_plugin{nullptr};
    static constexpr uint32_t                   m_empty_block_max_num{2};
    uint32_t                                    m_full_table_interval_num;
    xblock_builder_face_ptr_t                   m_fulltable_builder;
    xblock_builder_face_ptr_t                   m_emptytable_builder;
    xblock_builder_para_ptr_t                   m_default_builder_para;
    mutable std::mutex                          m_lock;
    mutable std::mutex                          m_index_upgrade_lock;
    xaccount_index_upgrade_t                    m_account_index_upgrade;
    bool                                        m_account_index_upgrade_finished{false};
};

class xeth_header_builder {
public:
    static std::string build(const xblock_consensus_para_t & cs_para, evm_common::xh256_t const & state_root, const std::vector<txexecutor::xatomictx_output_t> & pack_txs_outputs = {});
    static bool string_to_eth_header(const std::string & eth_header_str, data::xeth_header_t & eth_header);
};

using xtable_maker_ptr_t = xobject_ptr_t<xtable_maker_t>;

// TODO(jimmy) the whold table state do commit
class xtable_mpt_container : public base::xvblock_excontainer_base {
public:
   xtable_mpt_container(statectx::xstatectx_face_ptr_t const& _ctx) : m_statectx(_ctx) {}
   virtual void commit(base::xvblock_t* current_block) override {
       m_statectx->do_commit(current_block);
   }

private:
   statectx::xstatectx_face_ptr_t m_statectx;
};

NS_END2
