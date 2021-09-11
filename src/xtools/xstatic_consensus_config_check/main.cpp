// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xloader/xconfig_offchain_loader.h"

#include <iostream>

using namespace top;

#define CHECK(b, ...)                                                                                                                                                              \
    if (!(b)) {                                                                                                                                                                    \
        fprintf(stderr, __VA_ARGS__);                                                                                                                                              \
        printf("\n");                                                                                                                                                              \
        assert(false);                                                                                                                                                             \
        throw;                                                                                                                                                                     \
    }

void usage() {
    std::cout << "usage: ./xstatic_consensus_config_check xxxconfig.json" << std::endl;
    return;
}

void check_str(std::string const & name, std::string const & str, int group_num = -1) {
    std::vector<std::string> group_nodes_infos_str;
    top::base::xstring_utl::split_string(str, ',', group_nodes_infos_str);
    std::cout << name;
    if (group_num != -1) {
        std::cout << " " << group_num;
    }
    std::cout << "  size: " << group_nodes_infos_str.size() << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    for (auto _node_info : group_nodes_infos_str) {
        std::vector<std::string> one_node_info;
        top::base::xstring_utl::split_string(_node_info, '.', one_node_info);
        CHECK(one_node_info.size() == 3, "node info wrong at %s", _node_info.c_str());
        std::cout << "  node_id: " << one_node_info[0] << std::endl;
        std::cout << "  stake:   " << one_node_info[1] << std::endl;
        std::cout << "  pub_key: " << one_node_info[2] << std::endl;
        std::cout << "--------" << std::endl;
        CHECK(one_node_info[0].size() == 40, "node id length != 40 at %s", one_node_info[0].c_str());
        CHECK(one_node_info[2].size() == 88, "pub key length != 88 at %s", one_node_info[2].c_str());
    }
    std::cout << std::endl << std::endl;
}

void check_auditor_validator_str(std::string const & expect_name, std::string const & str, int group_num) {
    std::vector<std::string> group_type_and_nodes_infos;
    top::base::xstring_utl::split_string(str, ':', group_type_and_nodes_infos);
    CHECK(group_type_and_nodes_infos.size() == 2, "wrong group info after : %s", group_type_and_nodes_infos[1].c_str())
    CHECK(group_type_and_nodes_infos[0] == expect_name,
          "expect %s group_num %d info (possibly wrong order.)\n wrong group info at : %s ",
          expect_name.c_str(),
          group_num,
          group_type_and_nodes_infos[0].c_str());
    check_str(expect_name, group_type_and_nodes_infos[1], group_num);
}

void check_consensus_str(std::string const & str) {
    std::vector<std::string> group_string_value;
    top::base::xstring_utl::split_string(str, '|', group_string_value);

    auto const auditor_group_count = XGET_CONFIG(auditor_group_count);
    auto const validator_group_count = XGET_CONFIG(validator_group_count);

    CHECK(group_string_value.size() == auditor_group_count + validator_group_count,
          "validator + auditor group num wrong. %zu != %d",
          group_string_value.size(),
          auditor_group_count + validator_group_count);

    auto vi = 0;
    for (auto index = 1; index <= auditor_group_count; index++) {
        check_auditor_validator_str("auditor", group_string_value[vi++], index);
        for (auto index2 = 1; index2 <= validator_group_count / auditor_group_count; index2++) {
            check_auditor_validator_str("validator", group_string_value[vi++], (index - 1) * (validator_group_count / auditor_group_count) + 63 + index2);
        }
    }

    std::cout << std::endl;
}

void check_standby_str(std::string const & str){
        std::vector<std::string> node_standby_infos;
        top::base::xstring_utl::split_string(str, ',', node_standby_infos);
        std::cout << "standby size: " << node_standby_infos.size() << std::endl;
        // node_id:pub_key:type.stake|type.stake,node_id:pub_key:type.stake|type.stake,...
        for (auto const & nodes : node_standby_infos) {
            std::vector<std::string> one_node_info;
            top::base::xstring_utl::split_string(nodes, ':', one_node_info);
            std::string node_id_str = one_node_info[0];
            std::string node_pub_key_str = one_node_info[1];
            std::vector<std::string> type_stake_pairs;
            top::base::xstring_utl::split_string(one_node_info[2], '|', type_stake_pairs);
            std::cout << "--------" << std::endl;
            std::cout<<" node_id: "<<node_id_str<<std::endl;
            std::cout<<" pub_key: "<<node_pub_key_str<<std::endl;

            CHECK(node_id_str.size() == 40, "node id length != 40 at %s", node_id_str.c_str());
            CHECK(node_pub_key_str.size() == 88, "pub key length != 88 at %s", node_pub_key_str.c_str());

            for (auto const & each_pair : type_stake_pairs) {
                std::vector<std::string> type_stake;
                top::base::xstring_utl::split_string(each_pair, '.', type_stake);
                std::string node_type_str = type_stake[0];
                CHECK(
                    (node_type_str == "rec" || node_type_str == "zec" || node_type_str == "adv" || node_type_str == "con" || node_type_str == "edge" || node_type_str == "archive"),
                    "node_type wrong!");

                uint64_t stake = static_cast<std::uint64_t>(std::atoi(type_stake[1].c_str()));
                std::cout << "    type: " << node_type_str << "  stake: " << stake << std::endl;
            }
        }
        return;
}

int main(int argc, char * argv[]) {
    if (argc != 2) {
        usage();
        return -1;
    }
    std::string config_file = argv[1];
    auto & config_center = top::config::xconfig_register_t::get_instance();
    auto offchain_loader = std::make_shared<loader::xconfig_offchain_loader_t>(config_file, "");
    config_center.add_loader(offchain_loader);
    config_center.load();
    config_center.remove_loader(offchain_loader);

    std::string consensus_start_nodes_infos;
    config_center.get(std::string("consensus_start_nodes"), consensus_start_nodes_infos);
    CHECK(!consensus_start_nodes_infos.empty(), "consensus_start_nodes info is empty");
    check_consensus_str(consensus_start_nodes_infos);

    std::string zec_start_nodes_infos;
    config_center.get(std::string("zec_start_nodes"), zec_start_nodes_infos);
    CHECK(!zec_start_nodes_infos.empty(), "zec_start_nodes info is empty");
    check_str("zec", zec_start_nodes_infos);

    std::string edge_start_nodes_infos;
    config_center.get(std::string("edge_start_nodes"), edge_start_nodes_infos);
    CHECK(!edge_start_nodes_infos.empty(), "edge_start_nodes info is empty");
    check_str("edge", edge_start_nodes_infos);

    std::string archive_start_nodes_infos;
    config_center.get(std::string("archive_start_nodes"), archive_start_nodes_infos);
    CHECK(!archive_start_nodes_infos.empty(), "archive_start_nodes info is empty");
    check_str("archive", archive_start_nodes_infos);

    std::string standby_start_nodes_info;
    config_center.get(std::string("standby_start_nodes"),standby_start_nodes_info);
    CHECK(!standby_start_nodes_info.empty(),"standby_start_nodes info is empty");
    check_standby_str(standby_start_nodes_info);

    std::cout << std::endl << std::endl << "result: OK! config file format right" << std::endl;
    return 0;
}
