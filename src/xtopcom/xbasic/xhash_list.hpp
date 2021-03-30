// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xutility.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <list>
#include <stdexcept>
#include <map>
#include <unordered_map>

NS_BEG1(top)

enum class xenum_hash_list_update_direction : std::uint8_t
{
    to_front,
    to_back
};

using xhash_list_update_direction_t = xenum_hash_list_update_direction;

template <typename KeyT, typename T>
class xtop_hash_list final
{
private:
    using aux_map_t = std::map<KeyT, T>;

public:
    using value_type = typename aux_map_t::value_type;
    using mapped_type = typename aux_map_t::mapped_type;

private:
    using list_t = std::list<value_type>;

public:
    using size_type                = typename list_t::size_type;
    using reference                = typename list_t::reference;
    using const_reference          = typename list_t::const_reference;
    using list_iterator            = typename list_t::iterator;
    using list_const_iterator      = typename list_t::const_iterator;

private:
    using map_t = std::map<KeyT, list_iterator>;

public:
    using key_type                 = typename map_t::key_type;
    using hashtable_iterator       = typename map_t::iterator;
    using hashtable_const_iterator = typename map_t::const_iterator;

private:

    list_t m_list{};
    map_t m_map{};

public:

    void
    swap(xtop_hash_list & other) noexcept {
        m_list.swap(other.m_list);
        m_map.swap(other.m_map);
    }

    void
    push_back(std::pair<KeyT, T> const & value) {
        auto const map_iterator = m_map.find(top::get<key_type>(value));
        if (map_iterator != std::end(m_map)) {
            auto const lit = top::get<list_iterator>(*map_iterator);

            m_list.erase(lit);
            m_map.erase(map_iterator);
        }

        m_list.push_back(value);
        m_map[top::get<key_type>(value)] = std::prev(m_list.end());
    }

    void
    push_back(std::pair<KeyT, T> && value) {
        auto const map_iterator = m_map.find(top::get<key_type>(value));
        if (map_iterator != std::end(m_map)) {
            auto const lit = top::get<list_iterator>(*map_iterator);

            m_list.erase(lit);
            m_map.erase(map_iterator);
        }

        auto key = top::get<key_type>(value);
        m_list.push_back(std::move(value));
        m_map[key] = std::prev(m_list.end());
    }

    void
    push_front(std::pair<KeyT, T> const & value) {
        auto const map_iterator = m_map.find(top::get<key_type>(value));
        if (map_iterator != std::end(m_map))
        {
            auto const lit = top::get<list_iterator>(*map_iterator);

            m_list.erase(lit);
            m_map.erase(map_iterator);
        }

        m_list.push_front(value);
        m_map[top::get<key_type>(m_list.front())] = m_list.begin();
    }

    void
    push_front(std::pair<KeyT, T> && value) {
        auto const map_iterator = m_map.find(top::get<key_type>(value));
        if (map_iterator != std::end(m_map)) {
            auto const lit = top::get<list_iterator>(*map_iterator);

            m_list.erase(lit);
            m_map.erase(map_iterator);
        }

        m_list.push_front(std::move(value));
        m_map[top::get<key_type>(m_list.front())] = m_list.begin();
    }

    reference
    front() {
        return m_list.front();
    }

    const_reference
    front() const {
        return m_list.front();
    }

    reference
    back() {
        return m_list.back();
    }

    const_reference
    back() const {
        return m_list.back();
    }

    XATTRIBUTE_NODISCARD
    bool
    empty() const noexcept {
        return m_list.empty();
    }

    size_type
    size() const noexcept {
        return m_list.size();
    }

    void
    clear() noexcept {
        m_list.clear();
        m_map.clear();
    }

    void
    pop_back() {
        m_map.erase(top::get<key_type const>(m_list.back()));
        m_list.pop_back();
    }

    void
    pop_front() {
        m_map.erase(top::get<key_type const>(m_list.front()));
        m_list.pop_front();
    }

    size_type
    erase(key_type const & key) {
        auto const map_iterator = m_map.find(key);
        if (map_iterator == std::end(m_map)) {
            return 0;
        }

        auto const lci = top::get<list_iterator>(*map_iterator);
        auto ret = m_map.erase(key);
        m_list.erase(lci);

        return ret;
    }

    mapped_type &
    at(key_type const & key, xhash_list_update_direction_t const direction) {
        mapped_type value{};
        auto const map_iterator = m_map.find(key);
        if (map_iterator != std::end(m_map)) {
            auto const lci = top::get<list_iterator>(*map_iterator);

            // convert const_iterator to iterator
            auto li = begin();
            std::advance(li, std::distance<list_iterator>(li, lci));

            value = top::get<mapped_type>(std::move(*li));

            m_map.erase(map_iterator);
            m_list.erase(lci);
        }

        switch (direction) {
            case xhash_list_update_direction_t::to_back:
                m_list.push_back({ key, std::move(value) });
                m_map[top::get<key_type const>(m_list.back())] = std::prev(m_list.end());
                return top::get<mapped_type>(m_list.back());

            case xhash_list_update_direction_t::to_front:
                m_list.push_front({ key, std::move(value) });
                m_map[top::get<key_type const>(m_list.front())] = m_list.begin();
                return top::get<mapped_type>(m_list.front());

            default:
                assert(false);
                throw std::logic_error{ "unknown update_direction" };
        }
    }

    void
    reserve(size_type const count) {
        m_map.reserve(count);
    }

    list_iterator
    begin() noexcept {
        return m_list.begin();
    }

    list_const_iterator
    begin() const noexcept {
        return m_list.begin();
    }

    list_iterator
    end() noexcept {
        return m_list.end();
    }

    list_const_iterator
    end() const noexcept {
        return m_list.end();
    }
};

template <typename KeyT, typename ValueT>
using xhash_list_t = xtop_hash_list<KeyT, ValueT>;


template <typename KeyT, typename T>
class xtop_hash_list2 final
{
private:
    using aux_map_t = std::unordered_map<KeyT, T>;

public:
    using value_type = typename aux_map_t::value_type;
    using mapped_type = typename aux_map_t::mapped_type;

private:
    using list_t = std::list<value_type>;

public:
    using size_type                = typename list_t::size_type;
    using reference                = typename list_t::reference;
    using const_reference          = typename list_t::const_reference;
    using list_iterator            = typename list_t::iterator;
    using list_const_iterator      = typename list_t::const_iterator;

private:
    using map_t = std::unordered_map<KeyT, list_iterator>;

public:
    using key_type                 = typename map_t::key_type;
    using hashtable_iterator       = typename map_t::iterator;
    using hashtable_const_iterator = typename map_t::const_iterator;

private:

    std::size_t m_size_limit;
    list_t m_list{};
    map_t m_map{};

public:

    xtop_hash_list2(xtop_hash_list2 const &)             = delete;
    xtop_hash_list2 & operator=(xtop_hash_list2 const &) = delete;
    xtop_hash_list2(xtop_hash_list2 &&)                  = default;
    xtop_hash_list2 & operator=(xtop_hash_list2 &&)      = default;
    ~xtop_hash_list2()                                   = default;

    explicit
    xtop_hash_list2(std::size_t const size_limit)
        : m_size_limit{ size_limit } {
    }

    void
    swap(xtop_hash_list2 & other) noexcept {
        m_list.swap(other.m_list);
        m_map.swap(other.m_map);
    }

    void
    insert(std::pair<KeyT, T> const & value) {
        auto const map_iterator = m_map.find(top::get<key_type>(value));
        if (map_iterator == std::end(m_map)) {
            m_list.push_front(value);
            m_map[top::get<key_type const>(m_list.front())] = m_list.begin();

            if (m_map.size() >= m_size_limit) {
                pop();
            }
        }
    }

    void
    insert(std::pair<KeyT, T> && value) {
        auto const map_iterator = m_map.find(top::get<key_type>(value));
        if (map_iterator == std::end(m_map)) {
            m_list.push_front(std::move(value));
            m_map[top::get<key_type const>(m_list.front())] = m_list.begin();

            if (m_map.size() >= m_size_limit) {
                pop();
            }
        }
    }

    XATTRIBUTE_NODISCARD
    bool
    empty() const noexcept {
        return m_map.empty();
    }

    size_type
    size() const noexcept {
        return m_map.size();
    }

    void
    clear() noexcept {
        m_list.clear();
        m_map.clear();
    }

    void
    pop() {
        m_map.erase(top::get<key_type const>(m_list.back()));
        m_list.pop_back();
    }

    bool
    contains(key_type const & key) {
        return m_map.find(key) != std::end(m_map);
    }
};

template <typename KeyT, typename ValueT>
using xhash_list2_t = xtop_hash_list2<KeyT, ValueT>;
NS_END1
