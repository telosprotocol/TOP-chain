#pragma once
#include "xrouter/xrouter_face.h"
#include "xcommon/xsharding_info.h"
#include "xvnetwork/xversion.h"
#include "xvnetwork/xaddress.h"
#include "xcommon/xnode_id.h"
NS_BEG3(top, xrpc, test)
using namespace std;
using namespace common;
using namespace vnetwork;
using namespace network;

//class xmock_router:public router::xrouter_face {
// public:
//    top::vnetwork::xvnode_address_t get_shard(uint64_t , const std::string & )
//    {
//        uint32_t id = 1;
//        xsharding_info_t si{ xnetwork_id_t{ id }, xzone_id_t{ id }, xcluster_id_t{ id } };
//        vnetwork::xversion_t const v
//        {
//            static_cast<vnetwork::xversion_t::value_type>(id)
//        };
//        xcluster_address_t sa{ xnetwork_id_t{ id }, xzone_id_t{ id }, xcluster_id_t{ id }, xgroup_id_t{ id }, xvnode_type_t::cluster };
//        xaccount_address_t aa{ xnode_id_t{std::to_string(id)}};
//        xvnode_address_t node{ sa, aa, v};
//        return node;
//    }
//    top::vnetwork::xvnode_address_t get_zone(uint64_t , const std::string & )
//    {
//        uint32_t id = 1;
//        xsharding_info_t si{ xnetwork_id_t{ id }, xzone_id_t{ id }, xcluster_id_t{ id } };
//        vnetwork::xversion_t const v
//        {
//            static_cast<vnetwork::xversion_t::value_type>(id)
//        };
//        xcluster_address_t sa{ xnetwork_id_t{ id }, xzone_id_t{ id }, xcluster_id_t{ id }, xgroup_id_t{ id }, xvnode_type_t::cluster };
//        xaccount_address_t aa{ xnode_id_t{std::to_string(id)}};
//        xvnode_address_t node{ sa, aa, v};
//        return node;
//    }
//    bool is_account_in_local_shard(uint64_t , const std::string & ) {
//        return true;
//    }
//
//    virtual bool is_account_in_local_cluster(uint64_t , const std::string & ) {
//        return true;
//    }
//};

NS_END3
