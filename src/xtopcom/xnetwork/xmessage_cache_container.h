// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xnetwork/xendpoint.h"

#include <cstddef>
#include <deque>
#include <list>
#include <mutex>
#include <string>
#include <unordered_map>

NS_BEG1(top)

class xtop_message_cache_container final
{
public:
    using cache_element_key_t = std::string;
    using cache_element_value_t = std::deque<network::xendpoint_t>;

private:
    using cache_container_element_t = std::pair<cache_element_key_t, cache_element_value_t>;
    using message_container_t = std::list<cache_container_element_t>;

    mutable std::mutex m_cached_messages_mutex{};
    message_container_t m_cached_messages{};
    std::unordered_map<cache_element_key_t, message_container_t::iterator> m_cached_messages_index{};

    std::size_t const m_cache_size;

public:
    explicit xtop_message_cache_container(std::size_t const cache_size);

    xtop_message_cache_container(xtop_message_cache_container const &)              = delete;
    xtop_message_cache_container & operator=(xtop_message_cache_container const &)  = delete;
    xtop_message_cache_container(xtop_message_cache_container &&)                   = default;
    xtop_message_cache_container & operator=(xtop_message_cache_container &&)       = default;
    ~xtop_message_cache_container()                                                 = default;

    std::pair<cache_element_key_t, cache_element_value_t>
    insert(cache_element_key_t const & message_uuid, network::xendpoint_t const & endpoint);

    cache_element_value_t
    get(cache_element_key_t const & message_uuid) const;
};

using xmessage_cache_container_t = xtop_message_cache_container;

NS_END1
