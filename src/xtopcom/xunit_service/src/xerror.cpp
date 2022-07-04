// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunit_service/xerror/xerror.h"

#include <string>

NS_BEG3(top, xunit_service, error)

static char const * const errc_to_string(int code) {
    auto const ec = static_cast<xerrc_t>(code);

    switch (ec) {
    case xerrc_t::ok:
        return "ok";

    case xerrc_t::vnode_already_exist:
        return "vnode already exist";

    case xerrc_t::vnode_not_exist:
        return "vnode not exist";

    case xerrc_t::serialization_error:
        return "serialization error";

    case xerrc_t::packer_cert_block_invalid:
        return "packer_cert_block_invalid";

    case xerrc_t::packer_view_behind:
        return "packer_view_behind";

    default:
        return "unknown unit service module error";
    }
}

std::error_code make_error_code(xerrc_t const errc) noexcept {
    return std::error_code(static_cast<int>(errc), unit_service_category());
}

std::error_condition make_error_condition(xerrc_t const errc) noexcept {
    return std::error_condition(static_cast<int>(errc), unit_service_category());
}

class xtop_unit_service_category final : public std::error_category {
    char const * name() const noexcept override {
        return "unit service";
    }

    std::string message(int errc) const override {
        return errc_to_string(errc);
    }
};
using xunit_service_category_t = xtop_unit_service_category;

std::error_category const & unit_service_category() {
    static xunit_service_category_t category{};
    return category;
}

NS_END3
