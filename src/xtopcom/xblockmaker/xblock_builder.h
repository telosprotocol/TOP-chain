// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include "xdata/xblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xblockbuild.h"
#include "xblockmaker/xblock_maker_para.h"
#include "xblockmaker/xblockmaker_face.h"
#include "xstatectx/xstatectx.h"  // TODO(jimmy) xstatectx_face
#include "xtxexecutor/xbatchtx_executor.h"// TODO(jimmy) face

NS_BEG2(top, blockmaker)

class xunitbuildber_txkeys_mgr_t {
 public:
    void    add_pack_tx(const data::xcons_transaction_ptr_t & tx);
    base::xvtxkey_vec_t get_account_txkeys(const std::string & address);
 private:
    void    add_txkey(const std::string & address, const base::xvtxkey_t & txkey);
 private:
    std::map<std::string, base::xvtxkey_vec_t>  m_account_txkeys;
};

struct xunit_build_result_t {
    base::xvblock_ptr_t     unitblock;
    data::xunitstate_ptr_t  unitstate;
    std::string             unitstate_bin;
    base::xaccount_index_t  accountindex;
};
using xunit_build_result_ptr_t = std::shared_ptr<xunit_build_result_t>;

class xunitbuilder_t {
 public:
    static bool    can_make_full_unit_v2(uint64_t proposal_height);
    static base::xvblock_ptr_t  create_unit(std::string const& account, uint64_t height, std::string const& last_block_hash, const data::xunit_block_para_t & bodypara, const data::xblock_consensus_para_t & cs_para);
    static void     make_unitblock_and_unitstate(data::xaccountstate_ptr_t const& accountstate, const data::xblock_consensus_para_t & cs_para, xunit_build_result_t & result);
    static data::xaccountstate_ptr_t    create_accountstate(base::xvblock_t* genesis_unit);
    static data::xaccountstate_ptr_t    create_accountstate(data::xaccountstate_ptr_t const& prev_state, base::xvblock_t* current_unit);
};


class xtablebuilder_t {
 public:
    static void     make_table_prove_property_hashs(base::xvbstate_t* bstate, std::map<std::string, std::string> & property_hashs);
    static bool     update_receipt_confirmids(const data::xtablestate_ptr_t & tablestate, 
                                                  const std::map<base::xtable_shortid_t, uint64_t> & changed_confirm_ids);

    static void     make_table_block_para(const data::xtablestate_ptr_t & tablestate,
                                          txexecutor::xexecute_output_t const& execute_output, 
                                          data::xtable_block_para_t & lighttable_para);

    static data::xblock_ptr_t  make_light_block(const data::xblock_ptr_t & prev_block, const data::xblock_consensus_para_t & cs_para, data::xtable_block_para_t const& lighttable_para);
};

NS_END2
