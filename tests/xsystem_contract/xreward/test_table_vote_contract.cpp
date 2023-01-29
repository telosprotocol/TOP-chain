#include <sstream>

#include <gtest/gtest.h>

#define private public

#include "xchain_fork/xfork_points.h"
#include "xcommon/xaccount_address.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xnative_contract_address.h"
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
        votes_table.insert(std::make_pair(account_address.to_string(), 10));
    }
    uint64_t node_total_votes = 100;
    EXPECT_THROW(calc_advance_tickets(build_account_address(account_prefix, 0), 0, votes_table, true, node_total_votes), std::runtime_error);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets2){
    // add normal
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
        auto account_address = build_account_address(account_prefix, i);
        votes_table.insert(std::make_pair(account_address.to_string(), 10));
    }
    uint64_t node_total_votes = 100;
    calc_advance_tickets(build_account_address(account_prefix, 0), 50, votes_table, true, node_total_votes);
    EXPECT_EQ(node_total_votes, 100+50);
    EXPECT_EQ(votes_table[build_account_address(account_prefix, 0).to_string()], 10 + 50);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets3){
    // delete node not in table
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
        auto account_address = build_account_address(account_prefix, i);
        votes_table.insert(std::make_pair(account_address.to_string(), 10));
    }
    uint64_t node_total_votes = 100;
    EXPECT_THROW(calc_advance_tickets(build_account_address(account_prefix, 101), 10, votes_table, false, node_total_votes), std::runtime_error);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets4){
    // delete votes larger than node votes
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
        auto account = build_account_address(account_prefix, i);
        votes_table.insert(std::make_pair(account.to_string(), 10));
    }
    uint64_t node_total_votes = 100;
    EXPECT_THROW(calc_advance_tickets(build_account_address(account_prefix, 0), 101, votes_table, false, node_total_votes), std::runtime_error);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets5){
    // delete votes larger than node votes
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
        auto account = build_account_address(account_prefix, i);
        votes_table.insert(std::make_pair(account.to_string(), 10));
    }
    uint64_t node_total_votes = 100;
    EXPECT_THROW(calc_advance_tickets(build_account_address(account_prefix, 0), 20, votes_table, false, node_total_votes), std::runtime_error);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets6){
    // delete normal
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
       auto account = build_account_address(account_prefix, i);
        votes_table.insert(std::make_pair(account.to_string(), 20));
    }
    uint64_t node_total_votes = 100;
    calc_advance_tickets(build_account_address(account_prefix, 0), 10, votes_table, false, node_total_votes);
    EXPECT_EQ(node_total_votes, 90);
    EXPECT_EQ(votes_table[build_account_address(account_prefix, 0).to_string()], 10);
}

TEST_F(xtest_table_vote_contract_t, test_calc_advance_tickets7){
    // delete node whose votes 0
    std::map<std::string, uint64_t> votes_table;
    for(int i = 0; i < 100; i++){
       auto account = build_account_address(account_prefix, i);
        votes_table.insert(std::make_pair(account.to_string(), 10));
    }
    uint64_t node_total_votes = 100;
    calc_advance_tickets(build_account_address(account_prefix, 0), 10, votes_table, false, node_total_votes);
    EXPECT_EQ(node_total_votes, 90);
    EXPECT_EQ(votes_table.count(build_account_address(account_prefix, 0).to_string()), 0);
}

static std::vector<std::string> voters = {
    "T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc",
    "T00000LfxdAPxPUrbYvDCkpijvicSQCXTBT7J7WW",
    "T00000LhLPkC9q7BfcjPpwWcb4pgZ3fHqTxdUywi",
    "T00000LWfXZgVPa8mmuDhxQjAtSgVaB9exKsQBWE",
    "T00000LNomDyH8kN2zQhexGbg4dMJY1ZbAAekZ8Y",
    "T00000LcYvxYB3EAnPcBo9hPybrDM2ZB5v5JFcyg",
    "T00000LZhrRcNKoisFNYcRk8yRkkBuT3S2XdqCWt",
    "T00000LTQPcfP2MnUstYLm2s2ABCRCjxSeL4NPmx",
};

static std::vector<std::string> advs = {
    "T00000LYhXYm8rrkfKkYrrjT8ibbFtdi1SJDrHRD",
    "T00000LM3v9AxCW2ek3hWM2XYe77BnGTyN3wjjnc",
    "T00000LV5bD34qXGux3c3qwVw4RBRCP6RdNsGt5H",
    "T00000LZEmHiD2LZaEAF9eAFoskB6BowtPUhKZSc",
    "T00000LNmcQCfjjk1Dj691L75egthyzyBAdkRtx4",
    "T00000LaYJvA3drXAhjzmke4dmS1WAAUKxXraAua",
    "T00000LMuaSSzFVvrktEZhuP64tZjY5nkKrAMhW3",
    "T00000LeqRDKSdLtWBiMXuJp615d1esfswuTUhwi",
    "T00000LWD9azGw3EP8neeHjNyVxH3EL69oEza8ms",
    "T00000LR2wTU2H7jVCWxFph5oWbaNZJwmWKx3Ef8",
    "T00000LgrChMEn7uYbzgQjn4gVgdzg2hse4huCoH",
    "T00000LfjqY18epcoddktGM2uCdTtyNtfe8pJ1QC",
    "T00000LLzbhskFTjShEk9FHSgeuyW9VyBonkKL15",
    "T00000LdbJ89sVheXwtoM13UCG3ufuYfrK4wzEUo",
    "T00000Lh3TbVKYCgjMWuBPZGxsJCHtJx6wiDhcdB",
    "T00000LfubbRqcxBhtzg1hFJQKqX5K22PX9opvpu",
    "T00000LQRemCBd4nFK7d2qFu5nx9Mx4xxYMKZqQV",
    "T00000LKph1D2S5ENsLy1nYgdJkZuL3oPrk1Bsmc",
    "T00000LKktoA7i2632q1uppi81DcYYAky36PCryN",
    "T00000LPvFYq8fayare1kf2KoUY4sZHPmgnDvLK7",
    "T00000LZPvkgLukkLUCAXqB7kKGpynqTbpBK4Giy",
    "T00000LMx5PFtPqtehptpKkX9KiLWYSDTKrUWdV9",
    "T00000Lfg4DdD4vgcgB5FpLoHcYC54YoRtN1fFmU",
    "T00000Lccfxfbm7wN2b2ZaYMVineDywz9XkzuEkz",
    "T00000Lh4MmMm8oVx9nXbicJLzDHP8bBpm6hKds6",
    "T00000LNQ28AGSay56YRGyjyPdP37D4u9eM1KT2N",
    "T00000LdLjByQuB4GQ3YJW16oczF6Uzfk2z93U3m",
    "T00000LXnKEfteKAJbVi6CeKQ4EDyTRvgvvCJYEh",
    "T00000LPdRp5nEAHs2ubNtfio3JXtzJEEujKMMJQ",
    "T00000LUwsGb2qukJWrz7nJWHutwnwek8uDgSe2C",
    "T00000LbHHPogMqHdvjxzLrJvWHvat3Np5hsZ3zC",
    "T00000LYUST38rkkHeFn7edrYQJWbjhnJDqs58sq",
};

    // m_registered_miner_type

    // virtual const data::xtablestate_ptr_t &     get_table_state() const = 0;
    // virtual data::xaccountstate_ptr_t           load_account_state(common::xaccount_address_t const& address) = 0;
    // virtual data::xunitstate_ptr_t  load_unit_state(common::xaccount_address_t const& address) = 0;
    // virtual data::xunitstate_ptr_t  load_commit_unit_state(common::xaccount_address_t const& address) {return nullptr;}  // TODO(jimmy) just for accountcontext
    // virtual data::xunitstate_ptr_t  load_commit_unit_state(common::xaccount_address_t const& address, uint64_t height) {return nullptr;}  // TODO(jimmy) just for accountcontext
    // virtual bool                    do_rollback() = 0;
    // virtual size_t                  do_snapshot() = 0;
    // virtual void                    do_commit(base::xvblock_t* current_block) {return;}  // TODO(jimmy) do commit changed state to db
    // virtual std::string             get_table_address() const = 0;
    // virtual bool                    is_state_dirty() const = 0;

class xmock_statectx_t : public statectx::xstatectx_face_t {
 public:
    xmock_statectx_t() {
        m_state = make_object_ptr<base::xvbstate_t>(sys_contract_rec_registration_addr, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        m_state->new_string_map_var(data::system_contract::XPORPERTY_CONTRACT_REG_KEY, canvas.get());
        for (auto adv : advs) {
            data::system_contract::xreg_node_info info;
            info.m_registered_miner_type = common::xenum_miner_type::advance;
            base::xstream_t stream(base::xcontext_t::instance());
            info.serialize_to(stream);
            m_state->load_string_map_var(data::system_contract::XPORPERTY_CONTRACT_REG_KEY)->insert(adv, {reinterpret_cast<char *>(stream.data()), static_cast<size_t>(stream.size())}, canvas.get());
        }
        m_unitstate = std::make_shared<data::xunit_bstate_t>(m_state.get(), false);
    }

    ~xmock_statectx_t() = default;

    const data::xtablestate_ptr_t & get_table_state() const override {
        assert(false);
        static data::xtablestate_ptr_t ptr{nullptr};
        return ptr;
     }
    data::xaccountstate_ptr_t  load_account_state(const common::xaccount_address_t & addr) override {
        assert(false);
        return nullptr;
    }
    data::xunitstate_ptr_t  load_unit_state(const common::xaccount_address_t & addr) override {
        assert(false);
        return nullptr;
    }
    data::xunitstate_ptr_t load_commit_unit_state(const common::xaccount_address_t & addr) {
        assert(addr.to_string() == sys_contract_rec_registration_addr);
        return m_unitstate;
    }
    data::xunitstate_ptr_t load_commit_unit_state(const common::xaccount_address_t & addr, uint64_t height) {
        assert(addr.to_string() == sys_contract_rec_registration_addr);
        return m_unitstate;
    }
    bool do_rollback() override {
        return false;
    }
    size_t do_snapshot() override {
        return 0;
    }
    void do_commit(base::xvblock_t * current_block) override {
        return;
    }
    std::string get_table_address() const override {
        return {};
    }
    bool is_state_dirty() const override {
        return false;
    }

    xobject_ptr_t<base::xvbstate_t> m_state{nullptr};
    data::xunitstate_ptr_t m_unitstate{nullptr};
};

class xtest_table_vote_contract_dev_t : public testing::Test {
public:
    xtest_table_vote_contract_dev_t() = default;
    ~xtest_table_vote_contract_dev_t() override = default;

    void init() {
        m_exe_addr = std::string{"T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc"};
        m_table_addr = std::string{sys_contract_sharding_vote_addr} + "@0";
        m_exe_addr = m_table_addr;
        m_contract_addr = common::xnode_id_t{m_table_addr};
        m_vbstate = make_object_ptr<xvbstate_t>(m_table_addr, 1 , 1, std::string{}, std::string{}, 0, 0, 0);
        m_unitstate = std::make_shared<xunit_bstate_t>(m_vbstate.get());
        m_statectx = std::make_shared<xmock_statectx_t>();
        m_account_index = std::make_shared<xaccount_context_t>(m_unitstate, m_statectx, 0);
        m_contract_helper = std::make_shared<xvm::xcontract_helper>(m_account_index.get(), m_contract_addr, m_exe_addr);
        contract.set_contract_helper(m_contract_helper);
        contract.setup();
    }

    void SetUp() override {
        init();
    }

    void TearDown() override {
    }

    std::string m_exe_addr;
    std::string m_table_addr;
    common::xnode_id_t m_contract_addr;
    xobject_ptr_t<xvbstate_t> m_vbstate{nullptr};
    std::shared_ptr<xunit_bstate_t> m_unitstate{nullptr};
    statectx::xstatectx_face_ptr_t m_statectx{nullptr};
    std::shared_ptr<xaccount_context_t> m_account_index{nullptr};
    std::shared_ptr<xvm::xcontract_helper> m_contract_helper{nullptr};
    xstake::xtable_vote_contract contract{common::xnetwork_id_t(255)};
};

TEST_F(xtest_table_vote_contract_dev_t, test_get_set_all_time_ineffective_votes) {
    for (auto i = 0; i < voters.size(); ++i) {
        auto voter = common::xaccount_address_t{voters[i]};
        auto all_time_ineffective_votes = contract.get_all_time_ineffective_votes(voter);
        EXPECT_TRUE(all_time_ineffective_votes.empty());
    }
    for (auto i = 0; i < voters.size(); ++i) {
        std::map<std::uint64_t, xtable_vote_contract::vote_info_map_t> all_time_ineffective_votes;
        xtable_vote_contract::vote_info_map_t map1;
        map1[advs[i * 4]] = i * 2;
        map1[advs[i * 4 + 1]] = i * 2 + 1;
        all_time_ineffective_votes.insert({i, map1});
        xtable_vote_contract::vote_info_map_t map2;
        map2[advs[i * 4 + 2]] = i * 2 + 2;
        map2[advs[i * 4 + 3]] = i * 2 + 3;
        all_time_ineffective_votes.insert({i + 1, map2});
        auto voter = common::xaccount_address_t{voters[i]};
        contract.set_all_time_ineffective_votes(voter, all_time_ineffective_votes);
    }
    for (auto i = 0; i < voters.size(); ++i) {
        auto voter = common::xaccount_address_t{voters[i]};
        auto v = contract.get_all_time_ineffective_votes(voter);
        EXPECT_EQ(v.size(), 2);
        EXPECT_EQ(v[i].size(), 2);
        EXPECT_EQ(v[i + 1].size(), 2);
        EXPECT_EQ(v[i][advs[i * 4]], i * 2);
        EXPECT_EQ(v[i][advs[i * 4 + 1]], i * 2 + 1);
        EXPECT_EQ(v[i + 1][advs[i * 4 + 2]], i * 2 + 2);
        EXPECT_EQ(v[i + 1][advs[i * 4 + 3]], i * 2 + 3);
    }
}

TEST_F(xtest_table_vote_contract_dev_t, test_add_all_time_ineffective_votes_new_time) {
    std::map<std::uint64_t, xtable_vote_contract::vote_info_map_t> all_time_ineffective_votes;
    {
        xtable_vote_contract::vote_info_map_t map1;
        map1[advs[0]] = 0;
        map1[advs[1]] = 1;
        all_time_ineffective_votes.insert({0, map1});
        xtable_vote_contract::vote_info_map_t map2;
        map2[advs[2]] = 2;
        map2[advs[3]] = 3;
        all_time_ineffective_votes.insert({1, map2});
    }
    xtable_vote_contract::vote_info_map_t map;
    map[advs[2]] = 2;
    map[advs[4]] = 4;
    contract.add_all_time_ineffective_votes(2, map, all_time_ineffective_votes);
    EXPECT_EQ(all_time_ineffective_votes.size(), 3);
    EXPECT_EQ(all_time_ineffective_votes[0].size(), 2);
    EXPECT_EQ(all_time_ineffective_votes[1].size(), 2);
    EXPECT_EQ(all_time_ineffective_votes[2].size(), 2);
    EXPECT_EQ(all_time_ineffective_votes[0][advs[0]], 0);
    EXPECT_EQ(all_time_ineffective_votes[0][advs[1]], 1);
    EXPECT_EQ(all_time_ineffective_votes[1][advs[2]], 2);
    EXPECT_EQ(all_time_ineffective_votes[1][advs[3]], 3);
    EXPECT_EQ(all_time_ineffective_votes[2][advs[2]], 2);
    EXPECT_EQ(all_time_ineffective_votes[2][advs[4]], 4);
}

TEST_F(xtest_table_vote_contract_dev_t, test_add_all_time_ineffective_votes_old_time) {
    std::map<std::uint64_t, xtable_vote_contract::vote_info_map_t> all_time_ineffective_votes;
    {
        xtable_vote_contract::vote_info_map_t map1;
        map1[advs[0]] = 0;
        map1[advs[1]] = 1;
        all_time_ineffective_votes.insert({0, map1});
        xtable_vote_contract::vote_info_map_t map2;
        map2[advs[2]] = 2;
        map2[advs[3]] = 3;
        all_time_ineffective_votes.insert({1, map2});
    }
    xtable_vote_contract::vote_info_map_t map;
    map[advs[2]] = 2;
    map[advs[4]] = 4;
    contract.add_all_time_ineffective_votes(1, map, all_time_ineffective_votes);
    EXPECT_EQ(all_time_ineffective_votes.size(), 2);
    EXPECT_EQ(all_time_ineffective_votes[0].size(), 2);
    EXPECT_EQ(all_time_ineffective_votes[1].size(), 3);
    EXPECT_EQ(all_time_ineffective_votes[0][advs[0]], 0);
    EXPECT_EQ(all_time_ineffective_votes[0][advs[1]], 1);
    EXPECT_EQ(all_time_ineffective_votes[1][advs[2]], 4);
    EXPECT_EQ(all_time_ineffective_votes[1][advs[3]], 3);
    EXPECT_EQ(all_time_ineffective_votes[1][advs[4]], 4);
}

TEST_F(xtest_table_vote_contract_dev_t, test_del_all_time_ineffective_votes_del_all) {
    std::map<std::uint64_t, xtable_vote_contract::vote_info_map_t> all_time_ineffective_votes;
    {
        xtable_vote_contract::vote_info_map_t map1;
        map1[advs[1]] = 1;
        map1[advs[2]] = 2;
        map1[advs[3]] = 3;
        map1[advs[4]] = 4;
        all_time_ineffective_votes.insert({0, map1});
        xtable_vote_contract::vote_info_map_t map2;
        map2[advs[2]] = 2;
        map2[advs[3]] = 3;
        map2[advs[4]] = 4;
        map2[advs[5]] = 5;
        map2[advs[6]] = 6;
        all_time_ineffective_votes.insert({1, map2});
        xtable_vote_contract::vote_info_map_t map3;
        map3[advs[4]] = 4;
        map3[advs[5]] = 5;
        map3[advs[6]] = 6;
        map3[advs[7]] = 7;
        map3[advs[8]] = 8;
        all_time_ineffective_votes.insert({2, map3});
    }
    xtable_vote_contract::vote_info_map_t map;
    map[advs[1]] = 1;
    map[advs[2]] = 4;
    map[advs[3]] = 6;
    map[advs[4]] = 12;
    map[advs[5]] = 10;
    map[advs[6]] = 12;
    map[advs[7]] = 7;
    map[advs[8]] = 8;
    contract.del_all_time_ineffective_votes(map, all_time_ineffective_votes);
    EXPECT_TRUE(map.empty());
    EXPECT_TRUE(all_time_ineffective_votes.empty());
}

TEST_F(xtest_table_vote_contract_dev_t, test_del_all_time_ineffective_votes_del_fix) {
    std::map<std::uint64_t, xtable_vote_contract::vote_info_map_t> all_time_ineffective_votes;
    {
        xtable_vote_contract::vote_info_map_t map1;
        map1[advs[1]] = 1;
        map1[advs[2]] = 2;
        map1[advs[3]] = 3;
        map1[advs[4]] = 4;
        all_time_ineffective_votes.insert({0, map1});
        xtable_vote_contract::vote_info_map_t map2;
        map2[advs[2]] = 2;
        map2[advs[3]] = 3;
        map2[advs[4]] = 4;
        map2[advs[5]] = 5;
        map2[advs[6]] = 6;
        all_time_ineffective_votes.insert({1, map2});
        xtable_vote_contract::vote_info_map_t map3;
        map3[advs[4]] = 4;
        map3[advs[5]] = 5;
        map3[advs[6]] = 6;
        map3[advs[7]] = 7;
        map3[advs[8]] = 8;
        all_time_ineffective_votes.insert({2, map3});
    }
    xtable_vote_contract::vote_info_map_t map;
    map[advs[1]] = 1;
    map[advs[2]] = 4;
    map[advs[3]] = 5;
    map[advs[4]] = 10;
    map[advs[5]] = 20;
    map[advs[6]] = 18;
    contract.del_all_time_ineffective_votes(map, all_time_ineffective_votes);
    EXPECT_EQ(map.size(), 2);
    EXPECT_EQ(map[advs[5]], 10);
    EXPECT_EQ(map[advs[6]], 6);
    EXPECT_EQ(all_time_ineffective_votes.size(), 2);
    EXPECT_EQ(all_time_ineffective_votes[0][advs[3]], 1);
    EXPECT_EQ(all_time_ineffective_votes[0][advs[4]], 2);
    EXPECT_EQ(all_time_ineffective_votes[2][advs[7]], 7);
    EXPECT_EQ(all_time_ineffective_votes[2][advs[8]], 8);
}

TEST_F(xtest_table_vote_contract_dev_t, test_vote) {
    for (auto i = 0; i < voters.size(); ++i) {
        std::map<std::uint64_t, xtable_vote_contract::vote_info_map_t> all_time_ineffective_votes;
        xtable_vote_contract::vote_info_map_t map1;
        map1[advs[i * 2]] = i * 2;
        map1[advs[i * 2 + 1]] = i * 2 + 1;
        all_time_ineffective_votes.insert({i, map1});
        xtable_vote_contract::vote_info_map_t map2;
        map2[advs[i * 2 + 2]] = i * 2 + 2;
        map2[advs[i * 2 + 3]] = i * 2 + 3;
        all_time_ineffective_votes.insert({i + 1, map2});
        auto voter = common::xaccount_address_t{voters[i]};
        contract.set_all_time_ineffective_votes(voter, all_time_ineffective_votes);
    }
    m_account_index->m_timer_height = 100;
    xtable_vote_contract::vote_info_map_t map;
    map[advs[3]] = 3;
    map[advs[4]] = 4;
    map[advs[5]] = 5;
    contract.set_vote_info_v2(common::xaccount_address_t{voters[0]}, map, true);
    for (auto i = 0; i < voters.size(); ++i) {
        auto voter = common::xaccount_address_t{voters[i]};
        auto v = contract.get_all_time_ineffective_votes(voter);
        if (i != 0) {
            EXPECT_EQ(v.size(), 2);
        } else {
            EXPECT_EQ(v.size(), 3);
            EXPECT_EQ(v[100].size(), 3);
            v[100][advs[3]] = 3;
            v[100][advs[4]] = 4;
            v[100][advs[5]] = 5;
        }
        EXPECT_EQ(v[i].size(), 2);
        EXPECT_EQ(v[i + 1].size(), 2);
        EXPECT_EQ(v[i][advs[i * 2]], i * 2);
        EXPECT_EQ(v[i][advs[i * 2 + 1]], i * 2 + 1);
        EXPECT_EQ(v[i + 1][advs[i * 2 + 2]], i * 2 + 2);
        EXPECT_EQ(v[i + 1][advs[i * 2 + 3]], i * 2 + 3);
    }
    EXPECT_NE(contract.STRING_GET2(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY), xstring_utl::tostring(1));
}

TEST_F(xtest_table_vote_contract_dev_t, test_unvote) {
    {
        xtable_vote_contract::vote_info_map_t map;
        map[advs[0]] = 1000;
        map[advs[1]] = 2000;
        map[advs[2]] = 3000;
        map[advs[3]] = 4000;
        map[advs[4]] = 5000;
        contract.handle_votes(common::xaccount_address_t{voters[0]}, map, true, true);
    }
    for (auto i = 0u; i < voters.size(); ++i) {
        std::map<common::xlogic_time_t, xtable_vote_contract::vote_info_map_t> all_time_ineffective_votes;
        xtable_vote_contract::vote_info_map_t map1;
        map1[advs[i * 2]] = i * 2;
        map1[advs[i * 2 + 1]] = i * 2 + 1;
        map1[advs[i * 2 + 2]] = i * 2 + 2;
        all_time_ineffective_votes.insert({i, map1});
        xtable_vote_contract::vote_info_map_t map2;
        map2[advs[i * 2 + 1]] = i * 2 + 1;
        map2[advs[i * 2 + 2]] = i * 2 + 2;
        map2[advs[i * 2 + 3]] = i * 2 + 3;
        all_time_ineffective_votes.insert({i + 1, map2});
        auto voter = common::xaccount_address_t{voters[i]};
        contract.set_all_time_ineffective_votes(voter, all_time_ineffective_votes);
    }
    xtable_vote_contract::vote_info_map_t map;
    map[advs[1]] = 100;
    map[advs[2]] = 3;
    map[advs[3]] = 4003;
    contract.set_vote_info_v2(common::xaccount_address_t{voters[0]}, map, false);
    for (auto i = 0; i < voters.size(); ++i) {
        auto voter = common::xaccount_address_t{voters[i]};
        auto v = contract.get_all_time_ineffective_votes(voter);
        if (i != 0) {
            EXPECT_EQ(v.size(), 2);
            EXPECT_EQ(v[i].size(), 3);
            EXPECT_EQ(v[i + 1].size(), 3);
            EXPECT_EQ(v[i][advs[i * 2]], i * 2);
            EXPECT_EQ(v[i][advs[i * 2 + 1]], i * 2 + 1);
            EXPECT_EQ(v[i][advs[i * 2 + 2]], i * 2 + 2);
            EXPECT_EQ(v[i + 1][advs[i * 2 + 1]], i * 2 + 1);
            EXPECT_EQ(v[i + 1][advs[i * 2 + 2]], i * 2 + 2);
            EXPECT_EQ(v[i + 1][advs[i * 2 + 3]], i * 2 + 3);
        } else {
            EXPECT_EQ(v.size(), 1);
            EXPECT_EQ(v[i].size(), 2);
            EXPECT_EQ(v[i][advs[i * 2]], i * 2);
            EXPECT_EQ(v[i][advs[i * 2 + 2]], 1);
        }
    }
    EXPECT_EQ(contract.get_advance_tickets(common::xaccount_address_t{advs[1]}), 1902);
    EXPECT_EQ(contract.get_advance_tickets(common::xaccount_address_t{advs[0]}), 1000);
    EXPECT_EQ(contract.get_advance_tickets(common::xaccount_address_t{advs[2]}), 3000);
    EXPECT_EQ(contract.get_advance_tickets(common::xaccount_address_t{advs[4]}), 5000);
    auto ret = contract.get_table_votes_detail(common::xaccount_address_t{voters[0]});
    EXPECT_EQ(ret.size(), 4);
    EXPECT_EQ(ret[advs[0]], 1000);
    EXPECT_EQ(ret[advs[1]], 1902);
    EXPECT_EQ(ret[advs[2]], 3000);
    EXPECT_EQ(ret[advs[3]], 0);
    // EXPECT_EQ(contract.STRING_GET2(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY), xtable_vote_contract::flag_withdraw_tickets_10900);
}

TEST_F(xtest_table_vote_contract_dev_t, test_get_and_update_all_effective_votes_of_all_account) {
    std::map<std::string, uint64_t> index;
    for (auto i = 0; i < voters.size(); ++i) {
        std::map<std::uint64_t, xtable_vote_contract::vote_info_map_t> all_time_ineffective_votes;
        xtable_vote_contract::vote_info_map_t map1;
        map1[advs[i * 2]] = i * 2;
        map1[advs[i * 2 + 1]] = i * 2 + 1;
        all_time_ineffective_votes.insert({100, map1});
        xtable_vote_contract::vote_info_map_t map2;
        map2[advs[i * 2 + 1]] = i * 2 + 1;
        map2[advs[i * 2 + 2]] = i * 2 + 2;
        map2[advs[i * 2 + 3]] = i * 2 + 3;
        all_time_ineffective_votes.insert({200, map2});
        xtable_vote_contract::vote_info_map_t map3;
        map3[advs[i * 2 + 4]] = i * 2 + 4;
        map3[advs[i * 2 + 5]] = i * 2 + 5;
        all_time_ineffective_votes.insert({300, map3});
        auto voter = common::xaccount_address_t{voters[i]};
        contract.set_all_time_ineffective_votes(voter, all_time_ineffective_votes);
        index.insert({voters[i], i});
    }

    auto h99 = contract.get_and_update_all_effective_votes_of_all_account(99 + xtable_vote_contract::ineffective_period);
    EXPECT_TRUE(h99.empty());
    auto h200 = contract.get_and_update_all_effective_votes_of_all_account(200 + xtable_vote_contract::ineffective_period);
    EXPECT_EQ(h200.size(), voters.size());
    for (auto p : h200) {
        EXPECT_EQ(p.second.size(), 4);
        auto i = index[p.first.to_string()];
        EXPECT_EQ(p.second[advs[i * 2]], i * 2);
        EXPECT_EQ(p.second[advs[i * 2 + 1]], 2 * (i * 2 + 1));
        EXPECT_EQ(p.second[advs[i * 2 + 2]], i * 2 + 2);
        EXPECT_EQ(p.second[advs[i * 2 + 3]], i * 2 + 3);
    }
    for (auto i = 0; i < voters.size(); ++i) {
        auto ret = contract.get_all_time_ineffective_votes(common::xaccount_address_t{voters[i]});
        EXPECT_EQ(ret.size(), 1);
        EXPECT_EQ(ret[300][advs[i * 2 + 4]], i * 2 + 4);
        EXPECT_EQ(ret[300][advs[i * 2 + 5]], i * 2 + 5);
    }
}

TEST_F(xtest_table_vote_contract_dev_t, test_on_timer) {
    contract.on_timer(10000);
    EXPECT_TRUE(m_account_index->m_contract_txs.empty());
}

TEST_F(xtest_table_vote_contract_dev_t, test_on_timer_not_fork) {
    auto fork_time = fork_points::v1_7_0_block_fork_point->point;
    {
        xtable_vote_contract::vote_info_map_t map;
        map[advs[0]] = 1000;
        map[advs[1]] = 2000;
        map[advs[2]] = 3000;
        map[advs[3]] = 4000;
        contract.handle_votes(common::xaccount_address_t{voters[0]}, map, true, true);
    }
    {
        m_account_index->m_timer_height = fork_time + 100;
        xtable_vote_contract::vote_info_map_t map1;
        map1[advs[0]] = 100;
        map1[advs[1]] = 200;
        map1[advs[2]] = 300;
        contract.set_vote_info_v2(common::xaccount_address_t{voters[0]}, map1, true);
        m_account_index->m_timer_height = 200;
        xtable_vote_contract::vote_info_map_t map2;
        map2[advs[1]] = 100;
        map2[advs[2]] = 200;
        map2[advs[3]] = 300;
        contract.set_vote_info_v2(common::xaccount_address_t{voters[0]}, map2, true);
    }
    {
        xtable_vote_contract::vote_info_map_t map1;
        map1[advs[0]] = 100;
        map1[advs[1]] = 100;
        map1[advs[2]] = 3500;
        contract.set_vote_info_v2(common::xaccount_address_t{voters[0]}, map1, false);
    }
    if (fork_time == 0) {
        return;
    }
    contract.on_timer(fork_time - 1);
    {
        auto ret = contract.get_all_time_ineffective_votes(common::xaccount_address_t{voters[0]});
        EXPECT_EQ(ret.size(), 2);
        EXPECT_EQ(ret[fork_time + 100].size(), 1);
        EXPECT_EQ(ret[fork_time + 200].size(), 1);
        EXPECT_EQ(ret[fork_time + 100][advs[1]], 200);
        EXPECT_EQ(ret[fork_time + 200][advs[3]], 300);
    }
    EXPECT_EQ(contract.MAP_SIZE(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY), 4);
    EXPECT_EQ(contract.get_advance_tickets(common::xaccount_address_t{advs[0]}), 1000);
    EXPECT_EQ(contract.get_advance_tickets(common::xaccount_address_t{advs[1]}), 2000);
    EXPECT_EQ(contract.get_advance_tickets(common::xaccount_address_t{advs[2]}), 3000);
    EXPECT_EQ(contract.get_advance_tickets(common::xaccount_address_t{advs[3]}), 4000);

    EXPECT_EQ(m_account_index->m_contract_txs.size(), 0);
    EXPECT_EQ(contract.STRING_GET2(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY), xstring_utl::tostring(1));
}

#if !defined(XBUILD_CI) && !defined(XBUILD_DEV) && !defined(XBUILD_GALILEO) && !defined(XBUILD_BOUNTY)
TEST_F(xtest_table_vote_contract_dev_t, test_vote_on_timer) {
    auto fork_time = fork_points::v1_7_0_block_fork_point->point;
    {
        xtable_vote_contract::vote_info_map_t map;
        map[advs[0]] = 1000;
        map[advs[1]] = 2000;
        map[advs[2]] = 3000;
        map[advs[3]] = 4000;
        contract.handle_votes(common::xaccount_address_t{voters[0]}, map, true, true);
    }
    {
        xtable_vote_contract::vote_info_map_t map;
        map[advs[0]] = 2000;
        map[advs[1]] = 4000;
        map[advs[2]] = 6000;
        map[advs[3]] = 8000;
        contract.handle_votes(common::xaccount_address_t{voters[1]}, map, true, true);
    }
    {
        m_account_index->m_timer_height = fork_time + 100;
        xtable_vote_contract::vote_info_map_t map1;
        map1[advs[0]] = 100;
        map1[advs[1]] = 200;
        map1[advs[2]] = 300;
        contract.set_vote_info_v2(common::xaccount_address_t{voters[0]}, map1, true);
        m_account_index->m_timer_height = fork_time + 200;
        xtable_vote_contract::vote_info_map_t map2;
        map2[advs[1]] = 100;
        map2[advs[2]] = 200;
        map2[advs[3]] = 300;
        contract.set_vote_info_v2(common::xaccount_address_t{voters[0]}, map2, true);
    }
    {
        m_account_index->m_timer_height = fork_time + 200;
        xtable_vote_contract::vote_info_map_t map1;
        map1[advs[0]] = 200;
        map1[advs[1]] = 400;
        map1[advs[2]] = 800;
        contract.set_vote_info_v2(common::xaccount_address_t{voters[1]}, map1, true);
        m_account_index->m_timer_height = fork_time + 300;
        xtable_vote_contract::vote_info_map_t map2;
        map2[advs[1]] = 200;
        map2[advs[2]] = 400;
        map2[advs[3]] = 800;
        contract.set_vote_info_v2(common::xaccount_address_t{voters[1]}, map2, true);
    }

    contract.on_timer(fork_time + 500);
    EXPECT_TRUE(m_account_index->m_contract_txs.empty());

    contract.on_timer(fork_time + 8640 + 200);
    {
        auto ret = contract.get_all_time_ineffective_votes(common::xaccount_address_t{voters[0]});
        EXPECT_TRUE(ret.empty());
        ret = contract.get_all_time_ineffective_votes(common::xaccount_address_t{voters[1]});
        EXPECT_EQ(ret.size(), 1);
        EXPECT_EQ(ret[fork_time + 300][advs[1]], 200);
        EXPECT_EQ(ret[fork_time + 300][advs[2]], 400);
        EXPECT_EQ(ret[fork_time + 300][advs[3]], 800);
    }
    EXPECT_EQ(contract.MAP_SIZE(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY), 4);
    EXPECT_EQ(contract.get_advance_tickets(common::xaccount_address_t{advs[0]}), 3300);
    EXPECT_EQ(contract.get_advance_tickets(common::xaccount_address_t{advs[1]}), 6700);
    EXPECT_EQ(contract.get_advance_tickets(common::xaccount_address_t{advs[2]}), 10300);
    EXPECT_EQ(contract.get_advance_tickets(common::xaccount_address_t{advs[3]}), 12300);

    std::string data;
    {
        std::map<std::string, std::string> adv_votes;
        contract.MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, adv_votes);
        base::xstream_t call_stream(base::xcontext_t::instance());
        call_stream << contract.TIME();
        call_stream << adv_votes;
        data = std::string((char *)call_stream.data(), call_stream.size());
    }

    EXPECT_EQ(m_account_index->m_contract_txs.size(), 2);
    EXPECT_EQ(to_string(m_account_index->m_contract_txs[0]->m_tx->get_target_action_para()), data);
    EXPECT_EQ(to_string(m_account_index->m_contract_txs[1]->m_tx->get_target_action_para()), data);

    EXPECT_EQ(contract.STRING_GET2(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY), xstring_utl::tostring(0));
}

TEST_F(xtest_table_vote_contract_dev_t, test_unvote_on_timer) {
    auto fork_time = fork_points::v1_7_0_block_fork_point->point;
    {
        xtable_vote_contract::vote_info_map_t map;
        map[advs[0]] = 1000;
        map[advs[1]] = 2000;
        map[advs[2]] = 3000;
        map[advs[3]] = 4000;
        contract.handle_votes(common::xaccount_address_t{voters[0]}, map, true, true);
    }
    {
        m_account_index->m_timer_height = fork_time + 100;
        xtable_vote_contract::vote_info_map_t map1;
        map1[advs[0]] = 100;
        map1[advs[1]] = 200;
        map1[advs[2]] = 300;
        contract.set_vote_info_v2(common::xaccount_address_t{voters[0]}, map1, true);
        m_account_index->m_timer_height = 200;
        xtable_vote_contract::vote_info_map_t map2;
        map2[advs[1]] = 100;
        map2[advs[2]] = 200;
        map2[advs[3]] = 300;
        contract.set_vote_info_v2(common::xaccount_address_t{voters[0]}, map2, true);
    }
    {
        xtable_vote_contract::vote_info_map_t map1;
        map1[advs[0]] = 100;
        map1[advs[1]] = 100;
        map1[advs[2]] = 3500;
        contract.set_vote_info_v2(common::xaccount_address_t{voters[0]}, map1, false);
    }
    contract.on_timer(fork_time + 500);
    {
        auto ret = contract.get_all_time_ineffective_votes(common::xaccount_address_t{voters[0]});
        EXPECT_EQ(ret.size(), 2);
        EXPECT_EQ(ret[fork_time + 100].size(), 1);
        EXPECT_EQ(ret[fork_time + 200].size(), 1);
        EXPECT_EQ(ret[fork_time + 100][advs[1]], 200);
        EXPECT_EQ(ret[fork_time + 200][advs[3]], 300);
    }
    EXPECT_EQ(contract.MAP_SIZE(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY), 3);
    EXPECT_EQ(contract.get_advance_tickets(common::xaccount_address_t{advs[0]}), 1000);
    EXPECT_EQ(contract.get_advance_tickets(common::xaccount_address_t{advs[1]}), 2000);
    EXPECT_EQ(contract.get_advance_tickets(common::xaccount_address_t{advs[3]}), 4000);

    std::string data;
    {
        std::map<std::string, std::string> adv_votes;
        contract.MAP_COPY_GET(data::system_contract::XPORPERTY_CONTRACT_POLLABLE_KEY, adv_votes);
        base::xstream_t call_stream(base::xcontext_t::instance());
        call_stream << contract.TIME();
        call_stream << adv_votes;
        data = std::string((char *)call_stream.data(), call_stream.size());
    }

    EXPECT_EQ(m_account_index->m_contract_txs.size(), 2);
    EXPECT_EQ(to_string(m_account_index->m_contract_txs[0]->m_tx->get_target_action_para()), data);
    EXPECT_EQ(to_string(m_account_index->m_contract_txs[1]->m_tx->get_target_action_para()), data);

    EXPECT_EQ(contract.STRING_GET2(data::system_contract::XPORPERTY_CONTRACT_TIME_KEY), xstring_utl::tostring(0));
}
#endif

TEST_F(xtest_table_vote_contract_dev_t, test_vote_bug) {
    auto voter = std::string{"T800004e1a8db34504e7b57a881746d0826661cc68f9a0"};
    auto adv = std::string{"T00000LYhXYm8rrkfKkYrrjT8ibbFtdi1SJDrHRD"};
    {
        xtable_vote_contract::vote_info_map_t map;
        map[adv] = 100;
        contract.handle_votes(common::xaccount_address_t{voter}, map, true, true);

        std::map<std::uint64_t, xtable_vote_contract::vote_info_map_t> all_time_ineffective_votes;
        xtable_vote_contract::vote_info_map_t map1;
        map1[adv] = 100;
        all_time_ineffective_votes.insert({100, map1});
        contract.set_all_time_ineffective_votes(common::xaccount_address_t{voter}, all_time_ineffective_votes);
    }
    xtable_vote_contract::vote_info_map_t map;
    map[adv] = 150;
    contract.set_vote_info_v2(common::xaccount_address_t{voter}, map, false);
}

#if defined(XCHAIN_FORKED_BY_DEFAULT) && (XCHAIN_FORKED_BY_DEFAULT >= 10901)

TEST_F(xtest_table_vote_contract_dev_t, test_reset_10901_invalid_flag) {
    EXPECT_FALSE(contract.reset_v10901(xtable_vote_contract::flag_reset_tickets, {}, {}));
    EXPECT_FALSE(contract.reset_v10901(xtable_vote_contract::flag_upload_tickets_10901, {}, {}));
    EXPECT_FALSE(contract.reset_v10901(xtable_vote_contract::flag_withdraw_tickets_10901, {}, {}));
    EXPECT_FALSE(contract.reset_v10901(xtable_vote_contract::flag_upload_tickets_10902, {}, {}));
    EXPECT_FALSE(contract.reset_v10901(xtable_vote_contract::flag_withdraw_tickets_10902, {}, {}));
}

TEST_F(xtest_table_vote_contract_dev_t, test_reset_10901_empty) {
    EXPECT_FALSE(contract.reset_v10901(xtable_vote_contract::flag_withdraw_tickets_10900, {}, {}));
    EXPECT_FALSE(contract.reset_v10901(xtable_vote_contract::flag_upload_tickets_10900, {}, {}));
}

TEST_F(xtest_table_vote_contract_dev_t, test_reset_10901_voter_not_in_the_contract_table) {
    common::xaccount_address_t const voter{"T80000580bb76ca47813e2bc99254e9aacd86c0bbc952e"};
    EXPECT_NE(contract.SELF_ADDRESS().table_id(), voter.table_id());

    std::set<common::xaccount_address_t> const recvers{
        common::xaccount_address_t{"T80000fbd7868c3466043e05c1222f9db1729f88e5e3b2"},
        common::xaccount_address_t{"T800008dbd8e60feb115b98376a2e851433826c8bf2029"},
        common::xaccount_address_t{"T8000050abf6ff490a0d4462b750bd6407d79fc3269941"},
        common::xaccount_address_t{"T800005f7aeb7a667783bd648d845d08811750689bb983"},
        common::xaccount_address_t{"T800001781d5537e2be3e9e788641ecebb5d7daa2da8fc"},
        common::xaccount_address_t{"T8000058be6c0769a23393ed929143be1cd28a01029137"},
        common::xaccount_address_t{"T80000bc6fab3aa01def9f204aa907248ce88557a278ab"},
        common::xaccount_address_t{"T8000073c78478b96c07c08dafc9f79df265cb8c398a54"},
        common::xaccount_address_t{"T8000007dda7e1a9feb5540439cfae7ba9f5822f9f0f97"},
        common::xaccount_address_t{"T8000029ccde5543798e7ec2cf515cd44e5d29eab7ad07"},
    };
    std::map<common::xaccount_address_t, uint64_t> voting_data;
    for (auto const & recver : recvers) {
        voting_data[recver] = 1024;
    }
    std::map<common::xaccount_address_t, std::map<std::string, uint64_t>> contract_ticket_reset_data;
    for (auto const & voting_datum : voting_data) {
        contract_ticket_reset_data[voter][top::get<common::xaccount_address_t const>(voting_datum).to_string()] = top::get<uint64_t>(voting_datum);
    }

    EXPECT_FALSE(contract.reset_v10901(xtable_vote_contract::flag_upload_tickets_10900, contract_ticket_reset_data, {}));
    EXPECT_FALSE(contract.reset_v10901(xtable_vote_contract::flag_upload_tickets_10900, {}, {voter}));

    contract.reset_table_tickets_data();
    auto const & table_tickets_data = contract.table_tickets_data();
    EXPECT_TRUE(table_tickets_data.empty());
}

TEST_F(xtest_table_vote_contract_dev_t, test_reset_10901_voter_in_the_contract_table_with_reset_data) {
    common::xaccount_address_t const voter{"T80000107948390bf9a99480e033a1869ce687c4fb2ab2"};
    EXPECT_EQ(contract.SELF_ADDRESS().table_id(), voter.table_id());

    std::set<common::xaccount_address_t> const recvers{
        common::xaccount_address_t{"T80000fbd7868c3466043e05c1222f9db1729f88e5e3b2"},
        common::xaccount_address_t{"T800008dbd8e60feb115b98376a2e851433826c8bf2029"},
        common::xaccount_address_t{"T8000050abf6ff490a0d4462b750bd6407d79fc3269941"},
        common::xaccount_address_t{"T800005f7aeb7a667783bd648d845d08811750689bb983"},
        common::xaccount_address_t{"T800001781d5537e2be3e9e788641ecebb5d7daa2da8fc"},
        common::xaccount_address_t{"T8000058be6c0769a23393ed929143be1cd28a01029137"},
        common::xaccount_address_t{"T80000bc6fab3aa01def9f204aa907248ce88557a278ab"},
        common::xaccount_address_t{"T8000073c78478b96c07c08dafc9f79df265cb8c398a54"},
        common::xaccount_address_t{"T8000007dda7e1a9feb5540439cfae7ba9f5822f9f0f97"},
        common::xaccount_address_t{"T8000029ccde5543798e7ec2cf515cd44e5d29eab7ad07"},
    };
    std::set<common::xaccount_address_t> const recvers2{
        common::xaccount_address_t{"T80000fbd7868c3466043e05c1222f9db1729f88e5e3b2"}, common::xaccount_address_t{"T800008dbd8e60feb115b98376a2e851433826c8bf2029"},
        common::xaccount_address_t{"T8000050abf6ff490a0d4462b750bd6407d79fc3269941"}, common::xaccount_address_t{"T800005f7aeb7a667783bd648d845d08811750689bb983"},
        common::xaccount_address_t{"T800001781d5537e2be3e9e788641ecebb5d7daa2da8fc"}, common::xaccount_address_t{"T8000058be6c0769a23393ed929143be1cd28a01029137"},
        common::xaccount_address_t{"T80000bc6fab3aa01def9f204aa907248ce88557a278ab"}, common::xaccount_address_t{"T8000073c78478b96c07c08dafc9f79df265cb8c398a54"},
        common::xaccount_address_t{"T8000007dda7e1a9feb5540439cfae7ba9f5822f9f0f97"}, common::xaccount_address_t{"T8000029ccde5543798e7ec2cf515cd44e5d29eab7ad07"},
        common::xaccount_address_t{"T800007ba800e2b66222b8c2216a537f08037eaba48345"}, common::xaccount_address_t{"T80000cd67caf4e93b66475e128c5f67e1c2608b52ec88"},
        common::xaccount_address_t{"T800007867400ffd924f7e71a399752e75f63767d35eae"}, common::xaccount_address_t{"T80000b8bd22d885be63f45072d9f7ee0a8dbcc866cb4e"},
        common::xaccount_address_t{"T80000a1f35712e0ad0da84913b55488b705cf1c4d27ed"}, common::xaccount_address_t{"T8000009a55d8ea4695d2bd25ed703dff6946bc4bec2ba"},
        common::xaccount_address_t{"T800008897108b78e01cc502b103e3506c0e71653bfe2c"}, common::xaccount_address_t{"T800003ee6d69b5eec6c6c9eebc9d2e31e11647df7be47"},
        common::xaccount_address_t{"T8000081fe06448ee13c520192942786fe6ab0e7a81a17"}, common::xaccount_address_t{"T80000af41b8e709454807801babe8da9153e51adcf41f"},
    };

    std::map<common::xaccount_address_t, uint64_t> expected_voting_data;
    for (auto const & recver : recvers) {
        expected_voting_data[recver] = 1024;
    }
    std::map<common::xaccount_address_t, std::map<std::string, uint64_t>> contract_ticket_reset_data;
    for (auto const & voting_datum : expected_voting_data) {
        contract_ticket_reset_data[voter][top::get<common::xaccount_address_t const>(voting_datum).to_string()] = top::get<uint64_t>(voting_datum);
    }

    {
        EXPECT_TRUE(contract.reset_v10901(xtable_vote_contract::flag_withdraw_tickets_10900, contract_ticket_reset_data, {}));
        auto const & tickets_data = contract.tickets_data(calc_voter_tickets_storage_property_name(voter));
        EXPECT_EQ(1, tickets_data.size());
        EXPECT_TRUE(tickets_data.find(voter) != std::end(tickets_data));
        auto const & voter_data = tickets_data.at(voter);
        EXPECT_EQ(voter_data.size(), recvers.size());
        EXPECT_TRUE(voter_data == expected_voting_data);

        contract.reset_table_tickets_data();
        auto const & table_tickets_data = contract.table_tickets_data();
        EXPECT_TRUE(table_tickets_data == expected_voting_data);
    }

    // reset again
    expected_voting_data.clear();
    contract_ticket_reset_data.clear();

    for (auto const & recver : recvers2) {
        expected_voting_data[recver] = 2048;
    }
    for (auto const & voting_datum : expected_voting_data) {
        contract_ticket_reset_data[voter][top::get<common::xaccount_address_t const>(voting_datum).to_string()] = top::get<uint64_t>(voting_datum);
    }
    {
        EXPECT_TRUE(contract.reset_v10901(xtable_vote_contract::flag_upload_tickets_10900, contract_ticket_reset_data, {}));
        auto const & tickets_data = contract.tickets_data(calc_voter_tickets_storage_property_name(voter));
        EXPECT_EQ(1, tickets_data.size());
        EXPECT_TRUE(tickets_data.find(voter) != std::end(tickets_data));
        auto const & voter_data = tickets_data.at(voter);
        EXPECT_EQ(voter_data.size(), recvers2.size());
        EXPECT_TRUE(voter_data == expected_voting_data);

        contract.reset_table_tickets_data();
        auto const & table_tickets_data = contract.table_tickets_data();
        EXPECT_TRUE(table_tickets_data == expected_voting_data);
    }
}

TEST_F(xtest_table_vote_contract_dev_t, test_reset_10901_voter_in_the_contract_table_with_clear_data) {
    common::xaccount_address_t const voter{"T80000107948390bf9a99480e033a1869ce687c4fb2ab2"};
    EXPECT_EQ(contract.SELF_ADDRESS().table_id(), voter.table_id());

    std::set<common::xaccount_address_t> const recvers{
        common::xaccount_address_t{"T80000fbd7868c3466043e05c1222f9db1729f88e5e3b2"},
        common::xaccount_address_t{"T800008dbd8e60feb115b98376a2e851433826c8bf2029"},
        common::xaccount_address_t{"T8000050abf6ff490a0d4462b750bd6407d79fc3269941"},
        common::xaccount_address_t{"T800005f7aeb7a667783bd648d845d08811750689bb983"},
        common::xaccount_address_t{"T800001781d5537e2be3e9e788641ecebb5d7daa2da8fc"},
        common::xaccount_address_t{"T8000058be6c0769a23393ed929143be1cd28a01029137"},
        common::xaccount_address_t{"T80000bc6fab3aa01def9f204aa907248ce88557a278ab"},
        common::xaccount_address_t{"T8000073c78478b96c07c08dafc9f79df265cb8c398a54"},
        common::xaccount_address_t{"T8000007dda7e1a9feb5540439cfae7ba9f5822f9f0f97"},
        common::xaccount_address_t{"T8000029ccde5543798e7ec2cf515cd44e5d29eab7ad07"},
    };

    std::map<common::xaccount_address_t, uint64_t> expected_voting_data;
    for (auto const & recver : recvers) {
        expected_voting_data[recver] = 1024;
    }
    std::map<common::xaccount_address_t, std::map<std::string, uint64_t>> contract_ticket_reset_data;
    for (auto const & voting_datum : expected_voting_data) {
        contract_ticket_reset_data[voter][top::get<common::xaccount_address_t const>(voting_datum).to_string()] = top::get<uint64_t>(voting_datum);
    }

    contract.reset_v10901(xtable_vote_contract::flag_withdraw_tickets_10900, contract_ticket_reset_data, {});

    contract.reset_table_tickets_data();
    auto const & table_tickets_data = contract.table_tickets_data();
    EXPECT_TRUE(table_tickets_data == expected_voting_data);

    EXPECT_TRUE(contract.reset_v10901(xtable_vote_contract::flag_upload_tickets_10900, {}, {voter}));
    auto const & tickets_data = contract.tickets_data(calc_voter_tickets_storage_property_name(voter));
    EXPECT_TRUE(tickets_data.empty());
    auto const & ineffective_data = contract.ineffective_data();
    EXPECT_TRUE(ineffective_data.find(voter) == std::end(ineffective_data));

    contract.reset_table_tickets_data();
    EXPECT_TRUE(contract.table_tickets_data().empty());
}

#endif

#if defined(XCHAIN_FORKED_BY_DEFAULT) && (XCHAIN_FORKED_BY_DEFAULT >= 10902)

TEST_F(xtest_table_vote_contract_dev_t, test_reset_10902_invalid_flag) {
    EXPECT_FALSE(contract.reset_v10902(xtable_vote_contract::flag_reset_tickets, {}, {}));
    EXPECT_FALSE(contract.reset_v10902(xtable_vote_contract::flag_upload_tickets_10900, {}, {}));
    EXPECT_FALSE(contract.reset_v10902(xtable_vote_contract::flag_withdraw_tickets_10900, {}, {}));
    EXPECT_FALSE(contract.reset_v10902(xtable_vote_contract::flag_upload_tickets_10902, {}, {}));
    EXPECT_FALSE(contract.reset_v10902(xtable_vote_contract::flag_withdraw_tickets_10902, {}, {}));
}

TEST_F(xtest_table_vote_contract_dev_t, test_reset_10902_empty) {
    EXPECT_FALSE(contract.reset_v10902(xtable_vote_contract::flag_withdraw_tickets_10901, {}, {}));
    EXPECT_FALSE(contract.reset_v10902(xtable_vote_contract::flag_upload_tickets_10901, {}, {}));
}

TEST_F(xtest_table_vote_contract_dev_t, test_reset_10902_voter_not_in_the_contract_table) {
    common::xaccount_address_t const voter{"T80000580bb76ca47813e2bc99254e9aacd86c0bbc952e"};
    EXPECT_NE(contract.SELF_ADDRESS().table_id(), voter.table_id());

    std::set<common::xaccount_address_t> const recvers{
        common::xaccount_address_t{"T80000fbd7868c3466043e05c1222f9db1729f88e5e3b2"},
        common::xaccount_address_t{"T800008dbd8e60feb115b98376a2e851433826c8bf2029"},
        common::xaccount_address_t{"T8000050abf6ff490a0d4462b750bd6407d79fc3269941"},
        common::xaccount_address_t{"T800005f7aeb7a667783bd648d845d08811750689bb983"},
        common::xaccount_address_t{"T800001781d5537e2be3e9e788641ecebb5d7daa2da8fc"},
        common::xaccount_address_t{"T8000058be6c0769a23393ed929143be1cd28a01029137"},
        common::xaccount_address_t{"T80000bc6fab3aa01def9f204aa907248ce88557a278ab"},
        common::xaccount_address_t{"T8000073c78478b96c07c08dafc9f79df265cb8c398a54"},
        common::xaccount_address_t{"T8000007dda7e1a9feb5540439cfae7ba9f5822f9f0f97"},
        common::xaccount_address_t{"T8000029ccde5543798e7ec2cf515cd44e5d29eab7ad07"},
    };
    std::map<common::xaccount_address_t, uint64_t> voting_data;
    for (auto const & recver : recvers) {
        voting_data[recver] = 1024;
    }
    std::map<common::xaccount_address_t, std::map<std::string, uint64_t>> contract_ticket_reset_data;
    for (auto const & voting_datum : voting_data) {
        contract_ticket_reset_data[voter][top::get<common::xaccount_address_t const>(voting_datum).to_string()] = top::get<uint64_t>(voting_datum);
    }

    EXPECT_FALSE(contract.reset_v10902(xtable_vote_contract::flag_upload_tickets_10901, contract_ticket_reset_data, {}));
    EXPECT_FALSE(contract.reset_v10902(xtable_vote_contract::flag_upload_tickets_10901, {}, {voter}));

    contract.reset_table_tickets_data();
    auto const & table_tickets_data = contract.table_tickets_data();
    EXPECT_TRUE(table_tickets_data.empty());
}

TEST_F(xtest_table_vote_contract_dev_t, test_reset_10902_voter_in_the_contract_table_with_reset_data) {
    common::xaccount_address_t const voter{"T80000107948390bf9a99480e033a1869ce687c4fb2ab2"};
    EXPECT_EQ(contract.SELF_ADDRESS().table_id(), voter.table_id());

    std::set<common::xaccount_address_t> const recvers{
        common::xaccount_address_t{"T80000fbd7868c3466043e05c1222f9db1729f88e5e3b2"},
        common::xaccount_address_t{"T800008dbd8e60feb115b98376a2e851433826c8bf2029"},
        common::xaccount_address_t{"T8000050abf6ff490a0d4462b750bd6407d79fc3269941"},
        common::xaccount_address_t{"T800005f7aeb7a667783bd648d845d08811750689bb983"},
        common::xaccount_address_t{"T800001781d5537e2be3e9e788641ecebb5d7daa2da8fc"},
        common::xaccount_address_t{"T8000058be6c0769a23393ed929143be1cd28a01029137"},
        common::xaccount_address_t{"T80000bc6fab3aa01def9f204aa907248ce88557a278ab"},
        common::xaccount_address_t{"T8000073c78478b96c07c08dafc9f79df265cb8c398a54"},
        common::xaccount_address_t{"T8000007dda7e1a9feb5540439cfae7ba9f5822f9f0f97"},
        common::xaccount_address_t{"T8000029ccde5543798e7ec2cf515cd44e5d29eab7ad07"},
    };
    std::set<common::xaccount_address_t> const recvers2{
        common::xaccount_address_t{"T80000fbd7868c3466043e05c1222f9db1729f88e5e3b2"}, common::xaccount_address_t{"T800008dbd8e60feb115b98376a2e851433826c8bf2029"},
        common::xaccount_address_t{"T8000050abf6ff490a0d4462b750bd6407d79fc3269941"}, common::xaccount_address_t{"T800005f7aeb7a667783bd648d845d08811750689bb983"},
        common::xaccount_address_t{"T800001781d5537e2be3e9e788641ecebb5d7daa2da8fc"}, common::xaccount_address_t{"T8000058be6c0769a23393ed929143be1cd28a01029137"},
        common::xaccount_address_t{"T80000bc6fab3aa01def9f204aa907248ce88557a278ab"}, common::xaccount_address_t{"T8000073c78478b96c07c08dafc9f79df265cb8c398a54"},
        common::xaccount_address_t{"T8000007dda7e1a9feb5540439cfae7ba9f5822f9f0f97"}, common::xaccount_address_t{"T8000029ccde5543798e7ec2cf515cd44e5d29eab7ad07"},
        common::xaccount_address_t{"T800007ba800e2b66222b8c2216a537f08037eaba48345"}, common::xaccount_address_t{"T80000cd67caf4e93b66475e128c5f67e1c2608b52ec88"},
        common::xaccount_address_t{"T800007867400ffd924f7e71a399752e75f63767d35eae"}, common::xaccount_address_t{"T80000b8bd22d885be63f45072d9f7ee0a8dbcc866cb4e"},
        common::xaccount_address_t{"T80000a1f35712e0ad0da84913b55488b705cf1c4d27ed"}, common::xaccount_address_t{"T8000009a55d8ea4695d2bd25ed703dff6946bc4bec2ba"},
        common::xaccount_address_t{"T800008897108b78e01cc502b103e3506c0e71653bfe2c"}, common::xaccount_address_t{"T800003ee6d69b5eec6c6c9eebc9d2e31e11647df7be47"},
        common::xaccount_address_t{"T8000081fe06448ee13c520192942786fe6ab0e7a81a17"}, common::xaccount_address_t{"T80000af41b8e709454807801babe8da9153e51adcf41f"},
    };

    std::map<common::xaccount_address_t, uint64_t> expected_voting_data;
    for (auto const & recver : recvers) {
        expected_voting_data[recver] = 1024;
    }
    std::map<common::xaccount_address_t, std::map<std::string, uint64_t>> contract_ticket_reset_data;
    for (auto const & voting_datum : expected_voting_data) {
        contract_ticket_reset_data[voter][top::get<common::xaccount_address_t const>(voting_datum).to_string()] = top::get<uint64_t>(voting_datum);
    }

    {
        EXPECT_TRUE(contract.reset_v10902(xtable_vote_contract::flag_withdraw_tickets_10901, contract_ticket_reset_data, {}));
        auto const & tickets_data = contract.tickets_data(calc_voter_tickets_storage_property_name(voter));
        EXPECT_EQ(1, tickets_data.size());
        EXPECT_TRUE(tickets_data.find(voter) != std::end(tickets_data));
        auto const & voter_data = tickets_data.at(voter);
        EXPECT_EQ(voter_data.size(), recvers.size());
        EXPECT_TRUE(voter_data == expected_voting_data);

        contract.reset_table_tickets_data();
        auto const & table_tickets_data = contract.table_tickets_data();
        EXPECT_TRUE(table_tickets_data == expected_voting_data);
    }

    // reset again
    expected_voting_data.clear();
    contract_ticket_reset_data.clear();

    for (auto const & recver : recvers2) {
        expected_voting_data[recver] = 2048;
    }
    for (auto const & voting_datum : expected_voting_data) {
        contract_ticket_reset_data[voter][top::get<common::xaccount_address_t const>(voting_datum).to_string()] = top::get<uint64_t>(voting_datum);
    }
    {
        EXPECT_TRUE(contract.reset_v10902(xtable_vote_contract::flag_upload_tickets_10901, contract_ticket_reset_data, {}));
        auto const & tickets_data = contract.tickets_data(calc_voter_tickets_storage_property_name(voter));
        EXPECT_EQ(1, tickets_data.size());
        EXPECT_TRUE(tickets_data.find(voter) != std::end(tickets_data));
        auto const & voter_data = tickets_data.at(voter);
        EXPECT_EQ(voter_data.size(), recvers2.size());
        EXPECT_TRUE(voter_data == expected_voting_data);

        contract.reset_table_tickets_data();
        auto const & table_tickets_data = contract.table_tickets_data();
        EXPECT_TRUE(table_tickets_data == expected_voting_data);
    }
}

TEST_F(xtest_table_vote_contract_dev_t, test_reset_10902_voter_in_the_contract_table_with_clear_data) {
    common::xaccount_address_t const voter{"T80000107948390bf9a99480e033a1869ce687c4fb2ab2"};
    EXPECT_EQ(contract.SELF_ADDRESS().table_id(), voter.table_id());

    std::set<common::xaccount_address_t> const recvers{
        common::xaccount_address_t{"T80000fbd7868c3466043e05c1222f9db1729f88e5e3b2"},
        common::xaccount_address_t{"T800008dbd8e60feb115b98376a2e851433826c8bf2029"},
        common::xaccount_address_t{"T8000050abf6ff490a0d4462b750bd6407d79fc3269941"},
        common::xaccount_address_t{"T800005f7aeb7a667783bd648d845d08811750689bb983"},
        common::xaccount_address_t{"T800001781d5537e2be3e9e788641ecebb5d7daa2da8fc"},
        common::xaccount_address_t{"T8000058be6c0769a23393ed929143be1cd28a01029137"},
        common::xaccount_address_t{"T80000bc6fab3aa01def9f204aa907248ce88557a278ab"},
        common::xaccount_address_t{"T8000073c78478b96c07c08dafc9f79df265cb8c398a54"},
        common::xaccount_address_t{"T8000007dda7e1a9feb5540439cfae7ba9f5822f9f0f97"},
        common::xaccount_address_t{"T8000029ccde5543798e7ec2cf515cd44e5d29eab7ad07"},
    };

    std::map<common::xaccount_address_t, uint64_t> expected_voting_data;
    for (auto const & recver : recvers) {
        expected_voting_data[recver] = 1024;
    }
    std::map<common::xaccount_address_t, std::map<std::string, uint64_t>> contract_ticket_reset_data;
    for (auto const & voting_datum : expected_voting_data) {
        contract_ticket_reset_data[voter][top::get<common::xaccount_address_t const>(voting_datum).to_string()] = top::get<uint64_t>(voting_datum);
    }

    contract.reset_v10902(xtable_vote_contract::flag_withdraw_tickets_10901, contract_ticket_reset_data, {});

    contract.reset_table_tickets_data();
    auto const & table_tickets_data = contract.table_tickets_data();
    EXPECT_TRUE(table_tickets_data == expected_voting_data);

    EXPECT_TRUE(contract.reset_v10902(xtable_vote_contract::flag_upload_tickets_10901, {}, {voter}));
    auto const & tickets_data = contract.tickets_data(calc_voter_tickets_storage_property_name(voter));
    EXPECT_TRUE(tickets_data.empty());
    auto const & ineffective_data = contract.ineffective_data();
    EXPECT_TRUE(ineffective_data.find(voter) == std::end(ineffective_data));

    contract.reset_table_tickets_data();
    EXPECT_TRUE(contract.table_tickets_data().empty());
}

#endif
