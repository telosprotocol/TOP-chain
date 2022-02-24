// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "xcommon/xaddress.h"
#include "xcommon/xnode_type.h"
#include "xvm/xcontract_info.h"

NS_BEG2(top, contract)

class xtop_contract_deploy final {
public:
    static
    xtop_contract_deploy&
    instance();

    void
    deploy_sys_contracts();

    bool
    deploy(common::xaccount_address_t const & address,
           common::xnode_type_t roles,
           const std::string& broadcast_types = "",
           enum_broadcast_policy_t _broadcast_policy = enum_broadcast_policy_t::invalid,
           const std::string& block_monitors = "");

    xcontract_info_t*
    find(common::xaccount_address_t const & address);

    std::unordered_map<common::xaccount_address_t, xcontract_info_t*> const &
    get_map() const noexcept;

    void
    clear();

private:
    std::vector<std::string> str_to_list(const std::string& str, const char sep = ';');
    common::xnode_type_t str_to_broadcast_types(const std::string& str);

    std::unordered_map<common::xaccount_address_t, xcontract_info_t*> m_info_map;
};

using xcontract_deploy_t = xtop_contract_deploy;

NS_END2
