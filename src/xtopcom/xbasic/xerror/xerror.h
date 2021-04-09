#pragma once

#include "xbasic/xns_macro.h"

#include <limits>
#include <system_error>
#include <type_traits>

NS_BEG2(top, error)

enum class xenum_basic_errc {
    successful,
    serialization_error,
    deserialization_error,

    unknown_error = std::numeric_limits<std::underlying_type<xenum_basic_errc>::type>::max()
};
using xbasic_errc_t = xenum_basic_errc;

std::error_code make_error_code(xbasic_errc_t errc) noexcept;
std::error_condition make_error_condition(xbasic_errc_t errc) noexcept;

std::error_category const & basic_category();

class xtop_basic_error : public std::runtime_error {
public:
    xtop_basic_error(xbasic_errc_t const error_code);
    xtop_basic_error(xbasic_errc_t const error_code, std::string extra_msg);

    std::error_code const & code() const noexcept;

private:
    xtop_basic_error(std::error_code ec);
    xtop_basic_error(std::error_code ec, std::string extra_msg);

private:
    std::error_code m_ec;
};
using xbasic_error_t = xtop_basic_error;

NS_END2

NS_BEG1(std)

#if !defined(XCXX14_OR_ABOVE)

template <>
struct hash<top::error::xbasic_errc_t> final {
    size_t operator()(top::error::xbasic_errc_t errc) const noexcept;
};

template <>
struct is_error_code_enum<top::error::xbasic_errc_t> : std::true_type {};

template <>
struct is_error_condition_enum<top::error::xbasic_errc_t> : std::true_type {};

#endif

NS_END1
