// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xnetwork/xp2p/xnode_entry.hpp"

#include <cstddef>
#include <list>
#include <memory>

NS_BEG3(top, network, p2p)

template <std::size_t K>
class xtop_kbucket final
{
private:
    static constexpr std::size_t m_k{ K };
    std::list<std::weak_ptr<xnode_entry_t>> m_nodes{};

public:
    xtop_kbucket()                                 = default;
    xtop_kbucket(xtop_kbucket const &)             = delete;
    xtop_kbucket & operator=(xtop_kbucket const &) = delete;
    xtop_kbucket(xtop_kbucket &&)                  = default;
    xtop_kbucket & operator=(xtop_kbucket &&)      = default;
    ~xtop_kbucket()                                = default;

    static constexpr
    std::size_t
    k() noexcept {
        return m_k;
    }

    std::list<std::weak_ptr<xnode_entry_t>> &
    nodes() noexcept {
        return m_nodes;
    }

    std::list<std::weak_ptr<xnode_entry_t>> const &
    nodes() const noexcept {
        return m_nodes;
    }
};

template <std::size_t K>
using xkbucket_t = xtop_kbucket<K>;

NS_END3
