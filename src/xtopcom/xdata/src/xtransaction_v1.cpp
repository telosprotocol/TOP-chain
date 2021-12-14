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
#include "xdata/xtransaction_v1.h"
#include "xdata/xtransaction_v2.h"
#include "xdata/xproperty.h"
#include "xdata/xchain_param.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xmemcheck_dbg.h"
#include "xvm/manager/xcontract_address_map.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xmetrics/xmetrics.h"
#include "xbasic/xversion.h"
#include "xdata/xdata_defines.h"
#include "xvledger/xvaccount.h"

#include <cinttypes>
namespace top { namespace data {

int32_t xtransaction_v1_t::serialize_write(base::xstream_t & stream, bool is_write_without_len) const {
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
int32_t xtransaction_v1_t::serialize_read(base::xstream_t & stream) {
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

xtransaction_v1_t::xtransaction_v1_t() {
    MEMCHECK_ADD_TRACE(this, "tx_create");
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xtransaction_t, 1);
}

xtransaction_v1_t::~xtransaction_v1_t() {
    MEMCHECK_REMOVE_TRACE(this);
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xtransaction_t, -1);
}

void xtransaction_v1_t::construct_tx(enum_xtransaction_type tx_type, const uint16_t expire_duration, const uint32_t deposit, const uint32_t nonce, const std::string & memo, const xtx_action_info & info) {
    set_tx_type(tx_type);
    set_expire_duration(expire_duration);
    set_deposit(deposit);
    set_premium_price(0);
    set_last_nonce(nonce);
    set_fire_timestamp(get_gmttime_s());
    set_from_ledger_id(0);
    set_to_ledger_id(0);

    m_source_action.set_account_addr(info.m_source_addr);
    m_source_action.set_action_name(info.m_source_action_name);
    m_source_action.set_action_param(info.m_source_action_para);

    m_target_action.set_account_addr(info.m_target_addr);
    m_target_action.set_action_name(info.m_target_action_name);
    m_target_action.set_action_param(info.m_target_action_para);

    xtransaction_t::set_action_type_by_tx_type(static_cast<enum_xtransaction_type>(m_transaction_type));
}

int32_t xtransaction_v1_t::do_write_without_hash_signature(base::xstream_t & stream, bool is_write_without_len) const {
    const int32_t begin_pos = stream.size();
    serialize_write(stream, is_write_without_len);
    m_source_action.serialize_write(stream, is_write_without_len);
    m_target_action.serialize_write(stream, is_write_without_len);
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}

int32_t xtransaction_v1_t::do_read_without_hash_signature(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    serialize_read(stream);
    m_source_action.serialize_read(stream);
    m_target_action.serialize_read(stream);
    const int32_t end_pos = stream.size();
    return (begin_pos - end_pos);
}

int32_t xtransaction_v1_t::do_write(base::xstream_t & stream) {
    const int32_t begin_pos = stream.size();
    do_write_without_hash_signature(stream, false);
    stream << m_transaction_hash;
    stream << m_authorization;
    stream << m_edge_nodeid;
    stream << m_target_addr;
    const int32_t end_pos = stream.size();
    return (end_pos - begin_pos);
}
int32_t xtransaction_v1_t::do_read(base::xstream_t & stream) {
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
int32_t xtransaction_v1_t::add_ref() {
    MEMCHECK_ADD_TRACE(this, "tx_add");
    return base::xdataunit_t::add_ref();
}
int32_t xtransaction_v1_t::release_ref() {
    MEMCHECK_ADD_TRACE(this, "tx_release");
    xdbgassert(get_refcount() > 0);
    return base::xdataunit_t::release_ref();
}
#endif

void xtransaction_v1_t::adjust_target_address(uint32_t table_id) {
    if (m_target_addr.empty()) {
        m_target_addr = make_address_by_prefix_and_subaddr(m_target_action.get_account_addr(), table_id).value();
        xdbg("xtransaction_v1_t::adjust_target_address hash=%s,origin_addr=%s,new_addr=%s",
            get_digest_hex_str().c_str(), m_target_action.get_account_addr().c_str(), m_target_addr.c_str());        
    }
}

void xtransaction_v1_t::set_digest() {
    base::xstream_t stream(base::xcontext_t::instance());
    do_write_without_hash_signature(stream, true);
    m_transaction_hash = utl::xsha2_256_t::digest((const char*)stream.data(), stream.size());
    XMETRICS_GAUGE(metrics::cpu_hash_256_xtransaction_v1_calc, 1);
}

void xtransaction_v1_t::set_len() {
    base::xstream_t stream(base::xcontext_t::instance());
    const int32_t begin_pos = stream.size();
    serialize_write(stream, false);
    int32_t action_len = m_source_action.serialize_write(stream, false);
    m_source_action.set_action_size(action_len);
    action_len = m_target_action.serialize_write(stream, false);
    m_target_action.set_action_size(action_len);
    stream << m_transaction_hash;
    stream << m_authorization;
    set_tx_len(stream.size() - begin_pos);
}

bool xtransaction_v1_t::digest_check() const {
    base::xstream_t stream(base::xcontext_t::instance());
    do_write_without_hash_signature(stream, true);
    uint256_t hash = utl::xsha2_256_t::digest((const char*)stream.data(), stream.size());
    XMETRICS_GAUGE(metrics::cpu_hash_256_xtransaction_v1_calc, 1);
    if (hash != m_transaction_hash) {
        xwarn("xtransaction_v1_t::digest_check fail. %s %s",
            to_hex_str(hash).c_str(), to_hex_str(m_transaction_hash).c_str());
        return false;
    }
    return true;
}

bool xtransaction_v1_t::unuse_member_check() const {
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

bool xtransaction_v1_t::transaction_len_check() const {
    // TODO(nathan):tx parameters validity check
    if (get_memo().size() > MAX_TRANSACTION_MEMO_SIZE) {
        xwarn("xtransaction_v1_t::transaction_len_check memo size too long.size:%d", get_memo().size());
        return false;
    }
    base::xstream_t stream(base::xcontext_t::instance());

    const int32_t begin_pos = stream.size();
    serialize_write(stream, true);
    int32_t action_len = m_source_action.serialize_write(stream, true);
    if (action_len != m_source_action.get_action_size()) {
        xwarn("xtransaction_v1_t::transaction_len_check src action_len not match:%d:%d", action_len, m_source_action.get_action_size());
        return false;
    }

    action_len = m_target_action.serialize_write(stream, true);
    if (action_len != m_target_action.get_action_size()) {
        xwarn("xtransaction_v1_t::transaction_len_check tgt action_len not match:%d:%d", action_len, m_target_action.get_action_size());
        return false;
    }

    stream << m_transaction_hash;
    stream << m_authorization;

    if (stream.size() - begin_pos != get_tx_len()) {
        xwarn("xtransaction_v1_t::transaction_len_check tx length not match. %d %d", stream.size() - begin_pos, get_tx_len());
        return false;
    }
    return true;
}

int32_t xtransaction_v1_t::make_tx_create_user_account(const std::string & addr) {
    set_tx_type(xtransaction_type_create_user_account);
    int32_t ret = xaction_source_null::serialze_to(m_source_action);
    if (ret) { return ret; }
    ret = xaction_create_user_account::serialze_to(m_target_action, addr);
    if (ret) { return ret; }

    return xsuccess;
}

int32_t xtransaction_v1_t::make_tx_transfer(const data::xproperty_asset & asset) {
    set_tx_type(xtransaction_type_transfer);
    int32_t ret = xaction_asset_out::serialze_to(m_source_action, asset);
    if (ret) { return ret; }
    ret = xaction_asset_in::serialze_to(m_target_action, asset, xaction_type_asset_in);
    if (ret) { return ret; }
    return xsuccess;
}

int32_t xtransaction_v1_t::make_tx_run_contract(const data::xproperty_asset & asset_out, const std::string& function_name, const std::string& para) {
    set_from_ledger_id(0);
    set_to_ledger_id(0);
    set_tx_type(xtransaction_type_run_contract);
    int32_t ret = xaction_asset_out::serialze_to(m_source_action, asset_out);
    if (ret) { return ret; }
    ret = xaction_run_contract::serialze_to(m_target_action, function_name, para);
    if (ret) { return ret; }
    return xsuccess;
}

int32_t xtransaction_v1_t::make_tx_run_contract(std::string const & function_name, std::string const & param) {
    data::xproperty_asset asset_out{0};
    return make_tx_run_contract(asset_out, function_name, param);
}

int32_t xtransaction_v1_t::set_different_source_target_address(const std::string & src_addr, const std::string & dst_addr) {
    m_source_action.set_account_addr(src_addr);
    m_target_action.set_account_addr(dst_addr);
    return xsuccess;
}

int32_t xtransaction_v1_t::set_same_source_target_address(const std::string & addr) {
    return set_different_source_target_address(addr, addr);
}

void xtransaction_v1_t::set_last_trans_hash_and_nonce(uint256_t last_hash, uint64_t last_nonce) {
    set_last_nonce(last_nonce);
    set_last_hash(utl::xxh64_t::digest(last_hash.data(), last_hash.size()));
}

bool xtransaction_v1_t::check_last_trans_hash(const uint256_t & account_last_hash) {
    bool ret = get_last_hash() == utl::xxh64_t::digest(account_last_hash.data(), account_last_hash.size());
    if (!ret) {
        xinfo("xtransaction_v1_t::check_last_trans_hash account_lasthash_256:%s account_lasthash_64:%lx, tx_lasthash:%lx",
            to_hex_str(account_last_hash).c_str(), utl::xxh64_t::digest(account_last_hash.data(), account_last_hash.size()), get_last_hash());
    }

    return ret;
}

std::string xtransaction_v1_t::get_digest_hex_str() const {
    if (m_transaction_hash_str.empty()) {
        m_transaction_hash_str = to_hex_str(m_transaction_hash);
    }
    return m_transaction_hash_str;
}

bool xtransaction_v1_t::check_last_nonce(uint64_t account_nonce) {
    return (get_last_nonce() == account_nonce);
}

void xtransaction_v1_t::set_fire_and_expire_time(uint16_t expire_duration) {
    struct timeval val;
    base::xtime_utl::gettimeofday(&val);
    set_fire_timestamp((uint64_t)val.tv_sec);
    set_expire_duration(expire_duration);
}

bool xtransaction_v1_t::sign_check() const {
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

    utl::xecdsasig_t signature_obj((uint8_t *)m_authorization.c_str());
    return key_address.verify_signature(signature_obj, m_transaction_hash);
}

bool xtransaction_v1_t::pub_key_sign_check(xpublic_key_t const & pub_key) const {
    auto pub_data = base::xstring_utl::base64_decode(pub_key.to_string());
    xdbg("[global_trace][xtx_verifier][verify_tx_signature][pub_key_sign_check] pub_key: %s , decode.size():%d", pub_key.to_string().c_str(), pub_data.size());
    utl::xecdsasig_t signature_obj((uint8_t *)m_authorization.data());
    uint8_t out_publickey_data[utl::UNCOMPRESSED_PUBLICKEY_SIZE];
    memcpy(out_publickey_data, pub_data.data(), (size_t)std::min(utl::UNCOMPRESSED_PUBLICKEY_SIZE, (int)pub_data.size()));
    return utl::xsecp256k1_t::verify_signature(signature_obj, m_transaction_hash, out_publickey_data, false);
}

size_t xtransaction_v1_t::get_serialize_size() const {
    base::xstream_t stream(base::xcontext_t::instance());
    xassert(stream.size() == 0);
    const_cast<xtransaction_v1_t*>(this)->serialize_to(stream);
    return stream.size();
}

std::string xtransaction_v1_t::dump() const {
    char local_param_buf[256];
    xprintf(local_param_buf,    sizeof(local_param_buf),
    "{transaction:hash=%s,type=%u,from=%s,to=%s,nonce=%" PRIu64 ",refcount=%d,this=%p}",
    get_digest_hex_str().c_str(), (uint32_t)get_tx_type(), get_source_addr().c_str(), get_target_addr().c_str(),
    get_tx_nonce(), get_refcount(), this);
    return std::string(local_param_buf);
}

void xtransaction_v1_t::parse_to_json(xJson::Value& result_json, const std::string & version) const {
    result_json["tx_structure_version"] = get_tx_version();
    result_json["tx_deposit"] = get_deposit();
    result_json["tx_type"] = get_tx_type();
    result_json["tx_len"] = get_tx_len();
    result_json["tx_hash"] = data::uint_to_str(digest().data(), digest().size());
    result_json["tx_expire_duration"] = get_expire_duration();
    result_json["send_timestamp"] = static_cast<xJson::UInt64>(get_fire_timestamp());
    result_json["premium_price"] = get_premium_price();
    result_json["last_tx_nonce"] = static_cast<xJson::UInt64>(get_last_nonce());
    result_json["note"] = get_memo();
    result_json["ext"] = data::uint_to_str(get_ext().data(), get_ext().size());
    result_json["authorization"] = get_authorization();
    xdbg("authorization: %s", to_hex_str(get_authorization()).c_str());

    if (version == RPC_VERSION_V2) {
        result_json["edge_nodeid"] = m_edge_nodeid;

        std::string token_name;
        uint64_t amount{0};
        if (get_tx_type() == xtransaction_type_transfer) {
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)m_source_action.get_action_param().data(), m_source_action.get_action_param().size());
            stream >> token_name;
            stream >> amount;
        }
        result_json["amount"] = static_cast<xJson::UInt64>(amount);
        result_json["token_name"] = token_name;
        
        result_json["sender_account"] = m_source_action.get_account_addr();
        result_json["sender_action_name"] = m_source_action.get_action_name();
        result_json["sender_action_param"] = data::uint_to_str(m_source_action.get_action_param().data(), m_source_action.get_action_param().size());
        result_json["receiver_account"] = m_target_action.get_account_addr();
        result_json["receiver_action_name"] = m_target_action.get_action_name();
        result_json["receiver_action_param"] = data::uint_to_str(m_target_action.get_action_param().data(), m_target_action.get_action_param().size());
    } else {
        result_json["to_ledger_id"] = get_to_ledger_id();
        result_json["from_ledger_id"] = get_from_ledger_id();
        result_json["tx_random_nonce"] = get_random_nonce();
        result_json["last_tx_hash"] = data::uint64_to_str(get_last_hash());
        result_json["challenge_proof"] = get_challenge_proof();

        xJson::Value& s_action_json = result_json["sender_action"];
        s_action_json["action_hash"] = m_source_action.get_action_hash();
        s_action_json["action_type"] = m_source_action.get_action_type();
        s_action_json["action_size"] = m_source_action.get_action_size();
        s_action_json["tx_sender_account_addr"] = m_source_action.get_account_addr();
        s_action_json["action_name"] = m_source_action.get_action_name();
        s_action_json["action_param"] = data::uint_to_str(m_source_action.get_action_param().data(), m_source_action.get_action_param().size());
        s_action_json["action_ext"] = data::uint_to_str(m_source_action.get_action_ext().data(), m_source_action.get_action_ext().size());
        s_action_json["action_authorization"] = m_source_action.get_action_authorization();

        xJson::Value& t_action_json = result_json["receiver_action"];
        t_action_json["action_hash"] = m_target_action.get_action_hash();
        t_action_json["action_type"] = m_target_action.get_action_type();
        t_action_json["action_size"] = m_target_action.get_action_size();
        t_action_json["tx_receiver_account_addr"] = m_target_action.get_account_addr();
        t_action_json["action_name"] = m_target_action.get_action_name();
        t_action_json["action_param"] = data::uint_to_str(m_target_action.get_action_param().data(), m_target_action.get_action_param().size());
        t_action_json["action_ext"] = data::uint_to_str(m_target_action.get_action_ext().data(), m_target_action.get_action_ext().size());
        t_action_json["action_authorization"] = m_target_action.get_action_authorization();
    }
}

void xtransaction_v1_t::construct_from_json(xJson::Value& request) {
    set_tx_version(request["tx_structure_version"].asUInt());
    set_deposit(request["tx_deposit"].asUInt());
    set_to_ledger_id(static_cast<uint8_t>(request["to_ledger_id"].asUInt()));
    set_from_ledger_id(static_cast<uint8_t>(request["from_ledger_id"].asUInt()));
    set_tx_type(static_cast<uint16_t>(request["tx_type"].asUInt()));
    set_tx_len(static_cast<uint16_t>(request["tx_len"].asUInt()));
    set_expire_duration(static_cast<uint16_t>(request["tx_expire_duration"].asUInt()));
    set_fire_timestamp(request["send_timestamp"].asUInt64());
    set_random_nonce(request["tx_random_nonce"].asUInt());
    set_premium_price(request["premium_price"].asUInt());
    set_last_nonce(request["last_tx_nonce"].asUInt64());
    std::string last_trans_hash = request["last_tx_hash"].asString();
    set_last_hash(hex_to_uint64(last_trans_hash));
    set_challenge_proof(request["challenge_proof"].asString());
    set_memo(request["note"].asString());

    const auto & from = request["sender_action"]["tx_sender_account_addr"].asString();
    const auto & to = request["receiver_action"]["tx_receiver_account_addr"].asString();
    m_source_action.set_action_hash(request["sender_action"]["action_hash"].asUInt());
    m_source_action.set_action_type(static_cast<enum_xaction_type>(request["sender_action"]["action_type"].asUInt()));
    m_source_action.set_action_size(static_cast<uint16_t>(request["sender_action"]["action_size"].asUInt()));
    m_source_action.set_account_addr(from);
    m_source_action.set_action_name(request["sender_action"]["action_name"].asString());
    auto source_param_vec = hex_to_uint(request["sender_action"]["action_param"].asString());
    std::string source_param((char *)source_param_vec.data(), source_param_vec.size());
    m_source_action.set_action_param(std::move(source_param));
    auto source_ext_vec = hex_to_uint(request["sender_action"]["action_ext"].asString());
    std::string source_ext((char *)source_ext_vec.data(), source_ext_vec.size());
    m_source_action.set_action_ext(std::move(source_ext));

    m_source_action.set_action_authorization(request["sender_action"]["action_authorization"].asString());

    m_target_action.set_action_hash(request["receiver_action"]["action_hash"].asUInt());
    m_target_action.set_action_type(static_cast<enum_xaction_type>(request["receiver_action"]["action_type"].asUInt()));
    m_target_action.set_action_size(static_cast<uint16_t>(request["receiver_action"]["action_size"].asUInt()));
    m_target_action.set_account_addr(to);
    m_target_action.set_action_name(request["receiver_action"]["action_name"].asString());
    auto target_param_vec = hex_to_uint(request["receiver_action"]["action_param"].asString());
    std::string target_param((char *)target_param_vec.data(), target_param_vec.size());
    m_target_action.set_action_param(std::move(target_param));
    auto target_ext_vec = hex_to_uint(request["receiver_action"]["action_ext"].asString());
    std::string target_ext((char *)target_ext_vec.data(), target_ext_vec.size());
    m_target_action.set_action_ext(std::move(target_ext));

    m_target_action.set_action_authorization(request["receiver_action"]["action_authorization"].asString());

    auto ext_vec = hex_to_uint(request["ext"].asString());
    std::string ext((char *)ext_vec.data(), ext_vec.size());
    set_ext(ext);
    set_digest(std::move(hex_to_uint256(request["tx_hash"].asString())));

    auto signature = hex_to_uint(request["authorization"].asString());
    std::string signature_str((char *)signature.data(), signature.size());  // hex_to_uint client send xstream_t data 0xaaaa => string
    set_authorization(std::move(signature_str));
    
    set_len();
}

// should be deleted in the end
int32_t xtransaction_v1_t::parse(enum_xaction_type source_type, enum_xaction_type target_type, xtx_parse_data_t & tx_parse_data) {
    if (source_type == xaction_type_source_null) {
        data::xaction_source_null source_action;
        int32_t ret = source_action.parse(m_source_action);
        if (ret != xsuccess) {
            return ret;
        }
    }

    if (source_type == xaction_type_asset_out) {
        data::xaction_asset_out source_action;
        int32_t ret = source_action.parse(m_source_action);
        if (ret != xsuccess) {
            // should return error, but old topio won't be able to execute run_contract tx, because of source_action_param mistakenly set to ""
            if (get_tx_type() == xtransaction_type_run_contract) {
                xdbg("xtransaction_v1_t::parse empty source action param:%s, tx_hash:%s", get_source_action_para().c_str(), get_digest_hex_str().c_str());
            } else {
                return ret;
            }
        }
        tx_parse_data.m_asset = source_action.m_asset_out;
    }

#ifdef ENABLE_CREATE_USER  // debug use
    if (target_type == xaction_type_create_user_account) {
        data::xaction_create_user_account target_action;
        int32_t ret = target_action.parse(m_target_action);
        if (ret != xsuccess) {
            return ret;
        }
        tx_parse_data.m_new_account = target_action.m_address;
    }
#endif

    if (target_type == xaction_type_asset_in) {
        data::xaction_asset_in target_action;
        int32_t ret = target_action.parse(m_target_action);
        if (ret != xsuccess) {
            return ret;
        }
        if ((source_type == xaction_type_asset_out) && (target_action.m_asset.m_amount != tx_parse_data.m_asset.m_amount)) {
            return xverifier::xverifier_error::xverifier_error_trnsafer_tx_src_dst_amount_not_same;
        }
        tx_parse_data.m_asset = target_action.m_asset;
    }

    if (target_type == xaction_type_run_contract) {
        data::xaction_run_contract target_action;
        int32_t ret = target_action.parse(m_target_action);
        if (ret != xsuccess) {
            return ret;
        }
        tx_parse_data.m_function_name = target_action.m_function_name;
        tx_parse_data.m_function_para = target_action.m_para;
    }

    if (target_type == xaction_type_pledge_token) {
        data::xaction_pledge_token target_action;
        int32_t ret = target_action.parse(m_target_action);
        if (ret != xsuccess) {
            return ret;
        }
        tx_parse_data.m_asset = target_action.m_asset;
    }

    if (target_type == xaction_type_redeem_token) {
        data::xaction_redeem_token target_action;
        int32_t ret = target_action.parse(m_target_action);
        if (ret != xsuccess) {
            return ret;
        }
        tx_parse_data.m_asset = target_action.m_asset;
    }

    if (target_type == xaction_type_pledge_token_vote) {
        data::xaction_pledge_token_vote target_action;
        int32_t ret = target_action.parse(m_target_action);
        if (ret != xsuccess) {
            return ret;
        }
        tx_parse_data.m_vote_num = target_action.m_vote_num;
        tx_parse_data.m_lock_duration = target_action.m_lock_duration;
    }

    if (target_type == xaction_type_redeem_token_vote) {
        data::xaction_redeem_token_vote target_action;
        int32_t ret = target_action.parse(m_target_action);
        if (ret != xsuccess) {
            return ret;
        }
        tx_parse_data.m_vote_num = target_action.m_vote_num;
    }

    return 0;
}

}  // namespace data
}  // namespace top
