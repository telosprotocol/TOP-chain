// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xelect_net/demo/multilayer_network_demo.h"

#include "xelect_net/demo/elect_manager_demo.h"

#include <fstream>

namespace top {
namespace elect {

static const std::string kConfigFile("config/config.json");

int MultilayerNetworkDemo::HandleParamsAndConfig(int argc, char ** argv, top::base::Config & edge_config) {
    top::ArgsParser args_parser;
    if (ParseParams(argc, argv, args_parser) != top::kadmlia::kKadSuccess) {
        TOP_FATAL("parse params failed!");
        return 1;
    }

#ifdef DEBUG
    std::vector<std::string> p_vec = {"", "mode1", "mode2", "h", "help", "g", "show_cmd", "P", "local_port", "a", "local_ip", "m", "mode", "q", "aqaa"};
    for (const auto & key : p_vec) {
        if (args_parser.HasParam(key)) {
            std::string value;
            args_parser.GetParam(key, value);
            std::cout << "has param: " << key << " value:" << value << std::endl;
        } else {
            std::cout << "not has param: " << key << std::endl;
        }
    }
#endif

    if (args_parser.HasParam("help")) {  // attention: should use long name not short name
        std::cout << "Allowed options:" << std::endl;
        std::cout << "\t-h [help]                print help info" << std::endl;
        std::cout << "\t-g [show_cmd]            show_cmd or as deamon" << std::endl;
        std::cout << "\t-P [local_port]          local udp port" << std::endl;
        std::cout << "\t-a [local_ip]            local ip " << std::endl;
        std::cout << "\t-o [country_code]        local country code" << std::endl;
        std::cout << "\t-z [zone_id]             zone_id" << std::endl;
        std::cout << "\t-c [config_path]         config path" << std::endl;
        std::cout << "\t-d [db_path]             db path" << std::endl;
        std::cout << "\t-L [log_path]            log path" << std::endl;
        std::cout << "\t-f [frist_node]          this is the first node in this network" << std::endl;
        std::cout << "\t-p [public_endpoints]    bootstrap public_endpoints[ip:port]" << std::endl;
        std::cout << "\t-n [node_id]             account id" << std::endl;
        std::cout << "\t-v [version]             version" << std::endl;
        std::cout << "\t-x [xelect_net_demo] generate node_ids for xelect_net_demo" << std::endl;

        std::cout << "\t [test1] do nothing, just test" << std::endl;
        std::cout << "\t [test2] do nothing, just test" << std::endl;
        ::_Exit(0);
    }

    std::string config_path;
    args_parser.GetParam("c", config_path);
    if (config_path.empty()) {
        config_path = kConfigFile;
    }
    if (!edge_config.Init(config_path.c_str())) {
        TOP_FATAL("init config file failed: %s", config_path.c_str());
        return 1;
    }

    if (!ResetEdgeConfig(args_parser, edge_config)) {
        TOP_FATAL("reset edge config with arg parser failed!");
        return 1;
    }

    if (args_parser.HasParam("x")) {
        TOP_INFO("will generate xelectnetdemo nodes");
        GenerateXelectNetDemoNodes(edge_config);
        ::_Exit(0);
    }

    return 0;
}

bool MultilayerNetworkDemo::GenerateXelectNetDemoNodes(const top::base::Config & edge_config) {
    int total_node = 100;
    if (!edge_config.Get("xelect_net_demo", "total", total_node)) {
        TOP_FATAL("you must set [xelect_net_demo][total] for your test net");
        return false;
    }
    int rec = 4;
    int zec = 4;
    int edg = 2;

    int adv_net = 2;  // adv network number
    int adv = 60;     // adv network node number

    int val_net = 4;
    int val = 60;

    int arc = 60;

    edge_config.Get("xelect_net_demo", "rec", rec);
    edge_config.Get("xelect_net_demo", "zec", zec);
    edge_config.Get("xelect_net_demo", "edg", edg);
    edge_config.Get("xelect_net_demo", "adv_net", adv_net);
    edge_config.Get("xelect_net_demo", "adv", adv);
    edge_config.Get("xelect_net_demo", "val_net", val_net);
    edge_config.Get("xelect_net_demo", "val", val);
    edge_config.Get("xelect_net_demo", "arc", arc);

    std::cout << "\ttotal:      " << total_node << std::endl;
    std::cout << "\trec:        " << rec << std::endl;
    std::cout << "\tzec:        " << zec << std::endl;
    std::cout << "\tedg:        " << edg << std::endl;
    std::cout << "\tarc:        " << arc << std::endl;
    std::cout << "\tadv_net:    " << adv_net << std::endl;
    std::cout << "\tadv:        " << adv << std::endl;
    std::cout << "\tval_net:    " << val_net << std::endl;
    std::cout << "\tval:        " << val << std::endl;

    if (rec > total_node) {
        TOP_FATAL("rec:%d larger than total:%d", rec, total_node);
        return false;
    }
    if (zec > total_node) {
        TOP_FATAL("zec:%d larger than total:%d", zec, total_node);
        return false;
    }
    if (edg > total_node) {
        TOP_FATAL("edg:%d larger than total:%d", edg, total_node);
        return false;
    }
    if (adv > total_node) {
        TOP_FATAL("adv:%d larger than total:%d", adv, total_node);
        return false;
    }
    if (val > total_node) {
        TOP_FATAL("val:%d larger than total:%d", val, total_node);
        return false;
    }
    if (arc > total_node) {
        TOP_FATAL("arc:%d larger than total:%d", arc, total_node);
        return false;
    }
    if (val_net < adv_net or (val_net % adv_net != 0)) {
        TOP_FATAL("val_net:%d smaller than adv_net:%d or size not match", val_net, adv_net);
        return false;
    }

    json all_info;
    all_info["rec"] = json::array();  // list of dict
    all_info["zec"] = json::array();
    all_info["edg"] = json::array();
    all_info["arc"] = json::array();
    for (int n = 1; n <= adv_net; ++n) {
        std::string adv_key = "adv" + std::to_string(n);
        all_info[adv_key] = json::array();
    }
    for (int n = 1; n <= val_net; ++n) {
        std::string val_key = "val" + std::to_string(n);
        all_info[val_key] = json::array();
    }
    std::vector<std::string> node_id_vec;                               // all node accounts
    node_id_vec.push_back("DEMOFULLNODEACCOUNT!DEMOFULLNODEACCOUNT!");  // fix the first node  id
    for (int i = 1; i < total_node; ++i) {
        std::string node_id = HexEncode(RandomString(20));
        node_id_vec.push_back(node_id);
    }

    all_info["fullnode"] = node_id_vec[0];  // the 0 index node is full node

    auto callback = [&node_id_vec](std::vector<std::string> & random_node_id_vec, uint32_t number) {
        random_node_id_vec.clear();
        std::set<size_t> index_set;

        index_set.insert(0);  // 0 node if full node
        random_node_id_vec.push_back(node_id_vec[0]);

        while (random_node_id_vec.size() < number) {
            uint32_t rand_index = RandomUint32() % node_id_vec.size();
            if (index_set.find(rand_index) != index_set.end()) {
                continue;  // already in set, than random again
            }
            index_set.insert(rand_index);
            random_node_id_vec.push_back(node_id_vec[rand_index]);
        }
        return;
    };

    // generate node_id and xip

    std::vector<std::string> random_node_id_vec;

    random_node_id_vec.clear();
    callback(random_node_id_vec, static_cast<uint32_t>(rec));
    if (random_node_id_vec.size() != static_cast<uint32_t>(rec)) {
        TOP_FATAL("random_node_id:%lu choose failed for rec:%u", random_node_id_vec.size(), static_cast<uint32_t>(rec));
        return false;
    }
    for (int i = 0; i < rec; ++i) {
        base::XipParser platform_xip;
        platform_xip.set_xnetwork_id(0);
        platform_xip.set_zone_id(1);
        platform_xip.set_cluster_id(0);
        platform_xip.set_group_id(0);

        json node;
        node["node_id"] = random_node_id_vec[i];
        node["xip"] = HexEncode(platform_xip.xip());
        node["pubkey"] = random_node_id_vec[i];  // using node_id as pubkey
        node["gid"] = 0;                         // associated_gid
        all_info["rec"].push_back(node);
    }

    random_node_id_vec.clear();
    callback(random_node_id_vec, static_cast<uint32_t>(zec));
    if (random_node_id_vec.size() != static_cast<uint32_t>(zec)) {
        TOP_FATAL("random_node_id:%lu choose failed for zec:%u", random_node_id_vec.size(), static_cast<uint32_t>(zec));
        return false;
    }
    for (int i = 0; i < zec; ++i) {
        base::XipParser platform_xip;
        platform_xip.set_xnetwork_id(0);
        platform_xip.set_zone_id(2);
        platform_xip.set_cluster_id(0);
        platform_xip.set_group_id(0);

        json node;
        node["node_id"] = random_node_id_vec[i];
        node["xip"] = HexEncode(platform_xip.xip());
        node["pubkey"] = random_node_id_vec[i];  // using node_id as pubkey
        node["gid"] = 0;                         // associated_gid
        all_info["zec"].push_back(node);
    }

    random_node_id_vec.clear();
    callback(random_node_id_vec, static_cast<uint32_t>(edg));
    if (random_node_id_vec.size() != static_cast<uint32_t>(edg)) {
        TOP_FATAL("random_node_id:%lu choose failed for edg:%u", random_node_id_vec.size(), static_cast<uint32_t>(edg));
        return false;
    }
    for (int i = 0; i < edg; ++i) {
        base::XipParser platform_xip;
        platform_xip.set_xnetwork_id(0);
        platform_xip.set_zone_id(15);
        platform_xip.set_cluster_id(1);
        platform_xip.set_group_id(1);

        json node;
        node["node_id"] = random_node_id_vec[i];
        node["xip"] = HexEncode(platform_xip.xip());
        node["pubkey"] = random_node_id_vec[i];  // using node_id as pubkey
        node["gid"] = 0;                         // associated_gid
        all_info["edg"].push_back(node);
    }

    random_node_id_vec.clear();
    callback(random_node_id_vec, static_cast<uint32_t>(arc));
    if (random_node_id_vec.size() != static_cast<uint32_t>(arc)) {
        TOP_FATAL("random_node_id:%lu choose failed for arc:%u", random_node_id_vec.size(), static_cast<uint32_t>(arc));
        return false;
    }
    for (int i = 0; i < arc; ++i) {
        base::XipParser platform_xip;
        platform_xip.set_xnetwork_id(0);
        platform_xip.set_zone_id(14);
        platform_xip.set_cluster_id(1);
        platform_xip.set_group_id(1);

        json node;
        node["node_id"] = random_node_id_vec[i];
        node["xip"] = HexEncode(platform_xip.xip());
        node["pubkey"] = random_node_id_vec[i];  // using node_id as pubkey
        node["gid"] = 0;                         // associated_gid
        all_info["arc"].push_back(node);
    }

    // usually adv_net is [1,64)
    for (int n = 1; n <= adv_net; ++n) {
        std::string adv_key = "adv" + std::to_string(n);

        random_node_id_vec.clear();
        callback(random_node_id_vec, static_cast<uint32_t>(adv));
        if (random_node_id_vec.size() != static_cast<uint32_t>(adv)) {
            TOP_FATAL("random_node_id:%lu choose failed for %s:%u", random_node_id_vec.size(), adv_key.c_str(), static_cast<uint32_t>(adv));
            return false;
        }
        for (int i = 0; i < adv; ++i) {
            base::XipParser platform_xip;
            platform_xip.set_xnetwork_id(0);
            platform_xip.set_zone_id(0);
            platform_xip.set_cluster_id(1);
            platform_xip.set_group_id(n + 0);

            json node;
            node["node_id"] = random_node_id_vec[i];
            node["xip"] = HexEncode(platform_xip.xip());
            node["pubkey"] = random_node_id_vec[i];  // using node_id as pubkey
            node["gid"] = 0;                         // associated_gid
            all_info[adv_key].push_back(node);
        }
    }

    int group_size = val_net / adv_net;
    std::cout << "val group size is " << group_size << std::endl;

    // usually val is [64, 127)
    for (int v = 1; v <= val_net; ++v) {
        std::string val_key = "val" + std::to_string(v);
        int adv_group = (v - 1) / group_size + 1;

        random_node_id_vec.clear();
        callback(random_node_id_vec, static_cast<uint32_t>(val));
        if (random_node_id_vec.size() != static_cast<uint32_t>(val)) {
            TOP_FATAL("random_node_id:%lu choose failed for %s:%d", random_node_id_vec.size(), val_key.c_str(), static_cast<uint32_t>(val));
            return false;
        }
        for (int i = 0; i < val; ++i) {
            base::XipParser platform_xip;
            platform_xip.set_xnetwork_id(0);
            platform_xip.set_zone_id(0);
            platform_xip.set_cluster_id(1);
            platform_xip.set_group_id(v + 64);

            json node;
            node["node_id"] = random_node_id_vec[i];
            node["xip"] = HexEncode(platform_xip.xip());
            node["pubkey"] = random_node_id_vec[i];  // using node_id as pubkey
            node["gid"] = adv_group;                 // associated_gid
            all_info[val_key].push_back(node);
        }
    }

    all_info["all"] = node_id_vec;
    std::ofstream out("./config/all_node_info.json");
    out << std::setw(4) << all_info << std::endl;
    std::cout << "total:" << total_node << " nodes info saved in file:./config/all_node_info.json" << std::endl;
    out.close();
    return true;
}

bool MultilayerNetworkDemo::BuildXelectNetDemoNetwork() {
    // read a JSON file
    std::ifstream in("./config/all_node_info.json");
    if (!in) {
        TOP_FATAL("open file:./config/all_node_info.json error");
        return false;
    }

    json all_info;
    in >> all_info;
    in.close();

    elect_manager_->OnElectUpdated(all_info);
    return true;
}

int MultilayerNetworkDemo::ParseParams(int argc, char ** argv, top::ArgsParser & args_parser) {
    args_parser.AddArgType('h', "help", top::kNoValue);
    args_parser.AddArgType('g', "show_cmd", top::kMaybeValue);
    args_parser.AddArgType('P', "local_port", top::kMaybeValue);
    args_parser.AddArgType('a', "local_ip", top::kMaybeValue);
    args_parser.AddArgType('o', "country_code", top::kMaybeValue);
    args_parser.AddArgType('z', "zone_id", top::kMaybeValue);
    args_parser.AddArgType('c', "config_path", top::kMaybeValue);
    args_parser.AddArgType('d', "db_path", top::kMaybeValue);
    args_parser.AddArgType('L', "log_path", top::kMaybeValue);
    args_parser.AddArgType('f', "first_node", top::kMaybeValue);
    args_parser.AddArgType('p', "public_endpoints", top::kMaybeValue);
    args_parser.AddArgType('n', "node_id", top::kMaybeValue);  // node_id, usually account id(not kroot id or kad_key)
    args_parser.AddArgType('v', "version", top::kNoValue);
    args_parser.AddArgType('x', "xelect_net_demo", top::kMaybeValue);

    args_parser.AddArgType(0, "test1", top::kMaybeValue);
    args_parser.AddArgType(0, "test2", top::kMaybeValue);

    std::string tmp_params = "";
    for (int i = 1; i < argc; i++) {
        if (strlen(argv[i]) == 0) {
            tmp_params += static_cast<char>(31);
        } else {
            tmp_params += argv[i];
        }
        tmp_params += " ";
    }
    std::cout << "parse params :" << tmp_params << std::endl;
    std::string err_pos;
    if (args_parser.Parse(tmp_params, err_pos) != top::kadmlia::kKadSuccess) {
        std::cout << "parse params failed!" << std::endl;
        return top::kadmlia::kKadFailed;
    }

    return top::kadmlia::kKadSuccess;
}

bool MultilayerNetworkDemo::Init(const base::Config & config) {
    bool show_cmd = true;
    config.Get("node", "show_cmd", show_cmd);
    bool first_node = false;
    config.Get("node", "first_node", first_node);
    elect_cmd_.Init(first_node, show_cmd);
    if (!MultilayerNetwork::Init(config)) {
        TOP_WARN("MultilayerNetwork:Init failed");
        return false;
    }
    if (!GetEcNetcard()) {
        TOP_FATAL("ec_netcard empty");
        return false;
    }
    elect_cmd_.set_netcard(GetEcNetcard());
    // create elect_manager_chain
    elect_manager_ = std::make_shared<ElectManagerDemo>(GetCoreTransport(), config);
    if (!elect_manager_) {
        TOP_WARN("demo elect_manager  create failed");
        return false;
    }

    return true;
}

bool MultilayerNetworkDemo::Run(const base::Config & config) {
    if (ResetRootRouting(GetCoreTransport(), config) == top::kadmlia::kKadFailed) {
        TOP_WARN("kRoot join network failed");
        return false;
    }

    if (!elect_manager_->Start()) {
        std::cout << "elect network init failed!" << std::endl;
        return false;
    }

    return true;
}

void MultilayerNetworkDemo::Stop() {
    elect_manager_.reset();

    MultilayerNetwork::Stop();
}

}  // namespace elect

}  // namespace top
