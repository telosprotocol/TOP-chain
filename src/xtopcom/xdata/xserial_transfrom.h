#pragma once

#include <string>
#include "xbase/xrefcount.h"
#include "xevm_common/common.h"
#include "xevm_common/fixed_hash.h"

namespace top {
namespace data {

enum enum_ethtx_version {
    EIP_LEGACY = 0,
    EIP_2930 = 1,
    EIP_1559 = 2,
    EIP_TOP_V3 = 121
};

static std::string strNull = "";

class eip_xxxx_tx : virtual public top::base::xrefcount_t {
public:
    virtual const top::evm_common::u256 get_chainid() { return 0; }
    virtual const top::evm_common::u256 get_nonce() = 0;
    virtual const top::evm_common::u256 get_gasprice() { return 0;}
    virtual const top::evm_common::u256 get_max_priority_fee_per_gas() { return 0; }
    virtual const top::evm_common::u256 get_max_fee_per_gas() { return 0; }
    virtual const top::evm_common::u256 get_gas() = 0;
    virtual const std::string & get_to() = 0;
    virtual const top::evm_common::u256 get_value() = 0;
    virtual const std::string & get_data() = 0;
    virtual const std::string & get_accesslist() { return strNull; }
    virtual const std::string & get_token_name() { return strNull; }
    virtual const top::evm_common::u256 get_from_address_type() { return 0; }
    virtual const top::evm_common::u256 get_to_address_type() { return 0; }
    virtual const top::evm_common::u256 get_fire_timestamp() { return 0; }
    virtual const top::evm_common::u256 get_expire_duration() { return 0; }
    virtual const std::string & get_memo() { return strNull; }
    virtual const std::string & get_extend() { return strNull; }
    virtual const top::evm_common::u256 get_signV() = 0;
    virtual const top::evm_common::h256 get_signR() = 0;
    virtual const top::evm_common::h256 get_signS() = 0;
};

class eip_legacy_tx : public eip_xxxx_tx {
public:
    top::evm_common::u256 nonce;
    top::evm_common::u256 gasprice;
    top::evm_common::u256 gas;
    std::string to;
    top::evm_common::u256 value;
    std::string data;
    top::evm_common::u256 signV;
    top::evm_common::h256 signR;
    top::evm_common::h256 signS;
public:
    virtual const top::evm_common::u256 get_nonce() { return nonce; }
    virtual const top::evm_common::u256 get_gasprice() { return gasprice; }
    virtual const top::evm_common::u256 get_max_priority_fee_per_gas() { return gasprice; }
    virtual const top::evm_common::u256 get_max_fee_per_gas() { return gasprice; }
    virtual const top::evm_common::u256 get_gas() { return gas; };
    virtual const std::string & get_to() { return to; }
    virtual const top::evm_common::u256 get_value() { return value; }
    virtual const std::string & get_data() { return data; }
    virtual const top::evm_common::u256 get_signV() { return signV; }
    virtual const top::evm_common::h256 get_signR() { return signR; }
    virtual const top::evm_common::h256 get_signS() { return signS; }
};

class eip_1559_tx : public eip_xxxx_tx {
public:
    top::evm_common::u256 chainid;
    top::evm_common::u256 nonce;
    top::evm_common::u256 max_priority_fee_per_gas;
    top::evm_common::u256 max_fee_per_gas;
    top::evm_common::u256 gas;
    std::string to;
    top::evm_common::u256 value;
    std::string data;
    std::string accesslist;
    top::evm_common::u256 signV;
    top::evm_common::h256 signR;
    top::evm_common::h256 signS;

public:
    virtual const top::evm_common::u256 get_chainid() { return chainid; }
    virtual const top::evm_common::u256 get_nonce() { return nonce; }
    virtual const top::evm_common::u256 get_max_priority_fee_per_gas() { return max_priority_fee_per_gas; }
    virtual const top::evm_common::u256 get_max_fee_per_gas() { return max_fee_per_gas; }
    virtual const top::evm_common::u256 get_gas() { return gas; }
    virtual const std::string & get_to() { return to; }
    virtual const top::evm_common::u256 get_value() { return value; }
    virtual const std::string & get_data() { return data; }
    virtual const std::string & get_accesslist() { return accesslist; }
    virtual const top::evm_common::u256 get_signV() { return signV; }
    virtual const top::evm_common::h256 get_signR() { return signR; }
    virtual const top::evm_common::h256 get_signS() { return signS; }
};

class eip_top_v3_tx : public eip_xxxx_tx {
public:
    uint8_t sub_transaction_version;
    top::evm_common::u256 chainid;
    top::evm_common::u256 nonce;
    top::evm_common::u256 max_priority_fee_per_gas;
    top::evm_common::u256 max_fee_per_gas;
    top::evm_common::u256 gas;
    std::string to;
    top::evm_common::u256 value;
    std::string data;
    std::string accesslist;
    std::string token_name;
    top::evm_common::u256 from_address_type;
    top::evm_common::u256 to_address_type;
    top::evm_common::u256 fire_timestamp;
    top::evm_common::u256 expire_duration;
    std::string memo;
    std::string extend;
    top::evm_common::u256 signV;
    top::evm_common::h256 signR;
    top::evm_common::h256 signS;

public:
    virtual const top::evm_common::u256 get_chainid() { return chainid; }
    virtual const top::evm_common::u256 get_nonce() { return nonce; }
    virtual const top::evm_common::u256 get_max_priority_fee_per_gas() { return max_priority_fee_per_gas; }
    virtual const top::evm_common::u256 get_max_fee_per_gas() { return max_fee_per_gas; }
    virtual const top::evm_common::u256 get_gas() { return gas; }
    virtual const std::string & get_to() { return to; }
    virtual const top::evm_common::u256 get_value() { return value; }
    virtual const std::string & get_data() { return data; }
    virtual const std::string & get_accesslist() { return accesslist; }
    virtual const std::string & get_token_name() { return token_name; }
    virtual const top::evm_common::u256 get_from_address_type() { return from_address_type; }
    virtual const top::evm_common::u256 get_to_address_type() { return to_address_type; }
    virtual const top::evm_common::u256 get_fire_timestamp() { return fire_timestamp; }
    virtual const top::evm_common::u256 get_expire_duration() { return expire_duration; }
    virtual const std::string & get_memo() { return memo; }
    virtual const std::string & get_extend() { return extend; }
    virtual const top::evm_common::u256 get_signV() { return signV; }
    virtual const top::evm_common::h256 get_signR() { return signR; }
    virtual const top::evm_common::h256 get_signS() { return signS; }
};

class serial_transfrom {
    serial_transfrom() { }
    ~serial_transfrom() { }

public:
    static int eth_to_top(std::string strEth, uint8_t nEipVersion, std::string & strTop);
    static int top_to_eth(std::string strEth, uint8_t& nEipVersion, std::string & strTop);
};

}  // namespace data
}  // namespace top
