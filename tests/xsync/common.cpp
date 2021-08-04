#include "tests/xsync/common.h"
#include "xrouter/xrouter.h"

using namespace top;

#define DEFINE_FUNC(type, cluster_addr) \
    top::common::xnode_address_t\
    get_##type##_address(uint32_t nid, uint64_t version, const std::string& node_id) {\
        top::common::xaccount_election_address_t account_address{ top::common::xnode_id_t { node_id }, top::common::xslot_id_t{} };\
        return {cluster_addr, account_address, top::common::xelection_round_t{version}, 1, 1};\
    }

DEFINE_FUNC(beacon, top::common::build_committee_sharding_address(top::common::xbeacon_network_id))
DEFINE_FUNC(zec, top::common::build_zec_sharding_address(top::common::xbeacon_network_id))
DEFINE_FUNC(archive, top::common::build_archive_sharding_address(top::common::xbeacon_network_id))

#undef DEFINE_FUNC
#define DEFINE_FUNC(type, start) \
top::common::xnode_address_t \
get_##type##_address(uint32_t nid, uint64_t version, const std::string& node_id, int offset) {\
    top::common::xcluster_address_t cluster_addr{\
        top::common::xnetwork_id_t{nid},\
        top::common::xdefault_zone_id,\
        top::common::xdefault_cluster_id,\
        top::common::xgroup_id_t{ static_cast<top::common::xgroup_id_t::value_type>(start + offset) }\
    };\
    top::common::xaccount_election_address_t account_address{ top::common::xnode_id_t { node_id }, top::common::xslot_id_t{} };\
    return top::common::xnode_address_t(cluster_addr, account_address, top::common::xelection_round_t{version}, 1, 1);\
}

DEFINE_FUNC(auditor, 1)
DEFINE_FUNC(validator, 64)

#undef DEFINE_FUNC
#define DEFINE_FUNC(type) \
std::vector<top::common::xnode_address_t>\
get_##type##_addresses(std::uint32_t nid, std::uint64_t version, int offset, int count) {\
    std::vector<top::common::xnode_address_t> list;\
    for(int i=0;i<count;i++) {\
        list.push_back(get_##type##_address(nid, version, "test"+std::to_string(offset+i), offset+i));\
    }\
    return list;\
}

DEFINE_FUNC(auditor)
DEFINE_FUNC(validator)

#undef DEFINE_FUNC
#define DEFINE_FUNC(type) \
std::vector<top::common::xnode_address_t>\
get_##type##_addresses(uint32_t nid, uint64_t version, int count) {\
    std::vector<top::common::xnode_address_t> list;\
    for(int i=0;i<count;i++) {\
        list.push_back(get_##type##_address(nid, version, "test"+std::to_string(i)));\
    }\
    return list;\
}

DEFINE_FUNC(beacon)
DEFINE_FUNC(zec)
DEFINE_FUNC(archive)



common::xnode_address_t create_beacon_addr(uint16_t _slot_id, const std::string & name, std::size_t id) {
    top::common::xelection_round_t ver(id);

    // common::xnetwork_id_t nid{m_net_id++};
    common::xnetwork_id_t nid{1};
    auto const cluster_addr = common::build_committee_sharding_address(nid);

    common::xslot_id_t slot_id{_slot_id};
    top::common::xaccount_election_address_t account_address{common::xnode_id_t{name}, slot_id};

    top::common::xnode_address_t addr(cluster_addr, account_address, ver, 1, 1);

    return addr;
}

top::common::xnode_address_t create_zec_addr(uint16_t _slot_id, const std::string & name, std::size_t id) {
    common::xzone_id_t zid{1};

    top::common::xelection_round_t ver(id);

    // common::xnetwork_id_t nid{m_net_id++};
    common::xnetwork_id_t nid{1};
    auto const cluster_addr = common::build_zec_sharding_address(nid);

    common::xslot_id_t slot_id{_slot_id};
    top::common::xaccount_election_address_t account_address{common::xnode_id_t{name}, slot_id};

    top::common::xnode_address_t addr(cluster_addr, account_address, ver, 1, 1);

    return addr;
}

common::xnode_address_t create_archive_addr(uint16_t _slot_id, const std::string & name, std::size_t id) {
    common::xzone_id_t zid{1};

    top::common::xelection_round_t ver(id);

    // common::xnetwork_id_t nid{m_net_id++};
    common::xnetwork_id_t nid{1};
    auto const cluster_addr = common::build_archive_sharding_address(nid);

    common::xslot_id_t slot_id{_slot_id};
    top::common::xaccount_election_address_t account_address{common::xnode_id_t{name}, slot_id};

    top::common::xnode_address_t addr(cluster_addr, account_address, ver, 1, 1);

    return addr;
}

common::xnode_address_t create_auditor_addr(uint16_t _slot_id, const std::string & name, std::size_t id) {
    common::xzone_id_t zid{0};
    common::xgroup_id_t gid{1};

    top::common::xelection_round_t ver(id);

    // common::xnetwork_id_t nid{m_net_id++};
    common::xnetwork_id_t nid{1};
    auto const cluster_addr = common::build_consensus_sharding_address(gid, nid);

    common::xslot_id_t slot_id{_slot_id};
    top::common::xaccount_election_address_t account_address{common::xnode_id_t{name}, slot_id};

    top::common::xnode_address_t addr(cluster_addr, account_address, ver, 1, 1);

    return addr;
}

common::xnode_address_t create_validator_addr(uint16_t _slot_id, const std::string & name, std::size_t id) {
    common::xzone_id_t zid{1};
    top::common::xelection_round_t ver(id);

    // common::xnetwork_id_t nid{m_net_id++};
    common::xnetwork_id_t nid{1};

    router::xrouter_t router;
    auto taddr = router.address_of_table_id(0, common::xnode_type_t::consensus_validator, nid);
    auto const cluster_addr = common::build_consensus_sharding_address(taddr.group_id(), taddr.network_id());

    common::xslot_id_t slot_id{_slot_id};
    top::common::xaccount_election_address_t account_address{common::xnode_id_t{name}, slot_id};

    top::common::xnode_address_t addr(cluster_addr, account_address, ver, 1, 1);
    return addr;
}
