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
#include "xdata/xethtransaction.h"
#include "xcommon/xaddress.h"
#include "xvledger/xvaccount.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"
#include "xevm_common/address.h"
#include "xevm_common/data.h"

namespace top { namespace data {

class xtransaction_v3_t : public xbase_dataunit_t<xtransaction_v3_t, xdata_type_transaction_v3>, public xtransaction_t {
 public:
    xtransaction_v3_t();
    xtransaction_v3_t(xeth_transaction_t const& ethtx);
 protected:
    ~xtransaction_v3_t() override;
 public:
    virtual int32_t    do_write(base::xstream_t & stream) override;
    virtual int32_t    do_read(base::xstream_t & stream) override;
    virtual int32_t    serialize_to(base::xstream_t & stream) override {return xbase_dataunit_t<xtransaction_v3_t, xdata_type_transaction_v3>::serialize_to(stream);}
    virtual int32_t    serialize_from(base::xstream_t & stream) override {return xbase_dataunit_t<xtransaction_v3_t, xdata_type_transaction_v3>::serialize_from(stream);}      //serialize header and object,return how many bytes is readed
    virtual int32_t    serialize_to_string(std::string & bin_data) override {return xbase_dataunit_t<xtransaction_v3_t, xdata_type_transaction_v3>::serialize_to_string(bin_data);}
    virtual int32_t    serialize_from_string(const std::string & bin_data) override {return xbase_dataunit_t<xtransaction_v3_t, xdata_type_transaction_v3>::serialize_from_string(bin_data);}
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
    virtual void        set_digest(const uint256_t & digest) override {};
    virtual int32_t     set_different_source_target_address(const std::string & src_addr, const std::string & dts_addr) override;
    virtual int32_t     set_same_source_target_address(const std::string & addr) override;
    virtual void        set_last_trans_hash_and_nonce(uint256_t last_hash, uint64_t last_nonce) override;
    virtual void        set_fire_and_expire_time(uint16_t const expire_duration) override;

    void                set_source(const std::string & addr, const std::string & action_name, const std::string & para);
    void                set_target(const std::string & addr, const std::string & action_name, const std::string & para);
    virtual void        set_source_addr(const std::string & addr) override { m_source_addr = addr; }
    virtual void        set_source_action_type(const enum_xaction_type type) {}
    virtual void        set_source_action_name(const std::string & name) {}
    virtual void        set_source_action_para(const std::string & para) {}
    virtual void        set_target_addr(const std::string & addr) override { m_target_addr = addr; }
    virtual void        set_target_action_type(const enum_xaction_type type) {}
    virtual void        set_target_action_name(const std::string & name) {}
    virtual void        set_target_action_para(const std::string & para) {}
    virtual void        set_authorization(const std::string & authorization) override { m_authorization = authorization; }
    virtual void        set_len() override;

    virtual int32_t     make_tx_create_user_account(const std::string & addr) override;
    virtual int32_t     make_tx_transfer(const data::xproperty_asset & asset) override;
    virtual int32_t     make_tx_run_contract(const data::xproperty_asset & asset_out, const std::string& function_name, const std::string& para) override;
    virtual int32_t     make_tx_run_contract(std::string const & function_name, std::string const & param) override;
    // tx construction by input parameters, except transfer!
    virtual void        construct_tx(enum_xtransaction_type tx_type, const uint16_t expire_duration, const uint32_t deposit, const uint32_t nonce, const std::string & memo, const xtx_action_info & info) override ;

 public:  // get apis
     virtual uint256_t digest() const override { return m_ethtx.get_tx_hash(); }
    virtual std::string         get_digest_str()const override;
    virtual std::string         get_digest_hex_str() const override;
    virtual const std::string & get_source_addr()const override {return m_source_addr;}
    virtual const std::string & get_target_addr()const override {return m_target_addr;}
    virtual const std::string & get_origin_target_addr()const override {return m_target_addr;}
    virtual uint64_t            get_tx_nonce() const override {return get_last_nonce() + 1;}
    virtual std::string         dump() const override;  // just for debug purpose
    void set_action_type();
    virtual const std::string & get_source_action_name() const override { return strNull; }
    virtual const std::string & get_source_action_para() const override { return strNull; }
    virtual enum_xaction_type get_source_action_type() const {return xaction_type_max;}
    virtual std::string get_source_action_str() const;
    virtual const std::string & get_target_action_name() const override { return strNull; }
    virtual const std::string & get_target_action_para() const override { return strNull; }
    virtual enum_xaction_type get_target_action_type() const {return xaction_type_max;}
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
    virtual uint32_t get_tx_len() const override {return m_transaction_len;};
    virtual void set_tx_version(uint32_t version) override {}
    virtual uint32_t get_tx_version() const override {return xtransaction_version_3;}
    virtual void set_deposit(uint32_t deposit) override {xassert(false);}
    virtual uint32_t get_deposit() const override {return 0;};
    virtual void set_expire_duration(uint16_t duration) override {};
    virtual uint16_t get_expire_duration() const override { return 43200; } // 12 hours for eth tx.
    virtual void set_fire_timestamp(uint64_t timestamp) override {};
    virtual void set_fire_timestamp_ext(uint64_t timestamp) override {if (m_fire_timestamp == 0) {m_fire_timestamp = timestamp;}}
    virtual uint64_t get_fire_timestamp() const override {  return m_fire_timestamp; };
    virtual void set_amount(uint64_t amount) override{}
    virtual uint64_t get_amount() const noexcept override { xassert(false); return 0; }
    virtual top::evm_common::u256 get_amount_256() const noexcept override { return m_ethtx.get_value(); }
    virtual bool is_top_transfer() const noexcept override { return false; }
    virtual void set_premium_price(uint32_t premium_price) override { }
    virtual uint32_t get_premium_price() const override {return 0;}
    virtual void set_last_nonce(uint64_t last_nonce) override {  }
    virtual uint64_t get_last_nonce() const override { return (uint64_t)m_ethtx.get_nonce(); };
    virtual void set_last_hash(uint64_t last_hash) override {};
    virtual uint64_t get_last_hash() const override {return 0;};
    virtual void set_ext(const std::string & ext) override { };
    virtual const std::string & get_ext() const override { return strNull; }
    virtual void set_memo(const std::string & memo) override { };
    virtual const std::string & get_memo() const override { return strNull; }
    virtual bool is_evm_tx() const override {return m_transaction_type != xtransaction_type_transfer;}
    virtual int32_t get_object_size() const override;
public:
    virtual xbytes_t const& get_data() const override { return m_ethtx.get_data(); }
    virtual const top::evm_common::u256 get_gaslimit() const override {return m_ethtx.get_gas(); }
    virtual const top::evm_common::u256 get_max_fee_per_gas() const override { return m_ethtx.get_max_fee_per_gas(); }

    virtual xeth_transaction_t to_eth_tx(std::error_code & ec) const override {return m_ethtx;}

private:
    void    update_cache();
private:
    uint8_t             m_version{0};
    xeth_transaction_t  m_ethtx;

 private:  // TODO(jimmy) refactor local caches 
    std::string     m_source_addr;
    std::string     m_target_addr;
    enum_xtransaction_type m_transaction_type; // one byte
    std::string     m_authorization;  // serialize with compat_var
    mutable uint32_t m_transaction_len{0};     // max 64KB
    uint64_t        m_fire_timestamp{0};
    uint16_t        m_expire_duration{0};
};

using xtransaction_v3_ptr_t = xobject_ptr_t<xtransaction_v3_t>;

}  // namespace data
}  // namespace top
