// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/xwrouter_error.h"

#include <string>

NS_BEG2(top, xwrouter)

static std::string xwrouter_errc_map(int const errc) noexcept {
    auto const ec = static_cast<xwrouter_error_t>(errc);
    switch (ec) {
    case xwrouter_error_t::success:
        return "success";
    case xwrouter_error_t::hop_num_beyond_max:
        return "hop_num_beyond_max";
    case xwrouter_error_t::not_find_routing_table:
        return "not_find_routing_table";
    case xwrouter_error_t::empty_dst_address:
        return "empty_dst_address";
    case xwrouter_error_t::routing_find_zero_closest_nodes:
        return "routing_find_zero_closest_nodes";
    case xwrouter_error_t::crossing_network_fail:
        return "crossing_network_fail";
    case xwrouter_error_t::multi_send_partial_fail:
        return "multi_send_partial_fail";
    case xwrouter_error_t::serialized_fail:
        return "serialized_fail";
    default:
        return "unknown error";
    }
};

class xtop_wrouter_category final : public std::error_category {
public:
    const char * name() const noexcept override {
        return "wrouter";
    }

    std::string message(int errc) const override {
        return xwrouter_errc_map(errc);
    }
};
using xwrouter_category_t = xtop_wrouter_category;

std::error_code make_error_code(xwrouter_error_t const errc) noexcept {
    return std::error_code{static_cast<int>(errc), wrouter_category()};
}

std::error_condition make_error_condition(xwrouter_error_t const errc) noexcept {
    return std::error_condition{static_cast<int>(errc), wrouter_category()};
}

std::error_category const & wrouter_category() {
    static xwrouter_category_t category{};
    return category;
}

NS_END2
