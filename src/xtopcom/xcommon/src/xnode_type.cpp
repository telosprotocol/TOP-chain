// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xnode_type.h"

#include <cassert>

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
        string += "storage.";
    }

    if (has<xnode_type_t::frozen>(type)) {
        string += "frozen.";
    }

    if (has<xnode_type_t::consensus>(type)) {
        string += "consensus.";
    }

    if (has<xnode_type_t::full_node>(type)) {
        string += "fullnode.";
    }

    if (has<xnode_type_t::auditor>(type)) {
        string += "auditor.";
    }

    if (has<xnode_type_t::validator>(type)) {
        string += "validator.";
    }

    if (has<xnode_type_t::archive>(type)) {
        string += "archive.";
    }

    if (has<xnode_type_t::exchange>(type)) {
        string += "exchange.";
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
