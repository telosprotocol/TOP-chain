// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xlog.h"
#include "xbase/xutl.h"
#include "xbase/xcontext.h"
#include "xutility/xhash.h"
#include "xcrypto/xckey.h"

#include "xdata/xdata_error.h"
#include "xdata/xaction_parse.h"
#include "xdata/xtransaction.h"
#include "xdata/xproperty.h"
#include "xdata/xchain_param.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xmemcheck_dbg.h"
#include "xvm/manager/xcontract_address_map.h"
// TODO(jimmy) #include "xbase/xvledger.h"

#include "xbasic/xversion.h"
#include "xdata/xdata_defines.h"
#include <cinttypes>
namespace top { namespace data {

REG_CLS(xtransaction_t);
REG_CLS(xtransaction_store_t);

int32_t xtransaction_header::serialize_write(base::xstream_t & stream, bool is_write_without_len) const {
    const int32_t begin_pos = stream.size();
    stream << m_transaction_type;
    if (is_write_without_len) {
        uint16_t transaction_len = 0;
        stream << transaction_len;
    } else {
        stream << m_transaction_len;
    }
    stream << m_version;
    stream << m_to_ledger_id;
    stream << m_from_ledger_id;
    stream << m_deposit;
    stream << m_expire_duration;
    stream << m_fire_timestamp;
    stream << m_trans_random_nonce;
    stream << m_premium_price;
    stream << m_last_trans_nonce;
    stream << m_last_trans_hash;
    stream << m_challenge_proof;
    stream << m_ext;
    stream << m_memo;
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}
int32_t xtransaction_header::serialize_read(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    stream >> m_transaction_type;
    stream >> m_transaction_len;
    stream >> m_version;
    stream >> m_to_ledger_id;
    stream >> m_from_ledger_id;
    stream >> m_deposit;
    stream >> m_expire_duration;
    stream >> m_fire_timestamp;
    stream >> m_trans_random_nonce;
    stream >> m_premium_price;
    stream >> m_last_trans_nonce;
    stream >> m_last_trans_hash;
    stream >> m_challenge_proof;
    stream >> m_ext;
    stream >> m_memo;
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

xtransaction_t::xtransaction_t() {
    MEMCHECK_ADD_TRACE(this, "tx_create");
}

xtransaction_t::~xtransaction_t() {
    MEMCHECK_REMOVE_TRACE(this);
}

int32_t xtransaction_t::do_write_without_hash_signature(base::xstream_t & stream, bool is_write_without_len) const {
    const int32_t begin_pos = stream.size();
    xtransaction_header::serialize_write(stream, is_write_without_len);
    m_source_action.serialize_write(stream, is_write_without_len);
    m_target_action.serialize_write(stream, is_write_without_len);
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t xtransaction_t::do_read_without_hash_signature(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    xtransaction_header::serialize_read(stream);
    m_source_action.serialize_read(stream);
    m_target_action.serialize_read(stream);
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

int32_t xtransaction_t::do_write(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    do_write_without_hash_signature(stream, false);
    stream << m_transaction_hash;
    stream << m_authorization;
    stream << m_edge_nodeid;
    stream << m_target_addr;
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}
int32_t xtransaction_t::do_read(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    do_read_without_hash_signature(stream);
    stream >> m_transaction_hash;
    stream >> m_authorization;
    stream >> m_edge_nodeid;
    stream >> m_target_addr;
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

#ifdef XENABLE_PSTACK  // tracking memory
int32_t xtransaction_t::add_ref() {
    MEMCHECK_ADD_TRACE(this, "tx_add");
    return base::xdataobj_t::add_ref();
}
int32_t xtransaction_t::release_ref() {
    MEMCHECK_ADD_TRACE(this, "tx_release");
    xdbgassert(get_refcount() > 0);
    return base::xdataobj_t::release_ref();
}
#endif

void xtransaction_t::adjust_target_address(uint32_t table_id) {
    if (m_target_addr.empty()) {
        m_target_addr = make_address_by_prefix_and_subaddr(m_target_action.get_account_addr(), table_id).value();
        add_modified_count();
    }
}

void xtransaction_t::set_digest() {
    base::xstream_t stream(base::xcontext_t::instance());
    do_write_without_hash_signature(stream, true);
    m_transaction_hash = utl::xsha2_256_t::digest((const char*)stream.data(), stream.size());
    add_modified_count();
}

void xtransaction_t::set_signature(const std::string & signature) {
    m_authorization = signature;
}

void xtransaction_t::set_len() {
    base::xstream_t stream(base::xcontext_t::instance());
    const int32_t begin_pos = stream.size();
    xtransaction_header::serialize_write(stream, false);
    int32_t action_len = m_source_action.serialize_write(stream, false);
    m_source_action.set_action_size(action_len);
    action_len = m_target_action.serialize_write(stream, false);
    m_target_action.set_action_size(action_len);
    stream << m_transaction_hash;
    stream << m_authorization;
    set_tx_len(stream.size() - begin_pos);
}

bool xtransaction_t::digest_check() const {
    base::xstream_t stream(base::xcontext_t::instance());
    do_write_without_hash_signature(stream, true);
    uint256_t hash = utl::xsha2_256_t::digest((const char*)stream.data(), stream.size());
    if (hash != m_transaction_hash) {
        xwarn("xtransaction_t::digest_check fail. %s %s",
            to_hex_str(hash).c_str(), to_hex_str(m_transaction_hash).c_str());
        return false;
    }
    return true;
}

bool xtransaction_t::transaction_type_check() const {
    switch (get_tx_type()) {
#ifdef DEBUG  // debug use
        case xtransaction_type_create_user_account:
        case xtransaction_type_set_account_keys:
        case xtransaction_type_lock_token:
        case xtransaction_type_unlock_token:
        case xtransaction_type_alias_name:
        case xtransaction_type_create_sub_account:
        case xtransaction_type_pledge_token_disk:
        case xtransaction_type_redeem_token_disk:
#endif
        case xtransaction_type_create_contract_account:
        case xtransaction_type_run_contract:
        case xtransaction_type_transfer:
        case xtransaction_type_vote:
        case xtransaction_type_abolish_vote:
        case xtransaction_type_pledge_token_tgas:
        case xtransaction_type_redeem_token_tgas:
        case xtransaction_type_pledge_token_vote:
        case xtransaction_type_redeem_token_vote:
            return true;
        default:
            return false;
    }
}

bool xtransaction_t::unuse_member_check() const {
    if (get_tx_version() != 0) {
        return false;
    }
    if (get_to_ledger_id() != 0 || get_from_ledger_id() != 0) {
        return false;
    }
    if (get_random_nonce() != 0) {
        return false;
    }
    if (get_premium_price() != 0) {
        return false;
    }
    if (!get_challenge_proof().empty()) {
        return false;
    }
    if (!get_ext().empty()) {
        return false;
    }
    if (!m_edge_nodeid.empty()) {
        return false;
    }
    return true;
}

bool xtransaction_t::transaction_len_check() const {
    // TODO(nathan):tx parameters validity check
    if (get_memo().size() > MAX_TRANSACTION_MEMO_SIZE) {
        xwarn("xtransaction_t::transaction_len_check memo size too long.size:%d", get_memo().size());
        return false;
    }
    base::xstream_t stream(base::xcontext_t::instance());

    const int32_t begin_pos = stream.size();
    xtransaction_header::serialize_write(stream, true);
    int32_t action_len = m_source_action.serialize_write(stream, true);
    if (action_len != m_source_action.get_action_size()) {
        xwarn("xtransaction_t::transaction_len_check src action_len not match:%d:%d", action_len, m_source_action.get_action_size());
        return false;
    }

    action_len = m_target_action.serialize_write(stream, true);
    if (action_len != m_target_action.get_action_size()) {
        xwarn("xtransaction_t::transaction_len_check tgt action_len not match:%d:%d", action_len, m_target_action.get_action_size());
        return false;
    }

    stream << m_transaction_hash;
    stream << m_authorization;

    if (stream.size() - begin_pos != get_tx_len()) {
        xwarn("xtransaction_t::transaction_len_check tx length not match. %d %d", stream.size() - begin_pos, get_tx_len());
        return false;
    }
    return true;
}

int32_t xtransaction_t::make_tx_create_user_account(const std::string & addr) {
    set_tx_type(xtransaction_type_create_user_account);
    int32_t ret = xaction_source_null::serialze_to(m_source_action);
    if (ret) { return ret; }
    ret = xaction_create_user_account::serialze_to(m_target_action, addr);
    if (ret) { return ret; }

    return xsuccess;
}

int32_t xtransaction_t::make_tx_create_contract_account(const data::xproperty_asset & asset_out, uint64_t tgas_limit, const std::string& code) {
    set_tx_type(xtransaction_type_create_contract_account);
    int32_t ret = xaction_asset_out::serialze_to(m_source_action, asset_out);
    if (ret) { return ret; }
    ret = xaction_deploy_contract::serialze_to(m_target_action, tgas_limit, code);
    if (ret) { return ret; }

    return xsuccess;
}

int32_t xtransaction_t::make_tx_create_sub_account(const data::xproperty_asset & asset_out) {
    set_tx_type(xtransaction_type_create_sub_account);
    int32_t ret = xaction_asset_out::serialze_to(m_source_action, asset_out);
    if (ret) { return ret; }
    ret = xaction_asset_out::serialze_to(m_target_action, asset_out);
    if (ret) { return ret; }
    return xsuccess;
}

int32_t xtransaction_t::make_tx_transfer(const data::xproperty_asset & asset) {
    set_tx_type(xtransaction_type_transfer);
    int32_t ret = xaction_asset_out::serialze_to(m_source_action, asset);
    if (ret) { return ret; }
    ret = xaction_asset_in::serialze_to(m_target_action, asset, xaction_type_asset_in);
    if (ret) { return ret; }
    return xsuccess;
}

int32_t xtransaction_t::make_tx_run_contract(const data::xproperty_asset & asset_out, const std::string& function_name, const std::string& para) {
    set_tx_type(xtransaction_type_run_contract);
    int32_t ret = xaction_asset_out::serialze_to(m_source_action, asset_out);
    if (ret) { return ret; }
    ret = xaction_run_contract::serialze_to(m_target_action, function_name, para);
    if (ret) { return ret; }
    return xsuccess;
}

int32_t xtransaction_t::make_tx_run_contract2(const data::xproperty_asset & asset_out, const std::string & function_name, const std::string & para) {
    set_tx_type(xtransaction_type_run_contract2);
    int32_t ret = xaction_asset_out::serialze_to(m_source_action, asset_out);
    if (ret) {
        return ret;
    }
    ret = xaction_run_contract::serialze_to(m_target_action, function_name, para);
    if (ret) {
        return ret;
    }
    return xsuccess;
}

int32_t xtransaction_t::make_tx_run_contract(std::string const & function_name, std::string const & param) {
    data::xproperty_asset asset_out{0};
    return make_tx_run_contract(asset_out, function_name, param);
}

int32_t xtransaction_t::make_tx_run_contract2(std::string const & function_name, std::string const & param) {
    data::xproperty_asset asset_out{0};
    return make_tx_run_contract2(asset_out, function_name, param);
}

int32_t xtransaction_t::set_different_source_target_address(const std::string & src_addr, const std::string & dst_addr) {
    m_source_action.set_account_addr(src_addr);
    m_target_action.set_account_addr(dst_addr);
    return xsuccess;
}

int32_t xtransaction_t::set_same_source_target_address(const std::string & addr) {
    return set_different_source_target_address(addr, addr);
}

void xtransaction_t::set_last_trans_hash_and_nonce(uint256_t last_hash, uint64_t last_nonce) {
    set_last_nonce(last_nonce);
    set_last_hash(utl::xxh64_t::digest(last_hash.data(), last_hash.size()));
}

bool xtransaction_t::check_last_trans_hash(const uint256_t & account_last_hash) {
    bool ret = get_last_hash() == utl::xxh64_t::digest(account_last_hash.data(), account_last_hash.size());
    if (!ret) {
        xinfo("xtransaction_t::check_last_trans_hash account_lasthash_256:%s account_lasthash_64:%lx, tx_lasthash:%lx",
            to_hex_str(account_last_hash).c_str(), utl::xxh64_t::digest(account_last_hash.data(), account_last_hash.size()), get_last_hash());
    }

    return ret;
}

std::string xtransaction_t::get_digest_hex_str() const {
    if (m_transaction_hash_str.empty()) {
        m_transaction_hash_str = to_hex_str(m_transaction_hash);
    }
    return m_transaction_hash_str;
}

bool xtransaction_t::check_last_nonce(uint64_t account_nonce) {
    return (get_last_nonce() == account_nonce);
}

void xtransaction_t::set_fire_and_expire_time(uint16_t expire_duration) {
    struct timeval val;
    base::xtime_utl::gettimeofday(&val);
    set_fire_timestamp((uint64_t)val.tv_sec);
    set_expire_duration(expire_duration);
}


bool xtransaction_t::sign_check() const {
    static std::set<uint16_t> no_check_tx_type { xtransaction_type_lock_token, xtransaction_type_unlock_token };
    std::string addr_prefix;
    if (std::string::npos != get_source_addr().find_last_of('@')) {
        uint16_t subaddr;
        base::xvaccount_t::get_prefix_subaddr_from_account(get_source_addr(), addr_prefix, subaddr);
    } else {
        addr_prefix = get_source_addr();
    }

    utl::xkeyaddress_t key_address(addr_prefix);
    uint8_t     addr_type{255};
    uint16_t    network_id{65535};
    //get param from config
    uint16_t config_network_id = 0;//xchain_param.network_id
    if (!key_address.get_type_and_netid(addr_type, network_id) || config_network_id != network_id) {
        xwarn("network_id error:%d,%d", config_network_id, network_id);
        return false;
    }
    if (no_check_tx_type.find(get_tx_type()) != std::end(no_check_tx_type)) {  // no check for other key
        return true;
    }
    utl::xecdsasig_t signature_obj((uint8_t *)m_authorization.c_str());
    if (data::is_sub_account_address(common::xaccount_address_t{ get_source_addr() }) || data::is_user_contract_address(common::xaccount_address_t{ get_source_addr() })) {
        return key_address.verify_signature(signature_obj, m_transaction_hash, get_parent_account());
    } else {
        return key_address.verify_signature(signature_obj, m_transaction_hash);
    }

}

bool xtransaction_t::pub_key_sign_check(xpublic_key_t const & pub_key) const {
    auto pub_data = base::xstring_utl::base64_decode(pub_key.to_string());
    xdbg("[global_trace][xtx_verifier][verify_tx_signature][pub_key_sign_check] pub_key: %s , decode.size():%d", pub_key.to_string().c_str(), pub_data.size());
    utl::xecdsasig_t signature_obj((uint8_t *)m_authorization.data());
    uint8_t out_publickey_data[utl::UNCOMPRESSED_PUBLICKEY_SIZE];
    memcpy(out_publickey_data, pub_data.data(), (size_t)std::min(utl::UNCOMPRESSED_PUBLICKEY_SIZE, (int)pub_data.size()));
    return utl::xsecp256k1_t::verify_signature(signature_obj, m_transaction_hash, out_publickey_data, false);
}

size_t xtransaction_t::get_serialize_size() const {
    base::xstream_t stream(base::xcontext_t::instance());
    xassert(stream.size() == 0);
    const_cast<xtransaction_t*>(this)->serialize_to(stream);
    return stream.size();
}

std::string xtransaction_t::dump() const {
    char local_param_buf[256];
    xprintf(local_param_buf,    sizeof(local_param_buf),
    "{transaction:hash=%s,type=%u,subtype=%u,from=%s,to=%s,nonce=%" PRIu64 ",refcount=%d,this=%p}",
    get_digest_hex_str().c_str(), (uint32_t)get_tx_type(), (uint32_t)get_tx_subtype(), get_source_addr().c_str(), get_target_addr().c_str(),
    get_tx_nonce(), get_refcount(), this);
    return std::string(local_param_buf);
}

int32_t xtransaction_store_t::do_write(base::xstream_t & stream) {
    KEEP_SIZE();
    DEFAULT_SERIALIZE_PTR(m_raw_tx);
    SERIALIZE_FIELD_BT(m_send_unit_height);
    SERIALIZE_FIELD_BT(m_recv_unit_height);
    SERIALIZE_FIELD_BT(m_confirm_unit_height);
    SERIALIZE_FIELD_BT(m_flag);
    SERIALIZE_FIELD_BT(m_ext);
    return CALC_LEN();
}

int32_t xtransaction_store_t::do_read(base::xstream_t & stream) {
    KEEP_SIZE();
    DEFAULT_DESERIALIZE_PTR(m_raw_tx, xtransaction_t);
    DESERIALIZE_FIELD_BT(m_send_unit_height);
    DESERIALIZE_FIELD_BT(m_recv_unit_height);
    DESERIALIZE_FIELD_BT(m_confirm_unit_height);
    DESERIALIZE_FIELD_BT(m_flag);
    DESERIALIZE_FIELD_BT(m_ext);
    return CALC_LEN();
}

}  // namespace data
}  // namespace top
