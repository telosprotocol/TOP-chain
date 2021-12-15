// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <utility>
#include <vector>
#include <set>

#include "xcommon/xnode_id.h"
#include "xbasic/xrange.hpp"
#include "xcommon/xsharding_info.h"
#include "xbase/xobject_ptr.h"
#include "xdata/xdata_common.h"
#include "xconfig/xpredefined_configurations.h"
#include "json/value.h"
#include "xverifier/xverifier_utl.h"

namespace top { namespace data {

class xuser_params {
public:
    xuser_params(xuser_params const &)             = delete;
    xuser_params & operator=(xuser_params const &) = delete;
    xuser_params(xuser_params &&)                  = delete;
    xuser_params & operator=(xuser_params &&)      = delete;
    static xuser_params& get_instance() {
        static xuser_params instance;
        return instance;
    }
    bool is_valid();

    common::xnode_id_t                    account;                   // account for the node
    std::string                           publickey;                 // node public key(hex)
    std::string                           signkey;                 // node private key(hex)
#if defined XENABLE_MOCK_ZEC_STAKE
    common::xrole_type_t                  node_role_type;
#endif
private:
    xuser_params() = default;
};

class xdev_params {
public:
    xdev_params()                                = default;
    xdev_params(xdev_params const &)             = delete;
    xdev_params & operator=(xdev_params const &) = delete;
    xdev_params(xdev_params &&)                  = delete;
    xdev_params & operator=(xdev_params &&)      = delete;
    static xdev_params& get_instance() {
        static xdev_params instance;
        return instance;
    }

    std::string                           seed_edge_host;
};

class xplatform_params {
public:
    xplatform_params()                                = default;
    xplatform_params(xplatform_params const &)             = delete;
    xplatform_params & operator=(xplatform_params const &) = delete;
    xplatform_params(xplatform_params &&)                  = delete;
    xplatform_params & operator=(xplatform_params &&)      = delete;
    static xplatform_params& get_instance() {
        static xplatform_params instance;
        return instance;
    }
    void get_seed_edge_host(std::set<std::string>& seed_edge_host_set);
    // parse seed_edge_host from outsider
    void set_seed_edge_host(const std::string endpoints);

    std::string                           local_ip {};
    uint16_t                              local_port {9000};
    std::string                           log_path {};
    std::string                           public_endpoints {};
    std::string                           url_endpoints {};
    std::set<std::string>                 edge_endpoints {}; // just ip
    bool                                  show_cmd {false};
    uint32_t                              zone_id {static_cast<uint32_t>(-1)};
    std::string                           db_path {};
    std::string                           country { "US" };
};

struct xstaticec_params {
    int32_t total_working_advance_nodes {4};  // 3 - 9
    int32_t total_working_consensus_nodes {21};  // 3 - 21
    int32_t current_working_consensus_nodes {9};  // 3 - total
    int32_t round_consensus_nodes {3}; // 1 - current
    int32_t round_consensus_interval {60}; // >= 30
    int32_t total_working_edge_nodes {3};  // 1 - 3
    xJson::Value custom;

    static xstaticec_params& get_instance() {
        static xstaticec_params instance;
        return instance;
    }

    // init again
    xstaticec_params();
};

}  // namespace data
}  // namespace top
