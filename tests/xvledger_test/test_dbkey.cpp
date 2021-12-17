#include "gtest/gtest.h"
#include "xvledger/xvdbkey.h"
#include "xvledger/xvtxindex.h"

using namespace top;
using namespace top::base;

class test_dbkey : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


TEST_F(test_dbkey, txkey_1) {
    xassert(base::enum_txindex_type_send == base::xvtxkey_t::transaction_subtype_to_txindex_type(base::enum_transaction_subtype_self));
    xassert(base::enum_txindex_type_send == base::xvtxkey_t::transaction_subtype_to_txindex_type(base::enum_transaction_subtype_send));
    xassert(base::enum_txindex_type_receive == base::xvtxkey_t::transaction_subtype_to_txindex_type(base::enum_transaction_subtype_recv));
    xassert(base::enum_txindex_type_confirm == base::xvtxkey_t::transaction_subtype_to_txindex_type(base::enum_transaction_subtype_confirm));
}

TEST_F(test_dbkey, key_span) {
    base::xvaccount_t vaddr("Ta0000@1");
    {
        std::string old_key = xvdbkey_t::create_chain_key(vaddr);
        ASSERT_EQ(base::enum_xdbkey_type_account_span_height, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
    {
        std::string old_key = xvdbkey_t::create_chain_span_key(vaddr, 100);
        ASSERT_EQ(base::enum_xdbkey_type_account_span, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
    {
        std::string old_key = xvdbkey_t::create_account_span_genesis_height_key(vaddr);
        ASSERT_EQ(base::enum_xdbkey_type_account_span_height, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
    {
        std::string old_key = xvdbkey_t::create_account_span_key(vaddr, 100);
        ASSERT_EQ(base::enum_xdbkey_type_account_span, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }    
}

TEST_F(test_dbkey, key_meta) {
    base::xvaccount_t vaddr("Ta0000@1");
    {
        std::string old_key = xvdbkey_t::create_account_meta_key_old(vaddr);
        ASSERT_EQ(base::enum_xdbkey_type_account_meta, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
    {
        std::string old_key = xvdbkey_t::create_account_meta_key(vaddr);
        ASSERT_EQ(base::enum_xdbkey_type_account_meta, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
}

TEST_F(test_dbkey, key_tx) {
    std::string txhash = "1111";
    {
        std::string old_key = xvdbkey_t::create_tx_key(txhash);
        ASSERT_EQ(base::enum_xdbkey_type_transaction, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
    {
        std::string old_key = xvdbkey_t::create_tx_index_key(txhash, base::enum_txindex_type_receive);
        ASSERT_EQ(base::enum_xdbkey_type_transaction, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }    
    {
        std::string old_key = xvdbkey_t::create_prunable_tx_key(txhash);
        ASSERT_EQ(base::enum_xdbkey_type_transaction, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
    {
        std::string old_key = xvdbkey_t::create_prunable_tx_index_key(txhash, base::enum_txindex_type_receive);
        ASSERT_EQ(base::enum_xdbkey_type_transaction, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }    
}

TEST_F(test_dbkey, key_block_index) {
    base::xvaccount_t vaddr("Ta0000@1");
    {
        std::string old_key = xvdbkey_t::create_block_index_key(vaddr, 100);
        ASSERT_EQ(base::enum_xdbkey_type_block_index, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
    {
        std::string old_key = xvdbkey_t::create_block_index_key(vaddr, 100, 200);
        ASSERT_EQ(base::enum_xdbkey_type_block_index, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }    
    {
        std::string old_key = xvdbkey_t::create_prunable_block_index_key(vaddr, 100);
        ASSERT_EQ(base::enum_xdbkey_type_block_index, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
    {
        std::string old_key = xvdbkey_t::create_prunable_block_index_key(vaddr, 100, 200);
        ASSERT_EQ(base::enum_xdbkey_type_block_index, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }    
}

TEST_F(test_dbkey, key_block_object) {
    base::xvaccount_t vaddr("Ta0000@1");
    std::string blockhash = "1111";
    {
        std::string old_key = xvdbkey_t::create_block_object_key(vaddr, blockhash);
        ASSERT_EQ(base::enum_xdbkey_type_block_object, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
    {
        std::string old_key = xvdbkey_t::create_prunable_block_object_key(vaddr, 100);
        ASSERT_EQ(base::enum_xdbkey_type_block_object, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }    
    {
        std::string old_key = xvdbkey_t::create_prunable_block_object_key(vaddr, 100, 200);
        ASSERT_EQ(base::enum_xdbkey_type_block_object, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }    
}

TEST_F(test_dbkey, key_block_input) {
    base::xvaccount_t vaddr("Ta0000@1");
    std::string blockhash = "1111";
    {
        std::string old_key = xvdbkey_t::create_block_input_resource_key(vaddr, blockhash);
        ASSERT_EQ(base::enum_xdbkey_type_block_input_resource, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
    {
        std::string old_key = xvdbkey_t::create_prunable_block_input_resource_key(vaddr, 100, 200);
        ASSERT_EQ(base::enum_xdbkey_type_block_input_resource, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
}

TEST_F(test_dbkey, key_block_output) {
    base::xvaccount_t vaddr("Ta0000@1");
    std::string blockhash = "1111";
    {
        std::string old_key = xvdbkey_t::create_block_output_resource_key(vaddr, blockhash);
        ASSERT_EQ(base::enum_xdbkey_type_block_output_resource, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
    {
        std::string old_key = xvdbkey_t::create_prunable_block_output_resource_key(vaddr, 100, 200);
        ASSERT_EQ(base::enum_xdbkey_type_block_output_resource, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
}

TEST_F(test_dbkey, key_block_state) {
    base::xvaccount_t vaddr("Ta0000@1");
    std::string blockhash = "1111";
    {
        std::string old_key = xvdbkey_t::create_block_state_key(vaddr, blockhash);
        ASSERT_EQ(base::enum_xdbkey_type_state_object, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
    {
        std::string old_key = xvdbkey_t::create_prunable_state_key(vaddr, 100);
        ASSERT_EQ(base::enum_xdbkey_type_state_object, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
    {
        std::string old_key = xvdbkey_t::create_prunable_state_key(vaddr, 100, blockhash);
        ASSERT_EQ(base::enum_xdbkey_type_state_object, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }    
}

TEST_F(test_dbkey, key_unit_proof) {
    base::xvaccount_t vaddr("Ta0000@1");
    std::string blockhash = "1111";
    {
        std::string old_key = xvdbkey_t::create_prunable_unit_proof_key(vaddr, 100);
        ASSERT_EQ(base::enum_xdbkey_type_unit_proof, xvdbkey_t::get_dbkey_type(old_key));
        std::cout << old_key << std::endl;
    }
}
