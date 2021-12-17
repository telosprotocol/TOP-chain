// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <assert.h>

#include "xbasic/xmodule_type.h"

NS_BEG2(top, blockmaker)

enum enum_xblockmaker_error_type {
    xblockmaker_error_base = chainbase::enum_xmodule_type::xmodule_type_xblockmaker,

    xblockmaker_error_block_state_unmatch,
    xblockmaker_error_latest_table_blocks_invalid,
    xblockmaker_error_latest_unit_blocks_invalid,
    xblockmaker_error_property_unmatch,
    xblockmaker_error_missing_block,
    xblockmaker_error_missing_state,
    xblockmaker_error_property_load,
    xblockmaker_error_tx_execute,
    xblockmaker_error_tx_check,
    xblockmaker_error_no_need_make_unit,
    xblockmaker_error_null_unit,
    xblockmaker_error_no_need_make_table,

    xblockmaker_error_proposal_outofdate,
    xblockmaker_error_proposal_cannot_connect_to_cert,
    xblockmaker_error_proposal_too_future,
    xblockmaker_error_proposal_bad_input,
    xblockmaker_error_proposal_bad_drand,
    xblockmaker_error_proposal_bad_consensus_para,
    xblockmaker_error_proposal_not_match_local,
    xblockmaker_error_proposal_bad_input_txs,
    xblockmaker_error_proposal_unit_not_match_prev_block,
    xblockmaker_error_proposal_table_not_match_prev_block,
    xblockmaker_error_proposal_table_state_clone,

    xblockmaker_error_max,
};

#define XUNIT_TO_STR(val) #val

inline std::string xblockmaker_error_to_string(int32_t code) {
    assert(code > xblockmaker_error_base && code < xblockmaker_error_max);
    static const char* names[] = {
        XUNIT_TO_STR(xblockmaker_error_block_state_unmatch),
        XUNIT_TO_STR(xblockmaker_error_latest_table_blocks_invalid),
        XUNIT_TO_STR(xblockmaker_error_latest_unit_blocks_invalid),
        XUNIT_TO_STR(xblockmaker_error_property_unmatch),
        XUNIT_TO_STR(xblockmaker_error_missing_block),
        XUNIT_TO_STR(xblockmaker_error_missing_state),
        XUNIT_TO_STR(xblockmaker_error_property_load),
        XUNIT_TO_STR(xblockmaker_error_tx_execute),
        XUNIT_TO_STR(xblockmaker_error_tx_check),
        XUNIT_TO_STR(xblockmaker_error_no_need_make_unit),
        XUNIT_TO_STR(xblockmaker_error_null_unit),
        XUNIT_TO_STR(xblockmaker_error_no_need_make_table),

        XUNIT_TO_STR(xblockmaker_error_proposal_outofdate),
        XUNIT_TO_STR(xblockmaker_error_proposal_cannot_connect_to_cert),
        XUNIT_TO_STR(xblockmaker_error_proposal_too_future),
        XUNIT_TO_STR(xblockmaker_error_proposal_bad_input),
        XUNIT_TO_STR(xblockmaker_error_proposal_bad_drand),
        XUNIT_TO_STR(xblockmaker_error_proposal_bad_consensus_para),
        XUNIT_TO_STR(xblockmaker_error_proposal_not_match_local),
        XUNIT_TO_STR(xblockmaker_error_proposal_bad_input_txs),
        XUNIT_TO_STR(xblockmaker_error_proposal_unit_not_match_prev_block),
        XUNIT_TO_STR(xblockmaker_error_proposal_table_not_match_prev_block),
        XUNIT_TO_STR(xblockmaker_error_proposal_table_state_clone),
    };
    return names[code - xblockmaker_error_base - 1];
}



NS_END2
