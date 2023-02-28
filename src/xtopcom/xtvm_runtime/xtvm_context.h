// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xcommon/xaccount_address_fwd.h"
#include "xdata/xtop_action_fwd.h"
#include "xtxexecutor/xvm_face.h"

NS_BEG2(top, tvm)

class xtop_vm_context {
private:
    // data::xtop_evm_action_type m_action_type;
    std::unique_ptr<data::xbasic_top_action_t const> m_action;
    uint64_t m_gas_price;
    common::xaccount_address_t m_sender;
    uint64_t m_block_height;
    common::xaccount_address_t m_coinbase;
    uint64_t m_block_timestamp;
    uint64_t m_chain_id;
    xbytes_t m_input_data;

public:
    xtop_vm_context(std::unique_ptr<data::xbasic_top_action_t const> action, txexecutor::xvm_para_t const & vm_para);

public:
    // data::xtop_evm_action_type action_type() const;
    xbytes_t const & input_data() const;
    uint64_t gas_price() const;
    common::xaccount_address_t sender() const;
    uint64_t block_height() const;
    common::xaccount_address_t coinbase() const;
    uint64_t block_timestamp() const;
    uint64_t chain_id() const;
};
using xtvm_context_t = xtop_vm_context;

NS_END2