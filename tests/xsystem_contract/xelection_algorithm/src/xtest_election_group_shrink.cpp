#include "xvm/xsystem_contracts/xelection/xelect_consensus_group_contract.h"

#include "tests/xsystem_contract/xelection_algorithm/xtest_elect_group_contract_fixture.h"

#include <sstream>

NS_BEG3(top, tests, election)


std::map<int,std::vector<common::xnode_id_t>> re;
uint16_t round_count;

void show(data::election::xelection_group_result_t & election_group_result){
    for (auto index = 0; index < 16; ++index) {
        std::stringstream ss;
        ss << "slot id " << std::setw(2) << index << " : ";
        for (uint16_t index2 = 0; index2 < round_count; index2++) {
            if (re[index].size() <= index2)
                continue;
            auto nid = re[index][index2];
            if (nid.empty()) {
                ss << "** -> ";
            } else {
                auto id = nid.to_string().substr(13);
                ss << std::setw(2) << id << " -> ";
            }
        }
        std::printf("%s\n", ss.str().c_str());
    }
}


void record(data::election::xelection_group_result_t & election_group_result){
    for(auto & p :election_group_result){
        if(!broadcast(p.first)){
            auto slot_id = p.first.value();
            auto node_id = p.second.account_address();
            if(re[slot_id].size()<=round_count){
                re[slot_id].resize(round_count * 2 + 1);
            }
            re[slot_id][round_count] = node_id;
        }
    }
    round_count++;
}

#define SHOW show(election_network_result.result_of(node_type).result_of(cid).result_of(gid))

#define ELECT(group_range)                                                                                                                                                         \
    random_seed = static_cast<uint64_t>(rng());                                                                                                                                    \
    m_elect_consensus_group.test_elect(zid, cid, gid, start_time+1, start_time+2, random_seed, group_range, standby_network_result, election_network_result);                                            \
    record(election_network_result.result_of(node_type).result_of(cid).result_of(gid));\
    start_time += 2


TEST_F(xtest_elect_consensus_group_contract_fixture_t, shirnk) {
    common::xnode_type_t node_type{common::xnode_type_t::consensus_auditor};
    common::xzone_id_t zid{common::xconsensus_zone_id};
    common::xcluster_id_t cid{common::xdefault_cluster_id};
    common::xgroup_id_t gid{common::xauditor_group_id_begin};
    std::string node_id_prefix = "T00000LMZLAYy";
    add_nodes_to_standby(30, node_type, node_id_prefix);
    for (auto index = 0; index < 30; ++index) {
        re[index] = std::vector<common::xnode_id_t>{};
    }
    round_count = 0;

    xrange_t<config::xgroup_size_t> group_size_range{10, 15};
    xrange_t<config::xgroup_size_t> group_size_range2{7, 7};
    std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    auto random_seed = static_cast<uint64_t>(rng());
    common::xlogic_time_t start_time{0};

    ELECT(group_size_range);
    ELECT(group_size_range);
    ELECT(group_size_range);
    ELECT(group_size_range);
    ELECT(group_size_range);
    SHOW;

    std::printf("delete nodes 1-20 :\n");
    for (auto index = 1; index <= 20; ++index) {

        delete_standby_node(node_type, build_account_address(node_id_prefix,index));
    }
    ELECT(group_size_range);
    ELECT(group_size_range);
    ELECT(group_size_range);
    ELECT(group_size_range);
    ELECT(group_size_range);
    ELECT(group_size_range);
    ELECT(group_size_range);
    SHOW;
    
    std::printf("use group size 7-7 :\n");
    ELECT(group_size_range2);
    SHOW;
    ELECT(group_size_range2);
    ELECT(group_size_range2);
    ELECT(group_size_range2);
    ELECT(group_size_range2);
    ELECT(group_size_range2);
    ELECT(group_size_range2);
    ELECT(group_size_range2);
    ELECT(group_size_range2);
    SHOW;

    add_nodes_to_standby(10, node_type, node_id_prefix);
    ELECT(group_size_range);
    ELECT(group_size_range);
    ELECT(group_size_range);
    ELECT(group_size_range);
    ELECT(group_size_range);
    SHOW;
    
    ELECT(group_size_range2);
    ELECT(group_size_range2);
    ELECT(group_size_range2);
    ELECT(group_size_range2);
    ELECT(group_size_range2);
    SHOW;

}

NS_END3
