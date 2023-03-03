// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_base.h"
#include "xrouter/xrouter.h"
#include "xvm/manager/xcontract_address_map.h"
#include "xvm/manager/xcontract_manager.h"
#include "xchain_fork/xfork_points.h"
#include "xchain_fork/xutility.h"
#include "xcommon/xtop_event.h"

using namespace top::data;

NS_BEG3(top, xvm, xcontract)
using std::string;
using std::vector;

xcontract_base::xcontract_base(common::xnetwork_id_t const & network_id)
    : m_network_id{ network_id } {
}

void xcontract_base::set_contract_helper(shared_ptr<xcontract_helper> contract_helper) {
    m_contract_helper = contract_helper;
}

string xcontract_base::GET_EXEC_ACCOUNT() const {
    return m_contract_helper->get_source_account();
}

uint64_t xcontract_base::GET_BALANCE() const {
    return m_contract_helper->get_balance();
}

xtransaction_ptr_t xcontract_base::GET_TRANSACTION() const {
    return m_contract_helper->get_transaction();
}

common::xlogic_time_t xcontract_base::TIME() const {
    return m_contract_helper->get_timer_height();
}

void xcontract_base::CREATE(enum_type_t type, const std::string& prop_key) {
    switch(type) {
        case enum_type_t::map:
            m_contract_helper->map_create(prop_key);
            break;
        case enum_type_t::list:
            m_contract_helper->list_create(prop_key);
            break;
        default:
            m_contract_helper->string_create(prop_key);
            break;
    }
}

std::string xcontract_base::QUERY(enum_type_t type, const std::string& prop_key, const std::string& key, const std::string& addr) const {
    switch(type) {
        case enum_type_t::map:
            return m_contract_helper->map_get(prop_key, key, addr);
        case enum_type_t::list:
            return m_contract_helper->list_get(prop_key, std::atoi(key.c_str()), addr);
        default:
            return m_contract_helper->string_get(prop_key, addr);
    }
}

std::string xcontract_base::READ(enum_type_t type, const std::string& prop_key, const std::string& key) {
    return QUERY(type, prop_key, key, "");
}

void xcontract_base::WRITE(enum_type_t type, const std::string& prop_key, const std::string& value, const std::string& key) {
    switch(type) {
        case enum_type_t::map:
            m_contract_helper->map_set(prop_key, key, value);
            break;
        case enum_type_t::list:
            m_contract_helper->list_push_back(prop_key, value);
            break;
        default:
            m_contract_helper->string_set(prop_key, value);
            break;
    }
}

void xcontract_base::REMOVE(enum_type_t type, const std::string& prop_key, const std::string& key) {
    switch(type) {
        case enum_type_t::map:
            m_contract_helper->map_remove(prop_key, key);
            break;
        default:
            assert(0);
            break;
    }
}

void xcontract_base::CALL(common::xaccount_address_t const & target_addr, const std::string& action, const std::string& params) {
    m_contract_helper->generate_tx(target_addr, action, params);
}

void xcontract_base::ASYNC_CALL(const std::string& target_addr, const std::string& action, const std::string& params) {
    assert(0);
}

void xcontract_base::TRANSFER(const std::string& target_addr, uint64_t amount) {
    // if amount = 0, then not create transfer tx, because consesenus not allowed
    if (0 == amount) return;
    m_contract_helper->create_transfer_tx(target_addr, amount);
    if (true == chain_fork::xutility_t::is_forked(fork_points::v11100_event, TIME())) {
        EVENT_TRANSFER(SELF_ADDRESS().to_string(),target_addr,amount);
    }
}

bool xcontract_base::EXISTS(const std::string& addr) {
    xassert(0);
    return false;
}

int32_t xcontract_base::SIZE(enum_type_t type, const std::string& prop_key, const std::string& addr) {
    switch(type) {
        case enum_type_t::map:
            return m_contract_helper->map_size(prop_key, addr);
        case enum_type_t::list:
            return m_contract_helper->list_size(prop_key, addr);
        default:
            return m_contract_helper->string_exist(prop_key, addr) ? 1 : 0;
    }
}

void xcontract_base::CLEAR(enum_type_t type, const std::string& prop_key) {
    switch(type) {
        case enum_type_t::map:
            m_contract_helper->map_clear(prop_key);
            break;
        case enum_type_t::list:
            m_contract_helper->list_clear(prop_key);
            break;
        default:
            if(m_contract_helper->string_exist(prop_key)) {
                m_contract_helper->string_set(prop_key, "");
            }
            break;
    }
}

std::string xcontract_base::CALC_CONTRACT_ADDRESS(const std::string& contract_name, const uint32_t& table_id) {
    return contract::xcontract_address_map_t::calc_cluster_address(common::xaccount_address_t{contract_name}, table_id).to_string();
}

bool xcontract_base::EXTRACT_TABLE_ID(common::xaccount_address_t const & addr, uint32_t& table_id) const {
    if ( is_beacon_contract_address(addr) ||
       is_zec_contract_address(addr)) {
        table_id = 0;
        return true;
    }

    if (is_sys_sharding_contract_address(addr)) {
        return xdatautil::extract_table_id_from_address(addr.to_string(), table_id);
    }

    table_id = data::account_map_to_table_id(addr).get_subaddr();
    return true;
}

std::string xcontract_base::SOURCE_ADDRESS() {
    return m_contract_helper->get_source_account();
}

common::xaccount_address_t const & xcontract_base::SELF_ADDRESS() const noexcept {
    return m_contract_helper->get_self_account();
}

void xcontract_base::TOP_TOKEN_INCREASE(const uint64_t amount) {
    m_contract_helper->top_token_increase(amount);
}

void xcontract_base::TOP_TOKEN_DECREASE(const uint64_t amount) {
    m_contract_helper->top_token_decrease(amount);
}

void xcontract_base::STRING_CREATE(const string& key) {
    m_contract_helper->string_create(key);
}

void xcontract_base::STRING_SET(const string& key, const string& value) {
    m_contract_helper->string_set(key, value);
}

string xcontract_base::STRING_GET(const string& key) const {
    return m_contract_helper->string_get(key);
}

string xcontract_base::STRING_GET2(const string& key, const std::string& addr) const {
    return m_contract_helper->string_get2(key, addr);
}

bool xcontract_base::STRING_EXIST(const string& key) {
    return m_contract_helper->string_exist(key);
}

void xcontract_base::LIST_CREATE(const string& key) {
    m_contract_helper->list_create(key);
}

void xcontract_base::LIST_PUSH_BACK(const string& key, const string& value) {
    m_contract_helper->list_push_back(key, value);
}

void xcontract_base::LIST_PUSH_FRONT(const string& key, const string& value) {
    m_contract_helper->list_push_front(key, value);
}

void xcontract_base::LIST_POP_BACK(const std::string& key, std::string& value) {
    m_contract_helper->list_pop_back(key, value);
}

void xcontract_base::LIST_POP_FRONT(const std::string& key, std::string& value) {
    m_contract_helper->list_pop_front(key, value);
}

int32_t xcontract_base::LIST_SIZE(const string& key) {
    return m_contract_helper->list_size(key);
}

void xcontract_base::LIST_CLEAR(const std::string& key) {
    m_contract_helper->list_clear(key);
}

vector<string> xcontract_base::LIST_GET_ALL(const string& key, const string& addr) {
    return m_contract_helper->list_get_all(key, addr);
}

bool xcontract_base::LIST_EXIST(const string& key) {
    return m_contract_helper->list_exist(key);
}

void xcontract_base::MAP_CREATE(const string& key) {
    m_contract_helper->map_create(key);
}

string xcontract_base::MAP_GET(const string& key, const string& field) const {
    return m_contract_helper->map_get(key, field);
}

int32_t xcontract_base::MAP_GET2(const string& key, const string& field, string& value, const std::string& addr) const {
    return m_contract_helper->map_get2(key, field, value, addr);
}

void xcontract_base::MAP_SET(const string& key, const string& field, const string& value) {
    m_contract_helper->map_set(key, field, value);
}

void xcontract_base::MAP_REMOVE(const string& key, const string& field) {
    m_contract_helper->map_remove(key, field);
}

void xcontract_base::MAP_CLEAR(const std::string& key) {
    m_contract_helper->map_clear(key);
}

void xcontract_base::MAP_COPY_GET(const std::string& key, std::map<std::string, std::string> & map, const std::string& addr) const {
    return m_contract_helper->map_copy_get(key, map, addr);
}

int32_t xcontract_base::MAP_SIZE(const string& key) {
    return m_contract_helper->map_size(key);
}

bool xcontract_base::MAP_FIELD_EXIST(const string& key, const string& field) const {
    return m_contract_helper->map_field_exist(key, field);
}

void xcontract_base::GET_MAP_PROPERTY(const std::string& key, std::map<std::string, std::string>& value, uint64_t height, const std::string& addr) const {
    m_contract_helper->get_map_property(key, value, height, addr);
}

bool xcontract_base::MAP_PROPERTY_EXIST(const std::string& key) const {
    return m_contract_helper->map_property_exist(key);
}

void xcontract_base::GET_STRING_PROPERTY(const std::string& key, std::string& value, uint64_t height, const std::string& addr) const {
    m_contract_helper->get_string_property(key, value, height, addr);
}

void xcontract_base::GENERATE_TX(common::xaccount_address_t const & target_addr, const string& func_name, const string& func_param) {
    m_contract_helper->generate_tx(target_addr, func_name, func_param);
}

void xcontract_base::EVENT(const std::string & func_sign, const std::string & data) {
    common::arguments_t args;
    args.emplace_back(common::argument_t(data, common::enum_event_data_type::string, false));
    common::xtop_event_t event(func_sign, m_contract_helper->get_self_account(), args);
    m_contract_helper->event(event);
}

void xcontract_base::EVENT_TRANSFER(const std::string & indexed_from, const std::string & indexed_to, const uint64_t & data) {
    common::arguments_t args;
    std::string func_sign = R"(Transfer(address,address,uint256))";
    args.emplace_back(common::argument_t(data, common::enum_event_data_type::uint64, false));
    args.emplace_back(common::argument_t(indexed_from, common::enum_event_data_type::address, true));
    args.emplace_back(common::argument_t(indexed_to, common::enum_event_data_type::address, true));
    common::xtop_event_t e(func_sign, m_contract_helper->get_self_account(), args);
    m_contract_helper->event(e);
}

base::xauto_ptr<xblock_t>
xcontract_base::get_block_by_height(const std::string & owner, uint64_t height) const {
    assert(m_contract_helper);
    return m_contract_helper->get_block_by_height(owner, height);
}

base::xauto_ptr<xblock_t>
xcontract_base::get_next_fullblock(std::string const & owner, uint64_t const cur_full_height) const {
    assert(m_contract_helper);
    return m_contract_helper->get_next_fullblock(owner, cur_full_height);
}

std::uint64_t
xcontract_base::get_blockchain_height(const std::string & owner) const {
    assert(m_contract_helper);
    return m_contract_helper->get_blockchain_height(owner);
}

common::xnetwork_id_t const &
xcontract_base::network_id() const noexcept {
    return m_network_id;
}

int32_t
xcontract_base::get_account_from_xip(const xvip2_t & target_node, std::string &target_addr) {
    return top::contract::xcontract_manager_t::get_account_from_xip(target_node, target_addr);
}

NS_END3
