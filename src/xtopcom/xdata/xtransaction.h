// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "xdata/xproperty.h"
#include "xbasic/xhash.hpp"
#include "xbasic/xcrypto_key.h"
#include "xdata/xaction.h"
#include "xbase/xobject_ptr.h"
#include "xvledger/xdataobj_base.hpp"
#include "xbase/xrefcount.h"
#include "xbase/xmem.h"
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

enum enum_xtransaction_version {
    xtransaction_version_1 = 0,
    xtransaction_version_2 = 2
};

enum enum_xunit_tx_exec_status : uint8_t {
    enum_xunit_tx_exec_status_success   = 1,
    enum_xunit_tx_exec_status_fail      = 2,
};

const std::string RPC_VERSION_V1{"1.0"};
const std::string RPC_VERSION_V2{"2.0"};

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

class xtx_action_info {
public:
    xtx_action_info(const std::string & source_addr, const std::string & source_action_name, const std::string & source_action_para, 
                    const std::string & target_addr, const std::string & target_action_name, const std::string & target_action_para)
                  : m_source_addr(source_addr), m_source_action_name(source_action_name), m_source_action_para(source_action_para),
                    m_target_addr(target_addr), m_target_action_name(target_action_name), m_target_action_para(target_action_para)
                    {}

    std::string m_source_addr;
    std::string m_source_action_name;
    std::string m_source_action_para;
    std::string m_target_addr;
    std::string m_target_action_name;
    std::string m_target_action_para;
};

class xtransaction_t;
using xtransaction_ptr_t = xobject_ptr_t<xtransaction_t>;

class xtransaction_t : virtual public base::xrefcount_t {
 public:
    static std::string transaction_type_to_string(uint16_t type);
    static bool set_tx_by_serialized_data(xtransaction_ptr_t & tx_ptr, const std::string & data);
    static uint64_t get_gmttime_s();
    static std::string tx_exec_status_to_str(uint8_t exec_status);

 public:
    virtual int32_t       serialize_to(base::xstream_t & stream) = 0;        //serialize header and object,return how many bytes is writed
    virtual int32_t       serialize_from(base::xstream_t & stream) = 0;      //serialize header and object,return how many bytes is readed
    virtual int32_t       serialize_to_string(std::string & bin_data) = 0;
    virtual int32_t       serialize_from_string(const std::string & bin_data) = 0;

 public:  // check apis
    bool                transaction_type_check() const;
    virtual bool        unuse_member_check() const = 0;
    virtual bool        transaction_len_check() const = 0;
    virtual bool        digest_check() const = 0;
    virtual bool        sign_check() const = 0;
    virtual bool        pub_key_sign_check(xpublic_key_t const & pub_key) const = 0;
    virtual bool        check_last_trans_hash(const uint256_t & account_last_hash) = 0;
    virtual bool        check_last_nonce(uint64_t account_nonce) = 0;

 public:  // set apis
    virtual void        adjust_target_address(uint32_t table_id) = 0;
    virtual void        set_digest() = 0;
    virtual void        set_digest(const uint256_t & digest) = 0;
    virtual int32_t     set_different_source_target_address(const std::string & src_addr, const std::string & dts_addr) = 0;
    virtual int32_t     set_same_source_target_address(const std::string & addr) = 0;
    virtual void        set_last_trans_hash_and_nonce(uint256_t last_hash, uint64_t last_nonce) = 0;
    virtual void        set_fire_and_expire_time(uint16_t const expire_duration) = 0;
 
    void set_action_type_by_tx_type(const enum_xtransaction_type tx_type);
    virtual void        set_source_addr(const std::string & addr) = 0;
    virtual void        set_source_action_type(const enum_xaction_type type) = 0;
    virtual void        set_source_action_name(const std::string & name) = 0;
    virtual void        set_source_action_para(const std::string & para) = 0;
    virtual void        set_target_addr(const std::string & addr) = 0;
    virtual void        set_target_action_type(const enum_xaction_type type) = 0;
    virtual void        set_target_action_name(const std::string & name) = 0;
    virtual void        set_target_action_para(const std::string & para) = 0;
    virtual void        set_authorization(const std::string & authorization) = 0;
    virtual void        set_len() = 0;
 
    virtual int32_t     make_tx_create_user_account(const std::string & addr) = 0;
    virtual int32_t     make_tx_transfer(const data::xproperty_asset & asset) = 0;
    virtual int32_t     make_tx_run_contract(const data::xproperty_asset & asset_out, const std::string& function_name, const std::string& para) = 0;
    virtual int32_t     make_tx_run_contract(std::string const & function_name, std::string const & param) = 0;
    virtual void        construct_tx(enum_xtransaction_type tx_type, const uint16_t expire_duration, const uint32_t deposit, const uint32_t nonce, const std::string & memo, const xtx_action_info & info) = 0;

 public:  // get apis
    virtual uint256_t           digest() const = 0;
    virtual std::string         get_digest_str()const = 0;
    virtual std::string         get_digest_hex_str() const = 0;
    virtual const std::string & get_source_addr()const = 0;
    virtual const std::string & get_target_addr()const = 0;
    virtual const std::string & get_origin_target_addr()const = 0;
    virtual uint64_t            get_tx_nonce() const = 0;
    virtual size_t              get_serialize_size() const = 0;
    virtual std::string         dump() const = 0;  // just for debug purpose
    virtual const std::string & get_source_action_name() const = 0;
    virtual const std::string & get_source_action_para() const = 0;
    virtual enum_xaction_type get_source_action_type() const = 0;
    virtual std::string get_source_action_str() const = 0;
    virtual const std::string & get_target_action_name() const = 0;
    virtual const std::string & get_target_action_para() const = 0;
    virtual enum_xaction_type get_target_action_type() const = 0;
    virtual std::string get_target_action_str() const = 0;
    virtual const std::string & get_authorization() const = 0;
    virtual void                parse_to_json(xJson::Value& tx_json, const std::string & tx_version = RPC_VERSION_V2) const = 0;
    virtual void                construct_from_json(xJson::Value& tx_json) = 0;
    virtual int32_t             parse(enum_xaction_type source_type, enum_xaction_type target_type, xtx_parse_data_t & tx_parse_data) = 0;

 public: // header
    virtual int32_t    serialize_write(base::xstream_t & stream, bool is_write_without_len) const = 0;
    virtual int32_t    serialize_read(base::xstream_t & stream) = 0;
    
    virtual void set_tx_type(uint16_t type) = 0;
    virtual uint16_t get_tx_type() const = 0;
    virtual void set_tx_len(uint16_t len) = 0;
    virtual uint16_t get_tx_len() const = 0;
    virtual void set_tx_version(uint32_t version) = 0;
    virtual uint32_t get_tx_version() const = 0;
    virtual void set_deposit(uint32_t deposit) = 0;
    virtual uint32_t get_deposit() const = 0;
    virtual void set_expire_duration(uint16_t duration) = 0;
    virtual uint16_t get_expire_duration() const = 0;
    virtual void set_fire_timestamp(uint64_t timestamp) = 0;
    virtual uint64_t get_fire_timestamp() const = 0;
    inline  uint64_t get_delay_from_fire_timestamp(uint64_t now_s) const {return now_s > get_fire_timestamp() ? now_s - get_fire_timestamp() : 0;}
    virtual void set_amount(uint64_t amount) {}
    virtual void set_premium_price(uint32_t premium_price) = 0;
    virtual uint32_t get_premium_price() const = 0;
    virtual void set_last_nonce(uint64_t last_nonce) = 0;
    virtual uint64_t get_last_nonce() const = 0;
    virtual void set_last_hash(uint64_t last_hash) = 0;
    virtual uint64_t get_last_hash() const = 0;
    virtual void set_ext(const std::string & ext) = 0;
    virtual const std::string & get_ext() const = 0;
    virtual void set_memo(const std::string & memo) = 0;
    virtual const std::string & get_memo() const = 0;
    virtual const std::string & get_target_address() const = 0;
};

}  // namespace data
}  // namespace top
