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

NS_BEG2(top, data)

class xblockextract_t {
 public:
    static uint32_t                                 get_txactions_count(base::xvblock_t* _block);
    static std::vector<xlightunit_action_t>         unpack_txactions(base::xvblock_t* _block);
    static std::vector<xlightunit_action_t>         unpack_eth_txactions(base::xvblock_t* _block);
    static xlightunit_action_ptr_t                  unpack_one_txaction(base::xvblock_t* _block, std::string const& txhash);
    static xtransaction_ptr_t                       unpack_raw_tx(base::xvblock_t* _block, std::string const& txhash, std::error_code & ec);

    static void     unpack_ethheader(base::xvblock_t* _block, xeth_header_t & ethheader, std::error_code & ec);
    static void     unpack_relayblock(base::xvblock_t* _block, bool include_sig, xrelay_block & relayblock, std::error_code & ec);
    static void     unpack_relaywrapinfo(base::xvblock_t* _block, xrelay_wrap_info_t & wrap_info, std::error_code & ec);
    static void     unpack_relaywrapinfo_and_relay_block_data(base::xvblock_t* _block, xrelay_wrap_info_t & wrap_info, std::string & relay_block_data, std::error_code & ec);
    static void     get_tableheader_extra_from_block(base::xvblock_t* _block, data::xtableheader_extra_t &header_extra, std::error_code & ec);
};


NS_END2
