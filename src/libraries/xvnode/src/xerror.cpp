// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnode/xerror/xerror.h"

#include <cassert>

NS_BEG3(top, vnode, error)

static char const * errc_to_string(xerrc_t const errc) noexcept {
    switch (errc) {
    case xerrc_t::vnode_is_not_running:
        return "vnode is not running";

     case xerrc_t::invalid_address:
        return "invalid address";

    default:
        assert(false);
        return "unknown vnode category error";
    }
}

std::error_code make_error_code(xerrc_t const errc) noexcept {
    return std::error_code{static_cast<int>(errc), vnode_category() };
}

std::error_condition make_error_condition(xerrc_t const errc) noexcept {
    return std::error_condition{ static_cast<int>(errc), vnode_category() };
}

class xtop_vnode_category : public std::error_category {
public:
    const char * name() const noexcept override {
        return "vnode";
    }

    std::string message(int errc) const override {
        auto const ec = static_cast<xerrc_t>(errc);
        return errc_to_string(ec);
    }
};
using xvnode_category_t = xtop_vnode_category;

std::error_category const & vnode_category() noexcept {
    static xvnode_category_t c;
    return c;
}

NS_END3

NS_BEG1(std)

#if !defined(XCXX14)

size_t hash<top::vnode::error::xerrc_t>::operator()(top::vnode::error::xerrc_t const errc) const noexcept {
    return static_cast<size_t>(static_cast<int>(errc));
}

#endif

NS_END1
