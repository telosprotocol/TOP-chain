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

#include <fstream>

template <class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using json = nlohmann::basic_json<my_workaround_fifo_map>;

#define TABLE_STATE_KEY "table_state"
#define TABLE_DATA_KEY "table_data"
#define UNIT_STATE_KEY "unit_state"
#define UNIT_DATA_KEY "unit_data"
#define BLOCK_HEIGHT_KEY "height"
#define BLOCK_HASH_KEY "hash"

namespace top {
namespace data {

xcheckpoints_map_t xtop_chain_checkpoint::m_checkpoints_map;
xcheckpoints_state_map_t xtop_chain_checkpoint::m_checkpoints_state_map;

void xtop_chain_checkpoint::load() {
    json j_data;
    json j_state;
#ifdef CHECKPOINT_TEST
#    define CHECKPOINT_DATA_FILE "checkpoint_data.json"
#    define CHECKPOINT_STATE_FILE "checkpoint_state.json"
    xinfo("[xtop_chain_checkpoint::load] load from file");
    std::ifstream data_file(CHECKPOINT_DATA_FILE);
    if (data_file.good()) {
        data_file >> j_data;
    } else {
        xwarn("[xtop_chain_checkpoint::load] file %s open error, none cp data used!", CHECKPOINT_DATA_FILE);
    }
    std::ifstream state_file(CHECKPOINT_STATE_FILE);
    if (state_file.good()) {
        state_file >> j_state;
    } else {
        xwarn("[xtop_chain_checkpoint::load] file %s open error, none cp state used!", CHECKPOINT_STATE_FILE);
    }
    data_file.close();
    state_file.close();
#else
    xinfo("[xtop_chain_checkpoint::load] load from code");
    j_data = json::parse(checkpoint_data());
    j_state = json::parse(checkpoint_state());
#endif
    auto load_data = [](json & j, uint64_t & latest_cp) -> xcheckpoints_map_t {
        xcheckpoints_map_t m;
        for (auto it = j.cbegin(); it != j.cend(); it++) {
            auto clock_str = static_cast<std::string>(it.key());
            auto clock = base::xstring_utl::touint64(clock_str);
            latest_cp = clock;
            auto const & data_map = it.value();
            // table
            for (auto it_table = data_map.cbegin(); it_table != data_map.cend(); it_table++) {
                auto table_str = static_cast<std::string>(it_table.key());
                auto const & table_data = it_table->at(TABLE_DATA_KEY);
                {
                    data::xcheckpoint_data_t data;
                    data.height = base::xstring_utl::touint64(table_data.at(BLOCK_HEIGHT_KEY).get<std::string>());
                    data.hash = base::xstring_utl::from_hex(table_data.at(BLOCK_HASH_KEY).get<std::string>());
                    m[common::xaccount_address_t{table_str}].emplace(std::make_pair(clock, data));
                }
                auto const & unit_data_map = it_table->at(UNIT_DATA_KEY);
                // unit
                for (auto it_unit = unit_data_map.cbegin(); it_unit != unit_data_map.cend(); it_unit++) {
                    auto unit_str = static_cast<std::string>(it_unit.key());
                    auto const & unit_data = it_unit.value();
                    data::xcheckpoint_data_t data;
                    data.height = base::xstring_utl::touint64(unit_data.at(BLOCK_HEIGHT_KEY).get<std::string>());
                    data.hash = base::xstring_utl::from_hex(unit_data.at(BLOCK_HASH_KEY).get<std::string>());
                    m[common::xaccount_address_t{unit_str}].emplace(std::make_pair(clock, data));
                }
            }
        }
        return m;
    };
    auto load_state = [](json & j, uint64_t & latest_cp) -> xcheckpoints_state_map_t {
        xcheckpoints_state_map_t m;
        xassert(j.size() <= 1);     // latest only
        for (auto it = j.cbegin(); it != j.cend(); it++) {
            auto clock_str = static_cast<std::string>(it.key());
            auto clock = base::xstring_utl::touint64(clock_str);
            latest_cp = clock;
            auto const & state_map = it.value();
            // table
            for (auto it_table = state_map.cbegin(); it_table != state_map.cend(); it_table++) {
                auto table_str = static_cast<std::string>(it_table.key());
                auto table_state = base::xstring_utl::from_hex(it_table->at(TABLE_STATE_KEY).get<std::string>());
                m[common::xaccount_address_t{table_str}] = table_state;
                auto const & unit_state_map = it_table->at(UNIT_STATE_KEY);
                // unit
                for (auto it_unit = unit_state_map.cbegin(); it_unit != unit_state_map.cend(); it_unit++) {
                    auto unit_str = static_cast<std::string>(it_unit.key());
                    auto unit_state = base::xstring_utl::from_hex(it_unit.value().get<std::string>());
                    m[common::xaccount_address_t{unit_str}] = unit_state;
                }
            }
        }
        return m;
    };
    uint64_t latest_data_cp{0};
    uint64_t latest_state_cp{0};
    m_checkpoints_map = load_data(j_data, latest_data_cp);
    j_data.clear();
    m_checkpoints_state_map = load_state(j_state, latest_state_cp);
    j_state.clear();
    xassert(latest_data_cp == latest_state_cp);
    xinfo("[xtop_chain_checkpoint::load] cp size: %zu, %zu", m_checkpoints_map.size(), m_checkpoints_state_map.size());
}

xcheckpoint_data_t xtop_chain_checkpoint::get_latest_checkpoint(common::xaccount_address_t const & account, std::error_code & ec) {
    auto it = m_checkpoints_map.find(account);
    if (it == m_checkpoints_map.end()) {
        xwarn("[xtop_chain_checkpoint::get_checkpoints] %s not found!", account.c_str());
        ec = error::xenum_errc::checkpoint_not_found;
        return {};
    }
    return it->second.rbegin()->second;
}

xcheckpoints_t xtop_chain_checkpoint::get_checkpoints(common::xaccount_address_t const & account, std::error_code & ec) {
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

