// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xdata/xconsensus_action.h"

NS_BEG2(top, evm_runtime)

const uint32_t CURRENT_CALL_ARGS_VERSION = 1;

class xtop_evm_context {
private:
    // mock...
    std::string m_random_seed{""};

    xbytes_t m_input_data; // for deploy , is bytecode. for call , is serialized call args

    std::unique_ptr<data::xbasic_top_action_t const> m_action;

public:
    xtop_evm_context() = default;
    xtop_evm_context(xtop_evm_context const &) = delete;
    xtop_evm_context & operator=(xtop_evm_context const &) = delete;
    xtop_evm_context(xtop_evm_context &&) = default;
    xtop_evm_context & operator=(xtop_evm_context &&) = default;
    ~xtop_evm_context() = default;

    // explicit xtop_evm_context(std::unique_ptr<data::xbasic_top_action_t const> action, observer_ptr<xcontract_state_t> s) noexcept;
    explicit xtop_evm_context(std::unique_ptr<data::xbasic_top_action_t const> action) noexcept;

public:
    data::xtop_evm_action_type action_type() const;

    xbytes_t const & input_data() const;

    common::xaccount_address_t sender() const;
    common::xaccount_address_t recver() const;

    std::string const & random_seed() const noexcept;
};
using xevm_context_t = xtop_evm_context;

NS_END2