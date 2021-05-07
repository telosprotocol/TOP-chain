// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xproperties/xproperty_type.h"

#if !defined(XCXX14_OR_ABOVE)
#    include <type_traits>
#endif


#if !defined(XCXX14_OR_ABOVE)
NS_BEG1(std)

size_t hash<top::contract_common::properties::xproperty_type_t>::operator()(top::contract_common::properties::xproperty_type_t const property_type) const noexcept {
    return static_cast<size_t>(static_cast<std::underlying_type<top::contract_common::properties::xproperty_type_t>::type>(property_type));
}

NS_END1
#endif

