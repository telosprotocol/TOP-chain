// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <map>

NS_BEG1(top)

template <typename KeyT, typename ValueT>
class xtop_simple_map_result final {
private:
    using container_t = std::map<KeyT, ValueT>;
    container_t m_result;

public:
    typedef typename container_t::key_type key_type;
    typedef typename container_t::mapped_type mapped_type;
    typedef typename container_t::value_type value_type;
    typedef typename container_t::size_type size_type;
    typedef typename container_t::difference_type difference_type;
    typedef typename container_t::key_compare key_compare;
    typedef typename container_t::reference reference;
    typedef typename container_t::const_reference const_reference;
    typedef typename container_t::pointer pointer;
    typedef typename container_t::const_pointer const_pointer;
    typedef typename container_t::iterator iterator;
    typedef typename container_t::const_iterator const_iterator;

    std::pair<iterator, bool> insert(value_type const & value) {
        return m_result.insert(value);
    }

    std::pair<iterator, bool> insert(value_type && value) {
        return m_result.insert(std::move(value));
    }

    bool empty(key_type const & key) const noexcept {
        return m_result.find(key) == m_result.end();
    }

    bool empty() const noexcept {
        return m_result.empty();
    }

    std::map<KeyT, ValueT> const & results() const noexcept {
        return m_result;
    }

    void results(std::map<KeyT, ValueT> && r) noexcept {
        m_result = std::move(r);
    }

    ValueT const & result_of(KeyT const K) const {
        return m_result.at(K);
    }

    ValueT & result_of(KeyT const K) {
        return m_result[K];
    }

    std::size_t size() const noexcept {
        return m_result.size();
    }

    iterator begin() noexcept {
        return m_result.begin();
    }

    const_iterator begin() const noexcept {
        return m_result.begin();
    }

    const_iterator cbegin() const noexcept {
        return m_result.cbegin();
    }

    iterator end() noexcept {
        return m_result.end();
    }

    const_iterator end() const noexcept {
        return m_result.end();
    }

    const_iterator cend() const noexcept {
        return m_result.cend();
    }

    iterator erase(const_iterator pos) {
        return m_result.erase(pos);
    }

    size_type erase(KeyT const & key) {
        return m_result.erase(key);
    }
};

template <typename KeyT, typename ValueT>
using xsimple_map_result_t = xtop_simple_map_result<KeyT, ValueT>;

NS_END1