// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>
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

class xtable_maker_t : public xblock_maker_t {
 public:
    explicit xtable_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources);
    virtual ~xtable_maker_t();

 public:
    xblock_ptr_t            make_proposal(xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & result);
    int32_t                 verify_proposal(base::xvblock_t* proposal_block, const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para);
    bool                    is_make_relay_chain() const;

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
    std::vector<xblock_ptr_t> make_units(bool is_leader, const data::xblock_consensus_para_t & cs_para, statectx::xstatectx_ptr_t const& statectx_ptr, txexecutor::xexecute_output_t const& execute_output, std::error_code & ec);
    void update_receiptid_state(const xtablemaker_para_t & table_para, statectx::xstatectx_ptr_t const& statectx_ptr);
    void resource_plugin_make_txs(bool is_leader, statectx::xstatectx_ptr_t const& statectx_ptr, const data::xblock_consensus_para_t & cs_para, std::vector<xcons_transaction_ptr_t> & input_txs, std::error_code & ec);
    void rerource_plugin_make_resource(bool is_leader, const data::xblock_consensus_para_t & cs_para, data::xtable_block_para_t & lighttable_para, std::error_code & ec);

    xblock_resource_plugin_face_ptr_t           m_resource_plugin{nullptr};
    static constexpr uint32_t                   m_empty_block_max_num{2};
    uint32_t                                    m_full_table_interval_num;
    xblock_builder_face_ptr_t                   m_fulltable_builder;
    xblock_builder_face_ptr_t                   m_emptytable_builder;
    xblock_builder_para_ptr_t                   m_default_builder_para;
    mutable std::mutex                          m_lock;
};

class xeth_header_builder {
public:
    static const std::string build(const xblock_consensus_para_t & cs_para, const std::vector<txexecutor::xatomictx_output_t> & pack_txs_outputs = {});
    static bool string_to_eth_header(const std::string & eth_header_str, data::xeth_header_t & eth_header);
};

using xtable_maker_ptr_t = xobject_ptr_t<xtable_maker_t>;

NS_END2
