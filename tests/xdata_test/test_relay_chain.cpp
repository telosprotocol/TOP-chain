// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sstream>
#include <gtest/gtest.h>

#include "xdata/xrelay_block_store.h"
#include "xpbase/base/top_utils.h"
#include <trezor-crypto/sha3.h>
#include "xcrypto/xckey.h"
#include "xcrypto/xcrypto_util.h"
#include "xbase/xutl.h"
#include "xbase/xhash.h"
#include "xutility/xhash.h"
#include "xdata/xrelay_block.h"
#include "trezor-crypto/ed25519-donna/ed25519.h"

#include <cinttypes>
#include <fstream>

#if defined(XCXX20)
#    include <secp256k1.h>
#    include <secp256k1_recovery.h>
#else
#    include <secp256k1/secp256k1.h>
#    include <secp256k1/secp256k1_recovery.h>
#endif

#if 0

using namespace top;
using namespace top::base;
using namespace top::data;
using namespace top::evm_common;

class xrelay_test : public testing::Test {
public:
    void SetUp()
    {
    }
    void TearDown()
    {
    }
};

// test api
void printHexHash(std::string hash_name, h256 hash_data)
{
    std::cout << "hash name: " << hash_name << " value : " << hash_data.hex() << std::endl;
}

struct  xrelay_vote_def{
    xrelay_election_node_t election;
    top::utl::xecprikey_t raw_pri_key_obj;
};


std::vector<xrelay_block>    g_block_vector;
std::vector<xrelay_vote_def> g_vote_vector;
std::vector<xrelay_election_node_t> g_elections_vecotr;
std::vector<xrelay_receipt> m_receiptVector;

void relayer_receipts_create(uint32_t receipt_count)
{
    m_receiptVector.clear();
    for (uint32_t i = 0; i < receipt_count; i++) {

        xrelay_receipt receipt;
        receipt.m_status = 0x1;
        receipt.m_gasUsed = u256(50942);

        xrelay_receipt_log log;
        log.m_contract_address = h160 { "0xa82fF9aFd8f496c3d6ac40E2a0F282E47488CFc9" }; // todo

        std::string data = "0x0000000000000000000000000000000000000000000000000000000000003039";
        log.m_data = bytes(data.begin(), data.end());

        std::string address_str = "e71d898e741c743326bf045959221cc39e0718d2";
        std::string strAddress = top::HexDecode(address_str);
        top::evm_common::h2048 bloom;
        char szDigest[32] = { 0 };
        keccak_256((const unsigned char*)strAddress.data(), strAddress.size(), (unsigned char*)szDigest);

        top::evm_common::h256 hash_h256;
        bytesConstRef((const unsigned char*)szDigest, 32).copyTo(hash_h256.ref());
        bloom.shiftBloom<3>(hash_h256);

        std::vector<std::string> topics = { "0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef",
            "0x0000000000000000000000004dce5c8961e283786cb31ad7fc072347227d7ea2",
            "0x000000000000000000000000000000000000000000000000000000000000007b" };
        for (auto& topic : topics) {
            std::string strTopic = top::HexDecode(topic);
            char szDigest[32] = { 0 };
            keccak_256((const unsigned char*)strTopic.data(), strTopic.size(), (unsigned char*)szDigest);
            top::evm_common::h256 hash_h256;
            bytesConstRef((const unsigned char*)szDigest, 32).copyTo(hash_h256.ref());
            bloom.shiftBloom<3>(hash_h256);

            log.m_topics.push_back(hash_h256);
        }
        std::stringstream outstrbloom;
        outstrbloom << bloom;
        std::string bloom_str = outstrbloom.str();

        receipt.m_logsBloom = h2048 { bloom_str };
        receipt.m_logs.push_back(log);

        m_receiptVector.push_back(receipt);
    }
}


void xRelayer_vote_create()
{
    g_vote_vector.clear();
    g_elections_vecotr.clear();
    for (int i = 0; i < 1; i++) {
        xrelay_election_node_t election;
        xrelay_vote_def vote;
        byte publickey[32];

        utl::xecpubkey_t raw_pub_key_obj = vote.raw_pri_key_obj.get_public_key();
  

        bytesConstRef((const unsigned char*)raw_pub_key_obj.data()+1 , 32).copyTo(election.public_key_x.ref());
        bytesConstRef((const unsigned char*)raw_pub_key_obj.data()+33 , 32).copyTo(election.public_key_y.ref());
        election.stake = 1000;
        vote.election = election;
        g_elections_vecotr.emplace_back(election);
        g_vote_vector.emplace_back(vote);
    }
}

void sign_block_hash(xrelay_block &block, h256 block_hash)
{

    for(auto &vote: g_vote_vector){
        
        uint256_t hash_raw{block_hash.data()};

        top::utl::xecdsasig_t sig = vote.raw_pri_key_obj.sign(hash_raw);

        uint16_t network_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
        uint8_t  address_type = (uint8_t) base::enum_vaccount_addr_type_secp256k1_eth_user_account;
        utl::xecpubkey_t pub_key_obj = vote.raw_pri_key_obj.get_public_key();
        std::string m_source_account = pub_key_obj.to_address(address_type, network_id);
        std::cout << "signature. m_source_account  " << m_source_account << std::endl;

        char szOutput[65] = {0};
        std::cout << "signature. block_hash  " << block_hash.hex() << std::endl;
        top::utl::xsecp256k1_t::get_publickey_from_signature(sig, hash_raw, (uint8_t*)szOutput);
        xrelay_signature  signature;
        signature.v = sig.get_recover_id();
        std::cout << "signature.v  " << (unsigned)signature.v << std::endl;
        top::evm_common::bytes r_bytes(sig.get_raw_signature(), sig.get_raw_signature() + 32);
        signature.r = top::evm_common::fromBigEndian<top::evm_common::u256>(r_bytes);
        std::cout << "signature.r  " <<  signature.r << std::endl;
        top::evm_common::bytes s_bytes(sig.get_raw_signature() + 32, sig.get_raw_signature() + 64);
        signature.s = top::evm_common::fromBigEndian<top::evm_common::u256>(s_bytes);
         std::cout << "signature.s  " <<  signature.s << std::endl;
       // block.add_signature(signature);

        std::cout << "public " << std::endl;
        std::cout << "signature. block_hash  x " << vote.election.public_key_x.hex() << std::endl;
         std::cout << "signature. block_hash  y " << vote.election.public_key_y.hex() << std::endl;
    }
}




TEST(xrelay_test, test_relay_chain)
{
    // loop 
    for (int i = 0; i < 10; i++) {
        evm_common::h256  prev_hash{0} ;
        if(g_block_vector.size() > 0){
            auto last_block = g_block_vector.back();
            prev_hash = last_block.get_block_hash();
        }
        evm_common::u256  chain_bits = i;
        uint64_t table_height = i;
        uint64_t block_height = i;
        uint64_t epochID = i;
        uint64_t  timestamp = top::GetCurrentTimeMsec();
      
        xrelay_block block(0, prev_hash, chain_bits, table_height, block_height, epochID, timestamp);
        if((i % 5) == 0) {
             xRelayer_vote_create();
             block.set_elections_next(g_elections_vecotr);
        }else {
            relayer_receipts_create(5);
            block.set_receipts(m_receiptVector);
        }
       
        block.build_finish();
        h256 block_hash = block.get_block_hash();
        sign_block_hash(block, block_hash);
        block.save_block_trie();
        evm_common::RLPStream rlp_stream;
        block.streamRLP_header_to_contract(rlp_stream);

        std::cout << " inner hash " << block.get_inner_header_hash().hex().c_str() << std::endl;
        std::cout << " block hash " << block.get_block_hash().hex().c_str() << std::endl;

        std::string fileName = "block_index_" + top::base::xstring_utl::tostring(i) + ".bin";
        std::ofstream fin(fileName, std::ios::binary);
        for (auto c :  rlp_stream.out()) {
            fin.write((char*)&c, sizeof(uint8_t));
        }
        fin.close();
    }
    
}
#endif 
