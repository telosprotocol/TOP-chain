// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xelection/xv1/xelection_result_store.h"

#include "xbasic/xutility.h"

#include <exception>
#include <functional>
#include <iterator>
#include <utility>

NS_BEG4(top, data, election, v1)

std::map<common::xnetwork_id_t, xelection_network_result_t> const &
xtop_election_result_store::results() const noexcept {
    return m_results;
}

void
xtop_election_result_store::results(std::map<common::xnetwork_id_t, xelection_network_result_t> && r) noexcept {
    m_results = std::move(r);
}

xelection_network_result_t const &
xtop_election_result_store::result_of(common::xnetwork_id_t const network_id) const {
    return m_results.at(network_id);
}

xelection_network_result_t &
xtop_election_result_store::result_of(common::xnetwork_id_t const network_id) {
    return m_results[network_id];
}

std::size_t
xtop_election_result_store::size(common::xnetwork_id_t const network_id) const noexcept {
    try {
        return result_of(network_id).size();
    } catch (std::exception const &) {
        return 0;
    }
}

xtop_election_result_store::iterator
xtop_election_result_store::begin() noexcept {
    return m_results.begin();
}

xtop_election_result_store::const_iterator
xtop_election_result_store::begin() const noexcept {
    return m_results.begin();
}

xtop_election_result_store::const_iterator
xtop_election_result_store::cbegin() const noexcept {
    return m_results.cbegin();
}

xtop_election_result_store::iterator
xtop_election_result_store::end() noexcept {
    return m_results.end();
}

xtop_election_result_store::const_iterator
xtop_election_result_store::end() const noexcept {
    return m_results.end();
}

xtop_election_result_store::const_iterator
xtop_election_result_store::cend() const noexcept {
    return m_results.cend();
}

xtop_election_result_store::iterator
xtop_election_result_store::erase(const_iterator pos) {
    return m_results.erase(pos);
}

xtop_election_result_store::size_type
xtop_election_result_store::erase(key_type const & key) {
    return m_results.erase(key);
}

std::pair<xtop_election_result_store::iterator, bool>
xtop_election_result_store::insert(value_type const & value) {
    return m_results.insert(value);
}

std::pair<xtop_election_result_store::iterator, bool>
xtop_election_result_store::insert(value_type && value) {
    return m_results.insert(std::move(value));
}

xtop_election_result_store::iterator xtop_election_result_store::insert(const_iterator hint, value_type const & v) {
    return m_results.insert(hint, v);
}

xtop_election_result_store::iterator xtop_election_result_store::insert(const_iterator hint, value_type && v) {
    return m_results.insert(hint, std::move(v));
}

std::size_t
xtop_election_result_store::size() const noexcept {
    return m_results.size();
}

bool
xtop_election_result_store::empty() const noexcept {
    return m_results.empty();
}

v0::xelection_result_store_t xtop_election_result_store::v0() const {
    v0::xelection_result_store_t r;

    std::transform(std::begin(m_results), std::end(m_results), std::inserter(r, std::end(r)), [](value_type const & input) -> v0::xelection_result_store_t::value_type {
        return {top::get<key_type const>(input), top::get<mapped_type>(input).v0()};
    });

    return r;
}

NS_END4
