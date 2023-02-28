// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xvm_error_category.h"
#include "xvm_error_code.h"
#include <string>
NS_BEG2(top, xvm)
using std::string;

static string xcodec_errc_map(int const errc) noexcept {
    auto const ec = static_cast<enum_xvm_error_code>(errc);
    switch (ec) {
        case enum_xvm_error_code::ok:
            return "ok";
        case enum_xvm_error_code::query_contract_data_fail_to_get_block:
            return "query_contract_data_fail_to_get_block";
        case enum_xvm_error_code::query_contract_data_property_missing:
            return "query_contract_data_property_missing";
        case enum_xvm_error_code::query_contract_data_property_empty:
            return "query_contract_data_property_empty";
        default:
            return string("Unknown code " + std::to_string(errc));
    }
}

class xvm_category final : public std::error_category
{
    const char* name() const noexcept override {
        return "xvm::error";
    }

    std::string message(int errc) const override {
        return xcodec_errc_map(errc);
    }
};

const std::error_category& xvm_get_category() {
    static xvm_category category{};
    return category;
}
NS_END2
