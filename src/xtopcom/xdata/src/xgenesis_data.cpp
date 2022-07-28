// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xgenesis_data.h"

#include "xbase/xbase.h"
#include "xbasic/xerror/xerror.h"
#include "xdata/xerror/xerror.h"
#include "xvledger/xvaccount.h"

#include <cinttypes>

NS_BEG2(top, data)

static bool check_address_type(common::xaccount_address_t const & addr, base::enum_vaccount_addr_type const type) {
    assert(base::xvaccount_t::get_addrtype_from_account(addr.value()) == addr.type());
    return type == addr.type();
}

static bool check_address_type_and_zone(common::xaccount_address_t const & addr, base::enum_vaccount_addr_type const type, common::xzone_id_t const & zone_id) {
    assert(common::xzone_id_t{static_cast<uint8_t>(get_vledger_zone_index(base::xvaccount_t::get_xid_from_account(addr.value())))} == addr.ledger_id().zone_id());
    // auto              xid = base::xvaccount_t::get_xid_from_account(addr.value());
    // uint8_t           zone_index = get_vledger_zone_index(xid);
    return check_address_type(addr, type) && address_belongs_to_zone(addr, zone_id);
}

bool is_account_address(common::xaccount_address_t const & addr) {
    return is_t0(addr) || is_t6(addr) || is_t8(addr);
}

bool is_sys_contract_address(common::xaccount_address_t const & addr)  {
    return is_t2(addr);
}

bool is_black_hole_address(common::xaccount_address_t const & addr)  {
    return check_address_type(addr, base::enum_vaccount_addr_type_black_hole);
}

bool is_drand_address(common::xaccount_address_t const & addr)  {
    return check_address_type(addr, base::enum_vaccount_addr_type_drand);
}

//bool is_user_contract_address(common::xaccount_address_t const & addr) {
//    return check_address_type(addr, base::enum_vaccount_addr_type_custom_contract);
//}

bool is_contract_address(common::xaccount_address_t const & addr) {
    return is_sys_contract_address(addr);
}

bool is_beacon_contract_address(common::xaccount_address_t const & addr) {
    return check_address_type_and_zone(addr, base::enum_vaccount_addr_type_native_contract, common::xzone_id_t{static_cast<uint8_t>(base::enum_chain_zone_beacon_index)});
}

bool is_zec_contract_address(common::xaccount_address_t const & addr) {
    return check_address_type_and_zone(addr, base::enum_vaccount_addr_type_native_contract, common::xzone_id_t{
        static_cast<uint8_t>(base::enum_chain_zone_zec_index)});
}

bool is_sys_sharding_contract_address(common::xaccount_address_t const & addr) {
    return check_address_type_and_zone(addr, base::enum_vaccount_addr_type_native_contract, common::xzone_id_t{
        static_cast<uint8_t>(base::enum_chain_zone_consensus_index)});
}

bool is_sys_evm_table_contract_address(common::xaccount_address_t const & addr) {
    return check_address_type_and_zone(addr, base::enum_vaccount_addr_type_native_contract, common::xzone_id_t{
        static_cast<uint8_t>(base::enum_chain_zone_evm_index)});
}

bool is_sys_relay_table_contract_address(common::xaccount_address_t const & addr) {
    return check_address_type_and_zone(addr, base::enum_vaccount_addr_type_native_contract, common::xzone_id_t{
        static_cast<uint8_t>(base::enum_chain_zone_relay_index)});
}

bool is_block_contract_address(common::xaccount_address_t const & addr) {
    return check_address_type(addr, base::enum_vaccount_addr_type_block_contract);
}

common::xaccount_address_t make_address_by_prefix_and_subaddr(const std::string & prefix, uint16_t const subaddr) {
    if (std::string::npos == prefix.find('@')) {
        std::string final_account_address = prefix + "@" + base::xstring_utl::tostring(subaddr);
        return common::xaccount_address_t{std::move(final_account_address)};
    }
    return common::xaccount_address_t{prefix};
}

base::xtable_index_t account_map_to_table_id(common::xaccount_address_t const & addr) {
    const std::string & account = addr.value();
    base::xvaccount_t const vaddr(account);
    return vaddr.get_tableid();
}

std::string account_address_to_block_address(common::xaccount_address_t const & addr) {
    base::xtable_index_t const tableid = account_map_to_table_id(addr);
    return base::xvaccount_t::make_table_account_address(tableid.get_zone_index(), tableid.get_subaddr());
}

bool is_table_address(common::xaccount_address_t const & addr) {
    return check_address_type(addr, base::enum_vaccount_addr_type_block_contract);
}

bool is_unit_address(common::xaccount_address_t const & addr) {
    return !is_table_address(addr);
}

NS_END2
