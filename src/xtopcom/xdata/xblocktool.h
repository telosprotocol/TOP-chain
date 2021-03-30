// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include "xbase/xvledger.h"
#include "xbasic/xns_macro.h"
#include "xdata/xemptyblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xlightunit.h"
#include "xdata/xfullunit.h"
#include "xdata/xcons_transaction.h"

NS_BEG2(top, data)

class xblocktool_t {
 public:
    static base::xvblock_t*   create_genesis_empty_block(const std::string & account);
    static base::xvblock_t*   create_genesis_empty_unit(const std::string & account);
    static base::xvblock_t*   create_genesis_empty_table(const std::string & account);
    static base::xvblock_t*   create_genesis_lightunit(const std::string & account, int64_t top_balance);
    static base::xvblock_t*   create_genesis_lightunit(const std::string & account,
                                                       const xtransaction_ptr_t & genesis_tx,
                                                       const xtransaction_result_t & result);

    static base::xvblock_t*   create_next_emptyblock(base::xvblock_t* prev_block);
    static base::xvblock_t*   create_next_emptyblock(base::xvblock_t* prev_block, base::enum_xvblock_type blocktype);
    static base::xvblock_t*   create_next_lightunit(const xlightunit_block_para_t & para, base::xvblock_t* prev_block);
    static base::xvblock_t*   create_next_fullunit(const xfullunit_block_para_t & para, base::xvblock_t* prev_block);
    static base::xvblock_t*   create_next_tableblock(const xtable_block_para_t & para, base::xvblock_t* prev_block);
    static base::xvblock_t*   create_next_emptyblock(xblockchain2_t* chain);
    static base::xvblock_t*   create_next_lightunit(const xlightunit_block_para_t & para, xblockchain2_t* chain);
    static base::xvblock_t*   create_next_fullunit(xblockchain2_t* chain);
    static base::xvblock_t*   create_next_tableblock(const xtable_block_para_t & para, const xblock_consensus_para_t & cs_para, base::xvblock_t* prev_block);

 public:
   //  static uint16_t         get_chain_id_from_account(const std::string & account);
    static std::string      make_address_table_account(base::enum_xchain_zone_index zone, uint16_t subaddr);
    static std::string      make_address_shard_table_account(uint16_t subaddr);
    static std::string      make_address_user_account(const std::string & public_key_address);
    static std::string      make_address_native_contract(base::enum_xchain_zone_index zone, const std::string & public_key_address, uint16_t subaddr);
    static std::string      make_address_shard_sys_account(const std::string & public_key_address, uint16_t subaddr);
    static std::string      make_address_zec_sys_account(const std::string & public_key_address, uint16_t subaddr);
    static std::string      make_address_beacon_sys_account(const std::string & public_key_address, uint16_t subaddr);
    static std::string      make_address_user_contract(const std::string & public_key_address);

 public:
    static bool             is_connect_and_executed_block(base::xvblock_t* block);
    static base::xauto_ptr<base::xvblock_t> load_justify_block(base::xvblockstore_t* blockstore, const std::string & account, uint64_t height);
    static bool             verify_latest_blocks(const base::xblock_mptrs & latest_blocks);
    static bool             can_make_next_empty_block(const base::xblock_mptrs & latest_blocks, uint32_t max_empty_num);
};

// TODO(jimmy) xblocktool_t split to xblock_utl, xreceipt_utl, xaddress_utl


NS_END2
