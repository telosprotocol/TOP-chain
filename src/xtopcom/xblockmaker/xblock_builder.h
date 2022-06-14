// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include "xdata/xblock.h"
#include "xblockmaker/xblock_maker_para.h"
#include "xblockmaker/xblockmaker_face.h"

NS_BEG2(top, blockmaker)

class xunitbuilder_para_t {
 public:
    xunitbuilder_para_t(const base::xvtxkey_vec_t & txkeys)
    : m_txkeys(txkeys) {
    }
    const base::xvtxkey_vec_t & get_txkeys() const {return m_txkeys;}
 private:
    base::xvtxkey_vec_t    m_txkeys;
};

class xunitbuildber_txkeys_mgr_t {
 public:
    void    add_pack_tx(const data::xcons_transaction_ptr_t & tx);
    base::xvtxkey_vec_t get_account_txkeys(const std::string & address);
 private:
    void    add_txkey(const std::string & address, const base::xvtxkey_t & txkey);
 private:
    std::map<std::string, base::xvtxkey_vec_t>  m_account_txkeys;
};

class xunitbuilder_t {
 public:
    static bool    can_make_full_unit(const data::xblock_ptr_t & prev_block);
    static data::xblock_ptr_t  make_block(const data::xblock_ptr_t & prev_block, const data::xunitstate_ptr_t & proposal_state, const xunitbuilder_para_t & unitbuilder_para, const data::xblock_consensus_para_t & cs_para);
};


class xtablebuilder_t {
 public:
    static void     make_table_prove_property_hashs(base::xvbstate_t* bstate, std::map<std::string, std::string> & property_hashs);
    static bool     update_account_index_property(const data::xtablestate_ptr_t & tablestate, 
                                                  const std::vector<xblock_ptr_t> & batch_units,
                                                  const std::vector<data::xlightunit_tx_info_ptr_t> & txs_info);
    static bool     update_account_index_property(const data::xtablestate_ptr_t & tablestate, 
                                                  const xblock_ptr_t & unit,
                                                  const data::xunitstate_ptr_t & unit_state);
    static bool     update_receipt_confirmids(const data::xtablestate_ptr_t & tablestate, 
                                                  const std::map<base::xtable_shortid_t, uint64_t> & changed_confirm_ids);

    static data::xblock_ptr_t  make_light_block(const data::xblock_ptr_t & prev_block, const data::xtablestate_ptr_t & tablestate, const data::xblock_consensus_para_t & cs_para,
                                                int64_t tgas_balance_change,
                                                const std::vector<xblock_ptr_t> & batch_units,
                                                const std::vector<data::xlightunit_tx_info_ptr_t> & txs_info,
                                                const std::map<std::string, std::string> & property_hashs);
};

NS_END2
