// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xelection/xstandby_network_storage_result.h"

#include "xbasic/xutility.h"

NS_BEG3(top, data, election)

bool xtop_standby_network_storage_result::activated_state() const {
    return m_mainnet_activated;
}

void xtop_standby_network_storage_result::set_activate_state(bool _activated) noexcept {
    m_mainnet_activated = _activated;
}

xstandby_network_result_t xtop_standby_network_storage_result::network_result(common::xnode_type_t const node_type) const {
    xstandby_network_result_t standby_network_result;
    for (auto const & p : m_results) {
        auto const & node_id = get<common::xnode_id_t const>(p);
        auto const & standby_node_info = get<election::xstandby_node_info_t>(p);
        for (auto & _stake : standby_node_info.stake_container) {
            if (node_type == get<common::xnode_type_t const>(_stake))
                standby_network_result.result_of(node_type).insert(std::make_pair(p.first, standby_node_info));
        }
    }
    return standby_network_result;
}
xstandby_network_result_t xtop_standby_network_storage_result::network_result() const {
    xstandby_network_result_t standby_network_result;
    for (auto const & p : m_results) {
        auto const & node_id = get<common::xnode_id_t const>(p);
        auto const & standby_node_info = get<election::xstandby_node_info_t>(p);
        if (m_mainnet_activated || standby_node_info.is_genesis_node) {
            for (auto const & stake : standby_node_info.stake_container) {
                auto const & node_type = get<common::xnode_type_t const>(stake);
                standby_network_result.result_of(node_type).insert(std::make_pair(node_id, standby_node_info));
            }
        }
    }
    return standby_network_result;
}
xstandby_network_result_t xtop_standby_network_storage_result::all_network_result() const {
    xstandby_network_result_t standby_network_result;
    for (auto const & p : m_results) {
        auto const & node_id = get<common::xnode_id_t const>(p);
        auto const & standby_node_info = get<election::xstandby_node_info_t>(p);
        for (auto & _stake : standby_node_info.stake_container) {
            auto const & node_type = get<common::xnode_type_t const>(_stake);
            standby_network_result.result_of(node_type).insert(std::make_pair(p.first, standby_node_info));
        }
    }
    return standby_network_result;
}

std::pair<xtop_standby_network_storage_result::iterator, bool> xtop_standby_network_storage_result::insert(value_type const & value) {
    if (m_results.find(value.first) == m_results.end() || m_results[value.first] == value.second) {
        return m_results.insert(value);
    }
    for (auto & _stake : value.second.stake_container) {
        m_results.at(value.first).stake_container[top::get<common::xnode_type_t const>(_stake)] = top::get<uint64_t>(_stake);
    }
    return std::make_pair(m_results.find(value.first), true);
}

std::pair<xtop_standby_network_storage_result::iterator, bool> xtop_standby_network_storage_result::insert(value_type && value) {
    if (m_results.find(value.first) == m_results.end() || m_results[value.first] == value.second) {
        return m_results.insert(std::move(value));
    }
    for (auto & _stake : value.second.stake_container) {
        m_results.at(value.first).stake_container[top::get<common::xnode_type_t const>(_stake)] = top::get<uint64_t>(_stake);
    }
    return std::make_pair(m_results.find(value.first), true);
}

std::pair<xtop_standby_network_storage_result::iterator, bool> xtop_standby_network_storage_result::insert2(value_type const & value) {
    if (m_results.find(value.first) == m_results.end() || m_results[value.first] == value.second) {
        return m_results.insert(value);
    }
    m_results.at(value.first) = value.second;
    return std::make_pair(m_results.find(value.first), true);
}

std::pair<xtop_standby_network_storage_result::iterator, bool> xtop_standby_network_storage_result::insert2(value_type && value) {
    if (m_results.find(value.first) == m_results.end() || m_results[value.first] == value.second) {
        return m_results.insert(std::move(value));
    }
    m_results.at(value.first) = value.second;
    return std::make_pair(m_results.find(value.first), true);
}

bool xtop_standby_network_storage_result::empty() const noexcept {
    return m_results.empty();
}

std::map<common::xnode_id_t, xstandby_node_info_t> const & xtop_standby_network_storage_result::results() const noexcept {
    return m_results;
}

void xtop_standby_network_storage_result::results(std::map<common::xnode_id_t, xstandby_node_info_t> && r) noexcept {
    m_results = std::move(r);
}

xstandby_node_info_t const & xtop_standby_network_storage_result::result_of(common::xnode_id_t const & node_id) const {
    try {
        return m_results.at(node_id);
    } catch (std::out_of_range const &) {
        xwarn("xtop_standby_network_storage_result::result_of out_of_range");
        throw;
    } catch (std::exception const & eh) {
        xwarn("xtop_standby_network_storage_result::result_of %s", eh.what());
        throw;
    } catch (...) {
        xerror("xtop_standby_network_storage_result::result_of unknown exception");
        throw;
    }
}

xstandby_node_info_t & xtop_standby_network_storage_result::result_of(common::xnode_id_t const & node_id) {
    return m_results[node_id];
}

std::size_t xtop_standby_network_storage_result::size() const noexcept {
    return m_results.size();
}

xtop_standby_network_storage_result::iterator xtop_standby_network_storage_result::begin() noexcept {
    return m_results.begin();
}

xtop_standby_network_storage_result::const_iterator xtop_standby_network_storage_result::begin() const noexcept {
    return m_results.begin();
}

xtop_standby_network_storage_result::const_iterator xtop_standby_network_storage_result::cbegin() const noexcept {
    return m_results.cbegin();
}

xtop_standby_network_storage_result::iterator xtop_standby_network_storage_result::end() noexcept {
    return m_results.end();
}

xtop_standby_network_storage_result::const_iterator xtop_standby_network_storage_result::end() const noexcept {
    return m_results.end();
}

xtop_standby_network_storage_result::const_iterator xtop_standby_network_storage_result::cend() const noexcept {
    return m_results.end();
}

xtop_standby_network_storage_result::iterator xtop_standby_network_storage_result::erase(const_iterator pos) {
    return m_results.erase(pos);
}

xtop_standby_network_storage_result::size_type xtop_standby_network_storage_result::erase(key_type const & key) {
    return m_results.erase(key);
}

NS_END3
