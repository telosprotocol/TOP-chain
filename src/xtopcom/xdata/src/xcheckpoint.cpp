// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xcheckpoint.h"

#include "nlohmann/fifo_map.hpp"
#include "nlohmann/json.hpp"
#include "xdata/xcheckpoint_data.h"
#include "xdata/xcheckpoint_state.h"
#include "xdata/xerror/xerror.h"
#include "xvledger/xvaccount.h"

template <class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using json = nlohmann::basic_json<my_workaround_fifo_map>;

#define BLOCK_HEIGHT_KEY "height"
#define BLOCK_HASH_KEY "hash"
#define TABLE_STATE_KEY "table_state"
#define UNIT_STATE_KEY "unit_state"

namespace top {
namespace data {

auto load_data = []() -> xcheckpoints_map_t {
    xcheckpoints_map_t m;
    auto j = json::parse(checkpoint_data());
    for (auto it = j.cbegin(); it != j.cend(); it++) {
        auto clock_str = static_cast<std::string>(it.key());
        auto clock = base::xstring_utl::touint64(clock_str);
        auto const & data_map = it.value();
        for (auto it_in = data_map.cbegin(); it_in != data_map.cend(); it_in++) {
            xcheckpoint_data_t data;
            auto account_str = static_cast<std::string>(it_in.key());
            data.height = base::xstring_utl::touint64(it_in->at(BLOCK_HEIGHT_KEY).get<std::string>());
            data.hash = base::xstring_utl::from_hex(it_in->at(BLOCK_HASH_KEY).get<std::string>());
            m[common::xaccount_address_t{account_str}].emplace(std::make_pair(clock, data));
        }
    }
    j.clear();
    return m;
};

auto load_state = []() -> xcheckpoints_state_map_t {
    xcheckpoints_state_map_t m;
    auto j = json::parse(checkpoint_state());
    for (auto it = j.cbegin(); it != j.cend(); it++) {
        auto const & state_map = it.value();
        for (auto it_in = state_map.cbegin(); it_in != state_map.cend(); it_in++) {
            auto table_str = static_cast<std::string>(it_in.key());
            auto table_state_str = static_cast<std::string>(it_in->at(TABLE_STATE_KEY).get<std::string>());
            m.emplace(std::make_pair(common::xaccount_address_t{table_str}, table_state_str));
            auto const & unit_map = it_in->at(UNIT_STATE_KEY);
            for (auto it_unit = unit_map.cbegin(); it_unit != unit_map.cend(); it_unit++) {
                xcheckpoint_data_t data;
                auto unit_str = static_cast<std::string>(it_in.key());
                auto unit_state_str = static_cast<std::string>(it_in.value().get<std::string>());
                m.emplace(std::make_pair(common::xaccount_address_t{unit_str}, unit_state_str));
            }
        }
    }
    j.clear();
    return m;
};

xcheckpoints_map_t xtop_chain_checkpoint::m_checkpoints_map = load_data();
xcheckpoints_state_map_t xtop_chain_checkpoint::m_checkpoints_state_map = load_state();

xcheckpoint_data_t xtop_chain_checkpoint::get_checkpoint(common::xaccount_address_t const & account, const uint64_t cp_clock, std::error_code & ec) {
    if (account.type() != base::enum_vaccount_addr_type_block_contract) {
        xwarn("[xtop_chain_checkpoint::get_checkpoint] %s not a table account!", account.c_str());
        ec = error::xenum_errc::checkpoint_account_type_invalid;
        return {};
    }
    auto it = m_checkpoints_map.find(account);
    if (it == m_checkpoints_map.end()) {
        xwarn("[xtop_chain_checkpoint::get_checkpoint] %s not found!", account.c_str());
        ec = error::xenum_errc::checkpoint_not_found;
        return {};
    }
    auto it_h = it->second.find(cp_clock);
    if (it_h == it->second.end()) {
        xwarn("[xtop_chain_checkpoint::get_checkpoint] %s clock %lu checkpoint not found!", account.c_str(), clock);
        ec = error::xenum_errc::checkpoint_not_found;
        return {};
    }
    return it_h->second;
}

xcheckpoint_data_t xtop_chain_checkpoint::get_latest_checkpoint(common::xaccount_address_t const & account, std::error_code & ec) {
    if (account.type() != base::enum_vaccount_addr_type_block_contract) {
        xwarn("[xtop_chain_checkpoint::get_checkpoints] %s not a table account!", account.c_str());
        ec = error::xenum_errc::checkpoint_account_type_invalid;
        return {};
    }
    auto it = m_checkpoints_map.find(account);
    if (it == m_checkpoints_map.end()) {
        xwarn("[xtop_chain_checkpoint::get_checkpoints] %s not found!", account.c_str());
        ec = error::xenum_errc::checkpoint_not_found;
        return {};
    }
    return it->second.rbegin()->second;
}

xcheckpoints_t xtop_chain_checkpoint::get_checkpoints(common::xaccount_address_t const & account, std::error_code & ec) {
    if (account.type() != base::enum_vaccount_addr_type_block_contract) {
        xwarn("[xtop_chain_checkpoint::get_checkpoints] %s not a table account!", account.c_str());
        ec = error::xenum_errc::checkpoint_account_type_invalid;
        return {};
    }
    auto it = m_checkpoints_map.find(account);
    if (it == m_checkpoints_map.end()) {
        xwarn("[xtop_chain_checkpoint::get_checkpoints] %s not found!", account.c_str());
        ec = error::xenum_errc::checkpoint_not_found;
        return {};
    }
    return it->second;
}

xobject_ptr_t<base::xvbstate_t> xtop_chain_checkpoint::get_latest_checkpoint_state(common::xaccount_address_t const & account, std::error_code & ec) {
    auto it = m_checkpoints_state_map.find(account);
    if (it == m_checkpoints_state_map.end()) {
        xwarn("[xtop_chain_checkpoint::get_latest_checkpoint_state] %s not found!", account.c_str());
        ec = error::xenum_errc::checkpoint_not_found;
        return nullptr;
    }
    xobject_ptr_t<base::xvbstate_t> bstate =
        make_object_ptr<base::xvbstate_t>(account.value(), (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    bstate->serialize_from_string(it->second);
    return bstate;
}

}  // namespace chain_data
}  // namespace top

