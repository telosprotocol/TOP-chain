#include "xbasic/xhex.h"
#include "xcommon/xnode_id.h"
#include "xdata/xtable_bstate.h"
#include "xdbstore/xstore_face.h"
#include "xutility/xhash.h"
#include "xvledger/xvledger.h"

#include <gtest/gtest.h>

#define private public
#include "xstate_sync/src/xstate_sync.cpp"

namespace top {

class xmock_vnetwork_driver_t : public vnetwork::xvnetwork_driver_face_t {
public:
    xmock_vnetwork_driver_t() = default;
    xmock_vnetwork_driver_t(xmock_vnetwork_driver_t const &) = delete;
    xmock_vnetwork_driver_t & operator=(xmock_vnetwork_driver_t const &) = delete;
    xmock_vnetwork_driver_t(xmock_vnetwork_driver_t &&) = delete;
    xmock_vnetwork_driver_t & operator=(xmock_vnetwork_driver_t &&) = delete;
    ~xmock_vnetwork_driver_t() override = default;

    void start() override {
    }

    void stop() override {
    }

    void register_message_ready_notify(common::xmessage_category_t const message_category, vnetwork::xvnetwork_message_ready_callback_t cb) override {
    }

    void unregister_message_ready_notify(common::xmessage_category_t const message_category) override {
    }

    common::xnetwork_id_t network_id() const noexcept override {
        return {};
    }

    vnetwork::xvnode_address_t address() const override {
        return {};
    }

    void send_to(vnetwork::xvnode_address_t const & to, vnetwork::xmessage_t const & message, std::error_code & ec) override {
        if (m_send_error) {
            return;
        }
        m_msg.emplace_back(std::make_pair(to, message));
    }

    void send_to(common::xip2_t const & to, vnetwork::xmessage_t const & message, std::error_code & ec) override {
    }

    void broadcast(common::xip2_t const & to, vnetwork::xmessage_t const & message, std::error_code & ec) override {
        }

    common::xnode_id_t const & host_node_id() const noexcept override {
        static common::xnode_id_t id;
        return id;
    }

    vnetwork::xvnode_address_t parent_group_address() const override {
        return {};
    }

    std::map<common::xslot_id_t, data::xnode_info_t> neighbors_info2() const override {
        return {};
    }

    std::map<common::xslot_id_t, data::xnode_info_t> parents_info2() const override {
        return {};
    }

    std::map<common::xslot_id_t, data::xnode_info_t> children_info2(common::xgroup_id_t const & gid, common::xelection_round_t const & election_round) const override {
        return {};
    }

    observer_ptr<vnetwork::xvhost_face_t> virtual_host() const noexcept override {
        return nullptr;
    }

    common::xnode_type_t type() const noexcept override {
        return {};
    }

    std::vector<common::xnode_address_t> archive_addresses(common::xnode_type_t node_type) const override {
        return {};
    }

    std::vector<common::xnode_address_t> fullnode_addresses(std::error_code & ec) const override {
        return {};
    }

    std::vector<common::xnode_address_t> relay_addresses(std::error_code & ec) const override {
        return {};
    }

    std::vector<std::uint16_t> table_ids() const override {
        return {};
    }

    common::xelection_round_t const & joined_election_round() const override {
        static common::xelection_round_t round;
        return round;
    }

    std::vector<std::pair<vnetwork::xvnode_address_t, vnetwork::xmessage_t>> m_msg;
    bool m_send_error{false};
};

class test_state_sync_fixture : public testing::Test {
public:
    void SetUp() override {
        base::xvchain_t::instance().clean_all(true);
        std::shared_ptr<db::xdb_face_t> db = db::xdb_factory_t::create_memdb();
        m_store = store::xstore_factory::create_store_with_static_kvdb(db);
        base::xvchain_t::instance().set_xdbstore(m_store.get());
        m_db = base::xvchain_t::instance().get_xdbstore();

        std::error_code ec;
        auto table_address = common::xaccount_address_t{"Ta0000@0"};
        auto table_height = 100;
        auto block_hash = from_hex("f631f8bcb19ee7f1c8b98e3f8b8f8d192d5b03df145bc1cff7d83c871467ced5", ec);
        auto state_hash = from_hex("c9575dbb2804b365a5967a9cb368ba21be4f4cba41e5f3dee51bd1f873428908", ec);
        auto state = from_hex("ccff40003500000044913111000100f60864640008546130303030403000000100b7ff00001b00001500a00f085461303030304030", ec);
        auto mismatch_state = from_hex("ccff4000210000007c131600000000000065650008546130303030403000000000", ec);
        // auto root_hash = from_hex();
        assert(!ec);

        std::vector<std::string> account_str = {
            "T80000000008bfc384175aafadd993e242d5db111541d7",
            "T8000000001093d5a2bb46193c436f75af330d883a5c3c",
            "T8000000001ec53e7ca1fb36f5868bf06882bf1558c2a7",
            "T8000000002e1959e66783436dd79c37e4ac649df3afb8",
            "T8000000003593c0a4cb298c23e50e3302f5895dfed842",
            "T80000000059b09b48f0594a60b4a69ba5ef50c4278e1b",
            "T8000000005bf8275aadb639ae5f5457555ae34094b53b",
            "T8000000005cc9e8992b8b979f20a12419c7dc76405bc0",
            "T8000000007962e8c844d69097469603cccb3e2498ee9b",
            "T80000000080520c87c59051284de75c732cc745c4ff49",
        };


        std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network = std::make_shared<xmock_vnetwork_driver_t>();
        std::vector<vnetwork::xvnode_address_t> peers;
        common::xgroup_address_t group{};
        for (auto i = 0; i < 10; i++) {
            auto account = common::xaccount_election_address_t(common::xaccount_address_t{account_str[i]}, common::xslot_id_t{i});
            peers.emplace_back(vnetwork::xvnode_address_t(group, account));
        }

        auto peers_func = [=](const common::xtable_id_t &) -> state_sync::sync_peers { return {network, peers}; };
        auto track_func = [this](const state_sync::state_req & req) { m_track_reqs.emplace_back(req); };

        m_syncer =
            state_sync::xstate_sync_t::new_state_sync(table_address, table_height, xhash256_t(block_hash), xhash256_t(state_hash), xhash256_t(), peers_func, track_func, m_db, true);
        m_state_key = base::xvdbkey_t::create_prunable_state_key(table_address.value(), table_height, {block_hash.begin(), block_hash.end()});
        m_state_bytes = state;
        m_mismatch_state_bytes = mismatch_state;
    }
    void TearDown() override {
    }

    xobject_ptr_t<store::xstore_face_t> m_store{nullptr};
    base::xvdbstore_t * m_db{nullptr};

    std::shared_ptr<state_sync::xstate_sync_t> m_syncer{nullptr};
    std::string m_state_key;
    xbytes_t m_state_bytes;
    xbytes_t m_mismatch_state_bytes;
    std::vector<state_sync::state_req> m_track_reqs;
};

TEST_F(test_state_sync_fixture, construct_data) {
    auto table_address = common::xaccount_address_t{"Ta0000@0"};
    auto table_bstate = make_object_ptr<base::xvbstate_t>(table_address.value(), 100, 100, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    {
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        table_bstate->new_string_var(table_address.value(), canvas.get());
        auto obj = table_bstate->load_string_var(table_address.value());
        obj->reset(table_address.value(), canvas.get());
    }
    std::string table_state_str;
    table_bstate->serialize_to_string(table_state_str);
    auto table_state = std::make_shared<data::xtable_bstate_t>(table_bstate.get());
    auto table_snapshot = table_state->take_snapshot();
    auto table_state_hash_str = base::xcontext_t::instance().hash(table_snapshot, enum_xhash_type_sha2_256);
    auto table_block_hash = utl::xkeccak256_t::digest(table_address.value());
    std::string table_block_hash_str((char *)table_block_hash.data(), table_block_hash.size());

    auto table_bstate2 = make_object_ptr<base::xvbstate_t>(table_address.value(), 101, 101, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    std::string table_state_str2;
    table_bstate2->serialize_to_string(table_state_str2);
    auto table_state2 = std::make_shared<data::xtable_bstate_t>(table_bstate2.get());
    auto table_snapshot2 = table_state2->take_snapshot();
    auto table_state_hash_str2 = base::xcontext_t::instance().hash(table_snapshot2, enum_xhash_type_sha2_256);

    printf("block_hash: %s\n", to_hex(table_block_hash_str).c_str());
    printf("state_hash: %s\n", to_hex(table_state_hash_str).c_str());
    printf("state: %s\n", to_hex(table_state_str).c_str());
    printf("mismatch_state_hash: %s\n", to_hex(table_state_hash_str2).c_str());
    printf("mismatch_state: %s\n", to_hex(table_state_str2).c_str());
}

TEST_F(test_state_sync_fixture, test_process_table_sucess) {
    state_sync::state_req req;
    req.type = state_sync::state_req_type::enum_state_req_table;
    req.nodes_response.emplace_back(m_state_bytes);
    std::error_code ec;
    m_syncer->process_table(req, ec);

    EXPECT_FALSE(ec);
    EXPECT_TRUE(m_syncer->m_sync_table_finish);
    auto v = m_db->get_value(m_state_key);
    EXPECT_EQ(to_bytes(v), m_state_bytes);
}

TEST_F(test_state_sync_fixture, test_process_table_type_mismatch) {
    state_sync::state_req req;
    req.type = state_sync::state_req_type::enum_state_req_trie;
    req.nodes_response.emplace_back(m_state_bytes);
    std::error_code ec;
    m_syncer->process_table(req, ec);

    EXPECT_FALSE(ec);
    EXPECT_FALSE(m_syncer->m_sync_table_finish);
    auto v = m_db->get_value(m_state_key);
    EXPECT_TRUE(v.empty());
}

TEST_F(test_state_sync_fixture, test_process_table_already_finish) {
    state_sync::state_req req;
    req.type = state_sync::state_req_type::enum_state_req_table;
    req.nodes_response.emplace_back(m_state_bytes);
    m_syncer->m_sync_table_finish = true;
    std::error_code ec;
    m_syncer->process_table(req, ec);

    EXPECT_FALSE(ec);
    EXPECT_TRUE(m_syncer->m_sync_table_finish);
    auto v = m_db->get_value(m_state_key);
    EXPECT_TRUE(v.empty());
}

TEST_F(test_state_sync_fixture, test_process_table_empty_response) {
    state_sync::state_req req;
    req.type = state_sync::state_req_type::enum_state_req_table;
    std::error_code ec;
    m_syncer->process_table(req, ec);

    EXPECT_FALSE(ec);
    EXPECT_FALSE(m_syncer->m_sync_table_finish);
    auto v = m_db->get_value(m_state_key);
    EXPECT_TRUE(v.empty());
}

TEST_F(test_state_sync_fixture, test_process_table_hash_mismatch) {
    state_sync::state_req req;
    req.type = state_sync::state_req_type::enum_state_req_table;
    req.nodes_response.emplace_back(m_mismatch_state_bytes);
    std::error_code ec;
    m_syncer->process_table(req, ec);

    EXPECT_FALSE(ec);
    EXPECT_FALSE(m_syncer->m_sync_table_finish);
    auto v = m_db->get_value(m_state_key);
    EXPECT_TRUE(v.empty());
}

}