// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <gtest/gtest.h>

#include <fstream>

#include "content.h"
#include "xdata/xgenesis_data.h"
#include "xstore/test/xstore_face_mock.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xloader/xconfig_offchain_loader.h"
#include "xconfig/xconfig_register.h"
#include "xmbus/xevent_store.h"
#include "xstore/xstore_error.h"
#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xtimer_driver.h"


class xconfig_register_listener_face_mock_t
    : public top::config::xconfig_register_listener_face_t {
public:
    virtual bool config_updated(const std::map<std::string, std::string>& map) {
        if (map.empty()) {
            return false;
        }
        for (const auto& entry : map) {
            auto it = registration_cache.find(entry.first);
            if (it != registration_cache.end()) {
                registration_cache[entry.first] = entry.second;
            } else {
                registration_cache.insert({entry.first, entry.second});
            }
            std::cout << "in version_updated, first: " << entry.first
                    << ", second: " << registration_cache[entry.first]
                    << std::endl;
        }

        return false;
    }

    static void clear_cache() {
        registration_cache.clear();
    }

    // simulate a global parameter registration center
    static std::map<std::string, std::string> registration_cache;
};

std::map<std::string, std::string> xconfig_register_listener_face_mock_t::registration_cache{};


class xstore_tcc_mock_t : public xstore_face_mock_t {
public:

    xstore_tcc_mock_t() {
        m_mbus = new mbus::xmessage_bus_t(
                true, // enable mbus timer
                1000); // interval : 1000ms
/*
        m_initial_params.insert({"zone_election_trigger_interval", "1"});
        m_initial_params.insert({"auditor_group_count", "1"});
        m_initial_params.insert({"validator_group_count", "2"});
        m_initial_params.insert({"cluster_election_interval", "10"});
        m_initial_params.insert({"min_consensus_group_size", "3"});
        m_initial_params.insert({"max_consensus_group_size", "4"});
        m_initial_params.insert({"min_auditor_group_size", "3"});
        m_initial_params.insert({"max_auditor_group_size", "7"});
        m_initial_params.insert({"min_election_committee_size", "3"});
        m_initial_params.insert({"max_election_committee_size", "4"});
        m_initial_params.insert({"min_recommend_size", "1"});
        m_initial_params.insert({"max_recommend_size", "15"});
*/
        m_updated_params = m_initial_params;
    }

    ~xstore_tcc_mock_t() {
        if (m_mbus) {
            delete m_mbus;
        }
    }

    virtual int32_t string_get(const std::string& address, const std::string& key, std::string& value) {
        if(key == "update_action_type") {
            value = m_action_type;
            return top::store::xstore_success;
        }
        return -1;
    }

    virtual mbus::xmessage_bus_t* get_mbus() {return m_mbus;}

    void notify_update() {
        if (m_mbus) {
            top::mbus::xevent_store_ptr_t ptr = std::make_shared<top::mbus::xevent_store_t>(
                top::mbus::xevent_store_t::type_config_update,
                "version update test owner",
                top::mbus::xevent_t::to_listener);
            ptr->err = top::mbus::xevent_t::succ;
            m_mbus->push_event(ptr);
        }
    }

    virtual int32_t map_copy_get(const std::string& address, const std::string & key, std::map<std::string, std::string> & map) const override {
        if(m_updated_params.empty()) {
            return -1;
        }
        map.insert(m_updated_params.begin(), m_updated_params.end());
        return 0;
    }

    void reset_update() {
        m_updated_params.clear();
    }

    void update_action_type(const std::string& action_type) {
        m_action_type = action_type;
    }

    void update_param(const std::string & key, const std::string & value) {
        auto it = m_updated_params.find(key);
        if (it != m_updated_params.end()) {
            it->second = value;
        } else {
            m_updated_params.insert({key, value});
        }
    }

private:
    mbus::xmessage_bus_t* m_mbus {nullptr};
    std::map<std::string, std::string> m_initial_params {};

    std::string m_action_type;
    std::map<std::string, std::string> m_updated_params {};
};

TEST(xconfig_register, offchain_load) {
    std::ofstream ofs;
    std::string filename("test_config.json");
    ofs.open(filename);
    if (ofs.is_open()) {
        ofs << config_content;
        ofs.close();
    }

    auto& config_center = top::config::xconfig_register_t::get_instance();

    auto offchain_loader = std::make_shared<loader::xconfig_offchain_loader_t>(filename);
    config_center.add_loader(offchain_loader);
    config_center.load();
    config_center.dump();
    ASSERT_NE(config_center.size(), 0);
    config_center.clear_loaders();

    remove(filename.c_str());
}

TEST(xconfig_register, initial_onchain_load) {
    auto store = store::xstore_factory::create_store_with_memdb();
    std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
    std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
    auto chain_timer = make_object_ptr<time::xchain_timer_t>(timer_driver);
    auto mbus = new mbus::xmessage_bus_t();

    auto& config_center = top::config::xconfig_register_t::get_instance();

    config::xconfig_loader_ptr_t loader = std::make_shared<loader::xconfig_onchain_loader_t>(make_observer(store), make_observer(mbus), make_observer(chain_timer));
    config_center.add_loader(loader);
    config_center.load();

    ASSERT_NE(config_center.size(), 0);
    config_center.clear_loaders();
    store->close();
}

TEST(xconfig_register, load_on_chain) {
    xconfig_register_listener_face_mock_t l;
    auto& config_center = top::config::xconfig_register_t::get_instance();
    config_center.add_listener(&l);

    auto tcc_store = store::xstore_factory::create_store_with_memdb();

    auto mbus = new mbus::xmessage_bus_t();
    std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
    std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
    auto chain_timer = make_object_ptr<time::xchain_timer_t>(timer_driver);
    auto loader = std::make_shared<loader::xconfig_onchain_loader_t>(make_observer(tcc_store), make_observer(mbus), make_observer(chain_timer));
    config_center.add_loader(loader);
    config_center.load();

    {
        int min, max;
        ASSERT_TRUE(config_center.get<int>("min_auditor_group_size", min));
        ASSERT_EQ(min, 6);
        ASSERT_TRUE(config_center.get<int>("max_auditor_group_size", max));
        ASSERT_EQ(max, 64);
        ASSERT_NE(config_center.size(), 0);
    }

    {
        int temp = 0;

        ASSERT_FALSE(config_center.get<int>("new_min_auditor_group_size", temp));
        ASSERT_EQ(temp, 0);
        ASSERT_FALSE(config_center.get<int>("bad_min_auditor_group_size", temp));
    }
    config_center.clear_loaders();

    tcc_store->close();
}
