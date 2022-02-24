// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>

#include "xcommon/xlogic_time.h"
#include "xstore/xaccount_context.h"

NS_BEG2(top, xvm)
#define XCONTRACT_ENSURE(condition, msg)                                                     \
    do {                                                                                     \
        if (!(condition)) {                                                                  \
            std::error_code ec{ enum_xvm_error_code::enum_vm_exception };                    \
            top::error::throw_error(ec, msg);                                                \
        }                                                                                    \
    } while (false)

class xcontract_helper {
public:
    xcontract_helper(store::xaccount_context_t* account_context, common::xnode_id_t const & contract_account, const std::string& exec_account);
    void set_transaction(const data::xtransaction_ptr_t& ptr);
    data::xtransaction_ptr_t get_transaction() const;
    std::string get_source_account() const;
    std::string get_parent_account() const;
    common::xnode_id_t const & get_self_account() const noexcept;
    uint64_t get_balance() const;
    common::xlogic_time_t get_timer_height() const;
    const data::xaction_asset_out& get_pay_fee() const;
    void set_contract_code(const std::string& code);
    void get_contract_code(std::string &code) const;
    void create_transfer_tx(const std::string& grant_account, const uint64_t amount);

    void top_token_increase(const uint64_t amount);
    void top_token_decrease(const uint64_t amount);

    void string_create(const std::string& key);
    void string_set(const std::string& key, const std::string& value, bool native = false);
    std::string string_get(const std::string& key, const std::string& addr="");
    std::string string_get2(const std::string& key, const std::string& addr="");
    bool string_exist(const std::string& key, const std::string& addr="");

    void list_create(const std::string& key);
    void list_push_back(const std::string& key, const std::string& value, bool native = false);
    void list_push_front(const std::string& key, const std::string& value, bool native = false);
    void list_pop_back(const std::string& key, std::string& value, bool native = false);
    void list_pop_front(const std::string& key, std::string& value, bool native = false);
    int32_t list_size(const std::string& key, const std::string& addr="");
    std::vector<std::string> list_get_all(const std::string& key, const std::string& addr = "");
    bool list_exist(const std::string& key);
    void list_clear(const std::string& key, bool native = false);
    std::string list_get(const std::string& key, int32_t index, const std::string& addr="");

    void map_create(const std::string& key);
    std::string map_get(const std::string& key, const std::string& field, const std::string& addr="");
    int32_t map_get2(const std::string& key, const std::string& field, std::string& value, const std::string& addr="");
    void map_set(const std::string& key, const std::string& field, const std::string & value, bool native = false);
    void map_remove(const std::string& key, const std::string& field, bool native = false);
    int32_t map_size(const std::string& key, const std::string& addr="");
    void map_copy_get(const std::string & key, std::map<std::string, std::string> & map, const std::string& addr = "");
    bool map_field_exist(const std::string& key, const std::string& field) const;
    bool map_key_exist(const std::string& key);
    void map_clear(const std::string& key, bool native = false);
    void get_map_property(const std::string& key, std::map<std::string, std::string>& value, uint64_t height, const std::string& addr="");
    bool map_property_exist(const std::string& key);
    void get_string_property(const std::string& key, std::string& value, uint64_t height, const std::string& addr="");

    void generate_tx(common::xaccount_address_t const & target_addr, const std::string& func_name, const std::string& func_param);
    std::string get_random_seed() const;

    std::string cache_get(const std::string& key) const;
    void cache_set(const std::string& key, const std::string& value);

    std::uint64_t
    contract_height() const;

    base::xauto_ptr<xblock_t>
    get_block_by_height(const std::string & owner, uint64_t height) const;

    base::xauto_ptr<xblock_t>
    get_next_fullblock(std::string const & owner, uint64_t const cur_full_height) const;

    std::uint64_t
    get_blockchain_height(const std::string & owner) const;

    int32_t
    get_gas_and_disk_usage(std::uint32_t &gas, std::uint32_t &disk) const;

private:
    store::xaccount_context_t*      m_account_context;
    common::xnode_id_t const &      m_contract_account;
    const std::string&              m_exec_account;
    data::xtransaction_ptr_t              m_transaction{};
};

NS_END2
