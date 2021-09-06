// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xvm/xtype.h"

#include <type_traits>

NS_BEG1(std)

std::size_t hash<top::contract_runtime::vm::xtype_t>::operator()(top::contract_runtime::vm::xtype_t const type) const noexcept {
    return static_cast<std::size_t>(static_cast<std::underlying_type<top::contract_runtime::vm::xtype_t>::type>(type));
}

NS_END1
