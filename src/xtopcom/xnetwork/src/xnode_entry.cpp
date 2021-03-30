// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xnetwork/xp2p/xnode_entry.hpp"

NS_BEG1(std)

std::size_t
hash<top::network::p2p::xnode_entry_t>::operator()(top::network::p2p::xnode_entry_t const & node_entry) const noexcept {
    return std::hash<top::network::p2p::xnode_entry_t::id_type>{}(node_entry.id());
}

void
swap(top::network::p2p::xnode_entry_t & lhs, top::network::p2p::xnode_entry_t & rhs) noexcept {
    lhs.swap(rhs);
}

NS_END1
