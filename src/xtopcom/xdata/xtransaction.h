// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <chrono>
#include <string>
#include <vector>
#include "json/json.h"

#include "xbasic/xcrypto_key.h"
#include "xbase/xobject_ptr.h"
#include "xvledger/xdataobj_base.hpp"
#include "xdata/xaction.h"
#include "xdata/xproperty.h"
#include "xdata/xdatautil.h"
#include "xcommon/xaddress.h"
#include "xmetrics/xmetrics.h"

namespace top { namespace data {

#ifdef DEBUG
#if !defined(ENABLE_CREATE_USER)
#define ENABLE_CREATE_USER
#endif
#endif

enum enum_xtransaction_type {
    xtransaction_type_create_user_account        = 0,    // create user account

    xtransaction_type_run_contract               = 3,    // run contract
    xtransaction_type_transfer                   = 4,    // transfer asset

    xtransaction_type_vote                       = 20,
    xtransaction_type_abolish_vote               = 21,

    xtransaction_type_pledge_token_tgas          = 22,   // pledge token for tgas
    xtransaction_type_redeem_token_tgas          = 23,   // redeem token
    xtransaction_type_pledge_token_vote          = 27,   // pledge token for disk
    xtransaction_type_redeem_token_vote          = 28,   // redeem token

    xtransaction_type_max
};

enum enum_xunit_tx_exec_status : uint8_t {
    enum_xunit_tx_exec_status_success   = 1,
    enum_xunit_tx_exec_status_fail      = 2,
};

class xtx_used_resource_t {
 public:
    explicit xtx_used_resource_t(uint32_t tgas = 0, uint32_t deposit = 0, uint32_t disk = 0, uint64_t service_fee = 0):
    m_used_tgas(tgas), m_used_deposit(deposit), m_used_disk(disk), m_beacon_service_fee(service_fee)
    {}
    uint32_t    m_used_tgas{0};
    uint32_t    m_used_deposit{0};
    uint32_t    m_used_disk{0};
    uint64_t    m_beacon_service_fee{0};
};

class xtx_parse_data_t {
public:
#ifdef ENABLE_CREATE_USER  // debug use
    std::string                 m_new_account;
#endif
    data::xproperty_asset       m_asset{0};
    std::string                 m_function_name;
    std::string                 m_function_para;
    uint64_t                    m_vote_num;
    uint16_t                    m_lock_duration;
};

class xtransaction_t : public xbase_dataunit_t<xtransaction_t, xdata_type_transaction> {
 public:
    static std::string transaction_type_to_string(uint16_t type);
 public:
    xtransaction_t();
 protected:
    ~xtransaction_t() override;
 public:
    virtual int32_t    do_write(base::xstream_t & stream) override;
    virtual int32_t    do_read(base::xstream_t & stream) override;
#ifdef XENABLE_PSTACK //tracking memory
    virtual int32_t add_ref() override;
    virtual int32_t release_ref() override;
#endif

 private:  // not safe for multiple threads
    int32_t do_write_without_hash_signature(base::xstream_t & stream, bool is_write_without_len) const;
    int32_t do_read_without_hash_signature(base::xstream_t & stream);

 public:  // check apis
    bool        transaction_type_check() const;
    bool        unuse_member_check() const;
    bool        transaction_len_check() const;
    bool        digest_check() const;
    bool        sign_check() const;
    bool        pub_key_sign_check(xpublic_key_t const & pub_key) const;
    bool        check_last_trans_hash(const uint256_t & account_last_hash);
    bool        check_last_nonce(uint64_t account_nonce);

 public:  // set apis
    void        adjust_target_address(uint32_t table_id);
    void        set_digest();
    void        set_digest(const uint256_t & digest) {m_transaction_hash = digest;};
    int32_t     set_different_source_target_address(const std::string & src_addr, const std::string & dts_addr);
    int32_t     set_same_source_target_address(const std::string & addr);
    void        set_last_trans_hash_and_nonce(uint256_t last_hash, uint64_t last_nonce);
    void        set_fire_and_expire_time(uint16_t const expire_duration);
    void        set_signature(const std::string & signature);

    void        set_source_action(const xaction_t & action) {m_source_action = action;};
    void        set_target_action(const xaction_t & action) {m_target_action = action;};
    void        set_authorization(const std::string & authorization) {m_authorization = authorization;};
    void        set_len();

    int32_t     make_tx_create_user_account(const std::string & addr);
    int32_t     make_tx_create_contract_account(const data::xproperty_asset & asset_out, uint64_t tgas_limit, const std::string& code);
    int32_t     make_tx_transfer(const data::xproperty_asset & asset);
    int32_t     make_tx_run_contract(const data::xproperty_asset & asset_out, const std::string& function_name, const std::string& para);
    int32_t     make_tx_run_contract(std::string const & function_name, std::string const & param);

 public:  // get apis
    uint256_t           digest()const {return m_transaction_hash; }
    std::string         get_digest_str()const {return std::string(reinterpret_cast<char*>(m_transaction_hash.data()), m_transaction_hash.size());}
    std::string         get_digest_hex_str() const;
    const std::string & get_source_addr()const {return m_source_action.get_account_addr();}
    const std::string & get_target_addr()const {return m_target_addr.empty() ? m_target_action.get_account_addr() : m_target_addr;}
    const std::string & get_origin_target_addr()const {return m_target_action.get_account_addr();}
    uint64_t            get_tx_nonce() const {return get_last_nonce() + 1;}
    size_t              get_serialize_size() const;
    std::string         dump() const override;  // just for debug purpose
    xaction_t &         get_source_action() {return m_source_action;}
    xaction_t &         get_target_action() {return m_target_action;}
    const std::string & get_target_action_name() const {return m_target_action.get_action_name();}
    const std::string & get_authorization() const {return m_authorization;}
    void                parse_to_json(xJson::Value& tx_json) const;
    void                construct_from_json(xJson::Value& tx_json);
    int32_t             parse(enum_xaction_type source_type, enum_xaction_type target_type, xtx_parse_data_t & tx_parse_data);

    // header
 public:
    virtual int32_t    serialize_write(base::xstream_t & stream, bool is_write_without_len) const;
    virtual int32_t    serialize_read(base::xstream_t & stream);

 public:
    void set_tx_type(uint16_t type) {m_transaction_type = type;};
    uint16_t get_tx_type() const {return m_transaction_type;};
    void set_tx_len(uint16_t len) {m_transaction_len = len;};
    uint16_t get_tx_len() const {return m_transaction_len;};
    void set_tx_version(uint32_t version) {m_version = version;};
    uint32_t get_tx_version() const {return m_version;};
    void set_to_ledger_id(uint16_t id) {m_to_ledger_id = id;};
    uint16_t get_to_ledger_id() const {return m_to_ledger_id;};
    void set_from_ledger_id(uint16_t id) {m_from_ledger_id = id;};
    uint16_t get_from_ledger_id() const {return m_from_ledger_id;};
    void set_deposit(uint32_t deposit) {m_deposit = deposit;};
    uint32_t get_deposit() const {return m_deposit;};
    void set_expire_duration(uint16_t duration) {m_expire_duration = duration;};
    uint16_t get_expire_duration() const {return m_expire_duration;};
    void set_fire_timestamp(uint64_t timestamp) {m_fire_timestamp = timestamp;};
    uint64_t get_fire_timestamp() const {return m_fire_timestamp;};
    void set_random_nonce(uint32_t random_nonce) {m_trans_random_nonce = random_nonce;};
    uint32_t get_random_nonce() const {return m_trans_random_nonce;};
    void set_premium_price(uint32_t premium_price) {m_premium_price = premium_price;};
    uint32_t get_premium_price() const {return m_premium_price;};
    void set_last_nonce(uint64_t last_nonce) {m_last_trans_nonce = last_nonce;};
    uint64_t get_last_nonce() const {return m_last_trans_nonce;};
    void set_last_hash(uint64_t last_hash) {m_last_trans_hash = last_hash;};
    uint64_t get_last_hash() const {return m_last_trans_hash;};
    void set_challenge_proof(const std::string & challenge_proof) {m_challenge_proof = challenge_proof;};
    const std::string & get_challenge_proof() const {return m_challenge_proof;};
    void set_ext(const std::string & ext) {m_ext = ext;};
    const std::string & get_ext() const {return m_ext;};
    void set_memo(const std::string & memo) {m_memo = memo;};
    const std::string & get_memo() const {return m_memo;};
    void set_target_addr(const std::string & addr) {m_target_addr = addr;};
    const std::string & get_target_address() const {return m_target_addr;};

    // header
private:
    uint16_t          m_transaction_type{0};    // transfer,withdraw,deposit etc
    uint16_t          m_transaction_len{0};     // max 64KB
    uint32_t          m_version{0};
    uint16_t          m_to_ledger_id{0};        // transaction send to target network
    uint16_t          m_from_ledger_id{0};      // where is from transaction
    uint32_t          m_deposit{0};             // tx deposit

    uint16_t          m_expire_duration{0};     // seconds(max 18hour), expired when reach (fire_timestamp + expire_duration)
    uint64_t          m_fire_timestamp{0};      // GMT

    uint32_t          m_trans_random_nonce{0};  // random input
    uint32_t          m_premium_price{0};       // premium of the transaction, reserved for send tx order
    uint64_t          m_last_trans_nonce{0};    // pair with last last_trans_hash, increase by degrees
    uint64_t          m_last_trans_hash{0};     // point to the xxhash64 of the last valid message(e.g. transaction) ,it must be restrictly match as chain ledger
    std::string       m_challenge_proof{};
    std::string       m_ext{};
    std::string       m_memo{};

 private:
    xaction_t         m_source_action;        // source(sender) 'action
    xaction_t         m_target_action;        // target(receiver)'action
    uint256_t         m_transaction_hash{};     // 256 digest for signature as safety
    std::string       m_authorization{};        // signature for whole transaction
    std::string       m_edge_nodeid{};
    std::string       m_target_addr{};

 private:  // local member should not serialize
    mutable std::string m_transaction_hash_str{};
};

using xtransaction_ptr_t = xobject_ptr_t<xtransaction_t>;

}  // namespace data
}  // namespace top
