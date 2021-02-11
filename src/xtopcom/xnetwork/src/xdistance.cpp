// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xnetwork/xp2p/xdistance.hpp"

NS_BEG1(std)

std::size_t
hash<top::network::p2p::xdistance_t>::operator()(top::network::p2p::xdistance_t const & distance) const noexcept {
    return static_cast<std::size_t>(static_cast<std::size_t>(distance));
}

void
swap(top::network::p2p::xdistance_t & lhs, top::network::p2p::xdistance_t & rhs) noexcept {
    lhs.swap(rhs);
}

NS_END1