#pragma once

#include <thread>
#include <chrono>
#include <sstream>
#include <iostream>
#include <random>

#include <atomic>
#include <map>
#include <set>


#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
// TODO(jimmy) #include "xbase/xvledger.h"

#include "xdata/xdatautil.h"
#include "xdata/xemptyblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xdata/xtransaction_maker.hpp"

#include "test_blockmock.hpp"
#include "xstore/xstore.h"
#include "xblockstore/xblockstore_face.h"
#include "tests/mock/xtableblock_util.hpp"

using namespace std;
using namespace top;
using namespace top::data;
using namespace top::db;
using namespace top::base;
using namespace top::mock;
using namespace top::store;

struct table_info_t {
    using table_unit_set_t = std::map<std::string, std::set<std::string> >;
    using atomic_bool_ptr_t = std::shared_ptr<std::atomic<bool> >;

    size_t m_per_table_unit_count;
    const size_t table_count{256};

    std::set<std::string> empty_addr_set;

    std::map<uint16_t, std::string> table_id_map;
    std::map<std::string, std::set<std::string> > table_unit_map;

    std::map<std::string, atomic_bool_ptr_t> table_status;

    // record unit's previous block for building the next block
    std::map<std::string, xvblock_t*> prev_blocks;

    // record table's previous block for building the next block
    std::map<std::string, xvblock_t*> prev_tableblocks;

    // record table packed units in previous two height, since these unit addresses can't be packed in this height
    table_unit_set_t pending_table_units;

    std::mt19937 gen;

    table_info_t(size_t per_table_unit_count = 1000)
     : m_per_table_unit_count(per_table_unit_count),
       gen(std::chrono::steady_clock::now().time_since_epoch().count()) {

        uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
        uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);

        std::set<uint16_t> subaddr_set;

        for (uint16_t i = 0; i < table_count; ++i) {
            std::string table_addr = xblocktool_t::make_address_shard_table_account(i);
            add_table(table_addr);
            table_status.insert(std::make_pair(table_addr, atomic_bool_ptr_t(new std::atomic<bool>(true))));
            subaddr_set.insert(i);
        }

        while (!subaddr_set.empty()) {
            std::string unit_addr = utl::xcrypto_util::make_address_by_random_key(addr_type, ledger_id);
            if (table_size(unit_addr) < per_table_unit_count) {
                add_unit(unit_addr);
            } else {
                // this table has enough unit address
                uint16_t subaddr = get_vledger_subaddr(base::xvaccount_t::get_xid_from_account(unit_addr));
                subaddr_set.erase(subaddr);
            }
        }
    }
    ~table_info_t() {
        for (const auto& entry : prev_blocks) {
            if (entry.second != nullptr) {
                entry.second->release_ref();
            }
        }

        for (const auto& entry : prev_tableblocks) {
            if (entry.second != nullptr) {
                entry.second->release_ref();
            }
        }
    }

    size_t per_table_unit_count() const {
        return m_per_table_unit_count;
    }

    std::set<std::string> get_table_addresses() const {
        if (table_unit_map.empty()) {
            return empty_addr_set;
        }
        std::set<std::string> keys;
        for (const auto& entry : table_unit_map) {
            keys.insert(entry.first);
        }
        return std::move(keys);
    }

    std::set<std::string>& get_unit_addresses(const std::string& table_addr) {
        auto it = table_unit_map.find(table_addr);
        if (it == table_unit_map.end()) {
            return empty_addr_set;
        }
        return it->second;
    }

    std::string get_random_available_table() {
        std::uniform_int_distribution<size_t> table_dist(0, table_count - 1);
        while (true) {
            auto it = table_unit_map.begin();
            // 'advance' the iterator n times
            std::advance(it, table_dist(gen));

            bool available = true;
            if (table_status[it->first]->compare_exchange_strong(available, false)) {
                return it->first;
            }
        }
    }

    void set_table_available(const std::string& table_address) {
        table_status[table_address]->store(true, std::memory_order_release);
    }

    std::string get_random_unit_address(const std::string& table_addr) {
        std::uniform_int_distribution<size_t> unit_dist(0, m_per_table_unit_count - 1);
        const auto& unit_addresses = get_unit_addresses(table_addr);

        auto it = std::begin(unit_addresses);
        // 'advance' the iterator n times
        std::advance(it, unit_dist(gen));
        return *it;
    }

    bool add_table(const std::string& table_addr) {
        uint16_t ledger_subaddr = base::xvaccount_t::get_ledgersubaddr_from_account(table_addr);
        if (table_id_map.find(ledger_subaddr) == table_id_map.end()) {
            table_id_map.insert(std::make_pair(ledger_subaddr, table_addr));
        }
        if (table_unit_map.find(table_addr) == table_unit_map.end()) {
            table_unit_map.insert(std::make_pair(table_addr, std::set<std::string>()));
        }
        return true;
    }

    bool add_unit(const std::string& unit_addr) {
        uint16_t subaddr = get_vledger_subaddr(base::xvaccount_t::get_xid_from_account(unit_addr));
        auto it = table_id_map.find(subaddr);

        assert(it != table_id_map.end());

        auto& unit_addresses = table_unit_map[it->second];
        auto result = unit_addresses.insert(unit_addr);
        return result.second;
    }

    size_t table_size(const std::string& unit_addr) {
        uint16_t subaddr = get_vledger_subaddr(base::xvaccount_t::get_xid_from_account(unit_addr));
        auto it = table_id_map.find(subaddr);

        assert(it != table_id_map.end());
        const auto& unit_addresses = table_unit_map[it->second];
        return unit_addresses.size();
    }

    std::string get_table_address(const std::string& unit_addr) {
        uint16_t subaddr = get_vledger_subaddr(base::xvaccount_t::get_xid_from_account(unit_addr));
        auto it = table_id_map.find(subaddr);

        if (it == table_id_map.end()) {
            return {};
        }

        return it->second;
    }

    bool unit_in_pending_table(const std::string& unit_addr) {
        std::string table_addr = get_table_address(unit_addr);
        if (table_addr.empty()) {
            return false;
        }

        auto it = pending_table_units.find(table_addr);
        // check the units in the pending set
        if (it != pending_table_units.end()) {
            const auto& units = it->second;
            if (units.find(unit_addr) != units.end()) {
                return true;
            } else {
                return false;
            }
        }
        return false;
    }

    void insert_pending_table(const std::string& unit_address) {
        std::string table_addr = get_table_address(unit_address);
        if (table_addr.empty()) {
            return;
        }
        // add unit addresses to the pending table's unit list
        auto it = pending_table_units.find(table_addr);
        if (it != pending_table_units.end()) {
            it->second.insert(unit_address);
        } else {
            std::set<std::string> unit_addresses;
            unit_addresses.insert(unit_address);
            pending_table_units.insert(std::make_pair(table_addr, unit_addresses));
        }
    }

    // unit_addresses are all belonging to the same table
    void insert_pending_table(const std::set<std::string>& unit_addresses) {
        if (unit_addresses.empty()) {
            return;
        }

        auto addr_it = unit_addresses.begin();
        std::string table_addr = get_table_address(*addr_it);
        if (table_addr.empty()) {
            return;
        }
        // add unit addresses to the pending table's unit list
        auto it = pending_table_units.find(table_addr);
        if (it != pending_table_units.end()) {
            it->second.insert(unit_addresses.begin(), unit_addresses.end());
        } else {
            pending_table_units.insert(std::make_pair(table_addr, unit_addresses));
        }
    }

    void erase_pending_table(const std::string& unit_address) {
        std::string table_addr = get_table_address(unit_address);
        if (table_addr.empty()) {
            return;
        }
        // add unit addresses to the pending table's unit list
        auto it = pending_table_units.find(table_addr);
        if (it != pending_table_units.end()) {
            it->second.erase(unit_address);
        }
    }

    xvblock_t* get_prev_unitblock(const std::string& address) const {
        auto it = prev_blocks.find(address);
        if (it != prev_blocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    xvblock_t* get_prev_tableblock(const std::string& address) {
        auto it = prev_tableblocks.find(address);
        if (it != prev_tableblocks.end()) {
            return it->second;
        }
        return nullptr;
    }

    void update_prev_unitblock(const std::string& address, xvblock_t* block) {
        auto it = prev_blocks.find(address);
        if (it != prev_blocks.end()) {
            if (it->second != nullptr) {
                it->second->release_ref();
                it->second = block;
                it->second->add_ref();
            }
        } else {
            prev_blocks.insert(std::make_pair(address, block));
        }
    }

    void update_prev_tableblock(const std::string& address, xvblock_t* block) {
        auto it = prev_tableblocks.find(address);
        if (it != prev_tableblocks.end()) {
            if (it->second != nullptr) {
                it->second->release_ref();
                it->second = block;
                it->second->add_ref();
            }
        } else {
            prev_tableblocks.insert(std::make_pair(address, block));
        }
    }
};
