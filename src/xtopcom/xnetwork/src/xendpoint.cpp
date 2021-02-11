// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xnetwork/xendpoint.h"

NS_BEG2(top, network)

xtop_endpoint::xtop_endpoint(std::string address, std::uint16_t const p)
    : m_address{ std::move(address) }, m_port{ p }
{
}

bool
xtop_endpoint::operator==(xtop_endpoint const & other) const noexcept {
    return m_address == other.m_address && m_port == other.m_port;
}

bool xtop_endpoint::operator!=(xtop_endpoint const & other) const noexcept {
    return !(*this == other);
}

xtop_endpoint::operator bool() const noexcept {
    return !empty();
}

bool
xtop_endpoint::empty() const noexcept {
    return m_address.empty() && m_port == std::uint16_t{};
}

std::string const &
xtop_endpoint::address() const noexcept {
    return m_address;
}

std::uint16_t
xtop_endpoint::port() const noexcept {
    return m_port;
}

std::size_t
xtop_endpoint::hash() const {
    return std::hash<std::string>{}(address()) ^ std::hash<std::uint16_t>{}(port());
}

std::string
xtop_endpoint::to_string() const {
    return m_address + u8":" + std::to_string(m_port);
}

NS_END2

NS_BEG1(std)

std::size_t
hash<top::network::xendpoint_t>::operator()(top::network::xendpoint_t const & endpoint) const noexcept {
    return endpoint.hash();
}

NS_END1
