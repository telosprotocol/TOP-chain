// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xblockchain.h"
#include "xbase/xint.h"
#include "xbase/xmem.h"
#include "xbase/xutl.h"
#include "xbasic/xversion.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xblocktool.h"
#include "xdata/xdata_common.h"
#include "xdata/xdata_error.h"
#include "xdata/xfullunit.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xlightunit.h"
#include "xconfig/xpredefined_configurations.h"

#include <assert.h>
#include <string>
#include <vector>

namespace top {
namespace data {

REG_CLS(xblockchain2_t);

xblockchain2_t::xblockchain2_t(uint32_t chainid, const std::string & account, base::enum_xvblock_level level) : m_account(account), m_block_level(level) {
    add_modified_count();
}

xblockchain2_t::xblockchain2_t(const std::string & account, base::enum_xvblock_level level) : m_account(account), m_block_level(level) {
    add_modified_count();
}

xblockchain2_t::xblockchain2_t(const std::string & account) : m_account(account), m_block_level(base::enum_xvblock_level_unit) {
    add_modified_count();
}

xblockchain2_t::xblockchain2_t() {}

bool xblockchain2_t::is_property_behind() const {
    if (m_block_level != base::enum_xvblock_level_unit) {
        return false;
    }
    return m_max_block_height != m_property_confirm_height;
}

void xblockchain2_t::set_update_stamp(uint64_t timestamp) {
    if (m_update_stamp < timestamp) {
        m_update_stamp = timestamp;
        add_modified_count();
    }
}

int32_t xblockchain2_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    SERIALIZE_FIELD_BT(m_version);
    SERIALIZE_FIELD_BT(m_account);
    SERIALIZE_FIELD_BT(m_block_level);
    SERIALIZE_FIELD_BT(m_min_block_height);
    SERIALIZE_FIELD_BT(m_max_block_height);
    SERIALIZE_FIELD_BT(m_last_state_block_height);
    SERIALIZE_FIELD_BT(m_last_state_block_hash);
    SERIALIZE_FIELD_BT(m_update_stamp);
    SERIALIZE_FIELD_BT(m_property_confirm_height);
    SERIALIZE_FIELD_BT(m_last_full_block_height);
    SERIALIZE_FIELD_BT(m_last_full_block_hash);
    m_account_state.serialize_to(stream);
    SERIALIZE_FIELD_BT(m_ext);
    return CALC_LEN();
}

int32_t xblockchain2_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    DESERIALIZE_FIELD_BT(m_version);
    DESERIALIZE_FIELD_BT(m_account);
    DESERIALIZE_FIELD_BT(m_block_level);
    DESERIALIZE_FIELD_BT(m_min_block_height);
    DESERIALIZE_FIELD_BT(m_max_block_height);
    DESERIALIZE_FIELD_BT(m_last_state_block_height);
    DESERIALIZE_FIELD_BT(m_last_state_block_hash);
    DESERIALIZE_FIELD_BT(m_update_stamp);
    DESERIALIZE_FIELD_BT(m_property_confirm_height);
    DESERIALIZE_FIELD_BT(m_last_full_block_height);
    DESERIALIZE_FIELD_BT(m_last_full_block_hash);
    m_account_state.serialize_from(stream);
    DESERIALIZE_FIELD_BT(m_ext);
    return CALC_LEN();
}

void xblockchain2_t::update_min_max_height(uint64_t height) {
    // update number
    if (m_min_block_height == 0 || m_min_block_height > height) {
        m_min_block_height = height;
        add_modified_count();
    }

    if (m_max_block_height < height) {
        m_max_block_height = height;
        add_modified_count();
    }
}

void xblockchain2_t::set_min_chain_height(uint64_t height) {
    m_min_block_height = height;
    add_modified_count();
}

xtransaction_ptr_t xblockchain2_t::make_transfer_tx(const std::string & to, uint64_t amount, uint64_t firestamp, uint16_t duration, uint32_t deposit, const std::string& token_name) {
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset(token_name, amount);
    tx->make_tx_transfer(asset);
    tx->set_last_trans_hash_and_nonce(account_send_trans_hash(), account_send_trans_number());
    tx->set_different_source_target_address(get_account(), to);
    tx->set_fire_timestamp(firestamp);
    tx->set_expire_duration(duration);
    tx->set_deposit(deposit);
    tx->set_digest();
    tx->set_len();

    // update account send tx hash and number
    set_account_send_trans_hash(tx->digest());
    set_account_send_trans_number(tx->get_tx_nonce());
    return tx;
}

xtransaction_ptr_t xblockchain2_t::make_run_contract_tx(const std::string & to,
                                                        const std::string & func_name,
                                                        const std::string & func_param,
                                                        uint64_t amount,
                                                        uint64_t firestamp,
                                                        uint16_t duration,
                                                        uint32_t deposit) {
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset(amount);
    tx->make_tx_run_contract(asset, func_name, func_param);
    tx->set_last_trans_hash_and_nonce(account_send_trans_hash(), account_send_trans_number());
    tx->set_different_source_target_address(get_account(), to);
    tx->set_fire_timestamp(firestamp);
    tx->set_expire_duration(duration);
    tx->set_deposit(deposit);
    tx->set_digest();
    tx->set_len();
    // update account send tx hash and number
    set_account_send_trans_hash(tx->digest());
    set_account_send_trans_number(tx->get_tx_nonce());
    return tx;
}

bool xblockchain2_t::add_light_unit(const xblock_t * block) {
    const xlightunit_block_t * unit = dynamic_cast<const xlightunit_block_t *>(block);
    xassert(unit != nullptr);
    m_account_state.set_balance_change(unit->get_balance_change());
    m_account_state.set_burn_balance_change(unit->get_burn_balance_change());
    m_account_state.set_pledge_tgas_balance_change(unit->get_pledge_balance_change_tgas());
    m_account_state.set_pledge_disk_balance_change(unit->get_pledge_disk_change());
    m_account_state.set_pledge_vote_balance_change(unit->get_pledge_vote_change());
    m_account_state.set_lock_balance_change(unit->get_lock_balance_change());
    m_account_state.set_lock_tgas_change(unit->get_lock_tgas_change());
    m_account_state.set_unvote_number_change(unit->get_unvote_num_change());
    m_account_state.set_unconfirm_sendtx_num(unit->get_unconfirm_sendtx_num());

    m_account_state.set_propertys_hash_change(unit->get_property_hash_map());
    m_account_state.set_native_property_change(unit->get_native_property());

    uint64_t number;
    uint256_t hash;
    bool has_sendtx = unit->get_send_trans_info(number, hash);
    if (has_sendtx) {
        xassert(m_account_state.get_latest_send_trans_number() < number);
        m_account_state.set_latest_send_trans_number(number);
        m_account_state.set_latest_send_trans_hash(hash);
        xdbg("xblockchain2_t::add_light_unit update send tx info. block=%s trans_number=%ld,trans_hash=%s",
            block->dump().c_str(), number, to_hex_str(hash).c_str());
    }

    bool has_recvtx = unit->get_recv_trans_info(number, hash);
    if (has_recvtx) {
        m_account_state.set_latest_recv_trans_number(m_account_state.get_latest_recv_trans_number() + number);
        m_account_state.set_latest_recv_trans_hash(hash);
        xdbg("xblockchain2_t::add_light_unit update recv tx info. block=%s trans_number=%ld,trans_hash=%s",
            block->dump().c_str(), number, to_hex_str(hash).c_str());
    }
    return true;
}

bool xblockchain2_t::add_full_unit(const xblock_t * block) {
    xassert(block->is_fullunit());
    m_account_state = *block->get_fullunit_mstate();
    m_last_full_block_height = block->get_height();
    m_last_full_block_hash = block->get_block_hash();
    return true;
}

bool xblockchain2_t::update_last_block_state(const xblock_t * block) {
    if (block->is_lightunit()) {
        add_light_unit(block);
    } else if (block->is_fullunit()) {
        add_full_unit(block);
    }
    return true;
}

bool xblockchain2_t::update_state_by_genesis_block(const xblock_t * block) {
    xassert(!block->get_block_hash().empty());
    xassert(block->get_height() == 0);

    // if the genesis block is not the nil block, it must be a "genesis account".
    // the create time of genesis account should be set to the gmtime of genesis block
    if (!m_account_state.get_account_create_time() && block->get_header()->get_block_class() != base::enum_xvblock_class_nil) {
        m_account_state.set_account_create_time(block->get_cert()->get_gmtime());
        xdbg("[account change]address:%s account_create_time:%ld", block->get_account().c_str(), m_account_state.get_account_create_time());
    }
    // update last state
    if (m_last_state_block_height == 0) {
        update_last_block_state(block);
        m_last_state_block_height = block->get_height();
        m_last_state_block_hash = block->get_block_hash();
        add_modified_count();
    } else {
        // disorder set genesis block, only for empty genesis block
        xassert(block->is_emptyblock());
    }
    // genesis block as last full block hash
    if (m_last_full_block_height == 0) {
        m_last_full_block_height = block->get_height();
        m_last_full_block_hash = block->get_block_hash();
    }

    return true;
}

bool xblockchain2_t::update_state_by_next_height_block(const xblock_t * block) {
    xassert(!block->get_block_hash().empty());
    xassert(block->get_height() >= 1);
    if (m_last_state_block_height + 1 != block->get_height() || (m_last_state_block_hash != block->get_last_block_hash() && block->get_height() >= 2)) {
        xassert(0);
        return false;
    }

    // the create time of non-genesis account should be set to the gmtime of height#1 block
    if (block->get_height() == 1 && !m_account_state.get_account_create_time()) {
        m_account_state.set_account_create_time(block->get_cert()->get_gmtime());
        xdbg("[account change]address:%s account_create_time:%ld", block->get_account().c_str(), m_account_state.get_account_create_time());
    }

    auto ret = update_last_block_state(block);
    if (!ret) {
        return ret;
    }
    m_last_state_block_height = block->get_height();
    m_last_state_block_hash = block->get_block_hash();
    add_modified_count();
    return true;
}

bool xblockchain2_t::update_state_by_full_block(const xblock_t * block) {
    xassert(!block->get_block_hash().empty());
    xassert(block->get_block_class() == base::enum_xvblock_class_full);
    if (block->get_block_class() != base::enum_xvblock_class_full) {
        xassert(0);
        return false;
    }
    auto ret = update_last_block_state(block);
    if (!ret) {
        return ret;
    }
    xassert(block->get_height() > m_last_state_block_height);
    m_last_state_block_height = block->get_height();
    m_last_state_block_hash = block->get_block_hash();
    add_modified_count();
    return true;
}

std::string xblockchain2_t::to_basic_string() const {
    std::stringstream ss;
    ss << "{";
    ss << ",min_h=" << m_min_block_height;
    ss << ",max_h=" << m_max_block_height;
    ss << ",state_height=" << m_last_state_block_height;
    ss << ",state_hash=" << base::xstring_utl::to_hex(m_last_state_block_hash);
    ss << ",updatestamp=" << m_update_stamp;
    ss << "}";
    return ss.str();
}

uint64_t xblockchain2_t::get_free_tgas() const {
    uint64_t total_asset = balance() + lock_balance() + tgas_balance() + disk_balance() + vote_balance();
    if (total_asset >= XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_free_gas_balance)) {
        return XGET_ONCHAIN_GOVERNANCE_PARAMETER(free_gas);
    } else {
        return 0;
    }
}

// how many tgas you can get from pledging 1TOP
uint32_t xblockchain2_t::get_token_price(uint64_t onchain_total_pledge_token) {
    uint64_t initial_total_pledge_token = XGET_ONCHAIN_GOVERNANCE_PARAMETER(initial_total_locked_token);
    xinfo("tgas_disk get total pledge token from beacon: %llu, %llu", initial_total_pledge_token, onchain_total_pledge_token);
    uint64_t total_pledge_token = onchain_total_pledge_token + initial_total_pledge_token;
    return XGET_ONCHAIN_GOVERNANCE_PARAMETER(total_gas_shard) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(validator_group_count) * TOP_UNIT / total_pledge_token;
}

uint64_t xblockchain2_t::get_total_tgas(uint32_t token_price) const {
    uint64_t pledge_token = tgas_balance();
    uint64_t total_tgas = pledge_token * token_price / TOP_UNIT + get_free_tgas();
    uint64_t max_tgas;
    // contract account, max tgas is different
    if (is_user_contract_address(common::xaccount_address_t{m_account})) {
        max_tgas = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_contract);
    } else {
        max_tgas = XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_gas_account);
    }
    return std::min(total_tgas, max_tgas);
}

uint64_t xblockchain2_t::get_last_tx_hour() const {
    int32_t ret = 0;
    std::string v;
    auto native_property = get_native_property();
    ret = native_property.native_string_get(XPROPERTY_LAST_TX_HOUR_KEY, v);
    if (0 == ret) {
        return (uint64_t)std::stoull(v);
    }
    return 0;
}

uint64_t xblockchain2_t::get_used_tgas() const {
    int32_t ret = 0;
    std::string v;
    auto native_property = get_native_property();
    ret = native_property.native_string_get(XPROPERTY_USED_TGAS_KEY, v);
    if (0 == ret) {
        return (uint64_t)std::stoull(v);
    }
    return 0;
}

uint64_t xblockchain2_t::calc_decayed_tgas(uint64_t timer_height) const {
    uint32_t last_hour = get_last_tx_hour();
    uint64_t used_tgas{0};
    uint32_t decay_time = XGET_ONCHAIN_GOVERNANCE_PARAMETER(usedgas_decay_cycle);
    if (timer_height <= last_hour) {
        used_tgas = get_used_tgas();
    } else if (timer_height - last_hour < decay_time) {
        used_tgas = (decay_time - (timer_height - last_hour)) * get_used_tgas() / decay_time;
    }
    return used_tgas;
}

uint64_t xblockchain2_t::get_available_tgas(uint64_t timer_height, uint32_t token_price) const {
    uint64_t used_tgas = calc_decayed_tgas(timer_height);
    uint64_t total_tgas = get_total_tgas(token_price);
    uint64_t available_tgas{0};
    if (total_tgas > used_tgas) {
        available_tgas = total_tgas - used_tgas;
    }
    return available_tgas;
}

void xblockchain2_t::set_unconfirmed_accounts(const std::set<std::string> & accounts) {
    if (accounts.empty()) {
        m_ext.erase(enum_blockchain_ext_type_uncnfirmed_accounts);
    } else {
        base::xstream_t stream(base::xcontext_t::instance());
        stream << accounts;
        const std::string accounts_str((const char*)stream.data(),stream.size());
        m_ext[enum_blockchain_ext_type_uncnfirmed_accounts] = accounts_str;
    }
}

const std::set<std::string> xblockchain2_t::get_unconfirmed_accounts() const {
    std::set<std::string> accounts;
    auto iter = m_ext.find(enum_blockchain_ext_type_uncnfirmed_accounts);
    if (iter == m_ext.end()) {
        return accounts;
    }

    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)(iter->second.data()), (uint32_t)(iter->second.size()));
    stream >> accounts;
    return accounts;
}

}  // namespace data
}  // namespace top
