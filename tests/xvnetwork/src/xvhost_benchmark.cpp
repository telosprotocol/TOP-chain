// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xtimer_driver.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xnetwork/tests/xdummy_network_driver.h"
#include "xvnetwork/xcodec/xmsgpack/xvnetwork_message_codec.hpp"
#include "xvnetwork/xmessage_ready_callback.h"
#include "xvnetwork/xvhost.h"
#include "xvnetwork/xvnetwork_driver.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xelection/xcache/xdata_accessor.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cinttypes>

NS_BEG3(top, vnetwork, tests)

constexpr std::size_t version_count{ 2 };
XDEFINE_MSG_CATEGORY(xmessage_category_benchmark, 0x0001);
XDEFINE_MSG_ID(xmessage_category_benchmark, xmessage_id_benchmark, 0x00000001);

std::string const receiver_node_id_value{ "T00000LaeGMEceZRTZ7YAtz8NeCLRix2Bnxp9RYz"};
std::string const sender_node_id_value{ "T00000LSJ3zFwacQCviaEcx82cLxDPscZxz3EeX8" };

common::xnode_id_t receiver_node_id{ receiver_node_id_value };
common::xnode_id_t sender_node_id{ sender_node_id_value };

common::xsharding_address_t const auditor_sharding_address{
    common::xtestnet_id,
    common::xconsensus_zone_id,
    common::xdefault_cluster_id,
    common::xauditor_group_id_begin
};

common::xsharding_address_t const validator_sharding_address{
    common::xtestnet_id,
    common::xconsensus_zone_id,
    common::xdefault_cluster_id,
    common::xvalidator_group_id_begin
};

class xtop_benchmark_network_driver_helper final : public network::tests::xdummy_network_driver_t {
    network::xnetwork_message_ready_callback_t m_cb;

public:
    common::xnode_id_t const &
    host_node_id() const noexcept override {
        return receiver_node_id;
    }

    void
    register_message_ready_notify(network::xnetwork_message_ready_callback_t cb) noexcept override {
        m_cb = std::move(cb);
    }

    void
    unregister_message_ready_notify() {
        m_cb = nullptr;
    }

    std::chrono::high_resolution_clock::time_point send_data(std::size_t const limit) {
        assert(m_cb);
        std::vector<xbyte_buffer_t> messages;
        messages.reserve(limit);

        common::xnode_id_t random;
        for (auto i = 0u; i < limit; ++i) {
            // auto str = std::to_string(i);
            random.random();
            auto str = random.value();
            while (str.length() < 1000) {
                str += random.value();
            }

            top::common::xnode_address_t sender{
                auditor_sharding_address,
                top::common::xaccount_election_address_t{ sender_node_id, top::common::xslot_id_t{0} },
                common::xelection_round_t{ static_cast<common::xelection_round_t::value_type>(i % version_count) },
                std::uint16_t{1},
                std::uint64_t{i % version_count}
            };

            top::common::xnode_address_t receiver{
                validator_sharding_address,
                top::common::xaccount_election_address_t{ receiver_node_id, top::common::xslot_id_t{0} },
                common::xelection_round_t{ static_cast<common::xelection_round_t::value_type>(i % version_count) },
                std::uint16_t{1},
                std::uint64_t{i % version_count}
            };

            xvnetwork_message_t msg{
                sender,
                receiver,
                xmessage_t{ xbyte_buffer_t{ std::begin(str), std::end(str) }, xmessage_id_benchmark },
                static_cast<std::uint64_t>(i % version_count)
            };
            messages.push_back(codec::msgpack_encode(msg));
        }

        std::printf("start to send data...\n");
        auto begin = std::chrono::high_resolution_clock::now();
        assert(messages.size() == limit);
        for (auto i = 0u; i < limit; ++i) {
            m_cb(common::xnode_id_t{}, messages.at(i));
        }
        auto end = std::chrono::high_resolution_clock::now();

        auto const ms = static_cast<std::int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());
        std::printf("vhost accepts %zu packs using %" PRIi64 "ms.  tps = %f\n",
                    limit, ms, (limit / static_cast<double>(ms) * 1000));

        return begin;
    }
};
using xbenchmark_network_driver_helper_t = xtop_benchmark_network_driver_helper;
static std::unique_ptr<xbenchmark_network_driver_helper_t> network_driver;

static void build_network_driver() {
    network_driver = top::make_unique<xbenchmark_network_driver_helper_t>();
}

class xtop_chian_timer final : public time::xchain_time_face_t {
public:
    void start() override {
    }

    void stop() override {
    }

    void update_time(common::xlogic_time_t, time::xlogic_timer_update_strategy_t) override {
        return;
    }

    common::xlogic_time_t logic_time() const noexcept override {
        return common::xjudgement_day;
    }

    bool watch(const std::string &, uint64_t, time::xchain_time_watcher) override {
        return true;
    }

    bool unwatch(const std::string &) override {
        return true;
    }

    void init() override {
    }

    void close() override {
    }

    base::xiothread_t* get_iothread() const noexcept override {
        return nullptr;
    }
};
using xchain_timer_t = xtop_chian_timer;
static xobject_ptr_t<time::xchain_time_face_t> chain_timer = make_object_ptr<xchain_timer_t>();

static std::atomic<std::size_t> c{ 0 };
static void vnetwork_driver_cb(xvnode_address_t const &, xmessage_t const &, std::uint64_t const) {
    c.fetch_add(1, std::memory_order_relaxed);
}

std::vector<std::shared_ptr<top::vnetwork::xvnetwork_driver_face_t>> build_vnetwork_drivers(observer_ptr<top::vnetwork::xvhost_face_t> vhost,
                                                                                            uint64_t const version_count) {
    std::vector<std::shared_ptr<top::vnetwork::xvnetwork_driver_face_t>> r;
    r.reserve(version_count);
    for (auto i = 0u; i < version_count; ++i) {
        top::common::xnode_address_t address{
            validator_sharding_address,
            top::common::xaccount_election_address_t{ receiver_node_id, top::common::xslot_id_t{0} },
            common::xelection_round_t{ static_cast<common::xelection_round_t::value_type>(i % version_count) },
            std::uint16_t{1},
            std::uint64_t{i % version_count}
        };
        r.push_back(std::make_shared<top::vnetwork::xvnetwork_driver_t>(vhost, address, common::xelection_round_t{0}));
    }

    return r;
}

static std::unique_ptr<election::cache::xdata_accessor_face_t>  election_cache_data_accessor;
static void build_election_cache_data_accessor() {
    election_cache_data_accessor = top::make_unique<election::cache::xdata_accessor_t>(common::xtestnet_id,
                                                                                       make_observer(chain_timer));

    for (auto i = 0u; i < version_count; ++i) {
        top::data::election::xelection_result_store_t election_result_store{};
        auto & result_store = election_result_store.result_of(common::xtestnet_id);

        auto & auditor_election_result = result_store.result_of(common::xnode_type_t::consensus_auditor).result_of(common::xdefault_cluster_id).result_of(common::xauditor_group_id_begin);
        auditor_election_result.start_time(i);
        auditor_election_result.group_version(common::xelection_round_t{ static_cast<common::xelection_round_t::value_type>(i) });

        top::data::election::xelection_info_bundle_t election_info_bundle;
        election_info_bundle.node_id(sender_node_id);
        election_info_bundle.election_info().joined_version = common::xelection_round_t{ static_cast<common::xelection_round_t::value_type>(0) };

        auditor_election_result.insert(std::move(election_info_bundle));

        auto & validator_election_result = result_store.result_of(common::xnode_type_t::consensus_validator).result_of(common::xdefault_cluster_id).result_of(common::xvalidator_group_id_begin);
        validator_election_result.start_time(i);
        validator_election_result.group_version(common::xelection_round_t{ static_cast<common::xelection_round_t::value_type>(i) });
        validator_election_result.associated_group_version(common::xelection_round_t{ static_cast<common::xelection_round_t::value_type>(i) });
        validator_election_result.associated_group_id(common::xauditor_group_id_begin);

        election_info_bundle.node_id(receiver_node_id);
        election_info_bundle.election_info().joined_version = common::xelection_round_t{ static_cast<common::xelection_round_t::value_type>(0) };

        validator_election_result.insert(std::move(election_info_bundle));

        std::error_code ec;
        election_cache_data_accessor->update_zone(common::xdefault_zone_id, election_result_store, i, ec);
        assert(!ec);
    }
}

static std::shared_ptr<xvhost_face_t> target_vhost;
static void build_vhost() {
    build_network_driver();
    build_election_cache_data_accessor();

    target_vhost = std::make_shared<xvhost_t>(make_observer(network_driver.get()),
                                              make_observer(chain_timer),
                                              common::xtestnet_id,
                                              make_observer(election_cache_data_accessor));
    target_vhost->start();

    auto const vnetwork_drivers = build_vnetwork_drivers(make_observer(target_vhost), version_count);
    for (auto & driver : vnetwork_drivers) {
        driver->register_message_ready_notify(xmessage_category_benchmark, std::bind(&vnetwork_driver_cb,
                                                                                     std::placeholders::_1,
                                                                                     std::placeholders::_2,
                                                                                     std::placeholders::_3));
        driver->start();
    }
}

NS_END3

#if 0
TEST(vnet, benchmark) {
    constexpr std::size_t data_pack_count{ 100000 };
    top::vnetwork::tests::build_vhost();
    for (auto i = 0; i < 10; ++i) {
        std::printf("begin test...\n");
        auto begin = top::vnetwork::tests::network_driver->send_data(data_pack_count);
        // std::this_thread::sleep_for(std::chrono::seconds{ 5 });
        while (data_pack_count != top::vnetwork::tests::c.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds{ 50 });
            //printf("%zu\n", top::vnetwork::tests::c.load(std::memory_order_relaxed));
            //std::this_thread::sleep_for(std::chrono::seconds{10});
            //printf("%zu\n", top::vnetwork::tests::c.load(std::memory_order_relaxed));
            //std::this_thread::sleep_for(std::chrono::seconds{10});
            //printf("%zu\n", top::vnetwork::tests::c.load(std::memory_order_relaxed));
            //std::this_thread::sleep_for(std::chrono::seconds{10});
            //printf("%zu\n", top::vnetwork::tests::c.load(std::memory_order_relaxed));
            //std::this_thread::sleep_for(std::chrono::seconds{10});
            //printf("%zu\n", top::vnetwork::tests::c.load(std::memory_order_relaxed));
            //std::this_thread::sleep_for(std::chrono::seconds{10});
            //printf("%zu\n", top::vnetwork::tests::c.load(std::memory_order_relaxed));
            //std::this_thread::sleep_for(std::chrono::seconds{10});
            //printf("%zu\n", top::vnetwork::tests::c.load(std::memory_order_relaxed));
            //std::this_thread::sleep_for(std::chrono::seconds{10});
            //printf("%zu\n", top::vnetwork::tests::c.load(std::memory_order_relaxed));
            //std::this_thread::sleep_for(std::chrono::seconds{10});
            //printf("%zu\n", top::vnetwork::tests::c.load(std::memory_order_relaxed));
            //std::this_thread::sleep_for(std::chrono::seconds{10});
            //printf("%zu\n", top::vnetwork::tests::c.load(std::memory_order_relaxed));
            //break;
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto ms = static_cast<std::size_t>(std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count());

        std::printf("vhost processed in %zu milliseconds against %zu packs tps = %lf \n",
            ms,
            data_pack_count,
            static_cast<double>(data_pack_count) / ms * 1000);

        top::vnetwork::tests::c.store(0);
    }
}
#endif
