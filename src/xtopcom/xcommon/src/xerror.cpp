// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xerror/xerror.h"

#include <cassert>

NS_BEG3(top, common, error)

static char const * errc_to_string(xerrc_t const errc) noexcept {
    switch (errc) {
    case xerrc_t::invalid_rotation_status:
        return "invalid rotation status";

    case xerrc_t::invalid_account_address:
        return "invalid account address";

    case xerrc_t::invalid_account_base_address:
        return "invalid account base address";

    case xerrc_t::invalid_account_index:
        return "invalid account index";

    case xerrc_t::invalid_table_id:
        return "invalid table id";

    case xerrc_t::invalid_ledger_id:
        return "invalid ledger id";

    case xerrc_t::invalid_account_type:
        return "invalid account type";

    case xerrc_t::token_not_used:
        return "token not found";

    case xerrc_t::token_symbol_not_matched:
        return "token symbol not matched";

    case xerrc_t::token_not_predefined:
        return "not predefined token";

    case xerrc_t::invalid_rlp_stream:
        return "invalid rlp stream";

    case xerrc_t::empty_token_symbol:
        return "empty token symbol";

    case xerrc_t::token_symbol_unknown:
        return "unknown token symbol";

    case xerrc_t::invalid_block:
        return "invalid block";

    case xerrc_t::invalid_db_load:
        return "invalid db load";

    case xerrc_t::invalid_eth_tx:
        return "invalid eth tx";

    case xerrc_t::invalid_eth_header:
        return "invalid eth header";

    case xerrc_t::invalid_bloom:
        return "invalid bloom";

    case xerrc_t::table_id_mismatch:
        return "table id mismatched";

    default:
        assert(false);
        return "unknown common category error";
    }
}

std::error_code make_error_code(xerrc_t const errc) noexcept {
    return std::error_code{static_cast<int>(errc), common_category() };
}

std::error_condition make_error_condition(xerrc_t const errc) noexcept {
    return std::error_condition{ static_cast<int>(errc), common_category() };
}

class xtop_common_category : public std::error_category {
public:
    const char * name() const noexcept override {
        return "common";
    }

    std::string message(int errc) const override {
        auto const ec = static_cast<xerrc_t>(errc);
        return errc_to_string(ec);
    }
};
using xcommon_category_t = xtop_common_category;

std::error_category const & common_category() noexcept {
    static xcommon_category_t c;
    return c;
}

NS_END3

NS_BEG1(std)

#if !defined(XCXX14_OR_ABOVE)

size_t hash<top::common::error::xerrc_t>::operator()(top::common::error::xerrc_t const errc) const noexcept {
    return static_cast<size_t>(static_cast<int>(errc));
}

#endif

NS_END1
