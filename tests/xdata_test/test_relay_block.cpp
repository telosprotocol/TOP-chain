#include "gtest/gtest.h"
#include "xdata/xrelay_block.h"
#include "xbase/xmem.h"
#include "xpbase/base/top_utils.h"
#include<fstream>  
using namespace top;
using namespace top::base;
using namespace top::data;
using namespace top::evm_common;

class test_relay_block : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


//test data
const h160 test_address{"009b5f068bc20a5b12030fcb72975d8bddc4e84c"};
const h256 test_topics1{"4f89ece0f576ba3986204ba19a44d94601604b97cf3baa922b010a758d303842"};
const h256 test_topics2{"000000000000000000000000e22c0e020c99e9aed339618fdcea2871d678ef38"};
const h256 test_topics3{"000000000000000000000000f3b23b373dc8854cc2936f4ab4b8e782011ccf87"};
const h256 test_topics4{"000000000000000000000000f39fd6e51aad88f6f4ce6ab8827279cfffb92266"};
const uint8_t  test_status = 1;
const u256     test_gasUsed{0xccde};
const h2048    test_logsBloom{"00000001000000004000000000000000000000000000000000000000000000000000000000041000000000000000008000000000000080000000000000200000000000000000000000000008000000000000000000008000000000000000000010000000020000000004000100000800000000040000000000000012000000000000000020000000008000000000000000000000000000000000000000000000420000000000000000000000000000000000000000080000000000000000000000000002000000200000000000000000000008002000000000000000000020000010000200000000000000000000000000000000000000000000002000000000"};
const h256     test_public_key{"b72d55c76bd8f477f4b251763c33f75e6f5f5dd8af071e711e0cb9b2accc70ea"};
const uint64_t  test_stake = 0x12345678;
const h256    test_r{"4f89ece0f576ba39123456789123456781604b97cf3baa922b010a758d303842"} ;
const h256    test_s{"4f812345678abcdef1234ba19a44d94601604b97cf3baa922b010a758d303842"} ;
const byte    test_v = 0x1;
const uint64_t            test_version = 0;
const h256    test_inner_hash{"5e173f6ac3c669587538e7727cf19b782a4f2fda07c1eaa662c593e5e85e3051"}; 
const uint64_t            test_height = 123;
const uint64_t            test_epochID = 456;
const uint64_t            test_timestamp = 13579;
const h256    test_elections_hash {"19c2185f4f40634926ebed3af09070ca9e029f2edd5fae6253074896205f5f6c"};
const h256    test_txs_merkle_root{"c45f950382d542169ea207959ee0220ec1491755abe405cd7498d6b16adb6df8"};
const h256    test_receipts_merkle_root{"d25688cf0ab10afa1a0e2dba7853ed5f1e5bf1c631757ed4e103b593ff3f5620"};
const h256    test_state_merkle_root{"f37ec61d84cea03dcc5e8385db93248584e8af4b4d1c832d8c7453c0089687a7"};
const h256    test_block_merkle_root{"e3f407f83fc012470c26a93fdff534100f2c6f736439ce0ca90e9914f7d1c381"};
const h256    test_prev_hash        {"cda1f407f83fc012470c26a93fdff534100f2c6f736439ce0ca9acbde1234567"};
const h256    test_block_hash       {"1234acdeacbfc012470c26a93fdff534100f2c6f736439ce0ca9acbde1234123"};
const u256    test_chain_bits{0x45567};
const uint64_t test_table_height = 999;

#define  TEST_TOPICS_NUM        (4)
#define  TEST_RECEIPT_LOG_NUM   (2)
#define  TEST_SIGNATURE_NUM     (4)
#define  TEST_ELECTIONS_NUM     (4)
#define  TEST_RECEIPT_NUM       (2)



xrelay_receipt_log  xrelay_receipt_log_create() 
{
    xrelay_receipt_log _receipt_log;
    std::string test_str = "000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000a4ba11f3f36b12c71f2aef775583b306a3cf784a";
    std::string log_data = top::HexDecode(test_str);
    bytes    test_data =  bytes(log_data.begin(), log_data.end());
    _receipt_log.m_data = test_data;
    _receipt_log.m_contract_address = test_address;
    _receipt_log.m_topics.push_back(test_topics1);
    _receipt_log.m_topics.push_back(test_topics2);
    _receipt_log.m_topics.push_back(test_topics3);
    _receipt_log.m_topics.push_back(test_topics4);
    return _receipt_log;
}

xrelay_receipt xrelay_receipt_create()
{
    xrelay_receipt _receipt;
    _receipt.m_status = test_status;
    _receipt.m_gasUsed = test_gasUsed;
    _receipt.m_logsBloom = test_logsBloom;

    for (int i = 0; i < TEST_RECEIPT_LOG_NUM; i++) {
        xrelay_receipt_log log = xrelay_receipt_log_create();
        _receipt.m_logs.emplace_back(log);
    }
    return _receipt;
}

xrelay_election xrelay_election_create()
{
    xrelay_election _election;
    _election.public_key = test_public_key;
    _election.stake = test_stake;
    return _election;
}

xrelay_signature xrelay_signature_create(unsigned num = 0)
{
    xrelay_signature _signature;
    _signature.r = test_r;
    _signature.s = test_s;
    _signature.v = test_v + num;
    return _signature;
}

xrelay_block_inner_header xrelay_block_inner_header_create()
{
    xrelay_block_inner_header _inner_header;
    _inner_header.m_version = test_version;
 //   _inner_header.m_inner_hash = test_inner_hash;
    _inner_header.m_height = test_height;
    _inner_header.m_epochID = test_epochID;
    _inner_header.m_timestamp = test_timestamp;
    _inner_header.m_elections_hash = test_elections_hash;
    _inner_header.m_txs_merkle_root = test_txs_merkle_root;
    _inner_header.m_receipts_merkle_root = test_receipts_merkle_root;
    _inner_header.m_state_merkle_root = test_state_merkle_root;
    _inner_header.m_block_merkle_root = test_block_merkle_root;
    return _inner_header;
}

xrelay_block_header xrelay_block_header_create()
{
    xrelay_block_header _block_header;
    xrelay_block_inner_header _inner_header = xrelay_block_inner_header_create();
    _block_header.set_inner_header(_inner_header);
    _block_header.set_prev_hash(test_prev_hash);
    _block_header.set_block_hash(test_block_hash);
    _block_header.set_chain_bits(test_chain_bits);
    _block_header.set_table_height(test_table_height);    
    
    std::vector<xrelay_election>    election_vector;
    for (int i = 0; i< TEST_ELECTIONS_NUM; i++) {
        election_vector.push_back(xrelay_election_create());
    }
    _block_header.set_elections_next(election_vector);

    for (int i = 0; i < TEST_SIGNATURE_NUM; i++) {
        _block_header.add_signature(xrelay_signature_create(i));
    }
    return _block_header;
}

xrelay_block xrelay_block_create()
{
    xrelay_block  _relay_block;
    xrelay_block_header _block_header = xrelay_block_header_create();
    _relay_block.set_header(_block_header);

    std::vector<xrelay_receipt>   receipts_vector;
    for(int i = 0; i < TEST_RECEIPT_NUM; i ++) {
        receipts_vector.push_back(xrelay_receipt_create());
    }
    _relay_block.set_receipts(receipts_vector);

    return _relay_block;
}


TEST_F(test_relay_block, serialize_receipt_log) {

    xrelay_receipt_log log_src = xrelay_receipt_log_create();
    RLPStream rlp_log;
    log_src.streamRLP(rlp_log);
    bytes  rlp_bytes = rlp_log.out();

    xrelay_receipt_log log_dst;
    RLP rlp_dst = RLP(rlp_bytes);
    log_dst.decodeRLP(rlp_dst);

    EXPECT_EQ(log_dst.m_data, log_src.m_data);
    EXPECT_EQ(log_dst.m_contract_address, log_src.m_contract_address);
    for(unsigned i = 0; i < TEST_TOPICS_NUM; i++ ) {
        EXPECT_EQ(log_dst.m_topics[i], log_src.m_topics[i]);
    }
}

TEST_F(test_relay_block, serialize_receipt) {

    xrelay_receipt receipt_src = xrelay_receipt_create();
    RLPStream rlp_receipt;
    receipt_src.streamRLP(rlp_receipt);
    bytes  rlp_bytes = rlp_receipt.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_receipt receipt_dst;
    receipt_dst.decodeRLP(rlp_dst);

    EXPECT_EQ(receipt_dst.m_status, receipt_src.m_status);
    EXPECT_EQ(receipt_dst.m_gasUsed, receipt_src.m_gasUsed);
    EXPECT_EQ(receipt_dst.m_logsBloom, receipt_src.m_logsBloom);
    EXPECT_EQ(receipt_src.m_logs.size(), TEST_RECEIPT_LOG_NUM);
    EXPECT_EQ(receipt_dst.m_logs.size(), TEST_RECEIPT_LOG_NUM);
    
    for (unsigned i = 0; i < TEST_RECEIPT_LOG_NUM; i++) {
        xrelay_receipt_log log_src = receipt_src.m_logs[i];
        xrelay_receipt_log log_dst = receipt_dst.m_logs[i];
        EXPECT_EQ(log_dst.m_contract_address, log_src.m_contract_address);
        EXPECT_EQ(log_dst.m_data, log_src.m_data);
        EXPECT_EQ(log_dst.m_topics.size(), TEST_TOPICS_NUM);
        EXPECT_EQ(log_src.m_topics.size(), TEST_TOPICS_NUM);
        for(unsigned i = 0; i< TEST_TOPICS_NUM; i++ ) {
            EXPECT_EQ(log_dst.m_topics[i], log_src.m_topics[i]);
        }        
    }
}

TEST_F(test_relay_block, serialize_xrelay_election) {

    xrelay_election election_src = xrelay_election_create();

    RLPStream rlp_election;
    election_src.streamRLP(rlp_election);
    bytes  rlp_bytes = rlp_election.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_election election_dst;
    election_dst.decodeRLP(rlp_dst);

    EXPECT_EQ(election_dst.public_key, election_src.public_key);
    EXPECT_EQ(election_dst.stake, election_src.stake);
}


TEST_F(test_relay_block, serialize_xrelay_signature) {

    xrelay_signature signature_src = xrelay_signature_create();

    RLPStream rlp_signature;
    signature_src.streamRLP(rlp_signature);
    bytes  rlp_bytes = rlp_signature.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_signature signature_dst;
    signature_dst.decodeRLP(rlp_dst);

    EXPECT_EQ(signature_dst.r, signature_src.r);
    EXPECT_EQ(signature_dst.s, signature_src.s);
    EXPECT_EQ(signature_dst.v, signature_src.v);
}


TEST_F(test_relay_block, serialize_xrelay_inner_header) {

    xrelay_block_inner_header inner_hader_src = xrelay_block_inner_header_create();

    RLPStream rlp_innder_header;
    inner_hader_src.streamRLP(rlp_innder_header);
    bytes  rlp_bytes = rlp_innder_header.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_block_inner_header innder_header_dst;
    innder_header_dst.decodeRLP(rlp_dst);

    EXPECT_EQ(innder_header_dst.m_version, inner_hader_src.m_version);
  //  EXPECT_EQ(innder_header_dst.m_inner_hash, inner_hader_src.m_inner_hash);
    EXPECT_EQ(innder_header_dst.m_height, inner_hader_src.m_height);
    EXPECT_EQ(innder_header_dst.m_epochID, inner_hader_src.m_epochID);
    EXPECT_EQ(innder_header_dst.m_timestamp, inner_hader_src.m_timestamp);
    EXPECT_EQ(innder_header_dst.m_elections_hash, inner_hader_src.m_elections_hash);
    EXPECT_EQ(innder_header_dst.m_txs_merkle_root, inner_hader_src.m_txs_merkle_root);
    EXPECT_EQ(innder_header_dst.m_receipts_merkle_root, inner_hader_src.m_receipts_merkle_root);
    EXPECT_EQ(innder_header_dst.m_state_merkle_root, inner_hader_src.m_state_merkle_root);
    EXPECT_EQ(innder_header_dst.m_block_merkle_root, inner_hader_src.m_block_merkle_root);
   
   std::cout << " innder_header " << toHex(rlp_innder_header.out()) << std::endl;

}


TEST_F(test_relay_block, serialize_xrelay_block_header_without_signature) {

    xrelay_block_header block_header_src = xrelay_block_header_create();

    EXPECT_EQ(block_header_src.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(block_header_src.get_signatures_sets().size(), TEST_SIGNATURE_NUM);

    RLPStream rlp_block_header;
    block_header_src.streamRLP(rlp_block_header);
    bytes  rlp_bytes = rlp_block_header.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_block_header block_header_dst;
    block_header_dst.decodeRLP(rlp_dst);

    xrelay_block_inner_header inner_header_dst = block_header_dst.get_inner_header();
    xrelay_block_inner_header inner_header_src = block_header_src.get_inner_header();

    EXPECT_EQ(inner_header_dst.m_version, inner_header_src.m_version);
  //  EXPECT_EQ(inner_header_dst.m_inner_hash, inner_header_src.m_inner_hash);
    EXPECT_EQ(inner_header_dst.m_height, inner_header_src.m_height);
    EXPECT_EQ(inner_header_dst.m_epochID, inner_header_src.m_epochID);
    EXPECT_EQ(inner_header_dst.m_timestamp, inner_header_src.m_timestamp);
    EXPECT_EQ(inner_header_dst.m_elections_hash, inner_header_src.m_elections_hash);
    EXPECT_EQ(inner_header_dst.m_txs_merkle_root, inner_header_src.m_txs_merkle_root);
    EXPECT_EQ(inner_header_dst.m_receipts_merkle_root, inner_header_src.m_receipts_merkle_root);
    EXPECT_EQ(inner_header_dst.m_state_merkle_root, inner_header_src.m_state_merkle_root);
    EXPECT_EQ(inner_header_dst.m_block_merkle_root, inner_header_src.m_block_merkle_root);

    EXPECT_EQ(block_header_dst.get_prev_block_hash(), block_header_src.get_prev_block_hash());
    EXPECT_EQ(block_header_dst.get_block_hash(), block_header_src.get_block_hash());
    EXPECT_EQ(block_header_dst.get_bchain_bits(), block_header_src.get_bchain_bits());
    EXPECT_EQ(block_header_dst.get_table_height(), block_header_src.get_table_height());

    EXPECT_EQ(block_header_dst.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(block_header_src.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    for (unsigned i = 0; i < TEST_ELECTIONS_NUM; i++) {
        xrelay_election xrelay_election_dst = block_header_dst.get_elections_sets()[i];
        xrelay_election xrelay_election_src = block_header_src.get_elections_sets()[i];
        EXPECT_EQ(xrelay_election_dst.public_key,  xrelay_election_src.public_key);
        EXPECT_EQ(xrelay_election_dst.stake, xrelay_election_src.stake);
    }


    std::cout << " block_header serialize_xrelay_block_header_without_signature  " << toHex(rlp_block_header.out()) << std::endl;
}


TEST_F(test_relay_block, serialize_xrelay_block_header_with_signature) {

    xrelay_block_header block_header_src = xrelay_block_header_create();

    EXPECT_EQ(block_header_src.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(block_header_src.get_signatures_sets().size(), TEST_SIGNATURE_NUM);

    RLPStream rlp_block_header;
    block_header_src.streamRLP(rlp_block_header,true);

    bytes  rlp_bytes = rlp_block_header.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_block_header block_header_dst;
    block_header_dst.decodeRLP(rlp_dst, true);

    xrelay_block_inner_header inner_header_dst = block_header_dst.get_inner_header();
    xrelay_block_inner_header inner_header_src = block_header_src.get_inner_header();

    EXPECT_EQ(inner_header_dst.m_version, inner_header_src.m_version);
   // EXPECT_EQ(inner_header_dst.m_inner_hash, inner_header_src.m_inner_hash);
    EXPECT_EQ(inner_header_dst.m_height, inner_header_src.m_height);
    EXPECT_EQ(inner_header_dst.m_epochID, inner_header_src.m_epochID);
    EXPECT_EQ(inner_header_dst.m_timestamp, inner_header_src.m_timestamp);
    EXPECT_EQ(inner_header_dst.m_elections_hash, inner_header_src.m_elections_hash);
    EXPECT_EQ(inner_header_dst.m_txs_merkle_root, inner_header_src.m_txs_merkle_root);
    EXPECT_EQ(inner_header_dst.m_receipts_merkle_root, inner_header_src.m_receipts_merkle_root);
    EXPECT_EQ(inner_header_dst.m_state_merkle_root, inner_header_src.m_state_merkle_root);
    EXPECT_EQ(inner_header_dst.m_block_merkle_root, inner_header_src.m_block_merkle_root);

    EXPECT_EQ(block_header_dst.get_prev_block_hash(), block_header_src.get_prev_block_hash());
  //  EXPECT_EQ(block_header_dst.get_block_hash(), block_header_src.get_block_hash());
    EXPECT_EQ(block_header_dst.get_bchain_bits(), block_header_src.get_bchain_bits());
    EXPECT_EQ(block_header_dst.get_table_height(), block_header_src.get_table_height());

    EXPECT_EQ(block_header_dst.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(block_header_src.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    for (unsigned i = 0; i < TEST_ELECTIONS_NUM; i++) {
        xrelay_election xrelay_election_dst = block_header_dst.get_elections_sets()[i];
        xrelay_election xrelay_election_src = block_header_src.get_elections_sets()[i];
        EXPECT_EQ(xrelay_election_dst.public_key,  xrelay_election_src.public_key);
        EXPECT_EQ(xrelay_election_dst.stake, xrelay_election_src.stake);
    }

    EXPECT_EQ(block_header_dst.get_signatures_sets().size(), TEST_SIGNATURE_NUM);
    EXPECT_EQ(block_header_src.get_signatures_sets().size(), TEST_SIGNATURE_NUM);
    for (unsigned i = 0; i < TEST_SIGNATURE_NUM; i++) {
        xrelay_signature xrelay_signature_dst = block_header_dst.get_signatures_sets()[i];
        xrelay_signature xrelay_signature_src = block_header_src.get_signatures_sets()[i];
        EXPECT_EQ(xrelay_signature_dst.r,  xrelay_signature_src.r);
        EXPECT_EQ(xrelay_signature_dst.s, xrelay_signature_src.s);
        EXPECT_EQ(xrelay_signature_dst.v, xrelay_signature_src.v);
    }

}



TEST_F(test_relay_block, serialize_xrelay_block_header_with_contract) {

    xrelay_block_header block_header_src = xrelay_block_header_create();

    EXPECT_EQ(block_header_src.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(block_header_src.get_signatures_sets().size(), TEST_SIGNATURE_NUM);

    RLPStream rlp_block_header;
    block_header_src.streamRLP(rlp_block_header,true);

    bytes  rlp_bytes = rlp_block_header.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_block_header block_header_dst;
    block_header_dst.decodeRLP(rlp_dst, true);

    xrelay_block_inner_header inner_header_dst = block_header_dst.get_inner_header();
    xrelay_block_inner_header inner_header_src = block_header_src.get_inner_header();

    EXPECT_EQ(inner_header_dst.m_version, inner_header_src.m_version);
   // EXPECT_EQ(inner_header_dst.m_inner_hash, inner_header_src.m_inner_hash);
    EXPECT_EQ(inner_header_dst.m_height, inner_header_src.m_height);
    EXPECT_EQ(inner_header_dst.m_epochID, inner_header_src.m_epochID);
    EXPECT_EQ(inner_header_dst.m_timestamp, inner_header_src.m_timestamp);
    EXPECT_EQ(inner_header_dst.m_elections_hash, inner_header_src.m_elections_hash);
    EXPECT_EQ(inner_header_dst.m_txs_merkle_root, inner_header_src.m_txs_merkle_root);
    EXPECT_EQ(inner_header_dst.m_receipts_merkle_root, inner_header_src.m_receipts_merkle_root);
    EXPECT_EQ(inner_header_dst.m_state_merkle_root, inner_header_src.m_state_merkle_root);
    EXPECT_EQ(inner_header_dst.m_block_merkle_root, inner_header_src.m_block_merkle_root);

    EXPECT_EQ(block_header_dst.get_prev_block_hash(), block_header_src.get_prev_block_hash());
  //  EXPECT_EQ(block_header_dst.get_block_hash(), block_header_src.get_block_hash());
    EXPECT_EQ(block_header_dst.get_bchain_bits(), block_header_src.get_bchain_bits());
    EXPECT_EQ(block_header_dst.get_table_height(), block_header_src.get_table_height());

    EXPECT_EQ(block_header_dst.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(block_header_src.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    for (unsigned i = 0; i < TEST_ELECTIONS_NUM; i++) {
        xrelay_election xrelay_election_dst = block_header_dst.get_elections_sets()[i];
        xrelay_election xrelay_election_src = block_header_src.get_elections_sets()[i];
        EXPECT_EQ(xrelay_election_dst.public_key,  xrelay_election_src.public_key);
        EXPECT_EQ(xrelay_election_dst.stake, xrelay_election_src.stake);
    }

    EXPECT_EQ(block_header_dst.get_signatures_sets().size(), TEST_SIGNATURE_NUM);
    EXPECT_EQ(block_header_src.get_signatures_sets().size(), TEST_SIGNATURE_NUM);
    for (unsigned i = 0; i < TEST_SIGNATURE_NUM; i++) {
        xrelay_signature xrelay_signature_dst = block_header_dst.get_signatures_sets()[i];
        xrelay_signature xrelay_signature_src = block_header_src.get_signatures_sets()[i];
        EXPECT_EQ(xrelay_signature_dst.r,  xrelay_signature_src.r);
        EXPECT_EQ(xrelay_signature_dst.s, xrelay_signature_src.s);
        EXPECT_EQ(xrelay_signature_dst.v, xrelay_signature_src.v);
    }

    RLPStream rlp_block_header_contract;
    block_header_src.streamRLP_header_to_contract(rlp_block_header_contract);
    std::cout << " streamRLP_to_contract   " << toHex(rlp_block_header_contract.out()) << std::endl;
    std::ofstream fin("block_index_0.bin", std::ios::binary);
    for (auto c :  rlp_block_header_contract.out()) {
        fin.write((char*)&c, sizeof(uint8_t));
    }
}



TEST_F(test_relay_block, serialize_xrelay_block_without_signature) {

    xrelay_block  block_src = xrelay_block_create();

    RLPStream rlp_block;
    block_src.streamRLP(rlp_block);
    bytes  rlp_bytes = rlp_block.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_block  block_dst;
    block_dst.decodeRLP(rlp_dst);

    xrelay_block_header block_header_dst = block_dst.get_header();
    xrelay_block_header block_header_src = block_src.get_header();

    EXPECT_EQ(block_header_dst.get_prev_block_hash(), block_header_src.get_prev_block_hash());
    EXPECT_EQ(block_header_dst.get_block_hash(), block_header_src.get_block_hash());
    EXPECT_EQ(block_header_dst.get_bchain_bits(), block_header_src.get_bchain_bits());
    EXPECT_EQ(block_header_dst.get_table_height(), block_header_src.get_table_height());

    EXPECT_EQ(block_header_dst.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(block_header_src.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    for(unsigned i = 0; i < TEST_ELECTIONS_NUM; i++) {
        xrelay_election xrelay_election_dst = block_header_dst.get_elections_sets()[i];
        xrelay_election xrelay_election_src = block_header_src.get_elections_sets()[i];
        EXPECT_EQ(xrelay_election_dst.public_key,  xrelay_election_src.public_key);
        EXPECT_EQ(xrelay_election_dst.stake, xrelay_election_src.stake);
    }

    xrelay_block_inner_header inner_header_dst = block_dst.get_inner_header();
    xrelay_block_inner_header inner_header_src = block_src.get_inner_header();

    EXPECT_EQ(inner_header_dst.m_version, inner_header_src.m_version);
    EXPECT_EQ(inner_header_dst.m_inner_hash, inner_header_src.m_inner_hash);
    EXPECT_EQ(inner_header_dst.m_height, inner_header_src.m_height);
    EXPECT_EQ(inner_header_dst.m_epochID, inner_header_src.m_epochID);
    EXPECT_EQ(inner_header_dst.m_timestamp, inner_header_src.m_timestamp);
    EXPECT_EQ(inner_header_dst.m_elections_hash, inner_header_src.m_elections_hash);
    EXPECT_EQ(inner_header_dst.m_txs_merkle_root, inner_header_src.m_txs_merkle_root);
    EXPECT_EQ(inner_header_dst.m_receipts_merkle_root, inner_header_src.m_receipts_merkle_root);
    EXPECT_EQ(inner_header_dst.m_state_merkle_root, inner_header_src.m_state_merkle_root);
    EXPECT_EQ(inner_header_dst.m_block_merkle_root, inner_header_src.m_block_merkle_root);

    EXPECT_EQ(block_dst.get_block_receipts_set().size(), TEST_RECEIPT_NUM);
    EXPECT_EQ(block_src.get_block_receipts_set().size(), TEST_RECEIPT_NUM);
    for (unsigned i = 0; i < TEST_RECEIPT_NUM; i++) {
        xrelay_receipt receipt_src = block_src.get_block_receipts_set()[i];
        xrelay_receipt receipt_dst = block_dst.get_block_receipts_set()[i];
        EXPECT_EQ(receipt_src.m_status, receipt_dst.m_status);
        EXPECT_EQ(receipt_src.m_gasUsed, receipt_dst.m_gasUsed);
        EXPECT_EQ(receipt_src.m_logsBloom, receipt_dst.m_logsBloom);
        EXPECT_EQ(receipt_src.m_logs.size(), TEST_RECEIPT_LOG_NUM);
        EXPECT_EQ(receipt_dst.m_logs.size(), TEST_RECEIPT_LOG_NUM);
        
        for (unsigned i = 0; i < TEST_RECEIPT_LOG_NUM; i++) {
            xrelay_receipt_log log_src = receipt_src.m_logs[i];
            xrelay_receipt_log log_dst = receipt_dst.m_logs[i];
            EXPECT_EQ(log_dst.m_contract_address, log_src.m_contract_address);
            EXPECT_EQ(log_dst.m_data, log_src.m_data);
            EXPECT_EQ(log_dst.m_topics.size(), TEST_TOPICS_NUM);
            EXPECT_EQ(log_src.m_topics.size(), TEST_TOPICS_NUM);
            for(unsigned i = 0; i< TEST_TOPICS_NUM; i++ ) {
                EXPECT_EQ(log_dst.m_topics[i], log_src.m_topics[i]);
            }         
        }
    }
}


TEST_F(test_relay_block, serialize_xrelay_block_with_signature) {

    xrelay_block  block_src = xrelay_block_create();

    RLPStream rlp_block;
    block_src.streamRLP(rlp_block, true);
    bytes  rlp_bytes = rlp_block.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_block  block_dst;
    block_dst.decodeRLP(rlp_dst, true);

    xrelay_block_header block_header_dst = block_dst.get_header();
    xrelay_block_header block_header_src = block_src.get_header();

    EXPECT_EQ(block_header_dst.get_prev_block_hash(), block_header_src.get_prev_block_hash());
    EXPECT_EQ(block_header_dst.get_block_hash(), block_header_src.get_block_hash());
    EXPECT_EQ(block_header_dst.get_bchain_bits(), block_header_src.get_bchain_bits());
    EXPECT_EQ(block_header_dst.get_table_height(), block_header_src.get_table_height());

    EXPECT_EQ(block_header_dst.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    EXPECT_EQ(block_header_src.get_elections_sets().size(), TEST_ELECTIONS_NUM);
    for(unsigned i = 0; i < TEST_ELECTIONS_NUM; i++) {
        xrelay_election xrelay_election_dst = block_header_dst.get_elections_sets()[i];
        xrelay_election xrelay_election_src = block_header_src.get_elections_sets()[i];
        EXPECT_EQ(xrelay_election_dst.public_key,  xrelay_election_src.public_key);
        EXPECT_EQ(xrelay_election_dst.stake, xrelay_election_src.stake);
    }

    EXPECT_EQ(block_header_dst.get_signatures_sets().size(), TEST_SIGNATURE_NUM);
    EXPECT_EQ(block_header_src.get_signatures_sets().size(), TEST_SIGNATURE_NUM);
    for (unsigned i = 0; i < TEST_SIGNATURE_NUM; i++) {
        xrelay_signature xrelay_signature_dst = block_header_dst.get_signatures_sets()[i];
        xrelay_signature xrelay_signature_src = block_header_src.get_signatures_sets()[i];
        EXPECT_EQ(xrelay_signature_dst.r, xrelay_signature_src.r);
        EXPECT_EQ(xrelay_signature_dst.s, xrelay_signature_src.s);
        EXPECT_EQ(xrelay_signature_dst.v, xrelay_signature_src.v);
    }

    xrelay_block_inner_header inner_header_dst = block_dst.get_inner_header();
    xrelay_block_inner_header inner_header_src = block_src.get_inner_header();

    EXPECT_EQ(inner_header_dst.m_version, inner_header_src.m_version);
    EXPECT_EQ(inner_header_dst.m_inner_hash, inner_header_src.m_inner_hash);
    EXPECT_EQ(inner_header_dst.m_height, inner_header_src.m_height);
    EXPECT_EQ(inner_header_dst.m_epochID, inner_header_src.m_epochID);
    EXPECT_EQ(inner_header_dst.m_timestamp, inner_header_src.m_timestamp);
    EXPECT_EQ(inner_header_dst.m_elections_hash, inner_header_src.m_elections_hash);
    EXPECT_EQ(inner_header_dst.m_txs_merkle_root, inner_header_src.m_txs_merkle_root);
    EXPECT_EQ(inner_header_dst.m_receipts_merkle_root, inner_header_src.m_receipts_merkle_root);
    EXPECT_EQ(inner_header_dst.m_state_merkle_root, inner_header_src.m_state_merkle_root);
    EXPECT_EQ(inner_header_dst.m_block_merkle_root, inner_header_src.m_block_merkle_root);

    EXPECT_EQ(block_dst.get_block_receipts_set().size(), TEST_RECEIPT_NUM);
    EXPECT_EQ(block_src.get_block_receipts_set().size(), TEST_RECEIPT_NUM);
    for (unsigned i = 0; i < TEST_RECEIPT_NUM; i++) {
        xrelay_receipt receipt_src = block_src.get_block_receipts_set()[i];
        xrelay_receipt receipt_dst = block_dst.get_block_receipts_set()[i];
        EXPECT_EQ(receipt_src.m_status, receipt_dst.m_status);
        EXPECT_EQ(receipt_src.m_gasUsed, receipt_dst.m_gasUsed);
        EXPECT_EQ(receipt_src.m_logsBloom, receipt_dst.m_logsBloom);
        EXPECT_EQ(receipt_src.m_logs.size(), TEST_RECEIPT_LOG_NUM);
        EXPECT_EQ(receipt_dst.m_logs.size(), TEST_RECEIPT_LOG_NUM);
        
        for (unsigned i = 0; i < TEST_RECEIPT_LOG_NUM; i++) {
            xrelay_receipt_log log_src = receipt_src.m_logs[i];
            xrelay_receipt_log log_dst = receipt_dst.m_logs[i];
            EXPECT_EQ(log_dst.m_contract_address, log_src.m_contract_address);
            EXPECT_EQ(log_dst.m_data, log_src.m_data);
            EXPECT_EQ(log_dst.m_topics.size(), TEST_TOPICS_NUM);
            EXPECT_EQ(log_src.m_topics.size(), TEST_TOPICS_NUM);
            for(unsigned i = 0; i< TEST_TOPICS_NUM; i++ ) {
                EXPECT_EQ(log_dst.m_topics[i], log_src.m_topics[i]);
            }         
        }
    }
}


