// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xapplication/xerror/xerror.h"

#include <type_traits>

namespace top {
namespace application {
namespace error {

static char const * errc_to_message(int const errc) noexcept {
    auto ec = static_cast<xerrc_t>(errc);
    switch (ec) {
    case xerrc_t::successful:
        return "successful";

    case xerrc_t::load_election_data_failed:
        return "load election data failed";

    default:
        return "unknown error";
    }
}

class xtop_application_category : public std::error_category {
public:
    const char * name() const noexcept override {
        return "application";
    }

    std::string message(int errc) const override {
        return errc_to_message(errc);
    }
};
using xapplication_category_t = xtop_application_category;

std::error_code make_error_code(xerrc_t errc) noexcept {
    return std::error_code(static_cast<int>(errc), application_category());
}

std::error_condition make_error_condition(xerrc_t errc) noexcept {
    return std::error_condition(static_cast<int>(errc), application_category());
}

std::error_category const & application_category() {
    static xapplication_category_t category;
    return category;
}

}
}
}

NS_BEG1(std)

#if !defined(XCXX14_OR_ABOVE)

size_t hash<top::application::error::xbasic_errc_t>::operator()(top::application::error::xbasic_errc_t errc) const noexcept {
    return static_cast<size_t>(static_cast<std::underlying_type<top::application::error::xbasic_errc_t>::type>(errc));
}

#endif

NS_END1
