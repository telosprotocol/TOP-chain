// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xelect_net/include/multilayer_network.h"

#include "xdb/xdb_factory.h"
#include "xelect_net/include/http_client.h"
#include "xelect_net/include/https_client.h"
#include "xkad/routing_table/routing_utils.h"
#include "xtransport/udp_transport/udp_transport.h"
#include "xwrouter/multi_routing/multi_routing.h"
#include "xwrouter/multi_routing/service_node_cache.h"
#include "xwrouter/multi_routing/small_net_cache.h"

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

namespace top {
namespace elect {

static const std::string kKadmliaKeyDbKey = "KADMLIA_KEY_DB_KEY";
static const std::string kKadmliaKeyField("kad_key_");
static const std::string kConfigFile("config/config.json");

MultilayerNetwork::MultilayerNetwork(common::xnode_id_t const & node_id, const std::set<uint32_t> & xnetwork_id_set) : m_node_id_{node_id}, xnetwork_id_set_{xnetwork_id_set} {
    ec_netcard_ = std::make_shared<EcNetcard>();
    xinfo("create ec_netcard");
    multi_message_handler_ = std::make_shared<top::transport::MultiThreadHandler>();
    xinfo("create multi_message_handler");
    core_transport_ = std::make_shared<top::transport::UdpTransport>();
    xinfo("create core_transport");

    for (const auto & xnetwork_id : xnetwork_id_set_) {
        auto ecvhost = std::make_shared<EcVHost>(xnetwork_id, GetEcNetcard(), m_node_id_);
        {
            std::unique_lock<std::mutex> lock(vhost_map_mutex_);
            vhost_map_[xnetwork_id] = ecvhost;
        }
        xinfo("add ecvhost xnetwork_id:%u to map", xnetwork_id);
    }
}

void MultilayerNetwork::start() {
    auto & platform_params = data::xplatform_params::get_instance();

    top::base::Config config;
    if (HandleParamsAndConfig(platform_params, config) != 0) {
        xerror("load platform config failed");
        throw std::runtime_error{"HandleParamsAndConfig: load platform config failed"};
    }

    std::string public_endpoints;
    if (!config.Get("node", "public_endpoints", public_endpoints)) {
        xerror("get node public_endpoints from config failed!");
        throw std::runtime_error{"config.Get: get node public_endpoints from config failed!"};
    }
    platform_params.set_seed_edge_host(public_endpoints);

    if (!Init(config)) {
        xerror("start elect main init failed!");
        throw std::runtime_error{"start elect main init failed!"};
    }

    elect_manager_ = std::make_shared<ElectManager>(GetCoreTransport(), config);

    if (!elect_manager_) {
        xerror("elect_manager create failed");
        throw std::runtime_error{"elect_manager init failed"};
    }

    if (!Run(config)) {
        xerror("elect main start failed");
        throw std::runtime_error{"elect main start failed"};
    }
}

void MultilayerNetwork::stop() {
    for (const auto & xnetwork_id : xnetwork_id_set_) {
        auto ecvhost = GetEcVhost(xnetwork_id);
        if (!ecvhost) {
            continue;
        }
        ecvhost->stop();
    }

    elect_manager_.reset();

    // root_manager_ptr_.reset();
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

bool MultilayerNetwork::Init(const base::Config & config) {
    std::string db_path;
    if (!config.Get("db", "path", db_path)) {
        xerror("get db path from conf failed[%s]", db_path.c_str());
        return false;
    }

    if (!top::kadmlia::CreateGlobalXid(config)) {
        return false;
    }
    top::kadmlia::CallbackManager::Instance();

    if (!multi_message_handler_) {
        xerror("multi_message_handler empty");
        return false;
    }
    multi_message_handler_->Init();
    RegisterCallbackForMultiThreadHandler(multi_message_handler_);
    // attention: InitWrouter must put befor core_transport->Start
    InitWrouter(core_transport_, multi_message_handler_);

    std::string local_ip;
    if (!config.Get("node", "local_ip", local_ip)) {
        xerror("get node local_ip from config failed!");
        return false;
    }
    uint16_t local_port = 0;
    config.Get("node", "local_port", local_port);
    if (core_transport_->Start(local_ip, local_port, multi_message_handler_.get()) != top::kadmlia::kKadSuccess) {
        xerror("start local udp transport failed!");
        return false;
    }
    core_transport_->RegisterOfflineCallback(kadmlia::HeartbeatManagerIntf::OnHeartbeatCallback);

    wrouter::SmallNetNodes::Instance()->Init();
    xinfo("Init SmallNetNodes for Elect Network");
    wrouter::ServiceNodes::Instance()->Init();
    xinfo("Init ServiceNodes for cache nodes");

    if (!ec_netcard_) {
        xerror("ec_netcard creat failed");
        return false;
    }
    xinfo("created EcNetcard");
    ec_netcard_->Init();
    // elect_cmd_.set_netcard(ec_netcard_);
    return true;
}

bool MultilayerNetwork::Run(const base::Config & config) {
    if (ResetRootRouting(GetCoreTransport(), config) == top::kadmlia::kKadFailed) {
        xwarn("kRoot join network failed");
        return false;
    }

    for (const auto & xnetwork_id : xnetwork_id_set_) {
        auto ecvhost = GetEcVhost(xnetwork_id);
        if (!ecvhost) {
            continue;
        }
        ecvhost->start();
    }

    return true;
}

bool MultilayerNetwork::RegisterNodeCallback(std::function<int32_t(std::string const & node_addr, std::string const & node_sign)> cb) {
    if (GetCoreTransport() == nullptr) {
        xwarn("register node callback fail: core_transport is nullptr");
        return false;
    }

    GetCoreTransport()->RegisterNodeCallback(cb);
    xinfo("register node callback successful");
    return true;
}

bool MultilayerNetwork::UpdateNodeSizeCallback(std::function<bool(uint64_t & node_size)> cb) {
    return wrouter::MultiRouting::Instance()->UpdateNodeSizeCallback(cb);
}

std::shared_ptr<elect::xnetwork_driver_face_t> MultilayerNetwork::GetEcVhost(const uint32_t & xnetwork_id) const noexcept {
    std::unique_lock<std::mutex> lock(vhost_map_mutex_);
    auto ifind = vhost_map_.find(xnetwork_id);
    if (ifind == vhost_map_.end()) {
        return nullptr;
    }
    return std::static_pointer_cast<elect::xnetwork_driver_face_t>(ifind->second);
}

int MultilayerNetwork::HandleParamsAndConfig(const top::data::xplatform_params & platform_param, top::base::Config & edge_config) {
    if (!edge_config.Set("db", "path", platform_param.db_path)) {
        xerror("set config failed [db][path][%s]", platform_param.db_path.c_str());
        return 1;
    }

    if (!edge_config.Set("log", "path", platform_param.log_path)) {
        xerror("set config failed [log][log_path][%s]", platform_param.log_path.c_str());
        return 1;
    }

    if (!edge_config.Set("node", "country", platform_param.country)) {
        xerror("set config failed [node][country][%s]", platform_param.country.c_str());
        return 1;
    }

    if (!edge_config.Set("node", "zone_id", platform_param.zone_id)) {
        xerror("set config failed [node][zone_id][%d]", platform_param.zone_id);
        return 1;
    }

    if (!edge_config.Set("node", "local_ip", platform_param.local_ip)) {
        xerror("set config failed [node][local_ip][%s]", platform_param.local_ip.c_str());
        return 1;
    }

    if (!edge_config.Set("node", "local_port", platform_param.local_port)) {
        xerror("set config failed [node][local_port][%d]", platform_param.local_port);
        return 1;
    }

    std::string show_cmd_str = (platform_param.show_cmd ? "true" : "false");
    if (!edge_config.Set("node", "show_cmd", show_cmd_str)) {
        xerror("set config failed [node][show_cmd][%s]", show_cmd_str.c_str());
        return 1;
    }

    std::string public_endpoints(platform_param.public_endpoints);
    xinfo("config get public_endpoints %s", public_endpoints.c_str());

    std::string final_public_endpoints;
    final_public_endpoints = public_endpoints;

    edge_config.Set("node", "public_endpoints", final_public_endpoints);
    xinfo("config set final public_endpoints %s", final_public_endpoints.c_str());

    bool endpoints_status = false;
    if (final_public_endpoints.empty()) {
        endpoints_status = true;
        xwarn("config get no public_endpoints %s", final_public_endpoints.c_str());
    }

    if (platform_param.url_endpoints.find("http") == std::string::npos) {
        return endpoints_status;
    }
    xinfo("[seeds] url_endpoints url:%s", platform_param.url_endpoints.c_str());

    std::vector<std::string> url_seeds;
    if (platform_param.url_endpoints.find("https://") != std::string::npos) {
        auto seed_http_client = std::make_shared<SeedHttpsClient>(platform_param.url_endpoints);
        if (!seed_http_client) {
            xwarn("[seeds] create seed_http_client failed");
            return endpoints_status;
        }
        if (!seed_http_client->GetSeeds(url_seeds)) {
            xwarn("[seeds] get public endpoints failed from url:%s", platform_param.url_endpoints.c_str());
            return endpoints_status;
        }
    } else {  // http scheme
        auto seed_http_client = std::make_shared<SeedHttpClient>(platform_param.url_endpoints);
        if (!seed_http_client) {
            xwarn("[seeds] create seed_http_client failed");
            return endpoints_status;
        }
        if (!seed_http_client->GetSeeds(url_seeds)) {
            xwarn("[seeds] get public endpoints failed from url:%s", platform_param.url_endpoints.c_str());
            return endpoints_status;
        }
    }

    std::string bootstrap_nodes;
    for (const auto & item : url_seeds) {
        bootstrap_nodes += item;
        bootstrap_nodes += ",";
    }

    if (bootstrap_nodes.back() == ',') {
        bootstrap_nodes.pop_back();
    }
    xinfo("[seeds] fetch url:%s bootstrap_nodes:%s", platform_param.url_endpoints.c_str(), bootstrap_nodes.c_str());
    if (bootstrap_nodes.empty()) {
        return endpoints_status;
    }

    if (!final_public_endpoints.empty()) {
        final_public_endpoints += ",";
        final_public_endpoints += bootstrap_nodes;
    } else {
        final_public_endpoints = bootstrap_nodes;
    }

    edge_config.Set("node", "public_endpoints", final_public_endpoints);
    xinfo("[seeds] config set final public_endpoints %s", final_public_endpoints.c_str());
    return 0;
}

void MultilayerNetwork::InitWrouter(top::transport::TransportPtr transport, std::shared_ptr<top::transport::MultiThreadHandler> message_handler) {
    wrouter::Wrouter::Instance()->Init(base::xcontext_t::instance(),
                                       // io_thread->get_thread_id(),
                                       0,
                                       transport);
}

void MultilayerNetwork::RegisterCallbackForMultiThreadHandler(std::shared_ptr<top::transport::MultiThreadHandler> multi_thread_message_handler) {
    multi_thread_message_handler->register_on_dispatch_callback(std::bind(&wrouter::Wrouter::recv, wrouter::Wrouter::Instance(), std::placeholders::_1, std::placeholders::_2));
}

int MultilayerNetwork::CreateRootManager(std::shared_ptr<transport::Transport> transport,
                                         const top::base::Config & config,
                                         const std::set<std::pair<std::string, uint16_t>> & public_endpoints_config) {
    xinfo("enter CreateRootManager");
    top::base::Config new_config = config;
    if (!new_config.Set("edge", "service_list", "")) {
        xerror("set config edge service_list failed!");
        return top::kadmlia::kKadFailed;
    }

    // uint32_t zone_id = 0;
    // if (!kadmlia::GetZoneIdFromConfig(config, zone_id)) {
    //     xerror("get zone id from config failed!");
    //     return top::kadmlia::kKadFailed;
    // }

    // get kroot id
    assert(!global_node_id.empty());
    base::KadmliaKeyPtr kad_key_ptr = base::GetRootKadmliaKey(global_node_id);
    xinfo("get root kad key: %s",kad_key_ptr->Get().c_str());

    if (wrouter::MultiRouting::Instance()->CreateRootRouting(transport, new_config, kad_key_ptr) != top::kadmlia::kKadSuccess) {
        // if (root_manager_ptr->InitRootRoutingTable(transport, new_config, kad_key_ptr) != top::kadmlia::kKadSuccess) {
        xerror("<blueshi> add root_table[root] failed!");
        return top::kadmlia::kKadFailed;
    }
    xinfo("CreateRootManager ok.");
    return top::kadmlia::kKadSuccess;
}

int MultilayerNetwork::ResetRootRouting(std::shared_ptr<transport::Transport> transport, const base::Config & config) {
    std::set<std::pair<std::string, uint16_t>> public_endpoints_config;
    kadmlia::GetPublicEndpointsConfig(config, public_endpoints_config);

    return CreateRootManager(transport, config, public_endpoints_config);
}

std::vector<std::string> MultilayerNetwork::GetServiceNeighbours(const common::xip2_t & xip2) {
    std::vector<std::string> nodes_vec;

    auto kad_key = base::GetKadmliaKey(xip2);
    auto rt = wrouter::MultiRouting::Instance()->GetElectRoutingTable(kad_key->GetServiceType());
    if (!rt) {
        xwarn("no routinng table:%llu registered, GetServiceNeighbours failed", kad_key->GetServiceType().value());
        return {};
    }

    std::vector<kadmlia::NodeInfoPtr> vec;
    rt->GetRandomNodes(vec, rt->nodes_size());
    for (const auto & item : vec) {
        auto ifind = std::find(nodes_vec.begin(), nodes_vec.end(), item->public_ip);
        if (ifind == nodes_vec.end()) {
            nodes_vec.push_back(item->public_ip);
            xdbg("GetServiceNeighbours %llu add node:%s", kad_key->GetServiceType().value(), item->public_ip.c_str());
        }
    }
    xinfo("GetServiceNeighbours %u of service_type:%llu", nodes_vec.size(), kad_key->GetServiceType().value());
    return nodes_vec;
}

}  // namespace elect

}  // namespace top
