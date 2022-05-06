// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xvmethod.h"
#include "xdata/xunit_bstate.h"
#include "xstate_accessor/xstate_accessor.h"

namespace top {
namespace gasfee {

class xtop_gas_state_operator {
public:
    explicit xtop_gas_state_operator(std::shared_ptr<data::xunit_bstate_t> const & state);
    xtop_gas_state_operator(xtop_gas_state_operator const &) = delete;
    xtop_gas_state_operator & operator=(xtop_gas_state_operator const &) = delete;
    xtop_gas_state_operator(xtop_gas_state_operator &&) = default;
    xtop_gas_state_operator & operator=(xtop_gas_state_operator &&) = default;
    ~xtop_gas_state_operator() = default;

public:
    void burn(base::vtoken_t token, std::error_code & ec);
    void lock(base::vtoken_t token, std::error_code & ec);
    void unlock(base::vtoken_t token, std::error_code & ec);
    void state_set_used_tgas(uint64_t tgas_usage, std::error_code & ec);
    void state_set_last_time(uint64_t current_time, std::error_code & ec);

    uint64_t account_balance() const;
    uint64_t account_available_tgas(uint64_t current_time, uint64_t onchain_total_gas_deposit) const;
    uint64_t account_formular_used_tgas(uint64_t current_time) const;
private:
    void move_token(std::string const & from, std::string const & to, base::vtoken_t token, std::error_code & ec);

    std::shared_ptr<data::xunit_bstate_t> m_state{nullptr};
};
using xgas_state_operator_t = xtop_gas_state_operator;

}  // namespace gasfee
}  // namespace top
