#include "gtest/gtest.h"
#include "xdata/xrelay_block_store.h"
#include "xbase/xmem.h"
#include "xcommon/xeth_address.h"
#include "xdata/xethtransaction.h"
#include "xpbase/base/top_utils.h"
#include "xcrypto/xcrypto_util.h"
#include "xbase/xutl.h"
#include "xcertauth/xcertauth_face.h"
#include <cinttypes>
#include "xdata/xblocktool.h"
#include "xdata/xblockbuild.h"


using namespace top;
using namespace top::base;
using namespace top::data;
using namespace top::evm_common;
using namespace top::common;
#if 0

class test_relay_block_store : public testing::Test {
protected:
    void SetUp() override {

    }

    void TearDown() override {
    }
};



//test data
const std::string  test_signature {"1BETTgEv6HFFtxTVCQZBioXc5M2oXb5iPQgoO6qlXlPEzTPK4D2yuz4pAfQqfxwAC"};
const std::string  test_address   {"0xbc9b5f068bc20a5b12030fcb72975d8bddc4e84c"};
const std::string  test_to_address{"0xaaaaaf068bc20a5b12030fcb72975d8bddc4e84c"};
const h256 test_topics1{"4f89ece0f576ba3986204ba19a44d94601604b97cf3baa922b010a758d303842"};
const h256 test_topics2{"000000000000000000000000e22c0e020c99e9aed339618fdcea2871d678ef38"};
const h256 test_topics3{"000000000000000000000000f3b23b373dc8854cc2936f4ab4b8e782011ccf87"};
const h256 test_topics4{"000000000000000000000000f39fd6e51aad88f6f4ce6ab8827279cfffb92266"};

const uint64_t     test_gasUsed{0xccde};
const h2048    test_logsBloom{"00000001000000004000000000000000000000000000000000000000000000000000000000041000000000000000008000000000000080000000000000200000000000000000000000000008000000000000000000008000000000000000000010000000020000000004000100000800000000040000000000000012000000000000000020000000008000000000000000000000000000000000000000000000420000000000000000000000000000000000000000080000000000000000000000000002000000200000000000000000000008002000000000000000000020000010000200000000000000000000000000000000000000000000002000000000"};
const h256     test_public_key_x{"b72d55c76bd8f477f4b251763c33f75e6f5f5dd8af071e711e0cb9b2accc70ea"};
const h256     test_public_key_y{"b72d55c76bd8f477f4b251763c33f75e6f5f5dd8af071e711e0cb9b2accc70ea"};
//const uint64_t  test_stake = 0x12345678;
const h256    test_r{"4f89ece0f576ba39123456789123456781604b97cf3baa922b010a758d303842"} ;
const h256    test_s{"4f812345678abcdef1234ba19a44d94601604b97cf3baa922b010a758d303842"} ;
const byte    test_v = 0x1;
const uint8_t            test_version = 0;
const h256    test_inner_hash{"5e173f6ac3c669587538e7727cf19b782a4f2fda07c1eaa662c593e5e85e3051"}; 
const uint64_t            test_height = 123;
const uint64_t            test_epochID = 456;
const uint64_t            test_timestamp = 13579;
const uint8_t             test_poly_flag = 1;
const uint64_t            test_poly_height = 234;
const uint64_t            test_chain_id = 0x1212;
const uint64_t            test_chain_prev_height = 0x100;
const h256    test_elections_hash {"19c2185f4f40634926ebed3af09070ca9e029f2edd5fae6253074896205f5f6c"};
const h256    test_txs_merkle_root{"c45f950382d542169ea207959ee0220ec1491755abe405cd7498d6b16adb6df8"};
const h256    test_receipts_merkle_root{"d25688cf0ab10afa1a0e2dba7853ed5f1e5bf1c631757ed4e103b593ff3f5620"};
const h256    test_state_merkle_root{"f37ec61d84cea03dcc5e8385db93248584e8af4b4d1c832d8c7453c0089687a7"};
const h256    test_block_merkle_root{"e3f407f83fc012470c26a93fdff534100f2c6f736439ce0ca90e9914f7d1c381"};
const h256    test_prev_hash        {"cda1f407f83fc012470c26a93fdff534100f2c6f736439ce0ca9acbde1234567"};
const h256    test_block_hash       {"1234acdeacbfc012470c26a93fdff534100f2c6f736439ce0ca9acbde1234123"};
const u256    test_value{0x45567};
const u256    test_value_gas{0x12346};
const uint64_t test_table_height = 999;

#define  TEST_TOPICS_NUM        (4)
#define  TEST_RECEIPT_LOG_NUM   (2)
#define  TEST_SIGNATURE_NUM     (4)
#define  TEST_ELECTIONS_NUM     (4)
#define  TEST_RECEIPT_NUM       (2)


xevm_log_t  next_evm_log_create() 
{
    xeth_address_t address = xtop_eth_address::build_from(test_address);
    xh256s_t     topics;
    topics.push_back(test_topics1);
    topics.push_back(test_topics2);
    topics.push_back(test_topics3);
    topics.push_back(test_topics4);
    std::string test_str = "000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000a4ba11f3f36b12c71f2aef775583b306a3cf784a";
    std::string log_data = top::HexDecode(test_str);
    xbytes_t  data = bytes(log_data.begin(), log_data.end());

    xevm_log_t _evm_log(address, topics, data); 
    return _evm_log;
}

xeth_receipt_t next_receipt_create()
{
    xeth_receipt_t _receipt;
    xeth_address_t address = xtop_eth_address::build_from(test_address);
    evm_common::xevm_logs_t  logs;
    for (int i = 0; i < TEST_RECEIPT_LOG_NUM; i++) {
        xevm_log_t log = next_evm_log_create();
        logs.emplace_back(log);
    }
    _receipt.set_tx_status(ethreceipt_status_successful);
    _receipt.set_cumulative_gas_used(test_gasUsed);
    _receipt.set_logs(logs);
    _receipt.create_bloom();

    return _receipt;
}

xeth_transaction_t next_tx_create()
{
    xeth_address_t from_address = xtop_eth_address::build_from(test_address);
    xeth_address_t to_address = xtop_eth_address::build_from(test_to_address);
    std::string test_str = "000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000a4ba11f3f36b12c71f2aef775583b306a3cf784a";
    std::string log_data = top::HexDecode(test_str);
    xbytes_t  data = bytes(log_data.begin(), log_data.end());

    xeth_transaction_t tx(from_address, to_address, data, test_value, test_value_gas, test_value_gas);
    return tx;
}

xrelay_election_node_t next_election_create()
{
    xrelay_election_node_t _election;
    _election.public_key_x = test_public_key_x;
    _election.public_key_y = test_public_key_y;
    return _election;
}

xrelay_signature_node_t next_signature_node_t_create(bool exist)
{
    xrelay_signature_node_t _signature_node;
    if (exist) {
       _signature_node.exist = true;
        _signature_node.signature.r = test_r;
        _signature_node.signature.s = test_s;
        _signature_node.signature.v = test_v;
    } else {
        _signature_node.exist = false;
    }
    
    return _signature_node;
}


xrelay_signature_node_t next_signature_node_t_string_create(bool exist)
{
    xrelay_signature_node_t _signature_node;
    if (exist) {
       _signature_node = xrelay_signature_node_t(test_signature);
    } else {
        std::string error_signature = "abcdef";
        _signature_node = xrelay_signature_node_t(error_signature);
    }
    
    return _signature_node;
}

xrelay_block next_block_create(evm_common::h256  prev_hash, uint64_t block_height, uint64_t epochID, uint64_t timestamp,
                     uint64_t chainID, enum_block_cache_type type)
{
    xrelay_block _relay_block(prev_hash, block_height,  timestamp);

    if (type == cache_tx_block) {
        std::vector<xeth_transaction_t>   xeth_tx_vector;
        for(int i = 0; i < TEST_RECEIPT_NUM; i ++) {
            xeth_tx_vector.push_back(next_tx_create());
        }
        _relay_block.set_transactions(xeth_tx_vector);

        std::vector<xeth_receipt_t>   receipts_vector;
        for(int i = 0; i < TEST_RECEIPT_NUM; i ++) {
            receipts_vector.push_back(next_receipt_create());
        }
        _relay_block.set_receipts(receipts_vector);
    }else if(type == cache_poly_election_block) {
            
        xrelay_election_group_t election_set;
        election_set.election_epochID = epochID;
        for (int i = 0; i< TEST_ELECTIONS_NUM; i++) {
            election_set.elections_vector.push_back(next_election_create());
        }
        _relay_block.set_elections_next(election_set);
    }
    
    return _relay_block;
}

#if 0
TEST_F(test_relay_block_store, store_tx_block_save_test) {

    h256 empty_hash{0};
    auto genesis_block = data::xrootblock_t::get_genesis_relay_block();


    h256  prev_hash = genesis_block.get_block_hash();
    uint64_t block_height = genesis_block.get_block_height() + 1;

   // std::cout << "genesis_hash  " << prev_hash  << std::endl;
    for (uint64_t height = 1; height < 100; height++) {
        uint64_t chain_id = 0x1 << (height/2) ;
        uint64_t timestamp =  height*height;
        xrelay_block next_block  = next_block_create(prev_hash, height, chain_id, timestamp, chain_id, cache_tx_block);
        next_block.build_finish();

    //    std::cout << "prev_hash " << prev_hash << " block hash " << next_block.get_block_hash() << std::endl;
        prev_hash = next_block.get_block_hash();
        EXPECT_EQ(next_block.get_block_merkle_root_hash(), empty_hash);
    }

    xrelay_block_store::get_instance().clear_cache();
}


TEST_F(test_relay_block_store, store_tx_block_and_tx_poly_block_test) {

    h256 empty_hash{0};
    auto genesis_block = data::xrootblock_t::get_genesis_relay_block();


    std::vector<h256>  poly_tx_hash_vector;
    h256  prev_hash = genesis_block.get_block_hash();
    uint64_t block_height = genesis_block.get_block_height() + 1;

   // std::cout << "genesis_hash  " << prev_hash  << std::endl;
    for (uint64_t height = 1; height < 101; height++) {
        uint64_t chain_id = 0x1 << (height/2) ;
        uint64_t timestamp =  height*height;
        xrelay_block next_block;
        if ((height % 10) == 0) {
           next_block = next_block_create(prev_hash, height, chain_id, timestamp, chain_id, cache_poly_tx_block);
        } else {
            next_block = next_block_create(prev_hash, height, chain_id, timestamp, chain_id, cache_tx_block);
        }
        
        next_block.build_finish();
      
      //  std::cout << "height  " << height << " prev_hash " << prev_hash << " block hash " << next_block.get_block_hash() << std::endl;
        prev_hash = next_block.get_block_hash();
        if ((height % 10) == 0) {

            h256  block_root_hash{0};
            if (poly_tx_hash_vector.size() > 1) {
                std::vector<bytes> _blocks_hash_poly;
                for (auto &block_hash : poly_tx_hash_vector) {
                    _blocks_hash_poly.push_back(block_hash.to_bytes());
                }
                block_root_hash = orderedTrieRoot(_blocks_hash_poly);
            }
         //   xinfo("tx poly block root 111 hash[%s]", block_root_hash.hex().c_str());
            EXPECT_EQ(next_block.get_block_merkle_root_hash(), block_root_hash);
            poly_tx_hash_vector.clear();
        } else {
            poly_tx_hash_vector.push_back(next_block.get_block_hash());
            EXPECT_EQ(next_block.get_block_merkle_root_hash(), empty_hash);
        }
        
    }
     xrelay_block_store::get_instance().clear_cache();
}


TEST_F(test_relay_block_store, store_tx_block_and_tx_poly_block_and_election_block_test) {

    h256 empty_hash{0};
    auto genesis_block = data::xrootblock_t::get_genesis_relay_block();

    std::vector<h256>  poly_election_hash_vector;
    std::vector<h256>  poly_tx_hash_vector;
    h256  prev_hash = genesis_block.get_block_hash();
    uint64_t block_height = genesis_block.get_block_height() + 1;

   // std::cout << "genesis_hash  " << prev_hash  << std::endl;
    for (uint64_t height = 1; height < 1002; height++) {
        uint64_t chain_id = 0x1 << (height/2) ;
        uint64_t timestamp =  height*height;
        xrelay_block next_block;
        if ((height%100) == 0) {
           next_block = next_block_create(prev_hash, height, chain_id, timestamp, chain_id, cache_poly_election_block);
        } else if ((height % 10) == 0) {
           next_block = next_block_create(prev_hash, height, chain_id, timestamp, chain_id, cache_poly_tx_block);
        } else {
            next_block = next_block_create(prev_hash, height, chain_id, timestamp, chain_id, cache_tx_block);
        }
        
        next_block.build_finish();
      
      //  std::cout << "height  " << height << " prev_hash " << prev_hash << " block hash " << next_block.get_block_hash() << std::endl;
        prev_hash = next_block.get_block_hash();

        if ((height%100) == 0) {
             h256  block_root_hash{0};
            if (poly_election_hash_vector.size() > 1) {
                std::vector<bytes> _blocks_hash_poly;
                for (auto &block_hash : poly_election_hash_vector) {
                    _blocks_hash_poly.push_back(block_hash.to_bytes());
                }
                block_root_hash = orderedTrieRoot(_blocks_hash_poly);
            }
         //   xinfo("tx poly block root 111 hash[%s]", block_root_hash.hex().c_str());
            EXPECT_EQ(next_block.get_block_merkle_root_hash(), block_root_hash);
            poly_tx_hash_vector.clear();
            poly_election_hash_vector.clear();
        } else if ((height % 10) == 0) {

            h256  block_root_hash{0};
            if (poly_tx_hash_vector.size() > 1) {
                std::vector<bytes> _blocks_hash_poly;
                for (auto &block_hash : poly_tx_hash_vector) {
                    _blocks_hash_poly.push_back(block_hash.to_bytes());
                }
                block_root_hash = orderedTrieRoot(_blocks_hash_poly);
            }
         //   xinfo("tx poly block root 111 hash[%s]", block_root_hash.hex().c_str());
            EXPECT_EQ(next_block.get_block_merkle_root_hash(), block_root_hash);
            poly_tx_hash_vector.clear();
            poly_election_hash_vector.push_back(next_block.get_block_hash());
        } else {
            poly_tx_hash_vector.push_back(next_block.get_block_hash());
            poly_election_hash_vector.push_back(next_block.get_block_hash());
            EXPECT_EQ(next_block.get_block_merkle_root_hash(), empty_hash);
        }
        
    }
    xrelay_block_store::get_instance().clear_cache();
}



TEST_F(test_relay_block_store, test_relay_block_store_BENCH) {

    h256 empty_hash{0};
    auto genesis_block = data::xrootblock_t::get_genesis_relay_block();

    uint64_t test_bench[] = {10, 50, 100, 1000, 2000, 2500, 5000, 7500, 10000}; 
    struct timeval beg, end;
    h256  prev_hash = genesis_block.get_block_hash();
    uint64_t block_height = genesis_block.get_block_height() + 1;
    
    for (int test_index; test_index < 9; test_index++) {
        uint64_t test_count = 0;
        uint64_t us_total = 0;
        for (uint64_t height = 1; height < 100001; height++) {
            uint64_t timestamp =  height*height;
            xrelay_block next_block;
            uint64_t chain_id = 0;
            if ((height % test_bench[test_index]) == 0) {
                next_block = next_block_create(prev_hash, height, chain_id, timestamp, chain_id, cache_poly_tx_block);
            } else {
                next_block = next_block_create(prev_hash, height, chain_id, timestamp, chain_id, cache_tx_block);
                EXPECT_EQ(next_block.get_block_merkle_root_hash(), empty_hash);
            }
            
            if ((height % test_bench[test_index]) == 0) {
                test_count++;
                gettimeofday(&beg, NULL);
                next_block.build_finish();
                gettimeofday(&end, NULL);
              //  std::cout << "test  " << test_bench[test_index] << " test_count " << test_count <<std::endl;
                us_total += (end.tv_sec - beg.tv_sec) * 1000000 + (end.tv_usec - beg.tv_usec);

            }else {
                next_block.build_finish();
            }
            prev_hash = next_block.get_block_hash();
        }
        std::cout << "test " << test_bench[test_index] << " " << test_count << " block to build root hash:" << " time(us):" << us_total/test_count << " qps:" << (1.0/(us_total/test_count))*1000000 << std::endl;
    }
    
  
    xrelay_block_store::get_instance().clear_cache();
}


TEST_F(test_relay_block_store, store_tx_block_serialize)
{

    h256 empty_hash{0};
    auto genesis_block = data::xrootblock_t::get_genesis_relay_block();

    xbytes_t rlp_genesis_block_header_data = genesis_block.encodeBytes(false);
    std::string data((char*)rlp_genesis_block_header_data.data(), rlp_genesis_block_header_data.size());

    base::xtableheader_extra_t header_extra_src;
  //  header_extra_src.set_relay_wrap_info("");
    header_extra_src.set_relay_block_data(data);
    std::string extra_data;
    header_extra_src.serialize_to_string(extra_data);

    base::xtableheader_extra_t last_header_extra_new;
    auto ret = last_header_extra_new.deserialize_from_string(extra_data);
    if (ret ) {
       std::cout << "ok " << std::endl;
    }else {
        std::cout << "error " << std::endl;
    }
    

}

#endif 
#endif 