// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgasfee/xerror/xerror.h"

#include <type_traits>

namespace top {
namespace gasfee {
namespace error {

static char const * errc_to_message(int const errc) noexcept {
    auto ec = static_cast<xerrc_t>(errc);
    switch (ec) {
    case xerrc_t::ok:
        return "ok";
    
    case xerrc_t::account_withdraw_failed:
        return "account withdraw failed";

    case xerrc_t::account_deposit_failed:
        return "account deposit failed";

    case xerrc_t::account_property_set_failed:
        return "account property set failed";

    case xerrc_t::account_balance_not_enough:
        return "account balance not enough";

    case xerrc_t::tx_deposit_not_enough:
        return "tx deposit not enough";

    case xerrc_t::tx_out_of_gas:
        return "tx out of gas";

    case xerrc_t::invalid_param:
        return "invalid tx param";

    default:  // NOLINT(clang-diagnostic-covered-switch-default)
        return "unknown error";
    }
}

class xtop_gasfee_category : public std::error_category {
public:
    const char * name() const noexcept override {
        return "gasfee";
    }

    std::string message(int errc) const override {
        return errc_to_message(errc);
    }
};
using xgasfee_category_t = xtop_gasfee_category;

std::error_code make_error_code(xerrc_t errc) noexcept {
    return std::error_code{ static_cast<int>(errc), gasfee_category() };
}

std::error_condition make_error_condition(xerrc_t errc) noexcept {
    return std::error_condition{ static_cast<int>(errc), gasfee_category() };
}

std::error_category const & gasfee_category() {
    static xgasfee_category_t category;  // NOLINT(clang-diagnostic-exit-time-destructors)
    return category;
}

}
}
}

NS_BEG1(std)

#if !defined(XCXX14_OR_ABOVE)

size_t hash<top::gasfee::error::xerrc_t>::operator()(top::gasfee::error::xerrc_t errc) const noexcept {
    return static_cast<size_t>(static_cast<std::underlying_type<top::gasfee::error::xerrc_t>::type>(errc));
}

#endif

NS_END1
