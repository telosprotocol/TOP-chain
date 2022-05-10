// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxexecutor/xerror/xerror.h"

#include <type_traits>

namespace top {
namespace txexecutor {
namespace error {

static char const * errc_to_message(int const errc) noexcept {
    auto ec = static_cast<xerrc_t>(errc);
    switch (ec) {
    case xerrc_t::ok:
        return "ok";

    default:  // NOLINT(clang-diagnostic-covered-switch-default)
        return "unknown error";
    }
}

class xtop_txexecutor_category : public std::error_category {
public:
    const char * name() const noexcept override {
        return "txexecutor";
    }

    std::string message(int errc) const override {
        return errc_to_message(errc);
    }
};
using xtxexecutor_category_t = xtop_txexecutor_category;

std::error_code make_error_code(xerrc_t errc) noexcept {
    return std::error_code{ static_cast<int>(errc), txexecutor_category() };
}

std::error_condition make_error_condition(xerrc_t errc) noexcept {
    return std::error_condition{ static_cast<int>(errc), txexecutor_category() };
}

std::error_category const & txexecutor_category() {
    static xtxexecutor_category_t category;  // NOLINT(clang-diagnostic-exit-time-destructors)
    return category;
}

}
}
}

NS_BEG1(std)

#if !defined(XCXX14_OR_ABOVE)

size_t hash<top::txexecutor::error::xerrc_t>::operator()(top::txexecutor::error::xerrc_t errc) const noexcept {
    return static_cast<size_t>(static_cast<std::underlying_type<top::txexecutor::error::xerrc_t>::type>(errc));
}

#endif

NS_END1
