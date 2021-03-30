// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xutility.h"
#include "xdata/xelection/xstandby_network_result.h"

NS_BEG3(top, data, election)

std::pair<xtop_standby_network_result::iterator, bool>
xtop_standby_network_result::insert(value_type const & value) {
    return m_results.insert(value);
}

std::pair<xtop_standby_network_result::iterator, bool>
xtop_standby_network_result::insert(value_type && value) {
    return m_results.insert(std::move(value));
}

bool
xtop_standby_network_result::empty() const noexcept {
    return m_results.empty();
}

bool
xtop_standby_network_result::empty(common::xnode_type_t const role) const noexcept {
    auto const it = m_results.find(role);
    if (it == std::end(m_results)) {
        return true;
    }

    auto const & result = top::get<xstandby_result_t>(*it);
    return result.empty();
}

std::map<common::xnode_type_t, xtop_standby_result> const &
xtop_standby_network_result::results() const noexcept {
    return m_results;
}

void
xtop_standby_network_result::results(std::map<common::xnode_type_t, xtop_standby_result> && r) noexcept {
    m_results = std::move(r);
}

xstandby_result_t const &
xtop_standby_network_result::result_of(common::xnode_type_t const role) const {
    try {
        return m_results.at(role);
    } catch (std::out_of_range const &) {
        xwarn("xtop_standby_network_result::result_of out_of_range");
        throw;
    } catch (std::exception const & eh) {
        xwarn("xtop_standby_network_result::result_of %s", eh.what());
        throw;
    } catch (...) {
        xerror("xtop_standby_network_result::result_of unknown exception");
        throw;
    }
}

xstandby_result_t &
xtop_standby_network_result::result_of(common::xnode_type_t const role) {
    return m_results[role];
}

std::size_t
xtop_standby_network_result::size(common::xnode_type_t const role) const noexcept {
    if (empty(role)) {
        return 0;
    }

    return result_of(role).size();
}

std::size_t
xtop_standby_network_result::size() const noexcept {
    return m_results.size();
}

xtop_standby_network_result::iterator
xtop_standby_network_result::begin() noexcept {
    return m_results.begin();
}

xtop_standby_network_result::const_iterator
xtop_standby_network_result::begin() const noexcept {
    return m_results.begin();
}

xtop_standby_network_result::const_iterator
xtop_standby_network_result::cbegin() const noexcept {
    return m_results.cbegin();
}

xtop_standby_network_result::iterator
xtop_standby_network_result::end() noexcept {
    return m_results.end();
}

xtop_standby_network_result::const_iterator
xtop_standby_network_result::end() const noexcept {
    return m_results.end();
}

xtop_standby_network_result::const_iterator
xtop_standby_network_result::cend() const noexcept {
    return m_results.end();
}

xtop_standby_network_result::iterator
xtop_standby_network_result::erase(const_iterator pos) {
    return m_results.erase(pos);
}

xtop_standby_network_result::size_type
xtop_standby_network_result::erase(key_type const & key) {
    return m_results.erase(key);
}

NS_END3
