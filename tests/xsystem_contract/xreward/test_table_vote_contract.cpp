#include <sstream>
#define private public
#include <gtest/gtest.h>
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/xreward/xtable_vote_contract.h"

using namespace top;
using namespace top::xstake;
using namespace top::contract;

static top::common::xaccount_address_t build_account_address(std::string const & account_prefix, size_t const index) {
    auto account_string = account_prefix + std::to_string(index);
    if (account_string.length() < top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH) {
        account_string.append(top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH - account_string.length(), 'x');
    }
    assert(account_string.length() == top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH);
    return common::xaccount_address_t{account_string};
}

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

std::string const account_prefix{"T00000LLyxLtWoTxRt"};

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets1){
    // add votes 0
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
        auto account_address = build_account_address(account_prefix, i);
        votes_table.insert(std::make_pair(account_address.value(), 10));
    }
    uint64_t node_total_votes = 100;
    EXPECT_THROW(calc_advance_tickets(build_account_address(account_prefix, 0), 0, votes_table, true, node_total_votes), std::runtime_error);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets2){
    // add normal
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
        auto account_address = build_account_address(account_prefix, i);
        votes_table.insert(std::make_pair(account_address.value(), 10));
    }
    uint64_t node_total_votes = 100;
    calc_advance_tickets(build_account_address(account_prefix, 0), 50, votes_table, true, node_total_votes);
    EXPECT_EQ(node_total_votes, 100+50);
    EXPECT_EQ(votes_table[build_account_address(account_prefix, 0).value()], 10 + 50);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets3){
    // delete node not in table
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
        auto account_address = build_account_address(account_prefix, i);
        votes_table.insert(std::make_pair(account_address.value(), 10));
    }
    uint64_t node_total_votes = 100;
    EXPECT_THROW(calc_advance_tickets(build_account_address(account_prefix, 101), 10, votes_table, false, node_total_votes), std::runtime_error);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets4){
    // delete votes larger than node votes
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
        auto account = build_account_address(account_prefix, i);
        votes_table.insert(std::make_pair(account.value(), 10));
    }
    uint64_t node_total_votes = 100;
    EXPECT_THROW(calc_advance_tickets(build_account_address(account_prefix, 0), 101, votes_table, false, node_total_votes), std::runtime_error);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets5){
    // delete votes larger than node votes
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
        auto account = build_account_address(account_prefix, i);
        votes_table.insert(std::make_pair(account.value(), 10));
    }
    uint64_t node_total_votes = 100;
    EXPECT_THROW(calc_advance_tickets(build_account_address(account_prefix, 0), 20, votes_table, false, node_total_votes), std::runtime_error);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets6){
    // delete normal
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
       auto account = build_account_address(account_prefix, i);
        votes_table.insert(std::make_pair(account.value(), 20));
    }
    uint64_t node_total_votes = 100;
    calc_advance_tickets(build_account_address(account_prefix, 0), 10, votes_table, false, node_total_votes);
    EXPECT_EQ(node_total_votes, 90);
    EXPECT_EQ(votes_table[build_account_address(account_prefix, 0).value()], 10);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets7){
    // delete node whose votes 0
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
       auto account = build_account_address(account_prefix, i);
        votes_table.insert(std::make_pair(account.value(), 10));
    }
    uint64_t node_total_votes = 100;
    calc_advance_tickets(build_account_address(account_prefix, 0), 10, votes_table, false, node_total_votes);
    EXPECT_EQ(node_total_votes, 90);
    EXPECT_EQ(votes_table.count(build_account_address(account_prefix, 0).value()), 0);
}

