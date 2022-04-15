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

using h520 = top::evm_common::FixedHash<65>;
using Signature = h520;
using h512 = top::evm_common::FixedHash<64>;
using Public = h512;

enum Type {
    NullTransaction,   ///< Null transaction.
    ContractCreation,  ///< Transaction to create contracts - receiveAddress() is ignored.
    MessageCall        ///< Transaction to invoke a message call - receiveAddress() is used.
};

enum class CheckTransaction { None, Cheap, Everything };

enum IncludeSignature {
    WithoutSignature = 0,  ///< Do not include a signature.
    WithSignature = 1,     ///< Do include a signature.
};

namespace top {
namespace data {
class serial_transfrom {
    serial_transfrom() { }
    ~serial_transfrom() { }

public:
    static int eth_to_top(std::string strEth, std::string & strTop);

    static int top_to_eth(std::string strEth, std::string & strTop);
};
}  // namespace data
}  // namespace top