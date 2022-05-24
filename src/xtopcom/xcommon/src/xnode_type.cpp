// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xnode_type.h"

#include <cassert>
#include <type_traits>

NS_BEG2(top, common)

std::string
to_string(xnode_type_t const type) {
    std::string string;
    string.reserve(30);

    if (type == xnode_type_t::invalid) {
        string += "(null)";
    }

    if (has<xnode_type_t::committee>(type)) {
        string += "rec.";
    }

    if (has<xnode_type_t::zec>(type)) {
        string += "zec.";
    }

    if (has<xnode_type_t::edge>(type)) {
        string += "edge.";
    }

    if (has<xnode_type_t::storage>(type)) {
        if (has<xnode_type_t::storage_archive>(type)) {
            string += "storage.archive.";
        } else if (has<xnode_type_t::storage_exchange>(type)) {
            string += "storage.exchange.";
        } else {
            string += "storage.";
        }
    }

    if (has<xnode_type_t::frozen>(type)) {
        string += "frozen.";
    }

    if (has<xnode_type_t::consensus>(type)) {
        if (has<xnode_type_t::consensus_auditor>(type)) {
            string += "consensus.auditor.";
        } else if (has<xnode_type_t::consensus_validator>(type)) {
            string += "consensus.validator.";
        } else {
            string += "consensus.";
        }
    }

    if (has<xnode_type_t::fullnode>(type)) {
        string += "fullnode.";
    }

    if (has<xnode_type_t::evm>(type)) {
        if (has<xnode_type_t::evm_auditor>(type)) {
            string += "evm.auditor.";
        } else if (has<xnode_type_t::evm_validator>(type)) {
            string += "evm.validator.";
        } else {
            string += "evm.";
        }
    }

    if (has<xnode_type_t::relay>(type)) {
        string += "relay.";
    }

    if (has<xnode_type_t::group>(type)) {
        string += "group";
    }

    if (has<xnode_type_t::cluster>(type)) {
        string += "cluster";
    }

    if (has<xnode_type_t::zone>(type)) {
        string += "zone";
    }

    if (has<xnode_type_t::network>(type)) {
        string += "network";
    }

    assert(!string.empty());
    return string;
}

std::string to_presentation_string(xnode_type_t const type) {
    std::string name;

    switch (type) {
    case xnode_type_t::rec:
        name = "root_beacon";
        break;

    case xnode_type_t::zec:
        name = "sub_beacon";
        break;

    case xnode_type_t::edge:
        name = "edge";
        break;

    case xnode_type_t::fullnode:
        name = "fullnode";
        break;

    case xnode_type_t::consensus_auditor:
        name = "auditor";
        break;

    case xnode_type_t::consensus_validator:
        name = "validator";
        break;

    case xnode_type_t::storage_archive:
        name = "archive";
        break;

    case xnode_type_t::storage_exchange:
        name = "exchange";
        break;

    case xnode_type_t::evm_auditor:
        name = "evm_auditor";
        break;

    case xnode_type_t::evm_validator:
        name = "evm_validator";
        break;

    case xnode_type_t::relay:
        name = "relay";
        break;

    default:
        assert(false);
        name = std::to_string(static_cast<std::underlying_type<xnode_type_t>::type>(type));
        break;
    }

    return name;
}

std::string to_presentation_string_compatible(xnode_type_t const type) {
    if (type == xnode_type_t::storage_exchange) {
        return std::string{"full_node"};
    } else {
        return to_presentation_string(type);
    }
}

xnode_type_t &
operator &=(xnode_type_t & lhs, xnode_type_t const rhs) noexcept {
    lhs = lhs & rhs;
    return lhs;
}

xnode_type_t &
operator |=(xnode_type_t & lhs, xnode_type_t const rhs) noexcept {
    lhs = lhs | rhs;
    return lhs;
}

bool
has(xnode_type_t const target, xnode_type_t const input) noexcept {
    return target == (target & input);
}

xnode_type_t
real_part_type(xnode_type_t const in) noexcept {
    return xnode_type_t::real_part_mask & in;
}

xnode_type_t
virtual_part_type(xnode_type_t const in) noexcept {
    return xnode_type_t::virtual_part_mask & in;
}

xnode_type_t
reset_virtual_part(xnode_type_t const in, xnode_type_t const desired_virtual_part_type) noexcept {
    auto const high_part = desired_virtual_part_type & xnode_type_t::virtual_part_mask;
    auto const result = in & xnode_type_t::real_part_mask;
    return result | high_part;
}

xnode_type_t
reset_real_part(xnode_type_t const in, xnode_type_t const desired_real_part_type) noexcept {
    auto const low_part = desired_real_part_type & xnode_type_t::real_part_mask;
    auto const result = in & xnode_type_t::virtual_part_mask;
    return result | low_part;
}

NS_END2

NS_BEG1(std)

std::size_t
hash<top::common::xnode_type_t>::operator()(top::common::xnode_type_t const type) const noexcept {
    return static_cast<std::size_t>(static_cast<std::underlying_type<top::common::xnode_type_t>::type>(type));
}

NS_END1
