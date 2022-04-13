// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
// #include "xcontract_common/xcontract_state.h"
#include "xdata/xconsensus_action.h"
#include "xevm_contract_runtime/xevm_action.h"

NS_BEG2(top, evm_runtime)

class xtop_evm_context {
private:
    // mock...
    xbytes_t m_input_data;
    std::string m_random_seed{""};

    xtop_evm_action_type m_action_type{xtop_evm_action_type::invalid};

    // observer_ptr<contract_common::xcontract_state_t> m_contract_state{}; // we might not need this.
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
    xtop_evm_action_type action_type() const;

    // todo should be settled in constructor. delete later
    void input_data(xbytes_t const & data);
    // todo should get from relavent action. might not store inside self.
    xbytes_t const & input_data() const;

    common::xaccount_address_t sender() const;

    std::string const & random_seed() const noexcept;
};
using xevm_context_t = xtop_evm_context;

NS_END2