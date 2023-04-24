#include "test_state_sync_data.h"
#include "xdbstore/xstore_face.h"
#include "xevm_common/trie/xsecure_trie.h"
#include "xevm_common/xerror/xerror.h"
#include "xstate_sync/xerror.h"
#include "xutility/xhash.h"
#include "xvledger/xvledger.h"

#include <gtest/gtest.h>

#define private public
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"
#include "xstate_mpt/xstate_mpt.h"
#include "xstate_sync/xstate_sync.h"
#include "xvledger/xaccountindex.h"

namespace top {

class test_state_sync_fixture : public testing::Test {
public:
    void SetUp() override {
        base::xvchain_t::instance().clean_all(true);
        std::shared_ptr<db::xdb_face_t> db = db::xdb_factory_t::create_memdb();
        m_store = store::xstore_factory::create_store_with_static_kvdb(db);
        base::xvchain_t::instance().set_xdbstore(m_store.get());
        m_db = base::xvchain_t::instance().get_xdbstore();
        m_peers = peers_func(table_account_address.table_id());
        auto peers_func_impl = [&](const common::xtable_id_t &) -> state_sync::sync_peers { return m_peers; };
        auto track_func = [this](const state_sync::state_req & req) { m_track_reqs.emplace(std::make_pair(req.id, req)); };
        m_syncer = state_sync::xstate_sync_t::new_state_sync(common::xaccount_address_t::build_from(table_account_address.to_string()), table_height, block_hash, state_hash, root_hash, peers_func_impl, track_func, m_db, true);
    }
    void TearDown() override {
    }

    xobject_ptr_t<store::xstore_face_t> m_store{nullptr};
    base::xvdbstore_t * m_db{nullptr};

    std::shared_ptr<state_sync::xstate_sync_t> m_syncer{nullptr};
    state_sync::sync_peers m_peers;
    std::map<uint32_t, state_sync::state_req> m_track_reqs;

    void generate_state_mpt();
    void generate_table_state();

    void sync_helper();
    void sync_cancel_helper();
};

void test_state_sync_fixture::generate_state_mpt() {
    std::error_code ec;
    auto kv_db = std::make_shared<evm_common::trie::xkv_db_t>(m_db, table_account_address);
    auto trie_db = evm_common::trie::xtrie_db_t::NewDatabase(kv_db);
    auto trie = evm_common::trie::xsecure_trie_t::build_from({},trie_db,ec);
    EXPECT_FALSE(ec);

    // unit
    for (size_t i = 0; i < units_str.size(); i++) {
        auto bstate = make_object_ptr<base::xvbstate_t>(units_str[i], i + 1, i + 1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        bstate->new_string_var(std::to_string(i), canvas.get());
        auto obj = bstate->load_string_var(std::to_string(i));
        obj->reset(std::to_string(i), canvas.get());
        std::string unit_state_str;
        bstate->serialize_to_string(unit_state_str);
        auto unit_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
        auto snapshot = unit_state->take_snapshot();
        auto unit_state_hash_str = base::xcontext_t::instance().hash(snapshot, enum_xhash_type_sha2_256);
        auto unit_block_hash = utl::xkeccak256_t::digest(std::to_string(i));
        std::string unit_block_hash_str((char *)unit_block_hash.data(), unit_block_hash.size());
        base::xaccount_index_t index{base::enum_xaccountindex_version_snapshot_hash, i + 1, unit_block_hash_str, unit_state_hash_str, i + 1};
        state_mpt::xaccount_info_t info;
        info.account = common::xaccount_address_t(units_str[i]);
        info.index = index;
        auto info_str = info.encode();
        trie->update(to_bytes(units_str[i]), to_bytes(info_str));
        printf("unit, account: %s, value: %s, block_hash: %s, state_hash: %s, state: %s\n",
               units_str[i].c_str(),
               to_hex(info_str).c_str(),
               to_hex(unit_block_hash_str).c_str(),
               to_hex(unit_state_hash_str).c_str(),
               to_hex(unit_state_str).c_str());
    }
    auto trie_hash = trie->commit(ec);
    EXPECT_FALSE(ec);
    printf("hash: %s\n", to_hex(trie_hash.first).c_str());

    auto callback = [&](std::vector<xbytes_t> const & path, xbytes_t const & key, xbytes_t const & value, xh256_t const & req_hash, std::error_code & ec) {
        printf("on account key: %s, value: %s, req: %s\n", to_hex(key).c_str(), to_hex(value).c_str(), req_hash.hex().c_str());
    };
    auto sched = evm_common::trie::Sync::NewSync(kv_db);
    sched->Init(trie_hash.first, callback);

    std::vector<xh256_t> queue;
    auto res = sched->Missing(1);
    auto nodes = std::get<0>(res);
    queue.insert(queue.end(), nodes.begin(), nodes.end());
    while (queue.size() > 0) {
        for (auto q : queue) {
            auto v = trie_db->Node(q, ec);
            EXPECT_FALSE(ec);
            evm_common::trie::SyncResult result;
            result.Hash = xh256_t(q);
            result.Data = v;
            printf("node hash: %s, node value: %s\n", to_hex(result.Hash).c_str(), to_hex(result.Data).c_str());
            sched->Process(result, ec);
            EXPECT_FALSE(ec);
        }
        EXPECT_FALSE(ec);
        auto miss = sched->Missing(1);
        queue.clear();
        auto n = std::get<0>(miss);
        queue.insert(queue.end(), n.begin(), n.end());
    }
    sched->Commit(kv_db);
}

void test_state_sync_fixture::generate_table_state() {
    auto table_bstate = make_object_ptr<base::xvbstate_t>(table_account_address.to_string(), 100, 100, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    {
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        table_bstate->new_string_var(table_account_address.to_string(), canvas.get());
        auto obj = table_bstate->load_string_var(table_account_address.to_string());
        obj->reset(table_account_address.to_string(), canvas.get());
    }
    std::string table_state_str;
    table_bstate->serialize_to_string(table_state_str);
    auto table_state = std::make_shared<data::xtable_bstate_t>(table_bstate.get());
    auto table_snapshot = table_state->take_snapshot();
    auto table_state_hash_str = base::xcontext_t::instance().hash(table_snapshot, enum_xhash_type_sha2_256);
    auto table_block_hash = utl::xkeccak256_t::digest(table_account_address.to_string());
    std::string table_block_hash_str((char *)table_block_hash.data(), table_block_hash.size());

    auto table_bstate2 = make_object_ptr<base::xvbstate_t>(table_account_address.to_string(), 101, 101, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
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

void test_state_sync_fixture::sync_helper() {
    auto mock_net = std::dynamic_pointer_cast<xmock_vnetwork_driver_t>(m_peers.network);
    int node_cnt{0};
    while (true) {
        if (mock_net->m_msg.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        auto m = mock_net->m_msg.front();
        mock_net->m_msg.pop();

        if (m.second.id() == xmessage_id_sync_table_request) {
            base::xstream_t stream(base::xcontext_t::instance(), const_cast<uint8_t *>(m.second.payload().data()), (uint32_t)m.second.payload().size());
            std::string table;
            uint64_t height{0};
            xbytes_t hash;
            uint32_t id;
            stream >> table;
            stream >> height;
            stream >> hash;
            stream >> id;
            std::error_code ec;
            auto req = m_track_reqs.at(id);
            if (table == table_account_address.to_string() && height == table_height && xh256_t(hash) == block_hash) {
                req.nodes_response.emplace_back(state_bytes);
            }
            m_syncer->deliver_req(req);
        } else if (m.second.id() == xmessage_id_sync_trie_request) {
            base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)(m.second.payload().data()), (uint32_t)m.second.payload().size());
            std::string table;
            uint32_t id{0};
            std::vector<xbytes_t> nodes_bytes;
            std::vector<xbytes_t> units_bytes;
            stream >> table;
            stream >> id;
            stream >> nodes_bytes;
            stream >> units_bytes;
            auto req = m_track_reqs.at(id);
            for (auto it = nodes_bytes.begin(); it != nodes_bytes.end(); ++it) {
                req.nodes_response.emplace_back(from_hex(node_map.at(to_hex((*it)))));
                node_cnt++;
            }
            for (auto it = units_bytes.begin(); it != units_bytes.end(); ++it) {
                req.units_response.emplace_back(from_hex(unit_sync_map.at(to_hex((*it)))));
                node_cnt++;
            }
            m_syncer->deliver_req(req);
            if (node_cnt >= 75) {
                break;
            }
        } else {
            assert(false);
        }
    }
}

void test_state_sync_fixture::sync_cancel_helper() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    m_syncer->cancel();
}

// TEST_F(test_state_sync_fixture, generate_table_state) {
//     generate_table_state();
// }

// TEST_F(test_state_sync_fixture, generate_state_mpt) {
//     generate_state_mpt();
// }

TEST_F(test_state_sync_fixture, test_process_node_data_sucess) {
    std::error_code ec;
    auto res = m_syncer->m_sched->Missing(1);
    auto const & nodes = std::get<0>(res);
    auto blob = node_map[nodes[0].hex()];
    auto hash = m_syncer->process_node_data(to_bytes(from_hex(blob)), ec);
    EXPECT_FALSE(ec);
    EXPECT_EQ(hash, nodes[0]);
    EXPECT_FALSE(ec);
}

TEST_F(test_state_sync_fixture, test_process_node_data_error) {
    std::error_code ec;
    m_syncer->m_sched->Missing(1);
    auto hash = m_syncer->process_node_data(to_bytes(std::string("1234")), ec);
    EXPECT_EQ(ec, make_error_code(evm_common::error::xerrc_t::trie_sync_not_requested));
}

TEST_F(test_state_sync_fixture, test_process_unit_data_sucess) {
    std::error_code ec;
    while (true) {
        auto res = m_syncer->m_sched->Missing(1);
        auto const & nodes = std::get<0>(res);
        auto const & units = std::get<1>(res);
        if (nodes.empty() && units.empty()) {
            break;
        }
        if (!nodes.empty()) {
            auto const & node = nodes[0];
            auto blob = node_map[node.hex()];
            m_syncer->process_node_data(to_bytes(from_hex(blob)), ec);
            EXPECT_FALSE(ec);
        }
        if (!units.empty()) {
            auto const & unit = units[0];
            auto blob = unit_map[unit.hex()];
            auto hash = m_syncer->process_unit_data(to_bytes(from_hex(blob)), 0, ec);
            EXPECT_EQ(hash, unit);
            EXPECT_FALSE(ec);
            break;
        }
    }
}

TEST_F(test_state_sync_fixture, test_process_unit_data_error) {
    std::error_code ec;
    auto hash = m_syncer->process_unit_data(state_bytes, 0, ec);
    EXPECT_EQ(ec, make_error_code(evm_common::error::xerrc_t::trie_sync_not_requested));
    hash = m_syncer->process_unit_data(state_bytes, 1, ec);
    EXPECT_EQ(ec, make_error_code(evm_common::error::xerrc_t::trie_sync_not_requested));    
}

TEST_F(test_state_sync_fixture, test_process_trie_success) {
    std::error_code ec;
    while (true) {
        auto res = m_syncer->m_sched->Missing(1);
        auto const & nodes = std::get<0>(res);
        auto const & units = std::get<1>(res);
        auto const & unit_keys = std::get<2>(res);
        if (nodes.empty() && units.empty()) {
            break;
        }
        state_sync::state_req req;
        req.type = state_sync::state_req_type::enum_state_req_trie;
        if (!nodes.empty()) {
            state_sync::state_req req;
            auto const & node = nodes[0];
            auto blob = node_map[node.hex()];
            req.trie_tasks.emplace(node);
            req.nodes_response.emplace_back(to_bytes(from_hex(blob)));
            m_syncer->process_trie(req, ec);
            EXPECT_FALSE(ec);
        }
        if (!units.empty()) {
            state_sync::state_req req;
            auto const & unit = units[0];
            auto blob = unit_map[unit.hex()];
            req.unit_tasks.emplace(std::make_pair(unit, xbytes_t{}));
            req.units_response.emplace_back(to_bytes(from_hex(blob)));
            m_syncer->process_trie(req, ec);
            EXPECT_FALSE(ec);
            break;
        }
    }
}

TEST_F(test_state_sync_fixture, test_process_trie_type_mismatch) {
    state_sync::state_req req;
    req.type = state_sync::state_req_type::enum_state_req_table;

    std::error_code ec;
    req.trie_tasks.emplace(xh256_t(from_hex(node_map.begin()->first)));
    req.unit_tasks.emplace(std::make_pair(xh256_t(from_hex(unit_map.begin()->first)), from_hex(unit_sync_map.begin()->first)));
    req.nodes_response.emplace_back(from_hex(node_map.begin()->second));
    req.units_response.emplace_back(from_hex(unit_map.begin()->second));
    
    m_syncer->process_trie(req, ec);
    EXPECT_TRUE(req.trie_tasks.count(xh256_t(from_hex(node_map.begin()->first))));
    EXPECT_TRUE(req.unit_tasks.count(xh256_t(from_hex(unit_map.begin()->first))));
    EXPECT_TRUE(m_syncer->m_trie_tasks.empty());
    EXPECT_TRUE(m_syncer->m_unit_tasks.empty());
    EXPECT_FALSE(ec);
}

// TEST_F(test_state_sync_fixture, test_process_trie_not_found) {
//     state_sync::state_req req;
//     req.type = state_sync::state_req_type::enum_state_req_trie;

//     std::error_code ec;
//     req.trie_tasks.emplace(xh256_t(from_hex(node_map.begin()->first)));
//     req.unit_tasks.emplace(std::make_pair(xh256_t(from_hex(unit_map.begin()->first)), from_hex(unit_sync_map.begin()->first)));
//     req.nodes_response.emplace_back(from_hex(node_map.begin()->second));
//     req.units_response.emplace_back(from_hex(unit_map.begin()->second));
    
//     m_syncer->process_trie(req, ec);
//     EXPECT_FALSE(req.trie_tasks.count(xh256_t(from_hex(node_map.begin()->first))));
//     EXPECT_FALSE(req.unit_tasks.count(xh256_t(from_hex(unit_map.begin()->first))));
//     EXPECT_TRUE(m_syncer->m_trie_tasks.empty());
//     EXPECT_TRUE(m_syncer->m_unit_tasks.empty());
//     EXPECT_FALSE(ec);
// }

TEST_F(test_state_sync_fixture, test_process_table_sucess) {
    state_sync::state_req req;
    req.type = state_sync::state_req_type::enum_state_req_table;
    req.nodes_response.emplace_back(state_bytes);
    std::error_code ec;
    m_syncer->process_table(req, ec);

    EXPECT_FALSE(ec);
    EXPECT_TRUE(m_syncer->m_sync_table_finish);
    auto state_key = base::xvdbkey_t::create_prunable_state_key(table_account_address.to_string(), table_height, {block_hash.begin(), block_hash.end()});
    auto v = m_db->get_value(state_key);
    EXPECT_EQ(to_bytes(v), state_bytes);
}

TEST_F(test_state_sync_fixture, test_fill_tasks_use_left_node_task) {
    state_sync::state_req req;
    std::vector<xh256_t> nodes;
    std::vector<xbytes_t> units;
    m_syncer->m_trie_tasks.emplace(xh256_t(xbytes_t(32, 1)));
    m_syncer->fill_tasks(2, 2, req, nodes, units);

    EXPECT_EQ(nodes.size(), 2);
    EXPECT_EQ(nodes[0], xh256_t(xbytes_t(32, 1)));
    EXPECT_EQ(nodes[1], root_hash);
    EXPECT_TRUE(units.empty());
    EXPECT_EQ(req.n_items, 2);
}

TEST_F(test_state_sync_fixture, test_fill_tasks_use_left_unit_task) {
    state_sync::state_req req;
    std::vector<xh256_t> nodes;
    std::vector<xbytes_t> units;
    m_syncer->m_unit_tasks.emplace(std::make_pair(xh256_t(xbytes_t(32, 1)), xbytes_t(32, 2)));
    m_syncer->fill_tasks(2, 2, req, nodes, units);

    EXPECT_EQ(nodes.size(), 1);
    EXPECT_EQ(units.size(), 1);
    EXPECT_EQ(nodes[0], root_hash);
    EXPECT_EQ(units[0], xbytes_t(32, 2));
    EXPECT_EQ(req.n_items, 2);
}

TEST_F(test_state_sync_fixture, test_fill_tasks_not_use_left_task) {
    state_sync::state_req req;
    std::vector<xh256_t> nodes;
    std::vector<xbytes_t> units;
    m_syncer->m_trie_tasks.emplace(xh256_t(xbytes_t(32, 1)));
    m_syncer->m_unit_tasks.emplace(std::make_pair(xh256_t(xbytes_t(32, 2)), xbytes_t(32, 3)));
    m_syncer->fill_tasks(1, 1, req, nodes, units);

    EXPECT_EQ(nodes.size(), 1);
    EXPECT_EQ(nodes[0], xh256_t(xbytes_t(32, 1)));
    EXPECT_TRUE(units.empty());
    EXPECT_EQ(req.n_items, 1);
}

TEST_F(test_state_sync_fixture, test_assign_trie_tasks) {
    m_syncer->m_req_sequence_id = 100;
    m_syncer->assign_trie_tasks(m_peers);
    EXPECT_EQ(m_track_reqs.size(), 1);
    auto req = m_track_reqs.at(100);
    EXPECT_EQ(req.type, state_sync::state_req_type::enum_state_req_trie);
    EXPECT_EQ(req.id, 100);
    EXPECT_EQ(m_syncer->m_req_sequence_id, 101);
    auto it = std::find(m_peers.peers.begin(), m_peers.peers.end(), req.peer);
    EXPECT_TRUE(it != m_peers.peers.end());
    auto mock_net = std::dynamic_pointer_cast<xmock_vnetwork_driver_t>(m_peers.network);
    EXPECT_EQ(mock_net->m_msg.size(), 1);
    EXPECT_TRUE(mock_net->m_msg.front().first == (*it));
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)(mock_net->m_msg.front().second.payload().data()), (uint32_t)mock_net->m_msg.front().second.payload().size());
    std::string table;
    uint32_t id{0};
    std::vector<xbytes_t> nodes_bytes;
    std::vector<xbytes_t> units_bytes;
    stream >> table;
    stream >> id;
    stream >> nodes_bytes;
    stream >> units_bytes;
    EXPECT_EQ(table, table_account_address.to_string());
    EXPECT_EQ(id, 100);
    EXPECT_EQ(nodes_bytes[0], root_hash.to_bytes());
    EXPECT_EQ(nodes_bytes.size(), 1);
    EXPECT_EQ(units_bytes.size(), 0);
}

TEST_F(test_state_sync_fixture, test_assign_trie_tasks_empty) {
    state_sync::state_req req;
    std::vector<xh256_t> nodes;
    std::vector<xbytes_t> units;
    m_syncer->fill_tasks(1, 1, req, nodes, units);
    m_syncer->assign_trie_tasks(m_peers);
    EXPECT_TRUE(m_track_reqs.empty());
    auto mock_net = std::dynamic_pointer_cast<xmock_vnetwork_driver_t>(m_peers.network);
    EXPECT_TRUE(mock_net->m_msg.empty());
}

TEST_F(test_state_sync_fixture, test_assign_trie_tasks_cancel) {
    m_syncer->m_cancel = true;
    m_syncer->assign_trie_tasks(m_peers);
    EXPECT_TRUE(m_track_reqs.empty());
    auto mock_net = std::dynamic_pointer_cast<xmock_vnetwork_driver_t>(m_peers.network);
    EXPECT_TRUE(mock_net->m_msg.empty());
}

TEST_F(test_state_sync_fixture, test_process_table_type_mismatch) {
    state_sync::state_req req;
    req.type = state_sync::state_req_type::enum_state_req_trie;
    req.nodes_response.emplace_back(state_bytes);
    std::error_code ec;
    m_syncer->process_table(req, ec);

    EXPECT_FALSE(ec);
    EXPECT_FALSE(m_syncer->m_sync_table_finish);
    auto state_key = base::xvdbkey_t::create_prunable_state_key(table_account_address.to_string(), table_height, {block_hash.begin(), block_hash.end()});
    auto v = m_db->get_value(state_key);
    EXPECT_TRUE(v.empty());
}

TEST_F(test_state_sync_fixture, test_process_table_already_finish) {
    state_sync::state_req req;
    req.type = state_sync::state_req_type::enum_state_req_table;
    req.nodes_response.emplace_back(state_bytes);
    m_syncer->m_sync_table_finish = true;
    std::error_code ec;
    m_syncer->process_table(req, ec);

    EXPECT_FALSE(ec);
    EXPECT_TRUE(m_syncer->m_sync_table_finish);
    auto state_key = base::xvdbkey_t::create_prunable_state_key(table_account_address.to_string(), table_height, {block_hash.begin(), block_hash.end()});
    auto v = m_db->get_value(state_key);
    EXPECT_TRUE(v.empty());
}

TEST_F(test_state_sync_fixture, test_process_table_empty_response) {
    state_sync::state_req req;
    req.type = state_sync::state_req_type::enum_state_req_table;
    std::error_code ec;
    m_syncer->process_table(req, ec);

    EXPECT_FALSE(ec);
    EXPECT_FALSE(m_syncer->m_sync_table_finish);
    auto state_key = base::xvdbkey_t::create_prunable_state_key(table_account_address.to_string(), table_height, {block_hash.begin(), block_hash.end()});
    auto v = m_db->get_value(state_key);
    EXPECT_TRUE(v.empty());
}

TEST_F(test_state_sync_fixture, test_process_table_hash_mismatch) {
    state_sync::state_req req;
    req.type = state_sync::state_req_type::enum_state_req_table;
    req.nodes_response.emplace_back(mismatch_state_bytes);
    std::error_code ec;
    m_syncer->process_table(req, ec);

    EXPECT_FALSE(ec);
    EXPECT_FALSE(m_syncer->m_sync_table_finish);
    auto state_key = base::xvdbkey_t::create_prunable_state_key(table_account_address.to_string(), table_height, {block_hash.begin(), block_hash.end()});
    auto v = m_db->get_value(state_key);
    EXPECT_TRUE(v.empty());
}

TEST_F(test_state_sync_fixture, test_send_message) {
    base::xstream_t stream(base::xcontext_t::instance());
    stream << table_account_address.to_string();
    stream << 100;
    std::vector<xbytes_t> nodes_bytes;
    std::vector<xbytes_t> units_bytes;
    nodes_bytes.emplace_back(xbytes_t(32, 1));
    units_bytes.emplace_back(xbytes_t(32, 2));
    units_bytes.emplace_back(xbytes_t(32, 3));
    units_bytes.emplace_back(xbytes_t(32, 4));
    stream << nodes_bytes;
    stream << units_bytes;
    stream << rand();
    auto str = xbytes_t{stream.data(), stream.data() + stream.size()};
    auto node = m_syncer->send_message(m_peers, str, xmessage_id_sync_table_request);

    auto it = std::find(m_peers.peers.begin(), m_peers.peers.end(), node);
    EXPECT_TRUE(it != m_peers.peers.end());

    auto mock_net = std::dynamic_pointer_cast<xmock_vnetwork_driver_t>(m_peers.network);
    EXPECT_TRUE(mock_net->m_msg.front().first == (*it));
    EXPECT_EQ(mock_net->m_msg.front().second, vnetwork::xmessage_t(str, xmessage_id_sync_table_request));
}

TEST_F(test_state_sync_fixture, test_send_message_error) {
    base::xstream_t stream(base::xcontext_t::instance());
    stream << table_account_address.to_string();
    stream << 100;
    std::vector<xbytes_t> nodes_bytes;
    std::vector<xbytes_t> units_bytes;
    nodes_bytes.emplace_back(xbytes_t(32, 1));
    units_bytes.emplace_back(xbytes_t(32, 2));
    units_bytes.emplace_back(xbytes_t(32, 3));
    units_bytes.emplace_back(xbytes_t(32, 4));
    stream << nodes_bytes;
    stream << units_bytes;
    stream << rand();
    auto str = xbytes_t{stream.data(), stream.data() + stream.size()};

    auto mock_net = std::dynamic_pointer_cast<xmock_vnetwork_driver_t>(m_peers.network);
    mock_net->m_send_error = true;
    auto node = m_syncer->send_message(m_peers, str, xmessage_id_sync_table_request);

    auto it = std::find(m_peers.peers.begin(), m_peers.peers.end(), node);
    EXPECT_TRUE(it != m_peers.peers.end());
    EXPECT_TRUE(mock_net->m_msg.empty());
}

TEST_F(test_state_sync_fixture, test_assign_table_tasks) {
    m_syncer->m_req_sequence_id = 100;
    m_syncer->assign_table_tasks(m_peers);

    EXPECT_EQ(m_track_reqs.size(), 1);
    auto req = m_track_reqs.at(100);
    auto it = std::find(m_peers.peers.begin(), m_peers.peers.end(), req.peer);
    EXPECT_TRUE(it != m_peers.peers.end());
    EXPECT_EQ(req.id, 100);
    EXPECT_EQ(m_syncer->m_req_sequence_id, 101);
    EXPECT_EQ(req.type, state_sync::state_req_type::enum_state_req_table);

    auto mock_net = std::dynamic_pointer_cast<xmock_vnetwork_driver_t>(m_peers.network);
    auto msg = mock_net->m_msg.front();
    EXPECT_TRUE(msg.first == (*it));
    
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)(msg.second.payload().data()), (uint32_t)msg.second.payload().size());
    std::string table;
    uint64_t height{0};
    xbytes_t hash;
    uint32_t id;
    stream >> table;
    stream >> height;
    stream >> hash;
    stream >> id;
    EXPECT_EQ(table, table_account_address.to_string());
    EXPECT_EQ(hash, block_hash.to_bytes());
    EXPECT_EQ(height, table_height);
    EXPECT_EQ(id, 100);
}

TEST_F(test_state_sync_fixture, test_loop_network_network_error1) {
    m_peers.network = nullptr;
#if !defined(NDEBUG)
    m_syncer->running_thead_id_ = std::this_thread::get_id();
#endif

    std::error_code ec;
    m_syncer->sync_table(ec);
    
    EXPECT_EQ(ec, make_error_code(state_sync::error::xerrc_t::state_network_invalid));
    EXPECT_EQ(m_syncer->m_cancel, true);
}

void loop_empty_helper(std::shared_ptr<state_sync::xstate_sync_t> syncer, std::shared_ptr<vnetwork::xvnetwork_driver_face_t> network) {
    auto mock_net = std::dynamic_pointer_cast<xmock_vnetwork_driver_t>(network);
    state_sync::state_req req;
    req.type = state_sync::state_req_type::enum_state_req_table;
    int cnt{0};
    while (true) {
        if (mock_net->m_msg.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        auto m = mock_net->m_msg.front();
        mock_net->m_msg.pop();
        req.peer = m.first;
        syncer->deliver_req(req);
        cnt++;
        if (cnt >= 10) {
            break;
        }
    }
}

TEST_F(test_state_sync_fixture, test_loop_network_network_error2) {
#if !defined(NDEBUG)
    m_syncer->running_thead_id_ = std::this_thread::get_id();
#endif

    auto th = std::thread(loop_empty_helper, m_syncer, m_peers.network);

    std::error_code ec;
    m_syncer->sync_table(ec);

    th.join();
    EXPECT_EQ(ec, make_error_code(state_sync::error::xerrc_t::state_network_invalid));
    EXPECT_EQ(m_syncer->m_cancel, true);
}

TEST_F(test_state_sync_fixture, test_loop_cancel) {
    m_syncer->m_cancel = true;
#if !defined(NDEBUG)
    m_syncer->running_thead_id_ = std::this_thread::get_id();
#endif

    std::error_code ec;
    m_syncer->sync_table(ec);
    
    EXPECT_EQ(ec, make_error_code(state_sync::error::xerrc_t::state_sync_cancel));
    EXPECT_EQ(m_syncer->m_cancel, true);
}

TEST_F(test_state_sync_fixture, test_loop_process_error) {
    m_syncer->deliver_req({});
#if !defined(NDEBUG)
    m_syncer->running_thead_id_ = std::this_thread::get_id();
#endif

    std::error_code ec;
    auto condition = [this]() -> bool { return !m_syncer->m_sync_table_finish; };
    auto add_task = [this](state_sync::sync_peers const & peers) { return m_syncer->assign_table_tasks(peers); };
    auto process_task = [this](state_sync::state_req & req, std::error_code & ec) { ec = state_sync::error::xerrc_t::state_data_invalid; };
    m_syncer->loop(condition, add_task, process_task, ec);

    EXPECT_EQ(ec, make_error_code(state_sync::error::xerrc_t::state_data_invalid));
}

TEST_F(test_state_sync_fixture, test_sync_table_success) {
    state_sync::state_req req;
    req.type = state_sync::state_req_type::enum_state_req_table;
    req.nodes_response.emplace_back(state_bytes);
    m_syncer->deliver_req(req);
    auto state_key = base::xvdbkey_t::create_prunable_state_key(table_account_address.to_string(), table_height, {block_hash.begin(), block_hash.end()});
    EXPECT_EQ(m_db->get_value(state_key), std::string());
    EXPECT_FALSE(m_syncer->m_sync_table_finish);
#if !defined(NDEBUG)
    m_syncer->running_thead_id_ = std::this_thread::get_id();
#endif

    std::error_code ec;
    m_syncer->sync_table(ec);
    EXPECT_TRUE(m_syncer->m_sync_table_finish);
    EXPECT_FALSE(ec);
    EXPECT_EQ(m_db->get_value(state_key), to_string(state_bytes));
    EXPECT_FALSE(m_syncer->is_done());
    EXPECT_FALSE(m_syncer->error());
}

TEST_F(test_state_sync_fixture, test_sync_table_existed) {
#if !defined(NDEBUG)
    m_syncer->running_thead_id_ = std::this_thread::get_id();
#endif
    auto state_key = base::xvdbkey_t::create_prunable_state_key(table_account_address.to_string(), table_height, {block_hash.begin(), block_hash.end()});
    m_db->set_value(state_key, to_string(state_bytes));

    std::error_code ec;
    m_syncer->sync_table(ec);
    EXPECT_FALSE(ec);
    EXPECT_TRUE(m_syncer->m_sync_table_finish);
}

TEST_F(test_state_sync_fixture, test_sync_table_loop_error) {
    m_peers.network = nullptr;
#if !defined(NDEBUG)
    m_syncer->running_thead_id_ = std::this_thread::get_id();
#endif

    std::error_code ec;
    m_syncer->sync_table(ec);
    EXPECT_FALSE(m_syncer->m_sync_table_finish);
    EXPECT_EQ(ec, make_error_code(state_sync::error::xerrc_t::state_network_invalid));
    EXPECT_EQ(m_syncer->m_cancel, true);
}

TEST_F(test_state_sync_fixture, test_sync_trie_success) {
    std::error_code ec;
    auto th = std::thread(&test_state_sync_fixture::sync_helper, this);
#if !defined(NDEBUG)
    m_syncer->running_thead_id_ = std::this_thread::get_id();
#endif
    m_syncer->sync_trie(ec);
    th.join();
    EXPECT_FALSE(ec);
    EXPECT_FALSE(m_syncer->is_done());
    EXPECT_FALSE(m_syncer->error());

    for (auto & k : node_map) {
        auto v = m_syncer->m_kv_db->get(from_hex(k.first), ec);
        EXPECT_EQ(v, from_hex(k.second));
        EXPECT_FALSE(ec);
    }
    for (auto & k : unit_sync_map) {
        state_mpt::xaccount_info_t info;
        info.decode(to_string(from_hex(k.first)));
        auto dbkey = base::xvdbkey_t::create_prunable_unit_state_key(info.account.vaccount(), info.index.get_latest_unit_height(), info.index.get_latest_unit_hash());
        auto v = m_db->get_value(dbkey);
        EXPECT_EQ(to_bytes(v), from_hex(k.second));
        EXPECT_FALSE(ec);
    }
}

TEST_F(test_state_sync_fixture, test_sync_trie_loop_error) {
    m_peers.network = nullptr;
#if !defined(NDEBUG)
    m_syncer->running_thead_id_ = std::this_thread::get_id();
#endif

    std::error_code ec;
    m_syncer->sync_trie(ec);
    EXPECT_FALSE(m_syncer->m_sync_table_finish);
    EXPECT_EQ(ec, make_error_code(state_sync::error::xerrc_t::state_network_invalid));
    EXPECT_EQ(m_syncer->m_cancel, true);
}

TEST_F(test_state_sync_fixture, test_run_success) {
    auto th = std::thread(&test_state_sync_fixture::sync_helper, this);
    m_syncer->run();
    th.join();
    EXPECT_EQ(m_syncer->is_done(), true);
    auto res = m_syncer->result();
    EXPECT_EQ(res.account.to_string(), table_account_address.to_string());
    EXPECT_EQ(res.height, table_height);
    EXPECT_EQ(res.block_hash, block_hash);
    EXPECT_EQ(res.state_hash, state_hash);
    EXPECT_EQ(res.root_hash, root_hash);
    EXPECT_FALSE(res.ec);
    EXPECT_TRUE(m_syncer->m_sync_table_finish);
    auto state_key = base::xvdbkey_t::create_prunable_state_key(table_account_address.to_string(), table_height, {block_hash.begin(), block_hash.end()});
    EXPECT_EQ(m_db->get_value(state_key), to_string(state_bytes));
    std::error_code ec;
    for (auto & k : node_map) {
        auto v = m_syncer->m_kv_db->get(from_hex(k.first), ec);
        EXPECT_EQ(v, from_hex(k.second, ec));
        EXPECT_FALSE(ec);
    }
    for (auto & k : unit_sync_map) {
        state_mpt::xaccount_info_t info;
        info.decode(to_string(from_hex(k.first)));
        auto dbkey = base::xvdbkey_t::create_prunable_unit_state_key(info.account.vaccount(), info.index.get_latest_unit_height(), info.index.get_latest_unit_hash());
        auto v = m_db->get_value(dbkey);
        EXPECT_EQ(to_bytes(v), from_hex(k.second));
        EXPECT_FALSE(ec);
    }
}

TEST_F(test_state_sync_fixture, test_run_sync_table_error) {
    m_peers.network = nullptr;
    m_syncer->run();
    EXPECT_EQ(m_syncer->m_cancel, true);
    EXPECT_EQ(m_syncer->is_done(), true);
    auto res = m_syncer->result();
    EXPECT_EQ(res.account.to_string(), table_account_address.to_string());
    EXPECT_EQ(res.height, table_height);
    EXPECT_EQ(res.block_hash, block_hash);
    EXPECT_EQ(res.state_hash, state_hash);
    EXPECT_EQ(res.root_hash, root_hash);
    EXPECT_EQ(res.ec, make_error_code(state_sync::error::xerrc_t::state_network_invalid));
}

TEST_F(test_state_sync_fixture, test_run_cancel) {
    auto th = std::thread(&test_state_sync_fixture::sync_cancel_helper, this);
    m_syncer->run();
    th.join();
    EXPECT_EQ(m_syncer->is_done(), true);
    auto res = m_syncer->result();
    EXPECT_EQ(res.account.to_string(), table_account_address.to_string());
    EXPECT_EQ(res.height, table_height);
    EXPECT_EQ(res.block_hash, block_hash);
    EXPECT_EQ(res.state_hash, state_hash);
    EXPECT_EQ(res.root_hash, root_hash);
    EXPECT_EQ(res.ec, make_error_code(state_sync::error::xenum_errc::state_sync_cancel));
}

}
