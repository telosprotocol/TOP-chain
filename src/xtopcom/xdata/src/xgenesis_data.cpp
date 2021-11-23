// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <map>
#include <string>
#include <vector>

#include "xbase/xcontext.h"
#include "xbase/xint.h"
#include "xbase/xutl.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xbasic/xcrypto_key.h"
#include "xbase/xobject_ptr.h"
#include "xbasic/xutility.h"
#include "xcommon/xsharding_info.h"
#include "xdata/xblock.h"
#include "xdata/xchain_param.h"
#include "xdata/xdatautil.h"
#include "xdata/xelect_transaction.hpp"
#include "xdata/xgenesis_data.h"
#include "xdata/xblocktool.h"
#include "xcrypto/xckey.h"

namespace top { namespace data {

bool check_address_type(common::xaccount_address_t const & addr, base::enum_vaccount_addr_type type) {
    return type == base::xvaccount_t::get_addrtype_from_account(addr.value());
}
bool check_address_type_and_zone(common::xaccount_address_t const & addr, base::enum_vaccount_addr_type type, base::enum_xchain_zone_index zone) {
    auto              xid = base::xvaccount_t::get_xid_from_account(addr.value());
    uint8_t           zone_index = get_vledger_zone_index(xid);
    return type == base::xvaccount_t::get_addrtype_from_account(addr.value()) && zone == zone_index;
}

bool is_account_address(common::xaccount_address_t const & addr) {
    return check_address_type(addr, base::enum_vaccount_addr_type_secp256k1_user_account) || check_address_type(addr, base::enum_vaccount_addr_type_secp256k1_eth_user_account);
}

bool is_sub_account_address(common::xaccount_address_t const & addr) {
    return check_address_type(addr, base::enum_vaccount_addr_type_secp256k1_user_sub_account);
}

bool is_sys_contract_address(common::xaccount_address_t const & addr)  {
    return check_address_type(addr, base::enum_vaccount_addr_type_native_contract);
}

bool is_black_hole_address(common::xaccount_address_t const & addr)  {
    return check_address_type(addr, base::enum_vaccount_addr_type_black_hole);
}

bool is_drand_address(common::xaccount_address_t const & addr)  {
    return check_address_type(addr, base::enum_vaccount_addr_type_drand);
}

bool is_user_contract_address(common::xaccount_address_t const & addr) {
    return check_address_type(addr, base::enum_vaccount_addr_type_custom_contract);
}

bool is_contract_address(common::xaccount_address_t const & addr) {
    return is_sys_contract_address(addr) ||
           is_user_contract_address(addr);
}

bool is_beacon_contract_address(common::xaccount_address_t const & addr) {
    return check_address_type_and_zone(addr, base::enum_vaccount_addr_type_native_contract, base::enum_chain_zone_beacon_index);
}

bool is_zec_contract_address(common::xaccount_address_t const & addr) {
    return check_address_type_and_zone(addr, base::enum_vaccount_addr_type_native_contract, base::enum_chain_zone_zec_index);
}

bool is_sys_sharding_contract_address(common::xaccount_address_t const & addr) {
    return check_address_type_and_zone(addr, base::enum_vaccount_addr_type_native_contract, base::enum_chain_zone_consensus_index);
}

bool is_block_contract_address(common::xaccount_address_t const & addr) {
    return check_address_type(addr, base::enum_vaccount_addr_type_block_contract);
}

common::xaccount_address_t make_address_by_prefix_and_subaddr(const std::string & prefix, uint16_t subaddr) {
    if (std::string::npos == prefix.find("@")) {
        std::string final_account_address = prefix + "@" + base::xstring_utl::tostring(subaddr);
        return common::xaccount_address_t{final_account_address};
    }
    return common::xaccount_address_t{prefix};
}

base::xtable_index_t account_map_to_table_id(common::xaccount_address_t const & addr) {
    const std::string & account = addr.value();
    base::xvaccount_t _vaddr(account);
    return _vaddr.get_tableid();
}

std::string account_address_to_block_address(common::xaccount_address_t const & addr) {
    base::xtable_index_t tableid = account_map_to_table_id(addr);
    return xblocktool_t::make_address_table_account(tableid.get_zone_index(), tableid.get_subaddr());
}

bool is_table_address(common::xaccount_address_t const & addr) {
    return check_address_type(addr, base::enum_vaccount_addr_type_block_contract);
}

bool is_unit_address(common::xaccount_address_t const & addr) {
    return !is_table_address(addr);
}

}  // namespace data
}  // namespace top
