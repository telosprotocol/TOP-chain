// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include "xbase/xns_macro.h"
#include "xdata/xblockaction.h"
#include "xdata/xblockbuild.h"
#include "xdata/xethheader.h"
#include "xdata/xrelay_block.h"
#include "xdata/xrelayblock_build.h"

NS_BEG2(top, data)




class xblockextract_t {
 public:
    static void loop_all_txactions(base::xvblock_t* _block, std::function<void(const base::xvaction_t & _action)> _func);
    static void loop_eth_txactions(base::xvblock_t* _block, std::function<void(const base::xvaction_t & _action)> _func);
    static void loop_top_txactions(base::xvblock_t* _block, std::function<void(const base::xvaction_t & _action)> _func); 
    static void                                     extract_sub_txs(base::xvblock_t* _block, std::vector<base::xvtxindex_ptr> & sub_txs);
    static uint32_t                                 get_txactions_count(base::xvblock_t* _block);
    static std::vector<xlightunit_action_t>         unpack_txactions(base::xvblock_t* _block);
    static std::vector<xlightunit_action_t>         unpack_eth_txactions(base::xvblock_t* _block);
    static xlightunit_action_ptr_t                  unpack_one_txaction(base::xvblock_t* _block, std::string const& txhash);
    static xtransaction_ptr_t                       unpack_raw_tx(base::xvblock_t* _block, std::string const& txhash, std::error_code & ec);

    // relay block apis
    static std::shared_ptr<xrelay_block>            unpack_relay_block_from_table(base::xvblock_t* _block, std::error_code & ec);
    static xobject_ptr_t<base::xvblock_t>           pack_relayblock_to_wrapblock(xrelay_block const& relayblock, std::error_code & ec);
    static void     unpack_relayblock_from_wrapblock(base::xvblock_t* _block, xrelay_block & relayblock, std::error_code & ec);
    static xobject_ptr_t<base::xvblock_t>           unpack_wrap_relayblock_from_relay_table(base::xvblock_t* _block, std::error_code & ec);

    static void     unpack_ethheader(base::xvblock_t* _block, xeth_header_t & ethheader, std::error_code & ec);
    static evm_common::xh256_t get_state_root(base::xvblock_t * block, std::error_code & ec);
    static evm_common::xh256_t get_state_root_from_block(base::xvblock_t * block);
    static void     unpack_subblocks(base::xvblock_t* _block, std::vector<xobject_ptr_t<base::xvblock_t>> & sublocks, std::error_code & ec);
    static void     get_tableheader_extra_from_block(base::xvblock_t* _block, data::xtableheader_extra_t &header_extra, std::error_code & ec);
 

 private:
    static std::shared_ptr<xrelay_block>            unpack_commit_relay_block_from_relay_table(base::xvblock_t* _block, std::error_code & ec);    
};


NS_END2
