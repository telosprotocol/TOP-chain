// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xelect_net/include/multilayer_network_chain.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "xdata/xchain_param.h"
#include "xcommon/xip.h"

#include "xtransport/udp_transport/udp_transport.h"
#include "xkad/nat_detect/nat_manager_intf.h"
#include "xkad/routing_table/routing_utils.h"
#include "xwrouter/register_routing_table.h"
#include "xgossip/include/block_sync_manager.h"
#include "xwrouter/multi_routing/small_net_cache.h"
#include "xwrouter/multi_routing/service_node_cache.h"
#include "https_client/cJSON.h"
#include "https_client/u_https_client.h"
#include "xelect_net/include/https_client.h"
#include "xelect_net/include/http_client.h"

namespace top {
namespace elect {

MultilayerNetworkChain::MultilayerNetworkChain(common::xnode_id_t const & node_id, const std::set<uint32_t>& xnetwork_id_set)
    : m_node_id_ { node_id },
      xnetwork_id_set_ { xnetwork_id_set} {
    for (const auto& xnetwork_id : xnetwork_id_set_) {
        auto ecvhost = std::make_shared<EcVHost>(xnetwork_id, GetEcNetcard(), m_node_id_);
        AddEcVhost(xnetwork_id, ecvhost);
        TOP_INFO("add ecvhost xnetwork_id:%u to map", xnetwork_id);
    }
}

void MultilayerNetworkChain::start() {
    auto & platform_params = data::xplatform_params::get_instance();

    top::base::Config config;
    if (HandleParamsAndConfig(platform_params, config) != 0) {
        xerror("load platform config failed");
        throw std::runtime_error{ "HandleParamsAndConfig: load platform config failed" };
    }

    std::string public_endpoints;
    if (!config.Get("node", "public_endpoints", public_endpoints)) {
        xerror("get node public_endpoints from config failed!");
        throw std::runtime_error{ "config.Get: get node public_endpoints from config failed!" };
    }
    platform_params.set_seed_edge_host(public_endpoints);

    if (!Init(config)) {
        xerror("start elect main init failed!");
        throw std::runtime_error{ "start elect main init failed!" };
    }

    if (!Run(config)) {
        xerror("elect main start failed");
        throw std::runtime_error{ "elect main start failed" };
    }
}

void MultilayerNetworkChain::stop() {
    Stop();
}

void MultilayerNetworkChain::AddEcVhost(const uint32_t& xnetwork_id, const EcVHostPtr& ec_vhost) {
    std::unique_lock<std::mutex> lock(vhost_map_mutex_);
    vhost_map_[xnetwork_id] = ec_vhost;
}

std::shared_ptr<network::xnetwork_driver_face_t> MultilayerNetworkChain::GetEcVhost(const uint32_t& xnetwork_id) const noexcept {
    std::unique_lock<std::mutex> lock(vhost_map_mutex_);
    auto ifind = vhost_map_.find(xnetwork_id);
    if (ifind == vhost_map_.end()) {
        return nullptr;
    }
    return std::static_pointer_cast<network::xnetwork_driver_face_t>(ifind->second);
}


bool MultilayerNetworkChain::Init(const base::Config& config) {
    if (!MultilayerNetwork::Init(config)) {
        TOP_WARN("MultilayerNetwork:Init failed");
        return false;
    }
    if (!GetEcNetcard()) {
        TOP_FATAL("ec_netcard empty");
        return false;
    }
    // create elect_manager_chain
    elect_manager_ = std::make_shared<ElectManager>(
            GetCoreTransport(),
            config);
    if (!elect_manager_) {
        TOP_WARN("elect_manager create failed");
        return false;
    }

    /*
    for (const auto& xnetwork_id : xnetwork_id_set_) {
        auto ecvhost = std::make_shared<EcVHost>(xnetwork_id, GetEcNetcard(), m_node_id_);
        AddEcVhost(xnetwork_id, ecvhost);
        TOP_INFO("add ecvhost xnetwork_id:%u to map", xnetwork_id);
    }
    */

    return true;
}

bool MultilayerNetworkChain::Run(const base::Config& config) {
    if (ResetRootRouting(GetCoreTransport(), config) == top::kadmlia::kKadFailed) {
        TOP_WARN("kRoot join network failed");
        return false;
    }

    if (!elect_manager_->Start()) {
        std::cout << "elect network init failed!" << std::endl;
        return false;
    }

    for (const auto& xnetwork_id : xnetwork_id_set_) {
        auto ecvhost = GetEcVhost(xnetwork_id);
        if (!ecvhost) {
            continue;
        }
        ecvhost->start();
    }

    // will block here
    //GetElectCmd().Run();

    return true;
}

void MultilayerNetworkChain::Stop() {
    for (const auto& xnetwork_id : xnetwork_id_set_) {
        auto ecvhost = GetEcVhost(xnetwork_id);
        if (!ecvhost) {
            continue;
        }
        ecvhost->stop();
    }

    elect_manager_.reset();

    MultilayerNetwork::Stop();
}

int MultilayerNetworkChain::HandleParamsAndConfig(
        const top::data::xplatform_params& platform_param,
        top::base::Config& edge_config) {
    if (!edge_config.Set("db", "path", platform_param.db_path)) {
        TOP_ERROR("set config failed [db][path][%s]", platform_param.db_path.c_str());
        return 1;
    }

    if (!edge_config.Set("log", "path", platform_param.log_path)) {
        TOP_ERROR("set config failed [log][log_path][%s]", platform_param.log_path.c_str());
        return 1;
    }

    if (!edge_config.Set("node", "country", platform_param.country)) {
        TOP_ERROR("set config failed [node][country][%s]", platform_param.country.c_str());
        return 1;
    }

    if (!edge_config.Set("node", "zone_id", platform_param.zone_id)) {
        TOP_ERROR("set config failed [node][zone_id][%d]", platform_param.zone_id);
        return 1;
    }

    if (!edge_config.Set("node", "local_ip", platform_param.local_ip)) {
        TOP_ERROR("set config failed [node][local_ip][%s]", platform_param.local_ip.c_str());
        return 1;
    }

    if (!edge_config.Set("node", "local_port", platform_param.local_port)) {
        TOP_ERROR("set config failed [node][local_port][%d]", platform_param.local_port);
        return 1;
    }

    std::string first_node_str = (platform_param.first_node?"true":"false");
    if (!edge_config.Set("node", "first_node", first_node_str)) {
        TOP_ERROR("set config failed [node][first_node][%s]", first_node_str.c_str());
        return 1;
    }

    std::string show_cmd_str = (platform_param.show_cmd?"true":"false");
    if (!edge_config.Set("node", "show_cmd", show_cmd_str)) {
        TOP_ERROR("set config failed [node][show_cmd][%s]", show_cmd_str.c_str());
        return 1;
    }

    std::string client_mode_str = (platform_param.client_mode?"true":"false");
    if (!edge_config.Set("node", "client_mode", client_mode_str)) {
        TOP_ERROR("set config failed [node][client_mode][%s]", client_mode_str.c_str());
        return 1;
    }

    std::string public_endpoints(platform_param.public_endpoints);
    TOP_INFO("config get public_endpoints %s", public_endpoints.c_str());

    std::string final_public_endpoints;
    final_public_endpoints = public_endpoints;

    edge_config.Set("node", "public_endpoints", final_public_endpoints);
    TOP_INFO("config set final public_endpoints %s", final_public_endpoints.c_str());

    bool endpoints_status = false;
    if (final_public_endpoints.empty()) {
        endpoints_status = true;
        TOP_WARN("config get no public_endpoints %s", final_public_endpoints.c_str());
    }

    if (platform_param.url_endpoints.find("http") == std::string::npos) {
        return endpoints_status;
    }
    TOP_INFO("[seeds] url_endpoints url:%s", platform_param.url_endpoints.c_str());

    std::vector<std::string> url_seeds;
    if (platform_param.url_endpoints.find("https://") != std::string::npos) {
        auto seed_http_client = std::make_shared<SeedHttpsClient>(platform_param.url_endpoints);
        if (!seed_http_client) {
            TOP_WARN("[seeds] create seed_http_client failed");
            return endpoints_status;
        }
        if (!seed_http_client->GetSeeds(url_seeds)) {
            TOP_WARN("[seeds] get public endpoints failed from url:%s", platform_param.url_endpoints.c_str());
            return endpoints_status;
        }
    } else { // http scheme
        auto seed_http_client = std::make_shared<SeedHttpClient>(platform_param.url_endpoints);
        if (!seed_http_client) {
            TOP_WARN("[seeds] create seed_http_client failed");
            return endpoints_status;
        }
        if (!seed_http_client->GetSeeds(url_seeds)) {
            TOP_WARN("[seeds] get public endpoints failed from url:%s", platform_param.url_endpoints.c_str());
            return endpoints_status;
        }
    }

    /*
    std::string return_https;
    if (get_https(platform_param.url_endpoints, return_https) != 0) {
        printf("get https error\n");
        return endpoints_status;
    }
    std::cout << "github result:" << return_https << std::endl;

    std::vector<seeds_info_t> seeds_info;
    if (JsonParseSeeds(return_https, seeds_info) != 0) {
        printf("json parse error.\n");
        return endpoints_status;
    }
    std::string bootstrap_nodes;
    const int kBootstrapNumber = 8;
    if (seeds_info.size() <= kBootstrapNumber) {
        for (uint32_t i = 0; i < seeds_info.size(); i++){
	        //printf("ip:%s,port:%d\n", seeds_info[i].ip.c_str(), seeds_info[i].port);
            bootstrap_nodes += seeds_info[i].ip + ":" + std::to_string(seeds_info[i].port) + ",";
        }
    } else {
        srand((unsigned)::time(NULL));
        for (int i = 0; i < kBootstrapNumber; i++) {
            uint32_t rand_index = rand() % seeds_info.size();
            uint32_t count_seeds = 0;
            for (std::vector<seeds_info_t>::iterator it = seeds_info.begin() ; it != seeds_info.end(); ++it) {
                if (count_seeds++ == rand_index) {
                    bootstrap_nodes += seeds_info[rand_index].ip + ":" + std::to_string(seeds_info[rand_index].port) + ",";
                    seeds_info.erase(it);
                    break;
                }
            }
        }
    }
    */

    std::string bootstrap_nodes;
    for (const auto& item : url_seeds) {
        bootstrap_nodes += item;
        bootstrap_nodes += ",";
    }

    if (bootstrap_nodes.back() == ',') {
        bootstrap_nodes.pop_back();
    }
    TOP_INFO("[seeds] fetch url:%s bootstrap_nodes:%s", platform_param.url_endpoints.c_str(), bootstrap_nodes.c_str());
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
    TOP_INFO("[seeds] config set final public_endpoints %s", final_public_endpoints.c_str());
    return 0;
}

int MultilayerNetworkChain::JsonParseSeeds(const std::string& return_https, std::vector<seeds_info_t>& seeds_info) {
    cJSON* pRoot = cJSON_CreateObject();
    cJSON* pArray = cJSON_CreateArray();
  	pRoot = cJSON_Parse(return_https.c_str());
    pArray = cJSON_GetObjectItem(pRoot, "seeds");
    if (NULL == pArray) {
        return -1;
    }
    int iCount = cJSON_GetArraySize(pArray);
    int i = 0;
    for (; i < iCount; ++i) {
        cJSON* pItem = cJSON_GetArrayItem(pArray, i);
        if (NULL == pItem){
            continue;
        }
        char *ip = cJSON_GetObjectItem(pItem, "ip")->valuestring;
        int port = cJSON_GetObjectItem(pItem, "port")->valueint;
        seeds_info_t info;
        info.ip = ip;
        info.port = port;
        seeds_info.push_back(info);
    }

    cJSON_Delete(pRoot);
	return 0;
}

int MultilayerNetworkChain::RegisterNodeCallback(std::function<int32_t(std::string const& node_addr, std::string const& node_sign)> cb) {
    if (GetCoreTransport() == nullptr) {
        TOP_WARN("register node callback fail: core_transport is nullptr");
        return 1;
    }

    GetCoreTransport()->RegisterNodeCallback(cb);
    TOP_INFO("register node callback successful");
    return 0;
}

std::vector<std::string> MultilayerNetworkChain::GetServiceNeighbours(const common::xip2_t& xip2) {
    std::vector<std::string> nodes_vec;
    base::XipParser xip;
    xip.set_xnetwork_id(static_cast<uint32_t>(xip2.network_id().value()));
    xip.set_zone_id(static_cast<uint8_t>(xip2.zone_id().value()));
    xip.set_cluster_id(static_cast<uint8_t>(xip2.cluster_id().value()));
    xip.set_group_id(static_cast<uint8_t>(xip2.group_id().value()));
    //xip.set_network_type((uint8_t)(address.cluster_address().type()));

    auto kad_key = base::GetKadmliaKey(xip);
    auto rt = wrouter::GetRoutingTable(kad_key->GetServiceType(), false);
    if (!rt) {
        TOP_WARN("no routinng table:%llu registered, GetServiceNeighbours failed", kad_key->GetServiceType());
        return {};
    }

    std::vector<kadmlia::NodeInfoPtr> vec;
    rt->GetRandomNodes(vec, rt->nodes_size());
    for (const auto& item : vec) {
        auto ifind = std::find(nodes_vec.begin(), nodes_vec.end(), item->public_ip);
        if (ifind == nodes_vec.end()) {
            nodes_vec.push_back(item->public_ip);
            TOP_DEBUG("GetServiceNeighbours %llu add node:%s", kad_key->GetServiceType(), item->public_ip.c_str());
        }
    }
    TOP_INFO("GetServiceNeighbours %u of service_type:%llu", nodes_vec.size(), kad_key->GetServiceType());
    return nodes_vec;
}

}  // namespace elect

}  // namespace top
