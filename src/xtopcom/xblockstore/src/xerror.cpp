// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xblockstore/xerror/xerror.h"

#include <type_traits>

namespace top {
namespace store {
namespace error {

static char const * errc_to_message(int const errc) noexcept {
    auto ec = static_cast<xerrc_t>(errc);
    switch (ec) {
    case xerrc_t::ok:
        return "ok";

    case xerrc_t::store_create_genesis_cb_not_register:
        return "store_create_genesis_cb_not_register";

    default:  // NOLINT(clang-diagnostic-covered-switch-default)
        return "unknown error";
    }
}

class xtop_blockstore_category : public std::error_category {
public:
    const char * name() const noexcept override {
        return "blockstore";
    }

    std::string message(int errc) const override {
        return errc_to_message(errc);
    }
};
using xblockstore_category_t = xtop_blockstore_category;

std::error_code make_error_code(xerrc_t errc) noexcept {
    return std::error_code{ static_cast<int>(errc), blockstore_category() };
}

std::error_condition make_error_condition(xerrc_t errc) noexcept {
    return std::error_condition{ static_cast<int>(errc), blockstore_category() };
}

std::error_category const & blockstore_category() {
    static xblockstore_category_t category;  // NOLINT(clang-diagnostic-exit-time-destructors)
    return category;
}

}
}
}

NS_BEG1(std)

#if !defined(XCXX14_OR_ABOVE)

size_t hash<top::store::error::xerrc_t>::operator()(top::store::error::xerrc_t errc) const noexcept {
    return static_cast<size_t>(static_cast<std::underlying_type<top::store::error::xerrc_t>::type>(errc));
}

#endif

NS_END1
