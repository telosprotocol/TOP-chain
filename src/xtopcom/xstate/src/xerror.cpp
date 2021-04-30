// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate/xerror/xerror.h"

NS_BEG3(top, state, error)

static char const * errc_to_message(int const errc) noexcept {
    auto ec = static_cast<error::xerrc_t>(errc);
    switch (ec) {
    case xerrc_t::ok:
        return "successful";

    case xerrc_t::token_insufficient:
        return "token insufficient";

    default:
        return "unknown error";
    }
}

class xtop_state_category : public std::error_category {
public:
    const char * name() const noexcept override {
        return "state";
    }

    std::string message(int errc) const override {
        return errc_to_message(errc);
    }
};
using xstate_category_t = xtop_state_category;

std::error_code make_error_code(xerrc_t errc) noexcept {
    return std::error_code(static_cast<int>(errc), state_category());
}

std::error_condition make_error_condition(xerrc_t errc) noexcept {
    return std::error_condition(static_cast<int>(errc), state_category());
}

std::error_category const & state_category() {
    static xstate_category_t category;
    return category;
}

NS_END3

NS_BEG1(std)

#if !defined(XCXX14_OR_ABOVE)

size_t hash<top::state::error::xerrc_t>::operator()(top::state::error::xerrc_t errc) const noexcept {
    return static_cast<size_t>(static_cast<std::underlying_type<top::state::error::xerrc_t>::type>(errc));
}

#endif

NS_END1
