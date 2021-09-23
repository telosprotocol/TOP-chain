// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xrole_type.h"

#include "xbase/xutl.h"

#include <cassert>
#include <type_traits>

NS_BEG2(top, common)

std::int32_t operator<<(top::base::xstream_t & stream, xrole_type_t const & role_type) {
    return stream << static_cast<std::underlying_type<xrole_type_t>::type>(role_type);
}

std::int32_t operator>>(top::base::xstream_t & stream, xrole_type_t & role_type) {
    std::underlying_type<xrole_type_t>::type temp;
    auto r = stream >> temp;
    role_type = static_cast<xrole_type_t>(temp);
    return r;
}

std::string to_string(xrole_type_t const role) {
    switch (role) {
    case xrole_type_t::advance:
        return XMINER_TYPE_ADVANCE;

    case xrole_type_t::full_node:
        return XMINER_TYPE_FULL_NODE;

    case xrole_type_t::validator:
        return XMINER_TYPE_VALIDATOR;

    case xrole_type_t::archive:
        return XMINER_TYPE_ARCHIVE;

    case xrole_type_t::edge:
        return XMINER_TYPE_EDGE;

    default:
        assert(false);
        return "invalid(" + std::to_string(static_cast<std::underlying_type<xrole_type_t>::type>(role)) + ")";
    }
}

xrole_type_t & operator&=(xrole_type_t & lhs, xrole_type_t const rhs) noexcept {
    lhs = lhs & rhs;
    return lhs;
}

xrole_type_t & operator|=(xrole_type_t & lhs, xrole_type_t const rhs) noexcept {
    lhs = lhs | rhs;
    return lhs;
}

bool has(xrole_type_t const target, xrole_type_t const input) noexcept {
    return target == (target & input);
}

common::xrole_type_t to_role_type(std::string const & node_type) {
    common::xrole_type_t role_type = common::xrole_type_t::invalid;

    if (node_type == XMINER_TYPE_EDGE) {
        role_type = common::xrole_type_t::edge;
    } else if (node_type == XMINER_TYPE_ADVANCE) {
        role_type = common::xrole_type_t::advance;
    } else if (node_type == XMINER_TYPE_VALIDATOR) {
        role_type = common::xrole_type_t::validator;
    } else if (node_type == XMINER_TYPE_ARCHIVE) {
        role_type = common::xrole_type_t::archive;
    } else if (node_type == XMINER_TYPE_FULL_NODE) {
        role_type = common::xrole_type_t::full_node;
    }

    return role_type;
}

NS_END2
