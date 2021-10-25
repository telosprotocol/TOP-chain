// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xsymbol.h"
#include "xstate_accessor/xerror/xerror.h"

#include <cstdint>
#include <system_error>

NS_BEG2(top, state_accessor)

enum class xenum_token_type {
    invalid,
    TOP,
    VPN
};
using xtoken_type_t = xenum_token_type;

class xtop_token {
    uint64_t amount_{ 0 };
    common::xsymbol_t symbol_{common::SYMBOL_TOP_TOKEN};

public:
    xtop_token() = default;
    xtop_token(xtop_token const &) = delete;
    xtop_token & operator=(xtop_token const &) = delete;

    xtop_token & operator=(xtop_token && other);
    xtop_token(xtop_token && other) noexcept;

    explicit xtop_token(common::xsymbol_t symbol);
    explicit xtop_token(std::uint64_t const amount, common::xsymbol_t symbol);
    explicit xtop_token(std::uint64_t amount);

    ~xtop_token() noexcept;

public:

    bool operator==(xtop_token const & other) const noexcept;
    bool operator!=(xtop_token const & other) const noexcept;
    bool operator<(xtop_token const & other) const;
    bool operator>(xtop_token const & other) const;
    bool operator<=(xtop_token const & other) const;
    bool operator>=(xtop_token const & other) const;
    xtop_token & operator+=(xtop_token & other) noexcept;

    bool invalid() const noexcept;
    uint64_t amount() const noexcept;
    common::xsymbol_t const & symbol() const noexcept;
    void clear() noexcept;

    std::int32_t serialize_to(base::xstream_t & stream);
    std::int32_t serialize_from(base::xstream_t & stream);
    std::int32_t serialize_to(base::xbuffer_t & buffer);
    std::int32_t serialize_from(base::xbuffer_t & buffer);

private:
    int32_t do_read(base::xstream_t& stream);
    int32_t do_write(base::xstream_t& stream); // move token to serialize stream

};
using xtoken_t = xtop_token;


int32_t operator>>(base::xstream_t& stream, xtoken_t data_object);
int32_t operator<<(base::xstream_t& stream, xtoken_t data_object);
int32_t operator>>(base::xbuffer_t& buffer, xtoken_t data_object);
int32_t operator<<(base::xbuffer_t& buffer, xtoken_t data_object);

NS_END2

NS_BEG1(std)

template <>
struct hash<top::state_accessor::xtoken_t> {
    size_t operator()(top::state_accessor::xtoken_t const & amount) const noexcept;
};

NS_END1
