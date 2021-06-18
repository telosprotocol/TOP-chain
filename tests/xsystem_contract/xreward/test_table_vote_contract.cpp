#define private public
#include <gtest/gtest.h>
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/xreward/xtable_vote_contract.h"

using namespace top;
using namespace top::xstake;
using namespace top::contract;

class xtop_test_table_vote_contract : public xtable_vote_contract, public testing::Test {
    using xbase_t = xtable_vote_contract;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_test_table_vote_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_test_table_vote_contract); 

    xtop_test_table_vote_contract() : xtable_vote_contract(common::xnetwork_id_t{0}) {};

    xcontract_base * clone() override { return {}; }

    void exec(top::xvm::xvm_context * vm_ctx) { return; }

    static void SetUpTestCase() {
        top::config::config_register.get_instance().set(config::xmin_votes_num_onchain_goverance_parameter_t::name, std::to_string(1));
        top::config::config_register.get_instance().set(config::xmax_vote_nodes_num_onchain_goverance_parameter_t::name, std::to_string(10000));
    }
    static void TearDownTestCase() {}
};
using xtest_table_vote_contract_t = xtop_test_table_vote_contract;


TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets1){
    // add votes 0
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
        std::string account = std::string("node") + std::to_string(i);
        votes_table.insert(std::make_pair(account, 10));
    }
    uint64_t node_total_votes = 100;
    EXPECT_THROW(calc_advance_tickets(common::xaccount_address_t{"node0"}, 0, votes_table, true, node_total_votes), std::runtime_error);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets2){
    // add normal
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
        std::string account = std::string("node") + std::to_string(i);
        votes_table.insert(std::make_pair(account, 10));
    }
    uint64_t node_total_votes = 100;
    calc_advance_tickets(common::xaccount_address_t{"node0"}, 50, votes_table, true, node_total_votes);
    EXPECT_EQ(node_total_votes, 100+50);
    EXPECT_EQ(votes_table[std::string("node0")], 10+50);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets3){
    // delete node not in table
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
        std::string account = std::string("node") + std::to_string(i);
        votes_table.insert(std::make_pair(account, 10));
    }
    uint64_t node_total_votes = 100;
    EXPECT_THROW(calc_advance_tickets(common::xaccount_address_t{"node101"}, 10, votes_table, false, node_total_votes), std::runtime_error);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets4){
    // delete votes larger than node votes
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
        std::string account = std::string("node") + std::to_string(i);
        votes_table.insert(std::make_pair(account, 10));
    }
    uint64_t node_total_votes = 100;
    EXPECT_THROW(calc_advance_tickets(common::xaccount_address_t{"node0"}, 101, votes_table, false, node_total_votes), std::runtime_error);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets5){
    // delete votes larger than node votes
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
        std::string account = std::string("node") + std::to_string(i);
        votes_table.insert(std::make_pair(account, 10));
    }
    uint64_t node_total_votes = 100;
    EXPECT_THROW(calc_advance_tickets(common::xaccount_address_t{"node0"}, 20, votes_table, false, node_total_votes), std::runtime_error);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets6){
    // delete normal
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
        std::string account = std::string("node") + std::to_string(i);
        votes_table.insert(std::make_pair(account, 20));
    }
    uint64_t node_total_votes = 100;
    calc_advance_tickets(common::xaccount_address_t{"node0"}, 10, votes_table, false, node_total_votes);
    EXPECT_EQ(node_total_votes, 90);
    EXPECT_EQ(votes_table[std::string("node0")], 10);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets7){
    // delete node whose votes 0
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
        std::string account = std::string("node") + std::to_string(i);
        votes_table.insert(std::make_pair(account, 10));
    }
    uint64_t node_total_votes = 100;
    calc_advance_tickets(common::xaccount_address_t{"node0"}, 10, votes_table, false, node_total_votes);
    EXPECT_EQ(node_total_votes, 90);
    EXPECT_EQ(votes_table.count(std::string("node0")), 0);
}

