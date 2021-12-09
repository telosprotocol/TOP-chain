// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <chrono>
#include <string>
#include <vector>
#include "json/json.h"

#include "xdata/xtransaction.h"
#include "xdata/xdatautil.h"
#include "xcommon/xaddress.h"
#include "xvledger/xvaccount.h"

namespace top { namespace data {

class xtransaction_v2_t : public xbase_dataunit_t<xtransaction_v2_t, xdata_type_transaction_v2>, public xtransaction_t {
 public:
    xtransaction_v2_t();
 protected:
    ~xtransaction_v2_t() override;
 public:
    virtual int32_t    do_write(base::xstream_t & stream) override;
    virtual int32_t    do_read(base::xstream_t & stream) override;
    virtual int32_t    serialize_to(base::xstream_t & stream) override {return xbase_dataunit_t<xtransaction_v2_t, xdata_type_transaction_v2>::serialize_to(stream);}
    virtual int32_t    serialize_from(base::xstream_t & stream) override {return xbase_dataunit_t<xtransaction_v2_t, xdata_type_transaction_v2>::serialize_from(stream);}      //serialize header and object,return how many bytes is readed
    virtual int32_t    serialize_to_string(std::string & bin_data) override {return xbase_dataunit_t<xtransaction_v2_t, xdata_type_transaction_v2>::serialize_to_string(bin_data);}
    virtual int32_t    serialize_from_string(const std::string & bin_data) override {return xbase_dataunit_t<xtransaction_v2_t, xdata_type_transaction_v2>::serialize_from_string(bin_data);}

#ifdef XENABLE_PSTACK //tracking memory
    virtual int32_t add_ref() override;
    virtual int32_t release_ref() override;
#endif

 private:  // not safe for multiple threads
    int32_t do_write_without_hash_signature(base::xstream_t & stream) const;
    int32_t do_uncompact_write_without_hash_signature(base::xstream_t & stream) const;
    int32_t do_read_without_hash_signature(base::xstream_t & stream);
    
 public:  // check apis
    virtual bool        unuse_member_check() const override {return true;};
    virtual bool        transaction_len_check() const override ;
    virtual bool        digest_check() const override;
    virtual bool        sign_check() const override;
    virtual bool        pub_key_sign_check(xpublic_key_t const & pub_key) const override;
    virtual bool        check_last_trans_hash(const uint256_t & account_last_hash) override {return true;};
    virtual bool        check_last_nonce(uint64_t account_nonce) override;

 public:  // set apis
    virtual void        adjust_target_address(uint32_t table_id) override ;
    virtual void        set_digest() override;
    virtual void        set_digest(const uint256_t & digest) override {m_transaction_hash = digest;};
    virtual int32_t     set_different_source_target_address(const std::string & src_addr, const std::string & dts_addr) override;
    virtual int32_t     set_same_source_target_address(const std::string & addr) override;
    virtual void        set_last_trans_hash_and_nonce(uint256_t last_hash, uint64_t last_nonce) override;
    virtual void        set_fire_and_expire_time(uint16_t const expire_duration) override;

    void                set_source(const std::string & addr, const std::string & action_name, const std::string & para);
    void                set_target(const std::string & addr, const std::string & action_name, const std::string & para);
    virtual void        set_source_addr(const std::string & addr) override { m_source_addr = addr; }
    virtual void        set_source_action_type(const enum_xaction_type type) {m_source_action_type = type;}
    virtual void        set_source_action_name(const std::string & name) {m_source_action_name = name;}
    virtual void        set_source_action_para(const std::string & para) {m_source_action_para = para;}
    virtual void        set_target_addr(const std::string & addr) override { m_target_addr = addr; }
    virtual void        set_target_action_type(const enum_xaction_type type) {m_target_action_type = type;}
    virtual void        set_target_action_name(const std::string & name) {m_target_action_name = name;}
    virtual void        set_target_action_para(const std::string & para) {m_target_action_para = para;}
    virtual void        set_authorization(const std::string & authorization) override {m_authorization = authorization;}
    virtual void        set_len() override;

    virtual int32_t     make_tx_create_user_account(const std::string & addr) override;
    virtual int32_t     make_tx_transfer(const data::xproperty_asset & asset) override;
    virtual int32_t     make_tx_run_contract(const data::xproperty_asset & asset_out, const std::string& function_name, const std::string& para) override;
    virtual int32_t     make_tx_run_contract(std::string const & function_name, std::string const & param) override;
    // tx construction by input parameters, except transfer!
    virtual void        construct_tx(enum_xtransaction_type tx_type, const uint16_t expire_duration, const uint32_t deposit, const uint32_t nonce, const std::string & memo, const xtx_action_info & info) override ;

 public:  // get apis
    virtual uint256_t           digest()const override {return m_transaction_hash; }
    virtual std::string         get_digest_str()const override {return std::string(reinterpret_cast<char*>(m_transaction_hash.data()), m_transaction_hash.size());}
    virtual std::string         get_digest_hex_str() const override;
    virtual const std::string & get_source_addr()const override {return m_source_addr;}
    virtual const std::string & get_target_addr()const override {return m_adjust_target_addr.empty() ? m_target_addr : m_adjust_target_addr;}
    virtual const std::string & get_origin_target_addr()const override {return m_target_addr;}
    virtual uint64_t            get_tx_nonce() const override {return get_last_nonce() + 1;}
    virtual size_t              get_serialize_size() const override;
    virtual std::string         dump() const override;  // just for debug purpose
    void set_action_type();
    virtual const std::string & get_source_action_name() const override {return m_source_action_name;}
    virtual const std::string & get_source_action_para() const override {return m_source_action_para;}
    virtual enum_xaction_type get_source_action_type() const {return m_source_action_type;}
    virtual std::string get_source_action_str() const;
    virtual const std::string & get_target_action_name() const override {return m_target_action_name;}
    virtual const std::string & get_target_action_para() const override {return m_target_action_para;}
    virtual enum_xaction_type get_target_action_type() const {return m_target_action_type;}
    virtual std::string get_target_action_str() const;
    virtual const std::string & get_authorization() const override {return m_authorization;}
    virtual void                parse_to_json(xJson::Value& tx_json, const std::string & version = RPC_VERSION_V2) const override;
    virtual void                construct_from_json(xJson::Value& tx_json) override;
    virtual int32_t             parse(enum_xaction_type source_type, enum_xaction_type target_type, xtx_parse_data_t & tx_parse_data) override;

    // header
 public:
    virtual int32_t    serialize_write(base::xstream_t & stream, bool is_write_without_len) const override {return 0;};
    virtual int32_t    serialize_read(base::xstream_t & stream) override {return 0;};

 public:
    virtual void set_tx_type(uint16_t type) override {m_transaction_type = static_cast<enum_xtransaction_type>(type); set_action_type();}
    virtual uint16_t get_tx_type() const override {return m_transaction_type;};
    virtual void set_tx_len(uint16_t len) override {m_transaction_len = len;};
    virtual uint16_t get_tx_len() const override {return m_transaction_len;};
    virtual void set_tx_version(uint32_t version) override {}
    virtual uint32_t get_tx_version() const override {return xtransaction_version_2;}
    virtual void set_deposit(uint32_t deposit) override {m_deposit = deposit;};
    virtual uint32_t get_deposit() const override {return m_deposit;};
    virtual void set_expire_duration(uint16_t duration) override {m_expire_duration = duration;};
    virtual uint16_t get_expire_duration() const override {return m_expire_duration;};
    virtual void set_fire_timestamp(uint64_t timestamp) override {m_fire_timestamp = timestamp;};
    virtual uint64_t get_fire_timestamp() const override {return m_fire_timestamp;};
    virtual void set_amount(uint64_t amount) override{ m_amount = amount; } 
    virtual void set_premium_price(uint32_t premium_price) override {m_premium_price = premium_price;};
    virtual uint32_t get_premium_price() const override {return m_premium_price;};
    virtual void set_last_nonce(uint64_t last_nonce) override {m_last_trans_nonce = last_nonce;};
    virtual uint64_t get_last_nonce() const override {return m_last_trans_nonce;};
    virtual void set_last_hash(uint64_t last_hash) override {};
    virtual uint64_t get_last_hash() const override {return 0;};
    virtual void set_ext(const std::string & ext) override {m_ext = ext;};
    virtual const std::string & get_ext() const override {return m_ext;};
    virtual void set_memo(const std::string & memo) override {m_memo = memo;};
    virtual const std::string & get_memo() const override {return m_memo;};
    virtual const std::string & get_target_address() const override {return m_target_addr;};

private:
    std::string m_source_addr;
    std::string m_target_addr;
    enum_xtransaction_type m_transaction_type; // one byte
    uint16_t m_expire_duration{0};
    uint32_t m_deposit{0};   //serialize with compat_var
    uint32_t m_premium_price{0};   //serialize with compat_var
    uint64_t m_amount{0};   //serialize with compat_var
    std::string m_token_name;
    uint64_t m_last_trans_nonce{0};   //serialize with compat_var
    uint64_t m_fire_timestamp{0}; //serialize with compat_var
    std::string m_memo;    //serialize with compat_var
    std::string m_ext;   //serialize with compat_var
    std::string m_authorization; //serialize with compat_var
    std::string m_edge_nodeid; //serialize with compat_var   

    // optional,depends on m_tx_type
    std::string m_source_action_name; //serialize with compat_var
    std::string m_source_action_para; //serialize with compat_var
    std::string m_target_action_name; //serialize with compat_var
    std::string m_target_action_para; //serialize with compat_var

 private: // local members, not serialize
    uint256_t m_transaction_hash{};
    mutable uint16_t m_transaction_len{0};     // max 64KB
    mutable std::string m_transaction_hash_str{};
    std::string m_adjust_target_addr{};
    // just reserved for compatibility
    enum_xaction_type m_source_action_type;
    enum_xaction_type m_target_action_type;
};

using xtransaction_v2_ptr_t = xobject_ptr_t<xtransaction_v2_t>;

}  // namespace data
}  // namespace top
