// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xhashable.hpp"

#include <cstdint>
#include <functional>
#include <string>

NS_BEG2(top, network)

class xtop_endpoint final : public xhashable_t<xtop_endpoint, std::size_t>
{
    std::string m_address{};
    std::uint16_t m_port{};

public:
    xtop_endpoint()                                  = default;
    xtop_endpoint(xtop_endpoint const &)             = default;
    xtop_endpoint & operator=(xtop_endpoint const &) = default;
    xtop_endpoint(xtop_endpoint &&)                  = default;
    xtop_endpoint & operator=(xtop_endpoint &&)      = default;
    ~xtop_endpoint()                                 = default;

    xtop_endpoint(std::string addr, std::uint16_t const p);

    bool
    operator==(xtop_endpoint const & other) const noexcept;

    bool
    operator!=(xtop_endpoint const & other) const noexcept;

    operator bool() const noexcept;

    bool
    empty() const noexcept;

    std::string const &
    address() const noexcept;

    std::uint16_t
    port() const noexcept;

    std::size_t
    hash() const override;

    std::string
    to_string() const;
};

using xendpoint_t = xtop_endpoint;

NS_END2

NS_BEG1(std)

template <>
struct hash<top::network::xendpoint_t> final
{
    std::size_t
    operator()(top::network::xendpoint_t const & endpoint) const noexcept;
};

NS_END1
