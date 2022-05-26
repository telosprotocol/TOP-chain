#include "gtest/gtest.h"
#include "xdata/xrelay_block.h"
#include "xbase/xmem.h"
#include "xpbase/base/top_utils.h"

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

xrelay_receipt_log g_logs;
xrelay_election    g_xrelay_election;
xrelay_block_inner_header g_inner_header;
xrelay_receipt   g_receipt;
xrelay_block_header g_header;


//test 
h160 test_address{"009b5f068bc20a5b12030fcb72975d8bddc4e84c"};
h256 test_topics1{"4f89ece0f576ba3986204ba19a44d94601604b97cf3baa922b010a758d303842"};
h256 test_topics2{"000000000000000000000000e22c0e020c99e9aed339618fdcea2871d678ef38"};
h256 test_topics3{"000000000000000000000000f3b23b373dc8854cc2936f4ab4b8e782011ccf87"};
h256 test_topics4{"000000000000000000000000f39fd6e51aad88f6f4ce6ab8827279cfffb92266"};

uint8_t  test_status = 1;
u256     test_gasUsed{0xccde};
h2048    test_logsBloom{"00000001000000004000000000000000000000000000000000000000000000000000000000041000000000000000008000000000000080000000000000200000000000000000000000000008000000000000000000008000000000000000000010000000020000000004000100000800000000040000000000000012000000000000000020000000008000000000000000000000000000000000000000000000420000000000000000000000000000000000000000080000000000000000000000000002000000200000000000000000000008002000000000000000000020000010000200000000000000000000000000000000000000000000002000000000"};
bytes    test_data;
h256     test_public_key{"b72d55c76bd8f477f4b251763c33f75e6f5f5dd8af071e711e0cb9b2accc70ea"};
         //election stake
uint64_t  test_stake = 0x12345678;
h256    test_r{"4f89ece0f576ba39123456789123456781604b97cf3baa922b010a758d303842"} ;
h256    test_s{"4f812345678abcdef1234ba19a44d94601604b97cf3baa922b010a758d303842"} ;
evm_common::byte    test_v = 0x1;

uint64_t            test_version = 0;
h256    test_inner_hash{"5e173f6ac3c669587538e7727cf19b782a4f2fda07c1eaa662c593e5e85e3051"}; 
uint64_t            test_height = 123;
uint64_t            test_epochID = 456;
uint64_t            test_timestamp = 13579;
h256    test_elections_hash {"19c2185f4f40634926ebed3af09070ca9e029f2edd5fae6253074896205f5f6c"};
h256    test_txs_merkle_root{"c45f950382d542169ea207959ee0220ec1491755abe405cd7498d6b16adb6df8"};
h256    test_receipts_merkle_root{"d25688cf0ab10afa1a0e2dba7853ed5f1e5bf1c631757ed4e103b593ff3f5620"};
h256    test_state_merkle_root{"f37ec61d84cea03dcc5e8385db93248584e8af4b4d1c832d8c7453c0089687a7"};
h256    test_block_merkle_root{"e3f407f83fc012470c26a93fdff534100f2c6f736439ce0ca90e9914f7d1c381"};
h256    test_prev_hash        {"cda1f407f83fc012470c26a93fdff534100f2c6f736439ce0ca9acbde1234567"};
h256    test_block_hash       {"1234acdeacbfc012470c26a93fdff534100f2c6f736439ce0ca9acbde1234123"};
u256    test_chain_bits{0x45567};
uint64_t test_table_height = 999;



TEST_F(test_relay_block, serialize_receipt_log) {
    std::string test_str = "000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000a4ba11f3f36b12c71f2aef775583b306a3cf784a";
    std::string log_data = top::HexDecode(test_str);
    test_data =  bytes(log_data.begin(), log_data.end());
   
    xrelay_receipt_log log_src;
    log_src.m_contract_address = test_address;
    log_src.m_topics.push_back(test_topics1);
    log_src.m_topics.push_back(test_topics2);
    log_src.m_topics.push_back(test_topics3);
    log_src.m_topics.push_back(test_topics4);
    log_src.m_data = test_data ;// bytes(log_data3.begin(), log_data3.end());

    evm_common::RLPStream rlp_log;
    log_src.streamRLP(rlp_log);
    bytes  rlp_bytes = rlp_log.out();

    xrelay_receipt_log log_dst;
    RLP rlp_dst = RLP(rlp_bytes);
    log_dst.decodeRLP(rlp_dst);
    EXPECT_EQ(log_dst.m_contract_address, test_address);
    EXPECT_EQ(log_dst.m_topics[0], test_topics1);
    EXPECT_EQ(log_dst.m_topics[1], test_topics2);
    EXPECT_EQ(log_dst.m_topics[2], test_topics3);
    EXPECT_EQ(log_dst.m_topics[3], test_topics4);
    EXPECT_EQ(log_dst.m_data, test_data);
    g_logs = log_dst;
}

TEST_F(test_relay_block, serialize_receipt) {

    xrelay_receipt receipt_src;
    receipt_src.m_status = test_status;
    receipt_src.m_gasUsed = test_gasUsed;
    receipt_src.m_logsBloom = test_logsBloom;

    for (int i = 0; i < 3; i++) {
        receipt_src.m_logs.emplace_back(g_logs);
    }
    
    evm_common::RLPStream rlp_receipt;
    receipt_src.streamRLP(rlp_receipt);
    bytes  rlp_bytes = rlp_receipt.out();

    xrelay_receipt receipt_dst;
    RLP rlp_dst = RLP(rlp_bytes);
    receipt_dst.decodeRLP(rlp_dst);
    EXPECT_EQ(receipt_dst.m_status, test_status);
    EXPECT_EQ(receipt_dst.m_gasUsed, test_gasUsed);
    EXPECT_EQ(receipt_dst.m_logsBloom, test_logsBloom);
    EXPECT_EQ( receipt_dst.m_logs.size(), 3);
    for(auto & log : receipt_dst.m_logs)  {
        EXPECT_EQ(log.m_contract_address, test_address);
        EXPECT_EQ(log.m_topics[0], test_topics1);
        EXPECT_EQ(log.m_topics[1], test_topics2);
        EXPECT_EQ(log.m_topics[2], test_topics3);
        EXPECT_EQ(log.m_topics[3], test_topics4);
        EXPECT_EQ(log.m_data, test_data);
    }
    g_receipt = receipt_dst;
}


TEST_F(test_relay_block, serialize_xrelay_election) {

    xrelay_election election;
    election.public_key = test_public_key;
    election.stake = test_stake;

    evm_common::RLPStream rlp_election;
    election.streamRLP(rlp_election);
    bytes  rlp_bytes = rlp_election.out();

    xrelay_election election_dst;
    RLP rlp_dst = RLP(rlp_bytes);
    election_dst.decodeRLP(rlp_dst);

    EXPECT_EQ(election_dst.public_key, test_public_key);
    EXPECT_EQ(election_dst.stake, test_stake);
}


TEST_F(test_relay_block, serialize_xrelay_signature) {

    xrelay_signature signature;
    signature.r = test_r;
    signature.s = test_s;
    signature.v = test_v;

    evm_common::RLPStream rlp_signature;
    signature.streamRLP(rlp_signature);
    bytes  rlp_bytes = rlp_signature.out();

    xrelay_signature signature_dst;
    RLP rlp_dst = RLP(rlp_bytes);
    signature_dst.decodeRLP(rlp_dst);

    EXPECT_EQ(signature_dst.r, test_r);
    EXPECT_EQ(signature_dst.s, test_s);
    EXPECT_EQ(signature_dst.v, test_v);
}


TEST_F(test_relay_block, serialize_xrelay_inner_header) {

    xrelay_block_inner_header innder_header;
    innder_header.m_version = test_version;
    innder_header.m_inner_hash = test_inner_hash;
    innder_header.m_height = test_height;
    innder_header.m_epochID = test_epochID;
    innder_header.m_timestamp = test_timestamp;
    innder_header.m_elections_hash = test_elections_hash;
    innder_header.m_txs_merkle_root = test_txs_merkle_root;
    innder_header.m_receipts_merkle_root = test_receipts_merkle_root;
    innder_header.m_state_merkle_root = test_state_merkle_root;
    innder_header.m_block_merkle_root = test_block_merkle_root;

    evm_common::RLPStream rlp_innder_header;
    innder_header.streamRLP(rlp_innder_header);
    bytes  rlp_bytes = rlp_innder_header.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_block_inner_header innder_header_dst;
    innder_header_dst.decodeRLP(rlp_dst);

    EXPECT_EQ(innder_header_dst.m_version, test_version);
    EXPECT_EQ(innder_header_dst.m_inner_hash, test_inner_hash);
    EXPECT_EQ(innder_header_dst.m_height, test_height);
    EXPECT_EQ(innder_header_dst.m_epochID, test_epochID);
    EXPECT_EQ(innder_header_dst.m_timestamp, test_timestamp);
    EXPECT_EQ(innder_header_dst.m_elections_hash, test_elections_hash);
    EXPECT_EQ(innder_header_dst.m_txs_merkle_root, test_txs_merkle_root);
    EXPECT_EQ(innder_header_dst.m_receipts_merkle_root, test_receipts_merkle_root);
    EXPECT_EQ(innder_header_dst.m_state_merkle_root, test_state_merkle_root);
    EXPECT_EQ(innder_header_dst.m_block_merkle_root, test_block_merkle_root);
    g_inner_header = innder_header_dst;
}



TEST_F(test_relay_block, serialize_xrelay_block_header) {

    xrelay_block_header block_header;
    block_header.set_inner_header(g_inner_header);
    block_header.set_prev_hash(test_prev_hash);
    block_header.set_block_hash(test_block_hash);
    block_header.set_chain_bits(test_chain_bits);
    block_header.set_table_height(test_table_height);    
    
    std::vector<xrelay_election>    election_vector;
    for (int i = 0; i< 10; i++) {
        xrelay_election election;
        election.public_key = test_public_key;
        election.stake = test_stake ;
        election_vector.push_back(election);
    }
    block_header.set_elections_next(election_vector);

    for (int i = 0; i < 13; i++) {
        xrelay_signature signature;
        signature.r = test_r;
        signature.s = test_s;
        signature.v = test_v+i;
        block_header.add_signature(signature);
    }
    EXPECT_EQ(block_header.get_elections_sets().size(), 10);
    EXPECT_EQ(block_header.get_signatures_sets().size(), 13);

    evm_common::RLPStream rlp_block_header;
    block_header.streamRLP(rlp_block_header);
    bytes  rlp_bytes = rlp_block_header.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_block_header block_header_dst;
    block_header_dst.decodeRLP(rlp_dst);

    xrelay_block_inner_header inner_header = block_header_dst.get_inner_header();

    EXPECT_EQ(inner_header.m_version, test_version);
    EXPECT_EQ(inner_header.m_inner_hash, test_inner_hash);
    EXPECT_EQ(inner_header.m_height, test_height);
    EXPECT_EQ(inner_header.m_epochID, test_epochID);
    EXPECT_EQ(inner_header.m_timestamp, test_timestamp);
    EXPECT_EQ(inner_header.m_elections_hash, test_elections_hash);
    EXPECT_EQ(inner_header.m_txs_merkle_root, test_txs_merkle_root);
    EXPECT_EQ(inner_header.m_receipts_merkle_root, test_receipts_merkle_root);
    EXPECT_EQ(inner_header.m_state_merkle_root, test_state_merkle_root);
    EXPECT_EQ(inner_header.m_block_merkle_root, test_block_merkle_root);

    EXPECT_EQ(block_header_dst.get_prev_block_hash(), test_prev_hash);
    EXPECT_EQ(block_header_dst.get_block_hash(), test_block_hash);
    EXPECT_EQ(block_header_dst.get_bchain_bits(), test_chain_bits);
    EXPECT_EQ(block_header_dst.get_table_height(), test_table_height);

    EXPECT_EQ(block_header_dst.get_elections_sets().size(), 10);
    for(auto &_election: block_header_dst.get_elections_sets()) {
        EXPECT_EQ(_election.public_key, test_public_key);
        EXPECT_EQ(_election.stake, test_stake);
    }

    EXPECT_EQ(block_header_dst.get_signatures_sets().size(), 13);
    int i=0;
    for(auto &_signature: block_header_dst.get_signatures_sets()) {
        EXPECT_EQ(_signature.r, test_r);
        EXPECT_EQ(_signature.s, test_s);
        EXPECT_EQ(_signature.v, test_v + i);
        i++;
    }
    g_header = block_header_dst;
}



TEST_F(test_relay_block, serialize_xrelay_block) {

    xrelay_block  block_src;
    block_src.set_header(g_header);

    std::vector<xrelay_receipt>   receipts_vector;
    for(int i = 0; i < 20; i ++) {
        receipts_vector.push_back(g_receipt);
    }
    block_src.set_receipts(receipts_vector);

    evm_common::RLPStream rlp_block;
    block_src.streamRLP(rlp_block);
    bytes  rlp_bytes = rlp_block.out();
    RLP rlp_dst = RLP(rlp_bytes);

    xrelay_block  block_dst;
    block_dst.decodeRLP(rlp_dst);

    xrelay_block_header block_header_dst = block_dst.get_header();
    xrelay_block_inner_header inner_header = block_dst.get_inner_header();

    EXPECT_EQ(inner_header.m_version, test_version);
    EXPECT_EQ(inner_header.m_inner_hash, test_inner_hash);
    EXPECT_EQ(inner_header.m_height, test_height);
    EXPECT_EQ(inner_header.m_epochID, test_epochID);
    EXPECT_EQ(inner_header.m_timestamp, test_timestamp);
    EXPECT_EQ(inner_header.m_elections_hash, test_elections_hash);
    EXPECT_EQ(inner_header.m_txs_merkle_root, test_txs_merkle_root);
    EXPECT_EQ(inner_header.m_receipts_merkle_root, test_receipts_merkle_root);
    EXPECT_EQ(inner_header.m_state_merkle_root, test_state_merkle_root);
    EXPECT_EQ(inner_header.m_block_merkle_root, test_block_merkle_root);

    EXPECT_EQ(block_header_dst.get_prev_block_hash(), test_prev_hash);
    EXPECT_EQ(block_header_dst.get_block_hash(), test_block_hash);
    EXPECT_EQ(block_header_dst.get_bchain_bits(), test_chain_bits);
    EXPECT_EQ(block_header_dst.get_table_height(), test_table_height);

    EXPECT_EQ(block_header_dst.get_elections_sets().size(), 10);
    for(auto &_election: block_header_dst.get_elections_sets()) {
        EXPECT_EQ(_election.public_key, test_public_key);
        EXPECT_EQ(_election.stake, test_stake);
    }

    EXPECT_EQ(block_header_dst.get_signatures_sets().size(), 13);
    int i = 0;
    for(auto &_signature: block_header_dst.get_signatures_sets()) {
        EXPECT_EQ(_signature.r, test_r);
        EXPECT_EQ(_signature.s, test_s);
        EXPECT_EQ(_signature.v, test_v + i );
        i++;
    }

    EXPECT_EQ(block_dst.get_block_receipts_set().size(), 20);
    for(auto &receipt_dst: block_dst.get_block_receipts_set()) {
        EXPECT_EQ(receipt_dst.m_status, test_status);
        EXPECT_EQ(receipt_dst.m_gasUsed, test_gasUsed);
        EXPECT_EQ(receipt_dst.m_logsBloom, test_logsBloom);
        EXPECT_EQ( receipt_dst.m_logs.size(), 3);
        for(auto & log : receipt_dst.m_logs)  {
            EXPECT_EQ(log.m_contract_address, test_address);
            EXPECT_EQ(log.m_topics[0], test_topics1);
            EXPECT_EQ(log.m_topics[1], test_topics2);
            EXPECT_EQ(log.m_topics[2], test_topics3);
            EXPECT_EQ(log.m_topics[3], test_topics4);
            EXPECT_EQ(log.m_data, test_data);
        }
    }

}




