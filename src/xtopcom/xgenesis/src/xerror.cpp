// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgenesis/xerror/xerror.h"

#include <type_traits>

namespace top {
namespace genesis {
namespace error {

static char const * errc_to_message(int const errc) noexcept {
    auto ec = static_cast<xerrc_t>(errc);
    switch (ec) {
    case xerrc_t::ok:
        return "ok";

    case xerrc_t::genesis_root_has_not_ready:
        return "genesis root has not ready";

    case xerrc_t::genesis_block_hash_mismatch:
        return "genesis block hash mismatch";

    case xerrc_t::genesis_block_store_failed:
        return "genesis block store failed";

    case xerrc_t::genesis_set_data_state_failed:
        return "genesis set data state failed";

    case xerrc_t::genesis_account_invalid:
        return "genesis account invalid";

    default:  // NOLINT(clang-diagnostic-covered-switch-default)
        return "unknown error";
    }
}

class xtop_genesis_category : public std::error_category {
public:
    const char * name() const noexcept override {
        return "genesis";
    }

    std::string message(int errc) const override {
        return errc_to_message(errc);
    }
};
using xgenesis_category_t = xtop_genesis_category;

std::error_code make_error_code(xerrc_t errc) noexcept {
    return std::error_code{ static_cast<int>(errc), genesis_category() };
}

std::error_condition make_error_condition(xerrc_t errc) noexcept {
    return std::error_condition{ static_cast<int>(errc), genesis_category() };
}

std::error_category const & genesis_category() {
    static xgenesis_category_t category;  // NOLINT(clang-diagnostic-exit-time-destructors)
    return category;
}

}
}
}

NS_BEG1(std)

#if !defined(XCXX14_OR_ABOVE)

size_t hash<top::genesis::error::xerrc_t>::operator()(top::genesis::error::xerrc_t errc) const noexcept {
    return static_cast<size_t>(static_cast<std::underlying_type<top::genesis::error::xerrc_t>::type>(errc));
}

#endif

NS_END1
