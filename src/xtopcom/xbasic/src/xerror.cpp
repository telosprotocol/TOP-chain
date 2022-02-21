// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xerror/xerror.h"

#include "xbase/xlog.h"

#include <cinttypes>
#include <string>
#include <type_traits>

class xtop_base_category : public std::error_category {
public:
    const char * name() const noexcept override {
        return "base";
    }

    std::string message(int errc) const override {
        auto const ec = static_cast<enum_xerror_code>(errc);
        switch (ec) {
        case enum_xerror_code_bad_packet:
            return "bad packet";

        default:
            return "unknown error";
        }
    }
};
using xbase_category_t = xtop_base_category;

std::error_category const & base_category() {
    static xbase_category_t base_cagegory;
    return base_cagegory;
}

std::error_code make_error_code(enum_xerror_code ec) noexcept {
    return std::error_code{ static_cast<int>(ec), base_category() };
}

std::error_condition make_error_condition(enum_xerror_code ec) noexcept {
    return std::error_condition{ static_cast<int>(ec), base_category() };
}

NS_BEG2(top, error)

xtop_top_error::xtop_top_error(std::error_code ec) : base_t{ec.message()}, m_ec{std::move(ec)} {
}

xtop_top_error::xtop_top_error(std::error_code ec, char const * extra_what) : base_t{ec.message() + ": " + extra_what}, m_ec{std::move(ec)} {
}

xtop_top_error::xtop_top_error(std::error_code ec, std::string const & extra_what) : base_t{ec.message() + ": " + extra_what}, m_ec{std::move(ec)} {
}

xtop_top_error::xtop_top_error(int const ev, std::error_category const & ecat) : base_t{std::error_code{ev, ecat}.message()}, m_ec{ev, ecat} {
}

xtop_top_error::xtop_top_error(int const ev, std::error_category const & ecat, char const * extra_what)
  : base_t{std::error_code{ev, ecat}.message() + ": " + extra_what}, m_ec{ev, ecat} {
}

xtop_top_error::xtop_top_error(int const ev, std::error_category const & ecat, std::string const & extra_what)
  : base_t{std::error_code{ev, ecat}.message() + ": " + extra_what}, m_ec{ev, ecat} {
}

std::error_code const & xtop_top_error::code() const noexcept {
    return m_ec;
}

std::error_category const & xtop_top_error::category() const noexcept {
    return m_ec.category();
}

template <typename ExceptionT>
void throw_exception(ExceptionT const & eh) {
    throw eh;
}

static void do_throw_error(std::error_code const & ec) {
    xtop_error_t eh{ec};
    xwarn("throw_error. category %s, errc %" PRIi32 " msg %s", eh.category().name(), eh.code().value(), eh.what());
    throw_exception(eh);
}

static void do_throw_error(std::error_code const & ec, char const * extra_what) {
    xtop_error_t eh{ec, extra_what};
    xwarn("throw_error. category %s, errc %" PRIi32 " msg %s", eh.category().name(), eh.code().value(), eh.what());
    throw_exception(eh);
}

static void do_throw_error(std::error_code const & ec, std::string const & extra_what) {
    xtop_error_t eh{ec, extra_what};
    xwarn("throw_error. category %s, errc %" PRIi32 " msg %s", eh.category().name(), eh.code().value(), eh.what());
    throw_exception(eh);
}

void throw_error(std::error_code const & ec) {
    if (ec) {
        do_throw_error(ec);
    }
}

void throw_error(std::error_code const ec, char const * extra_what) {
    if (ec) {
        do_throw_error(ec, extra_what);
    }
}

void throw_error(std::error_code const ec, std::string const & extra_what) {
    if (ec) {
        do_throw_error(ec, extra_what);
    }
}

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
