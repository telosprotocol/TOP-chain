// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xutility.h"
#include "xnetwork/xmessage_cache_container.h"

NS_BEG1(top)

xtop_message_cache_container::xtop_message_cache_container(std::size_t const cache_size)
    : m_cache_size{ cache_size }
{
}

std::pair<xtop_message_cache_container::cache_element_key_t, xtop_message_cache_container::cache_element_value_t>
xtop_message_cache_container::insert(cache_element_key_t const & message_uuid, network::xendpoint_t const & endpoint) {
    std::lock_guard<std::mutex> lock{ m_cached_messages_mutex };

    auto const index_iterator = m_cached_messages_index.find(message_uuid);

    if (index_iterator != std::end(m_cached_messages_index)) {
        auto const element_iterator = top::get<message_container_t::iterator>(*index_iterator);
        auto endpoints = std::move(top::get<cache_element_value_t>(*element_iterator));
        endpoints.push_front(endpoint);

        if (endpoints.size() > m_cache_size) {
            endpoints.pop_back();
        }

        m_cached_messages.erase(element_iterator);
        m_cached_messages_index.erase(index_iterator);

        auto result = std::make_pair(message_uuid, std::move(endpoints));

        m_cached_messages.push_front(result);
        m_cached_messages_index[message_uuid] = m_cached_messages.begin();

        //xinfo("[dispatcher] found message %s in cache", message_uuid.c_str());

        return result;
    }

    auto result = std::make_pair(message_uuid, cache_element_value_t{ endpoint });

    m_cached_messages.push_front(result);
    m_cached_messages_index[message_uuid] = m_cached_messages.begin();

    if (m_cached_messages_index.size() > m_cache_size) {
        auto const last = std::prev(m_cached_messages.end());
        m_cached_messages_index.erase(top::get<cache_element_key_t>(*last));
        m_cached_messages.pop_back();
    }

    //xinfo("[dispatcher] message %s added into cache", message_uuid.c_str());

    return result;
}

xtop_message_cache_container::cache_element_value_t
xtop_message_cache_container::get(cache_element_key_t const & message_uuid) const {
    std::lock_guard<std::mutex> lock{ m_cached_messages_mutex };
    auto const index_iterator = m_cached_messages_index.find(message_uuid);
    if (index_iterator != std::end(m_cached_messages_index)) {
        auto const element_iterator = top::get<message_container_t::iterator>(*index_iterator);
        return top::get<cache_element_value_t>(*element_iterator);
    }

    return {};
}

NS_END1
