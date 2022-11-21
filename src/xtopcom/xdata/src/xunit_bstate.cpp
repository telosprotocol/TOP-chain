// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xunit_bstate.h"

#include "xbase/xint.h"
#include "xbase/xutl.h"
#include "xbasic/xutility.h"
#include "xcodec/xcodec.h"
#include "xcodec/xcodec_category.h"
#include "xcommon/xcodec/xmsgpack/xaccount_address_codec.hpp"
#include "xcommon/xerror/xerror.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xcodec/xmsgpack/xallowance_codec.h"
#include "xdata/xdata_error.h"
#include "xdata/xerror/xerror.h"
#include "xdata/xfullunit.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xevm/xevm.h"
#include "xevm_common/fixed_hash.h"
#include "xevm_common/xcodec/xmsgpack/xboost_u256_codec.h"
#include "xmetrics/xmetrics.h"
#include "xpbase/base/top_utils.h"

namespace top {
namespace data {

xunit_bstate_t::xunit_bstate_t(base::xvbstate_t* bstate, bool readonly)
: xbstate_ctx_t(bstate, readonly) {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_unit_state, 1);
}

xunit_bstate_t::~xunit_bstate_t() {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_unit_state, -1);
}

bool xunit_bstate_t::is_empty_state() const {
    return get_bstate()->get_property_num() == 0;
}
uint64_t xunit_bstate_t::get_free_tgas() const {
    uint64_t total_asset = balance() + lock_balance() + tgas_balance() + disk_balance() + vote_balance();
    if (total_asset >= XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_free_gas_asset)) {
        return XGET_ONCHAIN_GOVERNANCE_PARAMETER(free_gas);
    } else {
        return 0;
    }
}

// how many tgas you can get from pledging 1TOP
uint32_t xunit_bstate_t::get_token_price(uint64_t onchain_total_gas_deposit) {
    uint64_t initial_total_gas_deposit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(initial_total_gas_deposit);
    xdbg("tgas_disk get total gas deposit from beacon: %llu, %llu", initial_total_gas_deposit, onchain_total_gas_deposit);
    uint64_t total_gas_deposit = onchain_total_gas_deposit + initial_total_gas_deposit;
    return XGET_ONCHAIN_GOVERNANCE_PARAMETER(total_gas_shard) * XGET_CONFIG(validator_group_count) * TOP_UNIT / total_gas_deposit;
}

uint64_t xunit_bstate_t::get_total_tgas(uint32_t token_price) const {
    uint64_t pledge_token = tgas_balance();
    uint64_t total_tgas = pledge_token * token_price / TOP_UNIT + get_free_tgas();
    uint64_t max_tgas;
    // contract account, max tgas is different
    // if (is_user_contract_address(common::xaccount_address_t{get_account()})) {
    //     max_tgas = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_contract);
    // } else {
        max_tgas = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account);
    // }
    return std::min(total_tgas, max_tgas);
}

uint64_t xunit_bstate_t::get_last_tx_hour() const {
    std::string v;
    string_get(XPROPERTY_LAST_TX_HOUR_KEY, v);
    if (!v.empty()) {
        return (uint64_t)std::stoull(v);
    }
    return 0;
}

uint64_t xunit_bstate_t::get_used_tgas() const {
    std::string v;
    string_get(XPROPERTY_USED_TGAS_KEY, v);
    if (!v.empty()) {
        return (uint64_t)std::stoull(v);
    }
    return 0;
}

uint64_t xunit_bstate_t::calc_decayed_tgas(uint64_t timer_height) const {
    uint32_t last_hour = get_last_tx_hour();
    uint64_t used_tgas{0};
    uint32_t usedgas_reset_interval = XGET_ONCHAIN_GOVERNANCE_PARAMETER(usedgas_reset_interval);
    if (timer_height <= last_hour) {
        used_tgas = get_used_tgas();
    } else if (timer_height - last_hour < usedgas_reset_interval) {
        used_tgas = (usedgas_reset_interval - (timer_height - last_hour)) * get_used_tgas() / usedgas_reset_interval;
    }
    return used_tgas;
}

uint64_t xunit_bstate_t::get_available_tgas(uint64_t timer_height, uint32_t token_price) const {
    uint64_t used_tgas = calc_decayed_tgas(timer_height);
    uint64_t total_tgas = get_total_tgas(token_price);
    uint64_t available_tgas{0};
    if (total_tgas > used_tgas) {
        available_tgas = total_tgas - used_tgas;
    }
    return available_tgas;
}

uint64_t xunit_bstate_t::available_tgas(uint64_t timer_height, uint64_t onchain_total_gas_deposit) const {
    uint32_t token_price = get_token_price(onchain_total_gas_deposit);
    uint64_t used_tgas = calc_decayed_tgas(timer_height);
    uint64_t total_tgas = get_total_tgas(token_price);
    uint64_t available_tgas{0};
    if (total_tgas > used_tgas) {
        available_tgas = total_tgas - used_tgas;
    }
    return available_tgas;
}

uint64_t xunit_bstate_t::get_account_create_time() const {
    uint64_t create_time = uint64_property_get(XPROPERTY_ACCOUNT_CREATE_TIME);
    if (create_time < base::TOP_BEGIN_GMTIME) {
        // tackle create_time set to be clock bug
        return create_time * 10 + base::TOP_BEGIN_GMTIME;
    } else {
        return create_time;
    }
}

uint32_t xunit_bstate_t::get_unconfirm_sendtx_num() const {
    std::string value = map_get(XPROPERTY_TX_INFO, XPROPERTY_TX_INFO_UNCONFIRM_TX_NUM);
    if (value.empty()) {
        return 0;
    }
    return base::xstring_utl::touint32(value);
}
uint64_t xunit_bstate_t::get_latest_send_trans_number() const {
    std::string value = map_get(XPROPERTY_TX_INFO, XPROPERTY_TX_INFO_LATEST_SENDTX_NUM);
    if (value.empty()) {
        return 0;
    }
    return base::xstring_utl::touint64(value);
}

uint64_t xunit_bstate_t::account_recv_trans_number() const {
    std::string value = map_get(XPROPERTY_TX_INFO, XPROPERTY_TX_INFO_RECVTX_NUM);
    if (value.empty()) {
        return 0;
    }
    return base::xstring_utl::touint64(value);
}

uint256_t xunit_bstate_t::account_send_trans_hash() const {
    std::string value = map_get(XPROPERTY_TX_INFO, XPROPERTY_TX_INFO_LATEST_SENDTX_HASH);
    if (value.empty()) {
        uint256_t default_value;
        return default_value;
    }
    return uint256_t((uint8_t*)value.c_str());
}
uint64_t xunit_bstate_t::account_send_trans_number() const {
    return get_latest_send_trans_number();
}


//========= set apis ===========
int32_t xunit_bstate_t::set_account_create_time(uint64_t clock) {
    if (false == get_bstate()->find_property(XPROPERTY_ACCOUNT_CREATE_TIME)) {        
        uint64_t create_time = clock == 0 ? base::TOP_BEGIN_GMTIME : clock;
        return uint64_set(XPROPERTY_ACCOUNT_CREATE_TIME, create_time);
    }
    return xsuccess;
}

int32_t xunit_bstate_t::set_tx_info_latest_sendtx_num(uint64_t num) {
    std::string value = base::xstring_utl::tostring(num);
    return map_set(XPROPERTY_TX_INFO, XPROPERTY_TX_INFO_LATEST_SENDTX_NUM, value);
}
int32_t xunit_bstate_t::set_tx_info_latest_sendtx_hash(const std::string & hash) {
    return map_set(XPROPERTY_TX_INFO, XPROPERTY_TX_INFO_LATEST_SENDTX_HASH, hash);
}
int32_t xunit_bstate_t::set_tx_info_recvtx_num(uint64_t num) {
    std::string value = base::xstring_utl::tostring(num);
    return map_set(XPROPERTY_TX_INFO, XPROPERTY_TX_INFO_RECVTX_NUM, value);
}

std::string xunit_bstate_t::get_code() const {
    std::string v;
    string_get(XPROPERTY_EVM_CODE, v);
    return v;
}

std::string xunit_bstate_t::get_storage(const std::string& index_str) const {
    evm_common::u256 uindex = evm_common::fromBigEndian<top::evm_common::u256>(index_str);
    evm_common::h256 hindex = (evm_common::h256)uindex;
    std::string index = evm_common::toHex(hindex);

    std::string generation_str = top::HexEncode(string_get(data::XPROPERTY_EVM_GENERATION));
    xdbg("xunit_bstate_t::get_storage, %s, %s", generation_str.c_str(), index.c_str());
    std::string value_str = map_get(data::XPROPERTY_EVM_STORAGE, generation_str + index);

    if (value_str.empty()) {
        xwarn("xunit_bstate_t::get_storage fail");
        evm_common::h256 value;
        //js_rsp["result"] = std::string("0x") + evm_common::toHex(value);
        //return;
        value_str = std::string((char*)value.data(), value.size());
    }
    return value_str;
}

void xunit_bstate_t::transfer(common::xtoken_id_t const token_id, observer_ptr<xunit_bstate_t> const & recver_state, evm_common::u256 const & value, std::error_code & ec) {
    assert(recver_state != nullptr);
    assert(!ec);

    auto const amount_vtoken = static_cast<base::vtoken_t>(value.convert_to<uint64_t>());

    switch (token_id) {
    case common::xtoken_id_t::top: {
        if (token_withdraw(data::XPROPERTY_BALANCE_AVAILABLE, amount_vtoken)) {
            xwarn("sender_withdraw failed");
            ec = error::xerrc_t::update_state_failed;
            break;
        }

        if (recver_state->token_deposit(data::XPROPERTY_BALANCE_AVAILABLE, amount_vtoken)) {
            xwarn("recver_deposit failed");
            ec = error::xerrc_t::update_state_failed;
            break;
        }

        break;
    }

    case common::xtoken_id_t::usdt:
        XATTRIBUTE_FALLTHROUGH;
    case common::xtoken_id_t::usdc: {
        if (tep_token_withdraw(token_id, value)) {
            xwarn("sender_tep_withdraw failed");
            ec = error::xerrc_t::update_state_failed;
            break;
        }

        if (recver_state->tep_token_deposit(token_id, value)) {
            xwarn("recver_tep_deposit failed");
            ec = error::xerrc_t::update_state_failed;
            break;
        }

        break;
    }

    default:
        assert(false);
        ec = error::xerrc_t::action_address_type_error;
        break;
    }
}

evm_common::u256 xunit_bstate_t::allowance(common::xtoken_id_t const token_id, common::xaccount_address_t const & spender, std::error_code & ec) const {
    auto const raw_allowance_data = raw_allowance(token_id, ec);
    if (ec) {
        return 0;
    }

    return allowance_impl(raw_allowance_data, spender, ec);
}

std::map<common::xtoken_id_t, data::system_contract::xallowance_t> xunit_bstate_t::allowance(std::error_code & ec) const {
    auto const raw_allowance_data = raw_allowance(ec);
    if (ec) {
        return {};
    }

    return allowance_impl(raw_allowance_data, ec);
}

void xunit_bstate_t::approve(common::xtoken_id_t const token_id, common::xaccount_address_t const & spender, evm_common::u256 const & amount, std::error_code & ec) {
    assert(!ec);

    set_allowance(token_id, spender, amount, ec);
    assert(allowance(token_id, spender, ec) == amount);
}

void xunit_bstate_t::update_allowance(common::xtoken_id_t const token_id,
                                     common::xaccount_address_t const & spender,
                                     evm_common::u256 const & amount,
                                     xallowance_update_op_t const op,
                                     std::error_code & ec) {
    assert(!ec);

    switch (op) {
    case xallowance_update_op_t::increase:
        set_allowance(token_id, spender, amount, ec);
        return;

    case xallowance_update_op_t::decrease:
        dec_allowance(token_id, spender, amount, ec);
        return;

    default:
        assert(false);
        ec = error::xerrc_t::property_type_invalid;
        return;
    }
}

xbytes_t xunit_bstate_t::raw_allowance(common::xtoken_id_t const token_id, std::error_code & ec) const {
    assert(!ec);

    xobject_ptr_t<base::xmapvar_t<std::string>> raw_data;
    if (m_bstate->find_property(data::XPROPERTY_PRECOMPILED_ERC20_ALLOWANCE_KEY)) {
        raw_data = m_bstate->load_string_map_var(data::XPROPERTY_PRECOMPILED_ERC20_ALLOWANCE_KEY);
        xdbg("found allowance data");
    } else {
        raw_data = m_bstate->new_string_map_var(data::XPROPERTY_PRECOMPILED_ERC20_ALLOWANCE_KEY, m_canvas.get());
        xdbg("not found allowance data, tried to create a new one");
    }

    if (nullptr == raw_data) {
        ec = error::xerrc_t::property_not_exist;
        xwarn("failed to get allowance data");
        return {};
    }

    return top::to_bytes(raw_data->query(top::to_string(token_id)));
}

xobject_ptr_t<base::xmapvar_t<std::string>> xunit_bstate_t::raw_allowance(std::error_code & ec) const {
    assert(!ec);

    xobject_ptr_t<base::xmapvar_t<std::string>> raw_data;
    if (m_bstate->find_property(data::XPROPERTY_PRECOMPILED_ERC20_ALLOWANCE_KEY)) {
        raw_data = m_bstate->load_string_map_var(data::XPROPERTY_PRECOMPILED_ERC20_ALLOWANCE_KEY);
    } else {
        raw_data = m_bstate->new_string_map_var(data::XPROPERTY_PRECOMPILED_ERC20_ALLOWANCE_KEY, m_canvas.get());
    }

    if (nullptr == raw_data) {
        ec = error::xerrc_t::property_not_exist;
    }

    return raw_data;
}

void xunit_bstate_t::raw_allowance(common::xtoken_id_t const token_id, xbytes_t const & raw_data, std::error_code & ec) {
    assert(!ec);

    xobject_ptr_t<base::xmapvar_t<std::string>> property;
    if (m_bstate->find_property(data::XPROPERTY_PRECOMPILED_ERC20_ALLOWANCE_KEY)) {
        property = m_bstate->load_string_map_var(data::XPROPERTY_PRECOMPILED_ERC20_ALLOWANCE_KEY);
    } else {
        property = m_bstate->new_string_map_var(data::XPROPERTY_PRECOMPILED_ERC20_ALLOWANCE_KEY, m_canvas.get());
    }

    if (nullptr == property) {
        ec = error::xerrc_t::property_not_exist;
        return;
    }

    if (property->insert(top::to_string(token_id), top::to_string(raw_data), m_canvas.get()) == false) {
        ec = error::xerrc_t::update_state_failed;
    }
    assert(top::to_bytes(property->query(top::to_string(token_id))) == raw_data);
}

evm_common::u256 xunit_bstate_t::allowance_impl(xbytes_t const & serialized_allowance_map, common::xaccount_address_t const & spender, std::error_code & ec) const {
    assert(!ec);

    auto const map = codec::xcodec_t<data::system_contract::xallowance_t, codec::xcodec_type_t::msgpack>::decode(serialized_allowance_map, ec);
    if (ec) {
        return 0;
    }

    auto const it = map.find(spender);
    if (it != std::end(map)) {
        return top::get<evm_common::u256>(*it);
    }

    return 0;
}

std::map<common::xtoken_id_t, data::system_contract::xallowance_t> xunit_bstate_t::allowance_impl(
    xobject_ptr_t<base::xmapvar_t<std::string>> const & raw_allowance_data,
    std::error_code & ec) const {
    assert(!ec);

    std::map<common::xtoken_id_t, data::system_contract::xallowance_t> ret;
    auto const & raw_data = raw_allowance_data->query();
    for (auto const & raw_datum : raw_data) {
        auto const token_id = top::from_string<common::xtoken_id_t>(raw_datum.first);
        auto allowance = codec::xcodec_t<data::system_contract::xallowance_t, codec::xcodec_type_t::msgpack>::decode(top::to_bytes(raw_datum.second), ec);
        if (ec) {
            continue;
        }

        // ret[token_id] = std::move(allowance);
        ret.emplace(token_id, std::move(allowance));
    }

    return ret;
}

void xunit_bstate_t::set_allowance(common::xtoken_id_t const token_id, common::xaccount_address_t const & spender, evm_common::u256 const & amount, std::error_code & ec) {
    assert(!ec);

    auto raw_data = raw_allowance(token_id, ec);
    if (ec) {
        return;
    }

    auto map = codec::xcodec_t<data::system_contract::xallowance_t, codec::xcodec_type_t::msgpack>::decode(raw_data, ec);
    if (ec) {
        return;
    }

    auto it = map.find(spender);
    if (it == std::end(map)) {
        it = top::get<decltype(it)>(map.insert({spender, 0}));
    }
    assert(it != std::end(map));
    auto & approved = top::get<evm_common::u256>(*it);
    approved = amount;

    raw_data = codec::xcodec_t<data::system_contract::xallowance_t, codec::xcodec_type_t::msgpack>::encode(map, ec);
    if (ec) {
        return;
    }
    assert(approved == (codec::xcodec_t<data::system_contract::xallowance_t, codec::xcodec_type_t::msgpack>::decode(raw_data, ec).find(spender)->second));
    raw_allowance(token_id, raw_data, ec);
}

void xunit_bstate_t::dec_allowance(common::xtoken_id_t const token_id, common::xaccount_address_t const & spender, evm_common::u256 const & amount, std::error_code & ec) {
    assert(!ec);

    auto raw_data = raw_allowance(token_id, ec);
    if (ec) {
        return;
    }

    auto map = codec::xcodec_t<data::system_contract::xallowance_t, codec::xcodec_type_t::msgpack>::decode(raw_data, ec);
    if (ec) {
        xwarn("decode allowance failed. ec %d ecmsg %s ec cat %s", ec.value(), ec.message().c_str(), ec.category().name());
        return;
    }

    auto it = map.find(spender);
    if (it == std::end(map)) {
        ec = error::xerrc_t::erc20_allowance_spender_not_found;
        xwarn("allowance spender not found");
        return;
    }
    assert(it != std::end(map));
    auto & approved = top::get<evm_common::u256>(*it);
    if (approved < amount) {
        ec = error::xerrc_t::erc20_allowance_not_enough;
        xwarn("allowance not enough. approved %s value %s", approved.str().c_str(), amount.str().c_str());
        return;
    }
    approved -= amount;

    raw_data = codec::xcodec_t<data::system_contract::xallowance_t, codec::xcodec_type_t::msgpack>::encode(map, ec);
    if (ec) {
        xwarn("encode allowance failed.");
        return;
    }
    raw_allowance(token_id, raw_data, ec);
}

common::xaccount_address_t xunit_bstate_t::tep_token_owner(common::xchain_uuid_t const chain_uuid) const {
    std::error_code ec;

    do {
        auto const & raw_data = raw_owner(chain_uuid, ec);
        if (ec) {
            xwarn("get TEP token owner failed: ec %d msg %s category %s", ec.value(), ec.message().c_str(), ec.category().name());
            break;
        }

        if (raw_data.empty()) {
            xwarn("get TEP token owner failed: empty owner account for token %d", static_cast<int>(chain_uuid));
            break;
        }

        auto owner = common::xaccount_address_t::build_from(top::to_string(raw_data), ec);
        if (ec) {
            xwarn("get TEP token owner failed: failed to read account data for token %d", static_cast<int>(chain_uuid));
            break;
        }

        if (owner.empty()) {
            xwarn("get TEP token owner failed: extracted owner account is empty for token %d", static_cast<int>(chain_uuid));
            break;
        }

        return owner;
    } while (false);

#if defined(XBUILD_DEV) || defined(XBUILD_CI) || defined(XBUILD_GALILEO) || defined(XBUILD_BOUNTY)
    common::xeth_address_t const & default_owner = common::xeth_address_t::build_from("0xf8a1e199c49c2ae2682ecc5b4a8838b39bab1a38");
#else
    common::xeth_address_t const & default_owner = common::xeth_address_t::build_from("0x8b587045c7fcd6faddf022fdf8e09756f2fd6cc6");
#endif
    xkinfo("get TEP token owner: use default token owner %s for token %d", default_owner.c_str(), static_cast<int>(chain_uuid));

    common::xaccount_address_t default_owner_address = common::xaccount_address_t::build_from(default_owner, base::enum_vaccount_addr_type_secp256k1_evm_user_account);
    return default_owner_address;
}

void xunit_bstate_t::tep_token_owner(common::xchain_uuid_t const chain_uuid, common::xaccount_address_t const & new_owner, std::error_code & ec) {
    assert(!ec);
    if (new_owner.empty()) {
        ec = common::error::xerrc_t::invalid_account_address;
        xwarn("set TEP token owner failed: owner account is empty");
        return;
    }

    xobject_ptr_t<base::xmapvar_t<std::string>> const owner = raw_owner(ec);
    if (ec) {
        xwarn("set TEP token owner failed: get property failed.");
        return;
    }

    std::string const key = top::to_string(chain_uuid);
    do {
        auto const & owner_account_string = owner->query(key);
        if (owner_account_string.empty()) {
            break;
        }

        auto const & owner_account = common::xaccount_address_t::build_from(owner_account_string, ec);
        if (ec) {
            break;
        }

        if (owner_account.empty()) {
            break;
        }

        if (new_owner == eth_zero_address) {
            break;
        }

        ec = error::xerrc_t::erc20_owner_already_set;
        xwarn("set TEP token owner failed: owner already set for chain uuid %" PRIu16 ". input owner account %s", static_cast<uint16_t>(chain_uuid), new_owner.to_string().c_str());
        return;
    } while (false);

    if (owner->insert(key, new_owner.to_string(), m_canvas.get()) == false) {
        ec = error::xerrc_t::update_state_failed;
        xerror("update XPROPERTY_PRECOMPILED_ERC20_OWNER_KEY failed. token_id %" PRIu32, static_cast<uint32_t>(chain_uuid));
    }
    assert(owner->query(key) == new_owner.to_string());
}

xobject_ptr_t<base::xmapvar_t<std::string>> xunit_bstate_t::raw_owner(std::error_code & ec) const {
    assert(!ec);

    xobject_ptr_t<base::xmapvar_t<std::string>> raw_data;
    if (m_bstate->find_property(data::XPROPERTY_PRECOMPILED_ERC20_OWNER_KEY)) {
        raw_data = m_bstate->load_string_map_var(data::XPROPERTY_PRECOMPILED_ERC20_OWNER_KEY);
        xdbg("found owner data");
    } else {
        raw_data = m_bstate->new_string_map_var(data::XPROPERTY_PRECOMPILED_ERC20_OWNER_KEY, m_canvas.get());
        xdbg("not found owner data, tried to create a new one");
    }

    if (nullptr == raw_data) {
        ec = error::xerrc_t::property_not_exist;
        xerror("XPROPERTY_PRECOMPILED_ERC20_OWNER_KEY not exist");
    }

    return raw_data;
}

xbytes_t xunit_bstate_t::raw_owner(common::xchain_uuid_t const chain_uuid, std::error_code & ec) const {
    assert(!ec);

    xobject_ptr_t<base::xmapvar_t<std::string>> const raw_data = raw_owner(ec);
    if (ec) {
        xwarn("get owner for token %d failed.", static_cast<int>(chain_uuid));
        return {};
    }

    assert(raw_data != nullptr);
    return top::to_bytes(raw_data->query(top::to_string(chain_uuid)));
}

common::xaccount_address_t xunit_bstate_t::tep_token_controller(common::xchain_uuid_t const chain_uuid) const {
    std::error_code ec;

    auto const & raw_data = raw_controller(chain_uuid, ec);
    if (ec) {
        xwarn("get TEP token controller failed: ec %d msg %s category %s", ec.value(), ec.message().c_str(), ec.category().name());
        return eth_zero_address;
    }

    if (raw_data.empty()) {
        xwarn("get TEP token controller failed: empty controller account for token %d", static_cast<int>(chain_uuid));
        return eth_zero_address;
    }

    auto controller = common::xaccount_address_t::build_from(top::to_string(raw_data), ec);
    if (ec) {
        xwarn("get TEP token controller failed: failed to read account data for token %d", static_cast<int>(chain_uuid));
        return eth_zero_address;
    }

    if (controller.empty()) {
        xwarn("get TEP token controller failed: extracted controller account is empty for token %d", static_cast<int>(chain_uuid));
        return eth_zero_address;
    }

    return controller;
}

void xunit_bstate_t::tep_token_controller(common::xchain_uuid_t const chain_uuid, common::xaccount_address_t const & new_controller, std::error_code & ec) {
    assert(!ec);
    if (new_controller.empty()) {
        ec = common::error::xerrc_t::invalid_account_address;
        xwarn("set TEP token controller failed: controller account is empty");
        return;
    }

    xobject_ptr_t<base::xmapvar_t<std::string>> const controller = raw_controller(ec);
    if (ec) {
        return;
    }

    auto const key = top::to_string(chain_uuid);
    do {
        auto const & controller_account_string = controller->query(key);
        if (controller_account_string.empty()) {
            break;
        }

        auto const & controller_account = common::xaccount_address_t::build_from(controller_account_string, ec);
        if (ec) {
            xerror("controller account read from property invalid: %s", controller_account_string.c_str());
            break;
        }

        if (controller_account.empty()) {
            break;
        }

        if (new_controller == eth_zero_address) {
            break;
        }

#if defined(XBUILD_CI) && !defined(XENABLE_TESTS)
        if (new_controller != controller_account) {
            break;
        }
#endif

        ec = error::xerrc_t::erc20_controller_already_set;
        xwarn("set TEP token controller failed: controller already set for chain uuid %" PRIu16 ". input controller account %s",
              static_cast<uint16_t>(chain_uuid),
              new_controller.to_string().c_str());
        return;
    } while (false);

    if (controller->insert(key, new_controller.to_string(), m_canvas.get()) == false) {
        ec = error::xerrc_t::update_state_failed;
        xerror("update XPROPERTY_PRECOMPILED_ERC20_CONTROLLER_KEY failed. token_id %" PRIu32, static_cast<uint32_t>(chain_uuid));
    }
    assert(controller->query(key) == new_controller.to_string());
}

xobject_ptr_t<base::xmapvar_t<std::string>> xunit_bstate_t::raw_controller(std::error_code & ec) const {
    assert(!ec);

    xobject_ptr_t<base::xmapvar_t<std::string>> raw_data;
    if (m_bstate->find_property(data::XPROPERTY_PRECOMPILED_ERC20_CONTROLLER_KEY)) {
        raw_data = m_bstate->load_string_map_var(data::XPROPERTY_PRECOMPILED_ERC20_CONTROLLER_KEY);
        xdbg("found controller data");
    } else {
        raw_data = m_bstate->new_string_map_var(data::XPROPERTY_PRECOMPILED_ERC20_CONTROLLER_KEY, m_canvas.get());
        xdbg("not found controller data, tried to create a new one");
    }

    if (nullptr == raw_data) {
        ec = error::xerrc_t::property_not_exist;
        xerror("XPROPERTY_PRECOMPILED_ERC20_CONTROLLER_KEY not exist");
    }

    return raw_data;
}

xbytes_t xunit_bstate_t::raw_controller(common::xchain_uuid_t const chain_uuid, std::error_code & ec) const {
    assert(!ec);

    xobject_ptr_t<base::xmapvar_t<std::string>> const raw_data = raw_controller(ec);
    if (ec) {
        return {};
    }

    assert(raw_data != nullptr);
    return top::to_bytes(raw_data->query(top::to_string(chain_uuid)));
}

int32_t xunit_bstate_t::balance(uint64_t new_balance) {
    return set_token_balance(XPROPERTY_BALANCE_AVAILABLE, static_cast<base::vtoken_t>(new_balance));
}

int32_t xunit_bstate_t::burn_balance(uint64_t new_burn_balance) {
    return set_token_balance(XPROPERTY_BALANCE_BURN, static_cast<base::vtoken_t>(new_burn_balance));
}

int32_t xunit_bstate_t::tgas_balance(uint64_t new_tgas_balance) {
    return set_token_balance(XPROPERTY_BALANCE_PLEDGE_TGAS, static_cast<base::vtoken_t>(new_tgas_balance));
}

int32_t xunit_bstate_t::vote_balance(uint64_t new_vote_balance) {
    return set_token_balance(XPROPERTY_BALANCE_PLEDGE_VOTE, static_cast<base::vtoken_t>(new_vote_balance));
}

int32_t xunit_bstate_t::lock_balance(uint64_t new_lock_balance) {
    return set_token_balance(XPROPERTY_BALANCE_LOCK, static_cast<base::vtoken_t>(new_lock_balance));
}

}  // namespace data
}  // namespace top
