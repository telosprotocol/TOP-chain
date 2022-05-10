#pragma once

#include <string>
#include "xbase/xrefcount.h"
#include "xevm_common/common.h"
#include "xevm_common/fixed_hash.h"

namespace top {
namespace data {

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
    virtual const uint32_t get_token_id() { return 0; }
    virtual const std::string & get_from() { return strNull; }
    virtual const uint8_t get_transaction_type() { return 0; }
    virtual const top::evm_common::u256 get_fire_timestamp() { return 0; }
    virtual const top::evm_common::u256 get_expire_duration() { return 0; }
    virtual const std::string & get_memo() { return strNull; }
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
    top::evm_common::u256 chainid;
    top::evm_common::u256 nonce;
    top::evm_common::u256 max_priority_fee_per_gas;
    top::evm_common::u256 max_fee_per_gas;
    top::evm_common::u256 gas;
    std::string to;
    top::evm_common::u256 value;
    std::string data;
    std::string accesslist;
    uint8_t sub_transaction_version;
    uint8_t transaction_type;
    std::string from;
    uint32_t token_id;
    top::evm_common::u256 fire_timestamp;
    top::evm_common::u256 expire_duration;
    std::string memo;
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
    virtual const uint32_t get_token_id() { return token_id; }
    virtual const std::string & get_from() { return from; }
    virtual const uint8_t get_transaction_type() { return transaction_type; }
    virtual const top::evm_common::u256 get_fire_timestamp() { return fire_timestamp; }
    virtual const top::evm_common::u256 get_expire_duration() { return expire_duration; }
    virtual const std::string & get_memo() { return memo; }
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