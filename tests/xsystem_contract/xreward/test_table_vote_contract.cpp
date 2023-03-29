#include <sstream>

#include <gtest/gtest.h>

#define private public

#include "xchain_fork/xfork_points.h"
#include "xcommon/xaccount_address.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xnative_contract_address.h"
#include "xstore/xaccount_context.h"
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

static std::vector<std::string> validators = {
    "T80000b2a8697e7a72542637d1b6c696642ad97bec6feb", "T800007395208c7681bf66f74711ea41c766cd4bf1561a", "T80000382fc7b70e7c490f19a20f4a99402d51a984674a",
    "T80000a36a6d82fa55894ea36d7b043522b808fef45695", "T800009bd1a948541e82c66d9f408a29aa3d28a8501afc", "T80000cb9443e74cc8d362752c88f589c9d1d12feeb5a9",
    "T8000047af5ada6db181d000a0c90aaa4535e6b78c3804", "T80000a674a507fdd986ad2c40a90b8e9935c440d52893", "T80000bc1769315744f5dd54289f418374f9ef6b6864f5",
    "T800000c65dd5cce9d7f89079c72c37883a72186d4a3fc", "T80000ce1564070a34cea122a9e64d34ce7b19ef48bf8e", "T800007834238ad4f698e6a2689451468fb55e7b563d3b",
    "T80000c2a47a2ccd629b6677e0464ab4f61cfc54120e18", "T80000da9e3fa379cd18f396419968dd8f76fd76650c48", "T8000086ead9e12f1acccf291dc916bd20943f45b18695",
    "T8000019cd5e985a531ad03cfa078b770d33162c7a836b", "T80000421aee1ca67d86a95b7066d564a01208077b080f", "T80000ebfa4f6b30ac91f778a14ac2e2b066d1c347c38a",
    "T8000021318c50c2d830a6f8909d1586ed2a8b03cc4ee3", "T80000671d45285872bf2a2fbde0bc138b80a00a958eb0", "T8000011e5cb4b3ed7bc3b2aa2d70ea9c5c1c79e30d9e5",
    "T800002a7130d1de4984bebc04e5912317a758ebb4ab1d", "T80000265adc6da24906bb809abdaf05bbec8e70681fec", "T8000055364f052a0b56aa324e239df04117608159502a",
    "T80000fc67c750c620e2cca0829a29533f54a3d968e9f5", "T800001360a952acfdc3f8eac6f3733c04e912a423abda", "T80000359bcb3c1a761b373e5217c8047fe75367812e35",
    "T80000088f898caa66399039e4ef8f43dbe1af110a976a", "T80000a914cc9791057bd77d7d6744135c3e8d2a830533", "T80000caec82f6e880d37d8c6478591a0f317e4ae98b87",
    "T800006af5f308d4a9ee3d8f0989dc37d6129a894a20f6", "T800009d4560ba28bdb7d2bce559de6b6ffa676764c241",
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

class xmock_statectx_t final : public statectx::xstatectx_face_t {
 public:
    xmock_statectx_t() {
        m_state = make_object_ptr<base::xvbstate_t>(sys_contract_rec_registration_addr, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        m_state->new_string_map_var(data::system_contract::XPORPERTY_CONTRACT_REG_KEY, canvas.get());
        for (auto const & adv : advs) {
            data::system_contract::xreg_node_info info;
            info.miner_type(common::xenum_miner_type::advance);
            base::xstream_t stream(base::xcontext_t::instance());
            info.serialize_to(stream);
            m_state->load_string_map_var(data::system_contract::XPORPERTY_CONTRACT_REG_KEY)->insert(adv, {reinterpret_cast<char *>(stream.data()), static_cast<size_t>(stream.size())}, canvas.get());
        }
        for (auto const & val : validators) {
            data::system_contract::xreg_node_info info;
            info.miner_type(common::xenum_miner_type::validator);
            base::xstream_t stream(base::xcontext_t::instance());
            info.serialize_to(stream);
            m_state->load_string_map_var(data::system_contract::XPORPERTY_CONTRACT_REG_KEY)
                ->insert(val, {reinterpret_cast<char *>(stream.data()), static_cast<size_t>(stream.size())}, canvas.get());
        }
        m_unitstate = std::make_shared<data::xunit_bstate_t>(m_state.get(), m_state.get());
    }

    ~xmock_statectx_t() override = default;

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
    data::xunitstate_ptr_t load_commit_unit_state(const common::xaccount_address_t & addr) override {
        assert(addr.to_string() == sys_contract_rec_registration_addr);
        return m_unitstate;
    }
    data::xunitstate_ptr_t load_commit_unit_state(const common::xaccount_address_t & addr, uint64_t height) override {
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
    std::map<std::string, statectx::xunitstate_ctx_ptr_t> const& get_modified_unit_ctx() const override {
        return m_changed_ctxs;
    }

    std::map<std::string, statectx::xunitstate_ctx_ptr_t> m_changed_ctxs;
    xobject_ptr_t<base::xvbstate_t> m_state{nullptr};
    data::xunitstate_ptr_t m_unitstate{nullptr};
};

class xtest_table_vote_contract_dev_t : public testing::Test {
public:
    xtest_table_vote_contract_dev_t() = default;
    ~xtest_table_vote_contract_dev_t() override = default;

    void init() {
        m_exe_addr = top::common::xaccount_address_t::build_from(std::string{"T00000LWZ2K7Be3iMZwkLTZpi2saSmdp9AyWsCBc"});
        m_table_addr = common::xaccount_address_t::build_from(table_vote_contract_base_address, common::xtable_id_t{0});
        m_exe_addr = m_table_addr;
        m_contract_addr = m_table_addr;
        m_vbstate = make_object_ptr<xvbstate_t>(m_table_addr.to_string(), 1 , 1, std::string{}, std::string{}, 0, 0, 0);
        m_unitstate = std::make_shared<data::xunit_bstate_t>(m_vbstate.get());
        m_statectx = std::make_shared<xmock_statectx_t>();
        m_account_index = std::make_shared<store::xaccount_context_t>(m_unitstate, m_statectx, 0);
        m_contract_helper = std::make_shared<xvm::xcontract_helper>(m_account_index.get(), m_contract_addr, m_exe_addr);
        contract.set_contract_helper(m_contract_helper);
        contract.setup();
    }

    void SetUp() override {
        init();
    }

    void TearDown() override {
    }

    common::xaccount_address_t m_exe_addr;
    common::xaccount_address_t m_table_addr;
    common::xaccount_address_t m_contract_addr;
    xobject_ptr_t<xvbstate_t> m_vbstate{nullptr};
    std::shared_ptr<data::xunit_bstate_t> m_unitstate{nullptr};
    statectx::xstatectx_face_ptr_t m_statectx{nullptr};
    std::shared_ptr<store::xaccount_context_t> m_account_index{nullptr};
    std::shared_ptr<xvm::xcontract_helper> m_contract_helper{nullptr};
    xstake::xtable_vote_contract contract{common::xnetwork_id_t(255)};
};

TEST_F(xtest_table_vote_contract_dev_t, test_get_set_all_time_ineffective_votes) {
    for (auto i = 0u; i < voters.size(); ++i) {
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

TEST_F(xtest_table_vote_contract_dev_t, test_add_all_time_ineffective_votes_validator) {
    std::map<std::uint64_t, xtable_vote_contract::vote_info_map_t> all_time_ineffective_votes;
    xtable_vote_contract::vote_info_map_t map;
    map[validators[2]] = 2;
    map[validators[4]] = 4;
    EXPECT_THROW(contract.add_all_time_ineffective_votes(1, map, all_time_ineffective_votes), top::error::xtop_error_t);
    try {
        contract.add_all_time_ineffective_votes(1, map, all_time_ineffective_votes);
    } catch (top::error::xtop_error_t const & eh) {
        std::string const msg = eh.what();
        ASSERT_TRUE(msg.find("only auditor can be voted") != std::string::npos);
    }
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

std::string const legacy_flag{"9644333"};

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
    EXPECT_FALSE(contract.reset_v10902(legacy_flag, {}, {}));
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
    EXPECT_FALSE(contract.reset_v10902(xtable_vote_contract::flag_withdraw_tickets_10901, contract_ticket_reset_data, {}));
    EXPECT_FALSE(contract.reset_v10902(xtable_vote_contract::flag_withdraw_tickets_10901, {}, {voter}));
    EXPECT_FALSE(contract.reset_v10902(legacy_flag, contract_ticket_reset_data, {}));
    EXPECT_FALSE(contract.reset_v10902(legacy_flag, {}, {voter}));

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

    // reset again and again
    expected_voting_data.clear();
    contract_ticket_reset_data.clear();

    for (auto const & recver : recvers) {
        expected_voting_data[recver] = 1024;
    }
    for (auto const & voting_datum : expected_voting_data) {
        contract_ticket_reset_data[voter][top::get<common::xaccount_address_t const>(voting_datum).to_string()] = top::get<uint64_t>(voting_datum);
    }

    {
        EXPECT_TRUE(contract.reset_v10902(legacy_flag, contract_ticket_reset_data, {}));
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
