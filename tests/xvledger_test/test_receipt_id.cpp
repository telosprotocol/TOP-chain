#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xvledger/xreceiptid.h"

using namespace top;
using namespace top::base;

class test_receiptid : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


TEST_F(test_receiptid, sendids_check)
{
    xreceiptid_state_ptr_t receiptid_state = std::make_shared<xreceiptid_state_t>(1, 100);
    xreceiptid_pair_t pair1(1, 0, 0, 0, 0);
    receiptid_state->add_pair(0, pair1);

    xsendids_check_t sendids_check;
    sendids_check.set_id(0, 2);
    ASSERT_EQ(1, sendids_check.size());
    bool ret = sendids_check.check_continuous(receiptid_state);
    ASSERT_EQ(ret, true);
    sendids_check.set_id(0, 4);
    ret = sendids_check.check_continuous(receiptid_state);
    ASSERT_EQ(ret, false);
    sendids_check.set_id(0, 3);
    ret = sendids_check.check_continuous(receiptid_state);
    ASSERT_EQ(ret, true);

    xreceiptid_pairs_ptr_t modified_pairs = std::make_shared<base::xreceiptid_pairs_t>();

    sendids_check.get_modified_pairs(receiptid_state, modified_pairs);
    ASSERT_EQ(modified_pairs->get_size(), 1);
    auto & pairs = modified_pairs->get_all_pairs();
    ASSERT_EQ(pairs.size(), 1);
    auto iter = pairs.find(0);
    ASSERT_EQ(iter->first, 0);
    auto & receipt_pair = iter->second;
    ASSERT_EQ(receipt_pair.get_sendid_max(), 4);
}

TEST_F(test_receiptid, all_states_one_table)
{
    base::xreceiptid_all_table_states all_states;
    {
        xreceiptid_state_ptr_t receiptid_state = std::make_shared<xreceiptid_state_t>(0, 100);
        xreceiptid_pair_t pair1(2, 0, 0, 1, 0);
        receiptid_state->add_pair(0, pair1);
        all_states.add_table_receiptid_state(0, receiptid_state);
    }
    {
        auto info = all_states.get_unfinish_info();
        ASSERT_EQ(info.size(), 1);
        ASSERT_EQ(info[0].source_id, 0);
        ASSERT_EQ(info[0].target_id, 0);
        ASSERT_EQ(info[0].source_height, 100);
        ASSERT_EQ(info[0].target_height, 100);
        ASSERT_EQ(info[0].unrecv_num, 2);
        ASSERT_EQ(info[0].unconfirm_num, 1);
    }    
    {
        xreceiptid_state_ptr_t receiptid_state = std::make_shared<xreceiptid_state_t>(0, 101);
        xreceiptid_pair_t pair1(2, 0, 2, 1, 1);
        receiptid_state->add_pair(0, pair1);
        all_states.add_table_receiptid_state(0, receiptid_state);
    }    
    {
        auto info = all_states.get_unfinish_info();
        ASSERT_EQ(info.size(), 0);
    }        
}

TEST_F(test_receiptid, all_states_two_table)
{
    base::xreceiptid_all_table_states all_states;
    {
        xreceiptid_state_ptr_t receiptid_state = std::make_shared<xreceiptid_state_t>(0, 100);
        xreceiptid_pair_t pair1(2, 0, 0, 1, 0);
        receiptid_state->add_pair(1, pair1);
        all_states.add_table_receiptid_state(0, receiptid_state);
    }
    {
        xreceiptid_state_ptr_t receiptid_state = std::make_shared<xreceiptid_state_t>(1, 200);
        xreceiptid_pair_t pair1(0, 0, 0, 0, 0);
        receiptid_state->add_pair(0, pair1);
        all_states.add_table_receiptid_state(1, receiptid_state);        
    }
    {
        auto info = all_states.get_unfinish_info();
        ASSERT_EQ(info.size(), 1);
        ASSERT_EQ(info[0].source_id, 0);
        ASSERT_EQ(info[0].target_id, 1);
        ASSERT_EQ(info[0].source_height, 100);
        ASSERT_EQ(info[0].target_height, 200);
        ASSERT_EQ(info[0].unrecv_num, 2);
        ASSERT_EQ(info[0].unconfirm_num, 1);
    }
    {
        xreceiptid_state_ptr_t receiptid_state = std::make_shared<xreceiptid_state_t>(1, 202);
        xreceiptid_pair_t pair1(0, 0, 2, 0, 0);
        receiptid_state->add_pair(0, pair1);
        all_states.add_table_receiptid_state(1, receiptid_state);        
    }
    {
        auto info = all_states.get_unfinish_info();
        ASSERT_EQ(info.size(), 1);
        ASSERT_EQ(info[0].source_id, 0);
        ASSERT_EQ(info[0].target_id, 1);
        ASSERT_EQ(info[0].source_height, 100);
        ASSERT_EQ(info[0].target_height, 202);
        ASSERT_EQ(info[0].unrecv_num, 0);
        ASSERT_EQ(info[0].unconfirm_num, 1);
    }    
    {
        xreceiptid_state_ptr_t receiptid_state = std::make_shared<xreceiptid_state_t>(0, 100);
        xreceiptid_pair_t pair1(2, 0, 0, 1, 1);
        receiptid_state->add_pair(1, pair1);
        all_states.add_table_receiptid_state(0, receiptid_state);
    }
    {
        auto info = all_states.get_unfinish_info();
        ASSERT_EQ(info.size(), 0);
    }        
}

TEST_F(test_receiptid, all_states_one_three_tables)
{
    base::xreceiptid_all_table_states all_states;
    {
        xreceiptid_state_ptr_t receiptid_state = std::make_shared<xreceiptid_state_t>(0, 100);
        receiptid_state->add_pair(1, xreceiptid_pair_t(2, 0, 0, 1, 0));
        receiptid_state->add_pair(2, xreceiptid_pair_t(3, 0, 0, 2, 0));
        receiptid_state->add_pair(3, xreceiptid_pair_t(4, 0, 0, 4, 0));        
        all_states.add_table_receiptid_state(0, receiptid_state);
    }
    {
        xreceiptid_state_ptr_t receiptid_state1 = std::make_shared<xreceiptid_state_t>(1, 200);
        all_states.add_table_receiptid_state(1, receiptid_state1);        
        xreceiptid_state_ptr_t receiptid_state2 = std::make_shared<xreceiptid_state_t>(2, 300);
        all_states.add_table_receiptid_state(2, receiptid_state2);  
        xreceiptid_state_ptr_t receiptid_state3 = std::make_shared<xreceiptid_state_t>(3, 400);
        all_states.add_table_receiptid_state(3, receiptid_state3);                  
    }
    {
        auto info = all_states.get_unfinish_info();
        ASSERT_EQ(info.size(), 3);            
    }
    {
        xreceiptid_state_ptr_t receiptid_state1 = std::make_shared<xreceiptid_state_t>(1, 200);
        receiptid_state1->add_pair(0, xreceiptid_pair_t(0, 0, 2, 0, 0));
        all_states.add_table_receiptid_state(1, receiptid_state1);        
        xreceiptid_state_ptr_t receiptid_state2 = std::make_shared<xreceiptid_state_t>(2, 300);
        receiptid_state2->add_pair(0, xreceiptid_pair_t(0, 0, 3, 0, 0));
        all_states.add_table_receiptid_state(2, receiptid_state2);  
        xreceiptid_state_ptr_t receiptid_state3 = std::make_shared<xreceiptid_state_t>(3, 400);
        receiptid_state3->add_pair(0, xreceiptid_pair_t(0, 0, 4, 0, 0));
        all_states.add_table_receiptid_state(3, receiptid_state3);                  
    }
    {
        auto info = all_states.get_unfinish_info();
        ASSERT_EQ(info.size(), 3);            
    }    
    {
        xreceiptid_state_ptr_t receiptid_state = std::make_shared<xreceiptid_state_t>(0, 100);
        receiptid_state->add_pair(1, xreceiptid_pair_t(2, 0, 0, 1, 1));
        receiptid_state->add_pair(2, xreceiptid_pair_t(3, 0, 0, 2, 2));
        receiptid_state->add_pair(3, xreceiptid_pair_t(4, 0, 0, 4, 4));        
        all_states.add_table_receiptid_state(0, receiptid_state);
    }    
    {
        auto info = all_states.get_unfinish_info();
        ASSERT_EQ(info.size(), 0);            
    }        
}

TEST_F(test_receiptid, all_states_two_two_tables)
{
    base::xreceiptid_all_table_states all_states;
    {
        xreceiptid_state_ptr_t receiptid_state0 = std::make_shared<xreceiptid_state_t>(0, 100);
        receiptid_state0->add_pair(2, xreceiptid_pair_t(2, 0, 0, 2, 0));
        receiptid_state0->add_pair(3, xreceiptid_pair_t(3, 0, 0, 3, 0));        
        all_states.add_table_receiptid_state(0, receiptid_state0);
        xreceiptid_state_ptr_t receiptid_state1 = std::make_shared<xreceiptid_state_t>(1, 100);
        receiptid_state1->add_pair(2, xreceiptid_pair_t(2, 0, 0, 2, 0));
        receiptid_state1->add_pair(3, xreceiptid_pair_t(3, 0, 0, 3, 0));        
        all_states.add_table_receiptid_state(1, receiptid_state1);        
    }
    { 
        xreceiptid_state_ptr_t receiptid_state2 = std::make_shared<xreceiptid_state_t>(2, 300);
        all_states.add_table_receiptid_state(2, receiptid_state2);  
        xreceiptid_state_ptr_t receiptid_state3 = std::make_shared<xreceiptid_state_t>(3, 400);
        all_states.add_table_receiptid_state(3, receiptid_state3);                  
    }
    {
        auto info = all_states.get_unfinish_info();
        ASSERT_EQ(info.size(), 4);            
    }
    {
        xreceiptid_state_ptr_t receiptid_state2 = std::make_shared<xreceiptid_state_t>(2, 300);
        receiptid_state2->add_pair(0, xreceiptid_pair_t(0, 0, 2, 0, 0));
        receiptid_state2->add_pair(1, xreceiptid_pair_t(0, 0, 2, 0, 0));
        all_states.add_table_receiptid_state(2, receiptid_state2);  
        xreceiptid_state_ptr_t receiptid_state3 = std::make_shared<xreceiptid_state_t>(3, 400);
        receiptid_state3->add_pair(0, xreceiptid_pair_t(0, 0, 3, 0, 0));
        receiptid_state3->add_pair(1, xreceiptid_pair_t(0, 0, 3, 0, 0));
        all_states.add_table_receiptid_state(3, receiptid_state3);                  
    }
    {
        auto info = all_states.get_unfinish_info();
        ASSERT_EQ(info.size(), 4);            
    }    
    {
        xreceiptid_state_ptr_t receiptid_state0 = std::make_shared<xreceiptid_state_t>(0, 100);
        receiptid_state0->add_pair(2, xreceiptid_pair_t(2, 0, 0, 2, 2));
        receiptid_state0->add_pair(3, xreceiptid_pair_t(3, 0, 0, 3, 3));        
        all_states.add_table_receiptid_state(0, receiptid_state0);
        xreceiptid_state_ptr_t receiptid_state1 = std::make_shared<xreceiptid_state_t>(1, 100);
        receiptid_state1->add_pair(2, xreceiptid_pair_t(2, 0, 0, 2, 2));
        receiptid_state1->add_pair(3, xreceiptid_pair_t(3, 0, 0, 3, 3));        
        all_states.add_table_receiptid_state(1, receiptid_state1);        
    }
    {
        auto info = all_states.get_unfinish_info();
        xassert(info.size() == 0);
        ASSERT_EQ(info.size(), 0);            
    }        
}