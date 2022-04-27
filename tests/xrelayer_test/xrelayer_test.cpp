// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sstream>

#include "xrelayer/xrelayer.hpp"
#include <gtest/gtest.h>
#include <cinttypes>
#include "xpbase/base/top_utils.h"
#include <trezor-crypto/sha3.h>
#include <secp256k1/secp256k1.h>
#include <secp256k1/secp256k1_recovery.h>
#include "xcrypto/xckey.h"
#include "xcrypto/xcrypto_util.h"
#include "xbase/xutl.h"

#include "../../src/xtopcom/xdepends/include/trezor-crypto/ed25519-donna/ed25519.h"


using namespace top::evm_common;
using namespace top::xrelayer;

class xrelayer_test : public testing::Test {
public:
    void SetUp() {
        
    }
    void TearDown() {
    
    }
};

std::vector<xRelayerReceipt>  m_receiptVector;


void relayer_receipts_create(uint64_t height, uint32_t receipt_count) {

  struct timeval beg;
  uint64_t rand_time = 0;

  m_receiptVector.clear();
  for (uint32_t i = 0; i < receipt_count; i++) {

    xRelayerReceipt receipt;
    receipt.status = 0x1;
    receipt.gasUsed = h256(50942);
    
    xRelayerReceiptLog log;
    log.contractAddress = h160{"real contract address"};   //todo

    std::string data = "0x0000000000000000000000000000000000000000000000000000000000003039";
    log.data = bytes(data.begin(),data.end());

    std::string address_str = "e71d898e741c743326bf045959221cc39e0718d2";
    std::string strAddress =  top::HexDecode(address_str); 
    top::evm_common::h2048 bloom;
    char szDigest[32] = {0};
    keccak_256((const unsigned char *)strAddress.data(), strAddress.size(), (unsigned char *)szDigest);
    top::evm_common::h256 hash_h256;
    bytesConstRef((const unsigned char *)szDigest, 32).copyTo(hash_h256.ref());
    bloom.shiftBloom<3>(hash_h256);

    std::vector<std::string> topics = {"0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
                                       "0x0000000000000000000000004dce5c8961e283786cb31ad7fc072347227d7ea2",
                                       "0x000000000000000000000000000000000000000000000000000000000000007b"};
    for (auto & topic : topics) {
        std::string strTopic =  top::HexDecode(topic);
        char szDigest[32] = {0};
        keccak_256((const unsigned char *)strTopic.data(), strTopic.size(), (unsigned char *)szDigest);
        top::evm_common::h256 hash_h256;
        bytesConstRef((const unsigned char *)szDigest, 32).copyTo(hash_h256.ref());
        bloom.shiftBloom<3>(hash_h256);

        log.topics.push_back(bytes(topic.begin(), topic.end()));
    }
    std::stringstream outstrbloom;
    outstrbloom << bloom;
    std::string bloom_str = outstrbloom.str();

    receipt.logsBloom = h2048{bloom_str};
    receipt.logs.push_back(log);
    m_receiptVector.push_back(receipt);
  }

}

TEST(xrelayer_test, config) {
    relayer relayer_instance;

    //set proof
    std::vector<xRelayerBlockProducer> block_proofs;
    for (int i = 0; i < 10; i++) {
        xRelayerBlockProducer producers;    
        top::utl::xecprikey_t raw_pri_key_obj;
        producers.stake = 1000;
        byte publickey[32];
        ed25519_publickey(raw_pri_key_test.data(), publickey);
     
        bytesConstRef((const unsigned char *)raw_pri_key_obj.data(), 32).copyTo(producers.publicKey.p_k.ref());
        bytesConstRef((const unsigned char *)publickey, 32).copyTo(producers.publicKey.k.ref());
        block_proofs.push_back(producers);
    }
    relayer_instance.relayer_block_proofs_set(block_proofs);

    //set block proof
  /*  std::vector<xRelayerOptionalSignature>  approve_nexts;
    for (int i = 0; i < block_proofs.size(); i++) {
        xRelayerOptionalSignature approve;    
        top::utl::xecprikey_t raw_pri_key_obj;

        approve.some = 1;
        bytesConstRef((const unsigned char *)raw_pri_key_obj.data(), 32).copyTo(  approve.signature.p_k.ref());
       //approve.signature.p_k = h256{ raw_pri_key_obj.data()};
        approve.signature.r  = h256{raw_pri_key_obj.get_compress_public_key()};
        approve_nexts.emplace_back(approve);
    }
    relayer_instance.relayer_approvals_next_set(approve_nexts);*/

    relayer_receipts_create(1, 1);
    relayer_instance.relayer_receipts_set(m_receiptVector);

    relayer_instance.relayer_new_block_build();

    // block 2
    std::vector<xRelayerBlockProducer> block_proofs;
    for (int i = 0; i < 10; i++) {
        xRelayerBlockProducer producers;    
        top::utl::xecprikey_t raw_pri_key_obj;
        producers.stake = 1000;
        byte publickey[32];
        ed25519_publickey(raw_pri_key_test.data(), publickey);
     
        bytesConstRef((const unsigned char *)raw_pri_key_obj.data(), 32).copyTo(producers.publicKey.p_k.ref());
        bytesConstRef((const unsigned char *)publickey, 32).copyTo(producers.publicKey.k.ref());
        block_proofs.push_back(producers);
    }
    relayer_instance.relayer_block_proofs_set(block_proofs);
    relayer_instance.relayer_receipts_set(m_receiptVector);
    relayer_instance.relayer_new_block_build();
}


#if  0

/*
    std::vector<std:string> all_proof_leafs;
    for (auto & leaf:m_proofs) {
        all_proof_leafs.push_back( tostring(leaf.publicKey.k));
    }

    xmerkle_t<utl::xsha2_256_t, uint256_t> merkle(all_proof_leafs);   out_leafs
    for (size_t leaf_index = 0; leaf_index < all_proof_leafs.size(); leaf_index++) {
        int pos = 0;
        auto const &_leaf = all_proof_leafs[leaf_index];
        base::xmerkle_path_256_t hash_path;
        bool ret =   merkle.calc_path_hash(_leaf, hash_path.get_levels_for_write());
        if (!ret) {
            xerror("  calc merkle path fail");
            assert(false);
        }
        if( hash_path.pos == 2){
            pos  = 1;
        } else {
            pos  = 0;
        }

}
 * 
 * 
 * /TOP/rank-top/tests/xevm_engine_test/test_cases/erc20/erc20.json  
  "src_address": "T600044dce5c8961e283786cb31ad7fc072347227d7ea2",
            "target_address": "contract_one",
            "data": "0xa9059cbb000000000000000000000000000000000000000000000000000000000000007b0000000000000000000000000000000000000000000000000000000000003039",
            "gas_limit": 50942,
            "value": 0,
            "expected": {
                "status": 0,
                "extra_message": "",
                "gas_used": 50942,
                "logs": [
                    {
                        "address": "contract_one",
                        "topics": [
                            "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
                            "0x0000000000000000000000004dce5c8961e283786cb31ad7fc072347227d7ea2",
                            "0x000000000000000000000000000000000000000000000000000000000000007b"
                        ],
                        "data": "0x0000000000000000000000000000000000000000000000000000000000003039"
                    }
                ]
            }
        },
 */

#endif



