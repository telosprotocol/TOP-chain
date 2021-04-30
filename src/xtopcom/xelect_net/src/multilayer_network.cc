// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xelect_net/include/multilayer_network.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "xtransport/udp_transport/udp_transport.h"
#include "xkad/nat_detect/nat_manager_intf.h"
#include "xkad/routing_table/routing_utils.h"
#include "xwrouter/register_routing_table.h"
#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/multi_routing/small_net_cache.h"
#include "xwrouter/multi_routing/service_node_cache.h"
#include "xkad/proto/ledger.pb.h"
#include "xdb/xdb_factory.h"

namespace top {
namespace elect {

static const std::string kKadmliaKeyDbKey = "KADMLIA_KEY_DB_KEY";
static const std::string kKadmliaKeyField("kad_key_");
static const std::string kConfigFile("config/config.json");

MultilayerNetwork::MultilayerNetwork() {
    ec_netcard_ = std::make_shared<EcNetcard>();
    TOP_INFO("create ec_netcard");
    multi_message_handler_ = std::make_shared<top::transport::MultiThreadHandler>();
    TOP_INFO("create multi_message_handler");
    core_transport_ = std::make_shared<top::transport::UdpTransport>();
    TOP_INFO("create core_transport");
}


bool MultilayerNetwork::Init(const base::Config& config) {
    // bool show_cmd = true;
    // config.Get("node", "show_cmd", show_cmd);
    // bool first_node = false;
    // config.Get("node", "first_node", first_node);
    // elect_cmd_.Init(first_node, show_cmd);
    std::string db_path;
    if (!config.Get("db", "path", db_path)) {
        TOP_ERROR("get db path from conf failed[%s]", db_path.c_str());
        return false;
    }

    if (!top::kadmlia::CreateGlobalXid(config)) {
        return false;
    }
    top::kadmlia::CallbackManager::Instance();

    std::string country;
    if (!config.Get("node", "country", country)) {
        TOP_ERROR("get node country from conf failed[%s]", "country");
        return false;
    }

    if (!multi_message_handler_) {
        TOP_ERROR("multi_message_handler empty");
        return false;
    }
    multi_message_handler_->Init();
    RegisterCallbackForMultiThreadHandler(multi_message_handler_);
    // attention: InitWrouter must put befor core_transport->Start
    InitWrouter(core_transport_, multi_message_handler_);

    std::string local_ip;
    if (!config.Get("node", "local_ip", local_ip)) {
        TOP_ERROR("get node local_ip from config failed!");
        return false;
    }
    uint16_t local_port = 0;
    config.Get("node", "local_port", local_port);
    if (core_transport_->Start(
            local_ip,
            local_port,
            multi_message_handler_.get()) != top::kadmlia::kKadSuccess) {
        TOP_ERROR("start local udp transport failed!");
        return false;
    }
    core_transport_->RegisterOfflineCallback(kadmlia::HeartbeatManagerIntf::OnHeartbeatCallback);

    wrouter::SmallNetNodes::Instance()->Init();
    TOP_INFO("Init SmallNetNodes for Elect Network");
    wrouter::ServiceNodes::Instance()->Init();
    TOP_INFO("Init ServiceNodes for cache nodes");

    if (InitDb(config) != 0) {
        TOP_FATAL("init db failed!");
        return false;
    }

    if (!ec_netcard_) {
        TOP_FATAL("ec_netcard creat failed");
        return false;
    }
    TOP_INFO("created EcNetcard");
    ec_netcard_->Init();
    // elect_cmd_.set_netcard(ec_netcard_);
    return true;
}

bool MultilayerNetwork::Run(const base::Config& config) {
    if (ResetRootRouting(core_transport_, config) == top::kadmlia::kKadFailed) {
        TOP_WARN("kRoot join network failed");
        return false;
    }

    // will block here
    //elect_cmd_.Run();

    return true;
}

void MultilayerNetwork::Stop() {
    root_manager_ptr_.reset();
    if (core_transport_) {
        core_transport_->Stop();
        core_transport_.reset();
    }

    if (nat_transport_) {
        nat_transport_->Stop();
        nat_transport_.reset();
    }

    multi_message_handler_.reset();
}

int MultilayerNetwork::ParseParams(int argc, char** argv, top::ArgsParser& args_parser) {  // NOLINT
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
    args_parser.AddArgType('n', "node_id", top::kMaybeValue); // node_id, usually account id(not kroot id or kad_key)
    args_parser.AddArgType('v', "version", top::kNoValue);

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

int MultilayerNetwork::HandleParamsAndConfig(int argc, char** argv, top::base::Config& edge_config) {
    top::ArgsParser args_parser;
    if (ParseParams(argc, argv, args_parser) != top::kadmlia::kKadSuccess) {
        TOP_FATAL("parse params failed!");
        return 1;
    }

    if (args_parser.HasParam("h")) {
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

    return 0;
}

bool MultilayerNetwork::ResetEdgeConfig(top::ArgsParser& args_parser, top::base::Config& edge_config) {
    std::string node_id;
    if (args_parser.GetParam("n", node_id) == top::kadmlia::kKadSuccess) {
        if (!edge_config.Set("node", "node_id", node_id)) {
            TOP_ERROR("set config failed [node][node_id][%s]", node_id.c_str());
            return false;
        }
    }

    std::string db_path;
    if (args_parser.GetParam("d", db_path) == top::kadmlia::kKadSuccess) {
        if (!edge_config.Set("db", "path", db_path)) {
            TOP_ERROR("set config failed [db][path][%s]", db_path.c_str());
            return false;
        }
    }
    std::string country;
    args_parser.GetParam("o", country);
    if (!country.empty()) {
        if (!edge_config.Set("node", "country", country)) {
            TOP_WARN("set config failed [node][country][%s]", country.c_str());
            return false;
        }
    }

    int zone_id;
    if (args_parser.GetParam("z", zone_id) == top::kadmlia::kKadSuccess) {
        if (!edge_config.Set("node", "zone_id", zone_id)) {
            TOP_WARN("set config failed [node][zone_id][%u]", zone_id);
            return false;
        }
    }

    std::string local_ip;
    args_parser.GetParam("a", local_ip);
    if (!local_ip.empty()) {
        if (!edge_config.Set("node", "local_ip", local_ip)) {
            TOP_WARN("set config failed [node][local_ip][%s]", local_ip.c_str());
            return false;
        }
    }
    uint16_t local_port = 0;
    if (args_parser.GetParam("P", local_port) == top::kadmlia::kKadSuccess) {
        if (!edge_config.Set("node", "local_port", local_port)) {
            TOP_WARN("set config failed [node][local_port][%d]", local_port);
            return false;
        }
    }

    int first_node = 0;
    if (args_parser.GetParam("f", first_node) == top::kadmlia::kKadSuccess) {
        if (!edge_config.Set("node", "first_node", first_node)) {
            TOP_WARN("set config failed [node][first_node][%d]", first_node);
            return false;
        }
    }

    std::string peer;
    args_parser.GetParam("p", peer);
    if (!peer.empty()) {
        if (!edge_config.Set("node", "public_endpoints", peer)) {
            TOP_WARN("set config failed [node][public_endpoints][%s]", peer.c_str());
            return false;
        }
    }

    int show_cmd = 1;
    if (args_parser.GetParam("g", show_cmd) == top::kadmlia::kKadSuccess) {
        if (!edge_config.Set("node", "show_cmd", show_cmd == 1)) {
            TOP_WARN("set config failed [node][show_cmd][%d]", show_cmd);
            return false;
        }
    }

    std::string log_path;
    if (args_parser.GetParam("L", log_path) == top::kadmlia::kKadSuccess) {
        if (!edge_config.Set("log", "path", log_path)) {
            TOP_WARN("set config failed [log][log_path][%s]", log_path.c_str());
            return false;
        }
    }

    return true;
}

void MultilayerNetwork::InitWrouter(
        top::transport::TransportPtr transport,
        std::shared_ptr<top::transport::MultiThreadHandler> message_handler) {
    /*
    base::xiothread_t* io_thread = top::base::xiothread_t::create_thread(
            top::base::xcontext_t::instance(), 0, -1);

    if (io_thread == NULL) {
        TOP_ERROR("create xio thread failed!");
        assert(false);
        return;
    }
    */

    wrouter::Wrouter::Instance()->Init(
            base::xcontext_t::instance(),
            //io_thread->get_thread_id(),
            0,
            transport);
}

void MultilayerNetwork::RegisterCallbackForMultiThreadHandler(
        std::shared_ptr<top::transport::MultiThreadHandler> multi_thread_message_handler) {
    multi_thread_message_handler->register_on_dispatch_callback(std::bind(&wrouter::Wrouter::recv,
            wrouter::Wrouter::Instance(),
            std::placeholders::_1,
            std::placeholders::_2));
}

std::shared_ptr<top::wrouter::RootRoutingManager> MultilayerNetwork::CreateRootManager(
        bool client,
        std::shared_ptr<transport::Transport> transport,
        const top::base::Config& config,
        const std::set<std::pair<std::string, uint16_t>>& public_endpoints_config) {
    TOP_INFO("enter CreateRootManager");
    top::base::Config new_config = config;
    if (!new_config.Set("edge", "service_list", "")) {
        TOP_ERROR("set config edge service_list failed!");
        return nullptr;
    }

    if (client) {
        if (!new_config.Set("node", "client_mode", true)) {
            TOP_ERROR("set config node client_mode failed!");
            return nullptr;
        }
    }
    auto root_manager_ptr = wrouter::RootRoutingManager::Instance();
    wrouter::MultiRouting::Instance()->SetRootRoutingManager(root_manager_ptr);

    uint32_t zone_id = 0;
    if (!kadmlia::GetZoneIdFromConfig(config, zone_id)) {
        TOP_ERROR("get zone id from config failed!");
        return nullptr;
    }

    // get kroot id
    base::KadmliaKeyPtr kad_key_ptr = base::GetKadmliaKey(global_node_id, true);
    if (KadKey_GetFromDb(kad_key_ptr, kKadmliaKeyField + "root" + global_node_id) != 0) {
        if (KadKey_StoreInDb(kad_key_ptr, kKadmliaKeyField + "root" + global_node_id) != 0) {
            TOP_FATAL("save root kad key to db failed!");
            return nullptr;
        }
        TOP_INFO("save root kad key: %s to db success", HexEncode(kad_key_ptr->Get()).c_str());
    }
    TOP_INFO("get root kad key: %s from db success", HexEncode(kad_key_ptr->Get()).c_str());

    auto get_cache_callback = std::bind(&MultilayerNetwork::GetBootstrapCacheCallback, this, std::placeholders::_1, std::placeholders::_2);
    auto set_cache_callback = std::bind(&MultilayerNetwork::SetBootstrapCacheCallback, this, std::placeholders::_1, std::placeholders::_2);
    if (root_manager_ptr->AddRoutingTable(
            transport,
            new_config,
            kad_key_ptr,
            get_cache_callback,
            set_cache_callback) != top::kadmlia::kKadSuccess) {
        TOP_ERROR("<blueshi> add root_table[root] failed!");
        return nullptr;
    }
    TOP_INFO("CreateRootManager ok.");
    return root_manager_ptr;
}

int MultilayerNetwork::ResetRootRouting(
        std::shared_ptr<transport::Transport> transport,
        const base::Config& config) {
    root_manager_ptr_.reset();
    wrouter::MultiRouting::Instance()->SetRootRoutingManager(nullptr);
    std::set<std::pair<std::string, uint16_t>> public_endpoints_config;
    kadmlia::GetPublicEndpointsConfig(config, public_endpoints_config);

    root_manager_ptr_ = CreateRootManager(
            false,
            transport,
            config,
            public_endpoints_config);
    if (!root_manager_ptr_) {
        TOP_ERROR("create root manager failed!");
        return top::kadmlia::kKadFailed;
    }
    return top::kadmlia::kKadSuccess;
}
int MultilayerNetwork::InitDb(const base::Config& config) {
    std::string db_path;
    if (!config.Get("db", "path", db_path)) {
        TOP_FATAL("get db path from conf failed[%s]", db_path.c_str());
        return 1;
    }
    assert(!net_db_);
    net_db_ = db::xdb_factory_t::instance(db_path);
    if (!net_db_) {
        TOP_ERROR("init network layer db:%s failed", db_path.c_str());
        return 1;
    }
    
    TOP_INFO("init network layer db:%s", db_path.c_str());
    return 0;
}

int MultilayerNetwork::KadKey_GetFromDb(
        base::KadmliaKeyPtr& kadkey,
        const std::string& db_field) {
    if (!net_db_) {
        return 1;
    }

    std::string db_key(kKadmliaKeyDbKey);
    db_key += db_field;
    std::string kad_key_str_hex;
    if (!net_db_->read(db_key, kad_key_str_hex)) {
        TOP_WARN("read kad_key:%s from db failed", db_key.c_str());
        return 1;
    }

    kadkey = base::GetKadmliaKey(HexDecode(kad_key_str_hex));
    return 0;
}

int MultilayerNetwork::KadKey_StoreInDb(
        base::KadmliaKeyPtr& kadkey,
        const std::string& db_field) {
    if (!net_db_) {
        return 1;
    }

    std::string db_key(kKadmliaKeyDbKey);
    db_key += db_field;
    std::string kad_key_str_hex = HexEncode(kadkey->Get());
    net_db_->write(db_key, kad_key_str_hex);
    return 0;
}

// size
static std::string ToString(const VecBootstrapEndpoint& vec_bootstrap_endpoint) {
    top::kadmlia::pb::PbBootstrapCache pb_cache;
    for (const auto& ep : vec_bootstrap_endpoint) {
        auto pb_ep = pb_cache.add_endpoints();
        pb_ep->set_ip(ep.first);
        pb_ep->set_port(ep.second);
    }
    std::string ret;
    assert(pb_cache.SerializeToString(&ret));
    return ret;
}

static bool ToVecBootstrapEndpoint(const std::string& str, VecBootstrapEndpoint& vec_bootstrap_endpoint) {
    top::kadmlia::pb::PbBootstrapCache pb_cache;
    if (!pb_cache.ParseFromString(str)) {
        TOP_ERROR("cache.ParseFromString failed");
        return false;
    }

    vec_bootstrap_endpoint.clear();
    for (int i = 0; i < pb_cache.endpoints_size(); ++i) {
        const auto& pb_ep = pb_cache.endpoints(i);
        if (pb_ep.port() > std::numeric_limits<uint16_t>::max()) {
            TOP_WARN("ignore invalid endpoint(%s:%d)", pb_ep.ip().c_str(), (int)pb_ep.port());
            continue;
        }
        vec_bootstrap_endpoint.push_back(std::make_pair(pb_ep.ip(), (uint16_t)pb_ep.port()));
    }

    return true;
}


bool MultilayerNetwork::GetBootstrapCacheCallback(const uint64_t& service_type, VecBootstrapEndpoint& vec_bootstrap_endpoint) {
    if (!net_db_) {
        TOP_ERROR("network layer db invalid");
        return false;
    }
    const std::string field = std::to_string(service_type);
    std::string db_key(top::kadmlia::BOOTSTRAP_CACHE_DB_KEY);
    db_key += field;
    std::string value;

    if (!net_db_->read(db_key, value)) {
        TOP_WARN("read bootstrapcache of service_type:%llu from db failed", service_type);
        return false;
    }

    if (!ToVecBootstrapEndpoint(value, vec_bootstrap_endpoint)) {
        TOP_ERROR("parse vec_bootstrap_endpoint from value failed");
        return false;
    }

    return true;

}

bool MultilayerNetwork::SetBootstrapCacheCallback(const uint64_t& service_type, const VecBootstrapEndpoint& vec_bootstrap_endpoint) {
    if (!net_db_) {
        TOP_ERROR("network layer db invalid");
        return false;
    }
    const std::string field = std::to_string(service_type);
    const std::string value = ToString(vec_bootstrap_endpoint);
    std::string db_key(top::kadmlia::BOOTSTRAP_CACHE_DB_KEY);
    db_key += field;
    net_db_->write(db_key, value);
    TOP_DEBUG("dump service_type:%llu bootstrapcache to db", service_type);
    return true;

}

}  // namespace elect

}  // namespace top
