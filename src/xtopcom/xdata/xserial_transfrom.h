#pragma once

#include <stdio.h>
#include <iostream>
#include <string>
#include "xevm_common/rlp.h"
#include "xevm_common/address.h"
#include "xevm_common/common.h"
#include "xevm_common/fixed_hash.h"
#include "json/json.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "trezor-crypto/sha3.h"
#include "xcrypto/xckey.h"
#include "xutility/xhash.h"
#include "xbase/xutl.h"
#include <secp256k1/secp256k1.h>
#include <secp256k1/secp256k1_recovery.h>

using namespace std;
using namespace top::evm_common;
using namespace top::evm_common::rlp;
using h520 = FixedHash<65>;
using Signature = h520;
using h512 = FixedHash<64>;
using Public = h512;

enum Type {
    NullTransaction,   ///< Null transaction.
    ContractCreation,  ///< Transaction to create contracts - receiveAddress() is ignored.
    MessageCall        ///< Transaction to invoke a message call - receiveAddress() is used.
};

enum class CheckTransaction { None, Cheap, Everything };

u256 constexpr Invalid256 = 0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff_cppui256;

enum IncludeSignature {
    WithoutSignature = 0,  ///< Do not include a signature.
    WithSignature = 1,     ///< Do include a signature.
};

struct SignatureStruct {
    SignatureStruct() = default;
    SignatureStruct(Signature const & _s) {
        *(h520 *)this = _s;
    }
    SignatureStruct(h256 const & _r, h256 const & _s, byte _v) : r(_r), s(_s), v(_v) {
    }
    operator Signature() const {
        return *(h520 const *)this;
    }

    /// @returns true if r,s,v values are valid, otherwise false
    bool isValid() const noexcept;

    h256 r;
    h256 s;
    byte v = 0;
};

struct TransactionSkeleton {
    bool creation = false;
    Address from;
    Address to;
    u256 value;
    bytes data;
    u256 nonce = Invalid256;
    u256 gas = Invalid256;
    u256 gasPrice = Invalid256;
    SignatureStruct vrs;
};

bool isZeroSignature(u256 const & _r, u256 const & _s);

namespace top {
namespace data {
class serial_transfrom {
    serial_transfrom() { }
    ~serial_transfrom() { }

public:
    static int eth_to_top(string strEth, string & strTop);

    static int top_to_eth(string strEth, string & strTop);
};
}  // namespace data
}  // namespace top