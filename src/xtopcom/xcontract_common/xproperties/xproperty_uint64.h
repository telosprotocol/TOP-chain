// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcontract_common/xproperties/xbasic_property.h"
#include "xcontract_common/xbasic_contract.h"
#include "xcontract_common/xcontract_state.h"

#include <string>

NS_BEG3(top, contract_common, properties)

class xtop_uint64_property: public xbasic_property_t {
public:
    xtop_uint64_property(xtop_uint64_property const&) = delete;
    xtop_uint64_property& operator=(xtop_uint64_property const&) = delete;
    xtop_uint64_property(xtop_uint64_property&&) = default;
    xtop_uint64_property& operator=(xtop_uint64_property&&) = default;
    ~xtop_uint64_property() = default;

    explicit xtop_uint64_property(std::string const & prop_name, contract_common::xbasic_contract_t * contract);

    void create() override final;
    void update(uint64_t const & prop_value);
    void clear();
    uint64_t query() const;
    uint64_t query(common::xaccount_address_t const & contract) const;
};

using xuint64_property_t = xtop_uint64_property;

NS_END3
