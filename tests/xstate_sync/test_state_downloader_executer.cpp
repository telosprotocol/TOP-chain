#include "test_state_sync_data.h"
#include "xbase/xutl.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"
#include "xdbstore/xstore_face.h"
#include "xevm_common/trie/xsecure_trie.h"
#include "xevm_common/xerror/xerror.h"
#include "xstate_mpt/xstate_mpt.h"
#include "xstate_sync/xerror.h"
#include "xutility/xhash.h"
#include "xvledger/xvledger.h"

#include <gtest/gtest.h>

#define private public
#include "xstate_sync/xstate_downloader.h"
#include "xstate_sync/xstate_sync.h"
#include "xstate_sync/xstate_downloader_executer.h"

namespace top {

class test_state_downloader_executer_fixture : public testing::Test {
public:
    void SetUp() override {
        base::xvchain_t::instance().clean_all(true);
        std::shared_ptr<db::xdb_face_t> db = db::xdb_factory_t::create_memdb();
        m_store = store::xstore_factory::create_store_with_static_kvdb(db);
        base::xvchain_t::instance().set_xdbstore(m_store.get());
        m_db = base::xvchain_t::instance().get_xdbstore();

        m_thread = make_object_ptr<base::xiothread_t>();
        m_executer = std::make_shared<state_sync::xdownload_executer_t>(make_observer(m_thread), 0);
        m_peers = peers_func(table_account_address.table_id(), 15);
        auto peers_func_impl = [&](const common::xtable_id_t &) -> state_sync::sync_peers { return m_peers; };
        auto track_func = [this](state_sync::state_req const & sr) { m_executer->push_track_req(sr); };
        m_syncer = state_sync::xstate_sync_t::new_state_sync(common::xaccount_address_t::build_from(table_account_address.to_string()), table_height, block_hash, state_hash, root_hash, peers_func_impl, track_func, m_db, true);
    }
    void TearDown() override {
    }

    void run_helper(int type) {
        auto mock_net = std::dynamic_pointer_cast<xmock_vnetwork_driver_t>(m_peers.network);
        int cnt{0};
        int node_cnt{0};
        while (true) {
            if (mock_net->m_msg.empty()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            cnt++;
            if (type == 4) {
                EXPECT_TRUE(mock_net->m_msg.size() <= 1);
            }
            auto m = mock_net->m_msg.front();
            mock_net->m_msg.pop();
            if (type == 1) {
                if (cnt % 2 != 0) {
                    continue;
                }
            }
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
                state_sync::state_res res;
                res.id = id;
                if (type == 2 && (cnt % 2 != 0)) {
                    if (table == table_account_address.to_string() && height == table_height && xh256_t(hash) == block_hash) {
                        res.nodes.emplace_back(mismatch_state_bytes);
                    }
                } else if (type == 3 && (cnt % 2 != 0)) {
                } else {
                    if (table == table_account_address.to_string() && height == table_height && xh256_t(hash) == block_hash) {
                        res.nodes.emplace_back(state_bytes);
                    }
                }
                m_executer->push_state_pack(res);
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
                state_sync::state_res res;
                res.id = id;
                if (type == 2 && (cnt % 2 != 0)) {
                    for (auto it = nodes_bytes.begin(); it != nodes_bytes.end(); ++it) {
                        res.nodes.emplace_back(mismatch_node);
                    }
                    for (auto it = units_bytes.begin(); it != units_bytes.end(); ++it) {
                        res.units.emplace_back(mismatch_unit);
                    }
                } else if (type == 3 && (cnt % 2 != 0)) {
                } else {
                    for (auto it = nodes_bytes.begin(); it != nodes_bytes.end(); ++it) {
                        res.nodes.emplace_back(from_hex(node_map.at(to_hex((*it)))));
                        node_cnt++;
                    }
                    EXPECT_TRUE(units_bytes.size() <= m_max_unit_num);
                    for (auto it = units_bytes.begin(); it != units_bytes.end(); ++it) {
                        res.units.emplace_back(from_hex(unit_sync_map.at(to_hex((*it)))));
                        node_cnt++;
                    }
                }
                m_executer->push_state_pack(res);
                if (node_cnt >= 75) {
                    break;
                }
            } else {
                assert(false);
            }
        }
        printf("run helper exist\n");
    }

    void run_cancel_helper() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        m_executer->cancel();
    }

    xobject_ptr_t<store::xstore_face_t> m_store{nullptr};
    base::xvdbstore_t * m_db{nullptr};

    xobject_ptr_t<base::xiothread_t> m_thread;
    std::shared_ptr<state_sync::xdownload_executer_t> m_executer{nullptr};
    std::shared_ptr<state_sync::xstate_sync_t> m_syncer{nullptr};
    state_sync::sync_peers m_peers;

    int m_max_unit_num = 4;
};

TEST_F(test_state_downloader_executer_fixture, test_run_state_sync_success) {
    state_sync::sync_result res;
    auto callback = [&](state_sync::sync_result result) {
        res = result;
    };
    std::thread th(&test_state_downloader_executer_fixture::run_helper, this, 0);
    m_executer->run_state_sync(m_syncer, callback);
    th.join();
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

TEST_F(test_state_downloader_executer_fixture, test_run_state_sync_in_bad_connection) {
    state_sync::sync_result res;
    auto callback = [&](state_sync::sync_result result) {
        res = result;
    };
    std::thread th(&test_state_downloader_executer_fixture::run_helper, this, 1);
    m_executer->run_state_sync(m_syncer, callback);
    th.join();
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

TEST_F(test_state_downloader_executer_fixture, test_run_state_sync_part_wrong_data) {
    state_sync::sync_result res;
    auto callback = [&](state_sync::sync_result result) {
        res = result;
    };
    std::thread th(&test_state_downloader_executer_fixture::run_helper, this, 2);
    m_executer->run_state_sync(m_syncer, callback);
    th.join();
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

TEST_F(test_state_downloader_executer_fixture, test_run_state_sync_part_empty_data) {
    state_sync::sync_result res;
    auto callback = [&](state_sync::sync_result result) {
        res = result;
    };
    std::thread th(&test_state_downloader_executer_fixture::run_helper, this, 3);
    m_executer->run_state_sync(m_syncer, callback);
    th.join();
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

TEST_F(test_state_downloader_executer_fixture, test_run_state_sync_task_flooding) {
    m_peers = peers_func(table_account_address.table_id(), 32);
    state_sync::sync_result res;
    auto callback = [&](state_sync::sync_result result) {
        res = result;
    };
    std::thread th(&test_state_downloader_executer_fixture::run_helper, this, 3);
    m_syncer->m_items_per_task = 4;
    m_syncer->m_units_per_task = 4;
    m_executer->run_state_sync(m_syncer, callback);
    th.join();
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

TEST_F(test_state_downloader_executer_fixture, test_run_state_sync_overtime) {
    auto th = make_object_ptr<base::xiothread_t>();
    auto executer = std::make_shared<state_sync::xdownload_executer_t>(make_observer(th), 2);
    state_sync::sync_result res;
    auto callback = [&](state_sync::sync_result result) {
        res = result;
    };
    executer->run_state_sync(m_syncer, callback);
    EXPECT_EQ(res.account.to_string(), table_account_address.to_string());
    EXPECT_EQ(res.height, table_height);
    EXPECT_EQ(res.block_hash, block_hash);
    EXPECT_EQ(res.state_hash, state_hash);
    EXPECT_EQ(res.root_hash, root_hash);
    EXPECT_EQ(res.ec, make_error_code(state_sync::error::xerrc_t::state_sync_overtime));
}

TEST_F(test_state_downloader_executer_fixture, test_run_state_sync_cancel) {
    state_sync::sync_result res;
    auto callback = [&](state_sync::sync_result result) {
        res = result;
    };
    std::thread th(&test_state_downloader_executer_fixture::run_cancel_helper, this);
    m_executer->run_state_sync(m_syncer, callback);
    th.join();
    EXPECT_EQ(res.account.to_string(), table_account_address.to_string());
    EXPECT_EQ(res.height, table_height);
    EXPECT_EQ(res.block_hash, block_hash);
    EXPECT_EQ(res.state_hash, state_hash);
    EXPECT_EQ(res.root_hash, root_hash);
    EXPECT_EQ(res.ec, make_error_code(state_sync::error::xerrc_t::state_sync_cancel));
}

TEST_F(test_state_downloader_executer_fixture, test_run_state_sync_no_response) {
    uint32_t test_peer_cnt = 3;
    m_peers.peers = {m_peers.peers.begin(), m_peers.peers.begin() + test_peer_cnt};
    state_sync::sync_result res;
    auto callback = [&](state_sync::sync_result result) {
        res = result;
    };
    m_executer->run_state_sync(m_syncer, callback);
    EXPECT_EQ(res.account.to_string(), table_account_address.to_string());
    EXPECT_EQ(res.height, table_height);
    EXPECT_EQ(res.block_hash, block_hash);
    EXPECT_EQ(res.state_hash, state_hash);
    EXPECT_EQ(res.root_hash, root_hash);
    EXPECT_EQ(res.ec, make_error_code(state_sync::error::xerrc_t::state_network_invalid));

    auto mock_net = std::dynamic_pointer_cast<xmock_vnetwork_driver_t>(m_peers.network);
    EXPECT_EQ(mock_net->m_msg.size(), test_peer_cnt);
    while (!mock_net->m_msg.empty()) {
        auto m = mock_net->m_msg.front();
        auto it = std::find(m_peers.peers.begin(), m_peers.peers.end(), m.first);
        EXPECT_TRUE(it != m_peers.peers.end());
        mock_net->m_msg.pop();
    }
    EXPECT_EQ(m_syncer->m_req_sequence_id, test_peer_cnt);
}

TEST_F(test_state_downloader_executer_fixture, test_run_state_req_limit) {
    state_sync::sync_result res;
    auto callback = [&](state_sync::sync_result result) {
        res = result;
    };
    m_syncer->m_max_req_nums = 1;
    std::thread th(&test_state_downloader_executer_fixture::run_helper, this, 4);
    m_executer->run_state_sync(m_syncer, callback);
    th.join();
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

// TEST_F(test_state_downloader_executer_fixture, test_downloader) {
//     xobject_ptr_t<base::xiothread_t> executor_thread = make_object_ptr<base::xiothread_t>();
//     xobject_ptr_t<base::xiothread_t> syncer_thread = make_object_ptr<base::xiothread_t>();
//     auto downloader = std::make_shared<state_sync::xstate_downloader_t>(
//         m_db, nullptr, nullptr, executor_thread, syncer_thread);
//     std::error_code ec;
//     downloader->sync_state(table_account_address, table_height, block_hash, state_hash, root_hash, true, ec);

//     std::this_thread::sleep_for(std::chrono::seconds(5));
// }

}