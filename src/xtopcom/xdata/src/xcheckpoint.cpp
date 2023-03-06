// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xcheckpoint.h"

#include "nlohmann/fifo_map.hpp"
#include "nlohmann/json.hpp"
#include "xdata/xcheckpoint_data.h"
#include "xdata/xerror/xerror.h"
#include "xvledger/xvaccount.h"

#include <fstream>

template <class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using json = nlohmann::basic_json<my_workaround_fifo_map>;

#define TABLE_DATA_KEY "table_data"
#define UNIT_DATA_KEY "unit_data"
#define BLOCK_HEIGHT_KEY "height"
#define BLOCK_HASH_KEY "hash"

namespace top {
namespace data {

xcheckpoints_map_t xtop_chain_checkpoint::m_checkpoints_map;

void xtop_chain_checkpoint::load() {
    json j_data;
#ifdef CHECKPOINT_TEST
#    define CHECKPOINT_DATA_FILE "checkpoint_data.json"
    xinfo("[xtop_chain_checkpoint::load] load from file");
    std::ifstream data_file(CHECKPOINT_DATA_FILE);
    if (data_file.good()) {
        data_file >> j_data;
        data_file.close();
    } else {
        xwarn("[xtop_chain_checkpoint::load] file %s open error, none cp data used!", CHECKPOINT_DATA_FILE);
    }
#else
    xinfo("[xtop_chain_checkpoint::load] load from code");
    j_data = json::parse(checkpoint_data());
#endif
    auto load_data = [](json & j) -> xcheckpoints_map_t {
        xcheckpoints_map_t m;
        for (auto it = j.cbegin(); it != j.cend(); it++) {
            auto clock_str = static_cast<std::string>(it.key());
            auto clock = base::xstring_utl::touint64(clock_str);
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
            xinfo("[xtop_chain_checkpoint::load] load cp height: %lu", clock);
        }
        return m;
    };
    m_checkpoints_map = load_data(j_data);
    j_data.clear();
    xinfo("[xtop_chain_checkpoint::load] cp data size: %zu", m_checkpoints_map.size());
}

xcheckpoint_data_t xtop_chain_checkpoint::get_latest_checkpoint(common::xaccount_address_t const & account, std::error_code & ec) {
    auto it = m_checkpoints_map.find(account);
    if (it == m_checkpoints_map.end()) {
        xdbg("[xtop_chain_checkpoint::get_latest_checkpoint] %s not found!", account.to_string().c_str());
        ec = error::xenum_errc::checkpoint_not_found;
        return {};
    }
    return it->second.rbegin()->second;
}

xcheckpoints_t xtop_chain_checkpoint::get_checkpoints(common::xaccount_address_t const & account, std::error_code & ec) {
    auto it = m_checkpoints_map.find(account);
    if (it == m_checkpoints_map.end()) {
        xwarn("[xtop_chain_checkpoint::get_checkpoints] %s not found!", account.to_string().c_str());
        ec = error::xenum_errc::checkpoint_not_found;
        return {};
    }
    return it->second;
}

}  // namespace chain_data
}  // namespace top

