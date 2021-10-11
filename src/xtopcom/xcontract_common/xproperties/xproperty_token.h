// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcontract_common/xproperties/xbasic_property.h"
#include "xcontract_common/xbasic_contract.h"

#include <cstdint>
#include <string>
#include <system_error>

NS_BEG3(top, contract_common, properties)

class xtop_token_safe {
public:
    static bool transfer_safe_rule(uint64_t amount) noexcept;
    // static void set_max_safe_token(uint64_t amount) noexcept;

private:
   constexpr static uint64_t MAX_SAFE_TOKEN = 200 * 1e8 * 1e6; //utop
};
using xtoken_safe_t = xtop_token_safe;

// token object
class xtop_token_property: public xbasic_property_t {
    common::xsymbol_t symbol_{"TOP"};

public:
    xtop_token_property(xtop_token_property const&) = delete;
    xtop_token_property& operator=(xtop_token_property const&) = delete;
    xtop_token_property(xtop_token_property&&) = default;
    xtop_token_property& operator=(xtop_token_property&&) = default;
    ~xtop_token_property() =  default;

    explicit xtop_token_property(std::string const& prop_name, common::xsymbol_t symbol, contract_common::xbasic_contract_t* contract);
    explicit xtop_token_property(common::xsymbol_t symbol, contract_common::xbasic_contract_t * contract);
    explicit xtop_token_property(std::string const & prop_name, contract_common::xbasic_contract_t * contract);
    explicit xtop_token_property(contract_common::xbasic_contract_t * contract);

    void create() override final;

    uint64_t amount() const;
    state_accessor::xtoken_t withdraw(std::uint64_t amount);
    void deposit(state_accessor::xtoken_t tokens);
};
using xtoken_property_t = xtop_token_property;

NS_END3
