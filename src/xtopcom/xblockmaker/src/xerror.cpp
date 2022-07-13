// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xblockmaker/xerror/xerror.h"

#include <type_traits>

namespace top {
namespace blockmaker {
namespace error {

static char const * errc_to_message(int const errc) noexcept {
    auto ec = static_cast<xerrc_t>(errc);
    switch (ec) {
    case xerrc_t::ok:
        return "ok";

    case xerrc_t::blockmaker_load_block_fail:
        return "blockmaker_load_block_fail";

    case xerrc_t::blockmaker_cert_block_changed:
        return "blockmaker_cert_block_changed";

    case xerrc_t::blockmaker_connect_block_behind:
        return "blockmaker_connect_block_behind";

    case xerrc_t::blockmaker_drand_block_invalid:
        return "blockmaker_drand_block_invalid";

    case xerrc_t::blockmaker_make_table_state_fail:
        return "blockmaker_make_table_state_fail";

    case xerrc_t::blockmaker_create_statectx_fail:
        return "blockmaker_create_statectx_fail";

    case xerrc_t::blockmaker_make_unit_fail:
        return "blockmaker_make_unit_fail";

    case xerrc_t::blockmaker_relayblock_unconnect:
        return "blockmaker_relayblock_unconnect";

    case xerrc_t::blockmaker_load_unitstate:
        return "blockmaker_load_unitstate";

    case xerrc_t::blockmaker_property_invalid:
        return "blockmaker_property_invalid";

    default:  // NOLINT(clang-diagnostic-covered-switch-default)
        return "unknown error";
    }
}

class xtop_blockmaker_category : public std::error_category {
public:
    const char * name() const noexcept override {
        return "blockmaker";
    }

    std::string message(int errc) const override {
        return errc_to_message(errc);
    }
};
using xblockmaker_category_t = xtop_blockmaker_category;

std::error_code make_error_code(xerrc_t errc) noexcept {
    return std::error_code{ static_cast<int>(errc), blockmaker_category() };
}

std::error_condition make_error_condition(xerrc_t errc) noexcept {
    return std::error_condition{ static_cast<int>(errc), blockmaker_category() };
}

std::error_category const & blockmaker_category() {
    static xblockmaker_category_t category;  // NOLINT(clang-diagnostic-exit-time-destructors)
    return category;
}

}
}
}

NS_BEG1(std)

#if !defined(XCXX14_OR_ABOVE)

size_t hash<top::blockmaker::error::xerrc_t>::operator()(top::blockmaker::error::xerrc_t errc) const noexcept {
    return static_cast<size_t>(static_cast<std::underlying_type<top::blockmaker::error::xerrc_t>::type>(errc));
}

#endif

NS_END1
