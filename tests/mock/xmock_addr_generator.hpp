// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvnetwork/xaddress.h"
#include "xbase/xutl.h"
#include "xrouter/xrouter.h"

namespace top { namespace mock {

class xmock_addr_generator_t {
public:
    vnetwork::xvnode_address_t create_beacon_addr(uint16_t _slot_id, uint16_t sharding_size, const std::string &name) {

        std::size_t id = 1;
        top::common::xelection_round_t ver(id);

        //common::xnetwork_id_t nid{m_net_id++};
        common::xnetwork_id_t nid{1};
        auto const cluster_addr = common::build_committee_sharding_address(nid);

        common::xslot_id_t slot_id{_slot_id};
        top::common::xaccount_election_address_t account_address{ common::xnode_id_t { name }, slot_id };

        top::common::xnode_address_t addr(cluster_addr, account_address, ver, sharding_size, 1);

        return addr;
    }

    vnetwork::xvnode_address_t create_zec_addr(uint16_t _slot_id, uint16_t sharding_size, const std::string &name) {

        common::xzone_id_t zid{1};

        std::size_t id = 1;
        top::common::xelection_round_t ver(id);

        //common::xnetwork_id_t nid{m_net_id++};
        common::xnetwork_id_t nid{1};
        auto const cluster_addr = common::build_zec_sharding_address(nid);

        common::xslot_id_t slot_id{_slot_id};
        top::common::xaccount_election_address_t account_address{ common::xnode_id_t { name }, slot_id };

        top::common::xnode_address_t addr(cluster_addr, account_address, ver, sharding_size, 1);

        return addr;
    }

    vnetwork::xvnode_address_t create_archive_addr(uint16_t _slot_id, uint16_t sharding_size, const std::string &name) {
        common::xzone_id_t zid{1};

        std::size_t id = 1;
        top::common::xelection_round_t ver(id);

        //common::xnetwork_id_t nid{m_net_id++};
        common::xnetwork_id_t nid{1};
        common::xgroup_id_t gid{1};
        auto const cluster_addr = common::build_archive_sharding_address(gid, nid);

        common::xslot_id_t slot_id{_slot_id};
        top::common::xaccount_election_address_t account_address{ common::xnode_id_t { name }, slot_id };

        top::common::xnode_address_t addr(cluster_addr, account_address, ver, sharding_size, 1);

        return addr;
    }

    vnetwork::xvnode_address_t create_advance_addr(uint16_t _slot_id, uint16_t sharding_size, const std::string &name) {
        common::xzone_id_t zid{0};
        common::xgroup_id_t gid{1};

        std::size_t id = 1;
        top::common::xelection_round_t ver(id);

        //common::xnetwork_id_t nid{m_net_id++};
        common::xnetwork_id_t nid{1};
        auto const cluster_addr = common::build_consensus_sharding_address(gid, nid);

        common::xslot_id_t slot_id{_slot_id};
        top::common::xaccount_election_address_t account_address{ common::xnode_id_t { name }, slot_id };

        top::common::xnode_address_t addr(cluster_addr, account_address, ver, sharding_size, 1);

        return addr;
    }

    vnetwork::xvnode_address_t create_validator_addr(uint16_t _slot_id, uint16_t sharding_size, const std::string &name) {

        std::size_t id = 1;
        top::common::xelection_round_t ver(id);

        //common::xnetwork_id_t nid{m_net_id++};
        common::xnetwork_id_t nid{1};

        router::xrouter_t router;
        auto taddr = router.address_of_table_id(0, common::xnode_type_t::consensus_validator, nid);
        auto const cluster_addr = common::build_consensus_sharding_address(taddr.group_id(), taddr.network_id());

        common::xslot_id_t slot_id{_slot_id};
        top::common::xaccount_election_address_t account_address{ common::xnode_id_t { name }, slot_id };

        top::common::xnode_address_t addr(cluster_addr, account_address, ver, sharding_size, 1);
        return addr;
    }
};

}
}