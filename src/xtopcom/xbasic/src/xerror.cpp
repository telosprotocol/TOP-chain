#include "xbasic/xerror/xerror.h"

#include <type_traits>

NS_BEG2(top, error)

static char const * errc_to_message(int const errc) noexcept {
    auto ec = static_cast<error::xbasic_errc_t>(errc);
    switch (ec) {
    case xbasic_errc_t::successful:
        return "successful";

    case xbasic_errc_t::serialization_error:
        return "serialization error";

    case xbasic_errc_t::deserialization_error:
        return "deserialization error";

    default:
        return "unknown error";
    }
}

class xtop_basic_category : public std::error_category {
public:
    const char * name() const noexcept override {
        return "basic";
    }

    std::string message(int errc) const override {
        return errc_to_message(errc);
    }
};
using xbasic_category_t = xtop_basic_category;

std::error_code make_error_code(xbasic_errc_t errc) noexcept {
    return std::error_code(static_cast<int>(errc), basic_category());
}

std::error_condition make_error_condition(xbasic_errc_t errc) noexcept {
    return std::error_condition(static_cast<int>(errc), basic_category());
}

std::error_category const & basic_category() {
    static xbasic_category_t category;
    return category;
}

NS_END2

NS_BEG1(std)

#if !defined(XCXX14_OR_ABOVE)

size_t hash<top::error::xbasic_errc_t>::operator()(top::error::xbasic_errc_t errc) const noexcept {
    return static_cast<size_t>(static_cast<std::underlying_type<top::error::xbasic_errc_t>::type>(errc));
}

#endif

NS_END1
