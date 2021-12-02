// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xcontract_helper.h"

#include "xbasic/xerror/xthrow_error.h"
#include "xcontract_runtime/xerror/xerror.h"
#include "xstore/xstore_error.h"
#include "xchain_fork/xchain_upgrade_center.h"

using namespace top::data;

NS_BEG2(top, contract_runtime)
using store::xaccount_context_t;
using std::string;
using std::vector;
using store::xstore_success;
using store::xaccount_property_not_create;
using store::xaccount_property_map_field_not_create;

xcontract_helper::xcontract_helper(xaccount_context_t* account_context,
                                   common::xaccount_address_t const & contract_account,
                                   common::xaccount_address_t const & exec_account)
:m_account_context(account_context)
,m_contract_account(contract_account)
,m_exec_account(exec_account) {
}

void xcontract_helper::set_transaction(const xtransaction_ptr_t& ptr) {
    m_transaction = ptr;
}

xtransaction_ptr_t xcontract_helper::get_transaction() const {
    return m_transaction;
}

common::xaccount_address_t xcontract_helper::get_source_account() const {
    return m_exec_account;
}

std::string xcontract_helper::get_parent_account() const {
    std::string parent{""};
    // TODO(jimmy) not support lua now
    std::error_code ec{ error::xerrc_t::enum_vm_exception };
    top::error::throw_error(ec, "get contract parent account error");
    return parent;
}

common::xnode_id_t const & xcontract_helper::get_self_account() const noexcept {
    return m_contract_account;
}

uint64_t xcontract_helper::get_balance() const {
    return m_account_context->get_blockchain()->balance();
}

common::xlogic_time_t xcontract_helper::get_timer_height() const {
    return m_account_context->get_timer_height();
}

const data::xaction_asset_out& xcontract_helper::get_pay_fee() const {
    return m_account_context->get_source_pay_info();
}

void xcontract_helper::create_transfer_tx(const string& grant_account, const uint64_t amount) {
    m_account_context->create_transfer_tx(grant_account, amount);
}


void xcontract_helper::string_create(const string& key) {
    if (m_account_context->string_create(key)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "STRING_CREATE " + key + " error");
    }
}
void xcontract_helper::string_set(const string& key, const string& value, bool native) {
    if (m_account_context->string_set(key, value, native)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "STRING_SET " + key + " error");
    }
}
string xcontract_helper::string_get(const string& key, const std::string& addr) {
    string value;
    if (m_account_context->string_get(key, value, addr)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "STRING_GET " + key + " error");
    }
    return value;
}

string xcontract_helper::string_get2(const string& key, const std::string& addr) {
    string value;
    m_account_context->string_get(key, value, addr);
    return value;
}

bool xcontract_helper::string_exist(const string& key, const std::string& addr) {
    string value;
    int32_t ret = m_account_context->string_get(key, value, addr);
    if (xaccount_property_not_create == ret) {
        return false;
    } else if (xstore_success == ret) {
        return true;
    } else {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "STRING_EXIST error" + std::to_string(ret));
    }
    return true;
}

void xcontract_helper::list_create(const string& key) {
    if (m_account_context->list_create(key)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "LIST_CREATE " + key + " error");
    }
}

void xcontract_helper::list_push_back(const string& key, const string& value, bool native) {
    if (m_account_context->list_push_back(key, value, native)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "LIST_PUSH_BACK  " + key + " error");
    }
}

void xcontract_helper::list_push_front(const string& key, const string& value, bool native) {
    if (m_account_context->list_push_front(key, value, native)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "LIST_PUSH_FRONT " + key + " error");
    }
}

void xcontract_helper::list_pop_back(const string& key, string& value, bool native) {
    if (m_account_context->list_pop_back(key, value, native)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "LIST_POP_BACK " + key + " error");
    }
}

void xcontract_helper::list_pop_front(const string& key, string& value, bool native) {
    if (m_account_context->list_pop_front(key, value, native)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, key + " LIST_POP_FRONT " + key + " error");
    }
}

void xcontract_helper::list_clear(const string& key, bool native) {
    if (m_account_context->list_clear(key, native)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, key + " LIST_CLEAR " + key + " error");
    }
}

std::string xcontract_helper::list_get(const std::string& key, int32_t index, const std::string& addr) {
    std::string value{};
    if (m_account_context->list_get(key, index, value, addr)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "LIST_GET " + key + " error");
    }
    return value;
}

int32_t xcontract_helper::list_size(const string& key, const std::string& addr) {
    int32_t size;
    if (m_account_context->list_size(key, size, addr)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "LIST_SIZE " + key + " error");
    }
    return size;
}

vector<string> xcontract_helper::list_get_all(const string& key, const string& addr) {
    vector<string> value_list{};
    if (m_account_context->list_get_all(key, value_list, addr)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "LIST_GET_ALL " + key + " error");
    }
    return std::move(value_list);
}

bool xcontract_helper::list_exist(const string& key) {
    vector<string> value_list{};
    int32_t ret = m_account_context->list_get_all(key, value_list);
    if (xaccount_property_not_create == ret) {
        return false;
    } else if (xstore_success == ret) {
        return true;
    } else {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "LIST_EXIST error" + std::to_string(ret));
    }
    return true;
}

void xcontract_helper::map_create(const string& key) {
    if (m_account_context->map_create(key)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "MAP_CREATE " + key + " error");
    }
}

string xcontract_helper::map_get(const string& key, const string& field, const std::string& addr) {
    string value{};
    if (m_account_context->map_get(key, field, value, addr)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "MAP_GET " + key + " error");
    }
    return value;
}

int32_t xcontract_helper::map_get2(const string& key, const string& field, string& value, const std::string& addr) {
    return m_account_context->map_get(key, field, value, addr);
}

void xcontract_helper::map_set(const string& key, const string& field, const string & value, bool native) {
    if (m_account_context->map_set(key, field, value, native)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "MAP_SET " + key + " error");
    }
}

void xcontract_helper::map_remove(const string& key, const string& field, bool native) {
    if (m_account_context->map_remove(key, field, native)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "MAP_REMOVE " + key + " error");
    }
}

int32_t xcontract_helper::map_size(const string& key, const std::string& addr) {
    int32_t size{0};
    if (m_account_context->map_size(key, size, addr)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "MAP_SIZE " + key + " error");
    }
    return size;
}

void xcontract_helper::map_copy_get(const std::string & key, std::map<std::string, std::string> & map, const std::string& addr) {
    if (m_account_context->map_copy_get(key, map, addr)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "MAP_COPY_GET " + key + " error");
    }
}


bool xcontract_helper::map_field_exist(const string& key, const string& field) {
    string value{};
    int32_t ret = m_account_context->map_get(key, field, value);
    if (xaccount_property_map_field_not_create == ret || xaccount_property_not_create == ret) {
        return false;
    } else if (xstore_success == ret) {
        return true;
    } else {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "MAP_FIELD_EXIST error:" + std::to_string(ret));
    }
    return true;
}

bool xcontract_helper::map_key_exist(const std::string& key) {
    string field, value;
    int32_t ret = m_account_context->map_get(key, field, value);
    if (xaccount_property_not_create == ret) {
        return false;
    } else {
        return true;
    }
}

void xcontract_helper::map_clear(const std::string& key, bool native) {
    if (m_account_context->map_clear(key, native)) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "MAP_CLEAR " + key + " error");
    }
}

void xcontract_helper::get_map_property(const std::string& key, std::map<std::string, std::string>& value, uint64_t height, const std::string& addr) {
    m_account_context->get_map_property(key, value, height, addr);
}

bool xcontract_helper::map_property_exist(const std::string& key) {
    return m_account_context->map_property_exist(key) == 0;
}

void xcontract_helper::generate_tx(common::xaccount_address_t const & target_addr, const string& func_name, const string& func_param) {
    if (m_contract_account == target_addr) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "can't send to self " + target_addr.value());
    }
    int32_t ret = m_account_context->generate_tx(target_addr.value(), func_name, func_param);
    if (ret) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "generate tx fail " + store::xstore_error_to_string(ret));
    }
}

std::string xcontract_helper::get_random_seed() const {
    auto random_seed = m_account_context->get_random_seed();
    if (random_seed.empty()) {
        std::error_code ec{ error::xerrc_t::enum_vm_exception };
        top::error::throw_error(ec, "random_seed empty");
    }
    return random_seed;
}

std::uint64_t
xcontract_helper::contract_height() const {
    return m_account_context->get_chain_height();
}

base::xauto_ptr<xblock_t>
xcontract_helper::get_block_by_height(const std::string & owner, uint64_t height) const {
    return base::xauto_ptr<xblock_t>(m_account_context->get_block_by_height(owner, height));
}

std::uint64_t  xcontract_helper::get_blockchain_height(const std::string & owner) const {
    return m_account_context->get_blockchain_height(owner);
}

int32_t xcontract_helper::get_gas_and_disk_usage(std::uint32_t &gas, std::uint32_t &disk) const {
    store::xtransaction_result_t result;
    m_account_context->get_transaction_result(result);

    for (auto const & tx : result.m_contract_txs) {
        uint32_t size = tx->get_transaction()->get_tx_len();
        gas += 2 * size;
        xdbg("[xcontract_helper::get_gas_and_disk_usage] size: %u, gas: %u, disk: %u\n", size, gas, disk);
    }
    return 0;
}

NS_END2
