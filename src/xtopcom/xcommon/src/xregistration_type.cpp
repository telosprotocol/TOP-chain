// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xregistration_type.h"

#include "xbase/xutl.h"

#include <cassert>
#include <type_traits>

NS_BEG2(top, common)

common::xregistration_type_t to_registration_type(std::string const & reg_type) {
    common::xregistration_type_t registration_type = common::xregistration_type_t::invalid;
    if (reg_type == XREG_TYPE_PRIMARY) {
        registration_type = common::xregistration_type_t::primary;
    } else if (reg_type == XREG_TYPE_INTERMEDIATE) {
        registration_type = common::xregistration_type_t::intermediate;
    } else if (reg_type == XREG_TYPE_SENIOR) {
        registration_type = common::xregistration_type_t::senior;
    }
    return registration_type;
}

std::string to_string(xregistration_type_t const & registration_type) {
    switch (registration_type) {
    case xregistration_type_t::hardcode:
        return XREG_TYPE_HARDCODE;
    case xregistration_type_t::senior:
        return XREG_TYPE_SENIOR;
    case xregistration_type_t::intermediate:
        return XREG_TYPE_INTERMEDIATE;
    case xregistration_type_t::primary:
        return XREG_TYPE_PRIMARY;

    default:
        assert(false);
        return "invalid";
    }
}

xregistration_type_t & operator&=(xregistration_type_t & lhs, xregistration_type_t const rhs) noexcept {
    lhs = lhs & rhs;
    return lhs;
}

xregistration_type_t & operator|=(xregistration_type_t & lhs, xregistration_type_t const rhs) noexcept {
    lhs = lhs | rhs;
    return lhs;
}

NS_END2
