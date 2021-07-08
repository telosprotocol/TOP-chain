// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <inttypes.h>

#include "xelection/xvnode_house.h"
#include "xmbus/xevent_behind.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xblock.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xdata/xcodec/xmsgpack/xelection_result_store_codec.hpp"
#include "xdata/xelection/xelection_group_result.h"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xunit_bstate.h"
#include "xbasic/xutility.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvstate.h"
#include "xvledger/xvledger.h"
#include "xtopcl/include/user_info.h"
#include "xpbase/base/top_utils.h"

NS_BEG2(top, election)

using namespace base;
using namespace data;

xvnode_house_t::xvnode_house_t(common::xnode_id_t const & node_id, std::string const & sign_key,
        xobject_ptr_t<base::xvblockstore_t> const & blockstore, observer_ptr<mbus::xmessage_bus_face_t> const & bus):
xvnodesrv_t(),
m_node_id(node_id),
m_sign_key(sign_key),
m_blockstore(blockstore),
m_bus(bus) {
}

base::xauto_ptr<base::xvnode_t> xvnode_house_t::get_node(const xvip2_t & target_node) const {
    xdbg("[xvnode_house_t::get_node] target node : {%" PRIu64 ", %" PRIu64 "}", target_node.high_addr, target_node.low_addr);
    base::xauto_ptr<base::xvnodegroup_t> group_ptr = get_group_internal(target_node);
    if (group_ptr == nullptr) {
        const uint64_t group_key = get_group_key(target_node);
        std::string elect_address = get_elect_address(target_node);
        uint64_t elect_height = get_network_height_from_xip2(target_node);
        notify_lack_elect_info(group_key, elect_address, elect_height);
        return nullptr;
    }

    xvnode_t *node_ptr = group_ptr->get_node(target_node);
    if(node_ptr != NULL)
        node_ptr->add_ref();

    return node_ptr;
}

base::xauto_ptr<base::xvnodegroup_t> xvnode_house_t::get_group(const xvip2_t & target_group) const {
    xdbg("xvnode_house_t::get_group");
    base::xauto_ptr<base::xvnodegroup_t> group_ptr = get_group_internal(target_group);
    if (group_ptr == nullptr) {

        const uint64_t group_key = get_group_key(target_group);
        std::string elect_address = get_elect_address(target_group);
        uint64_t elect_height = get_network_height_from_xip2(target_group);
        notify_lack_elect_info(group_key, elect_address, elect_height);
    }

    return group_ptr;
}

/*
XIP definition as total 64bit = [xaddress_domain:1bit | xaddress_type:2bit | xnetwork_type:5bit] [xnetwork_version#:3bit][xnetwork-id: 7-7-7 bit][xhost-id:32bit] =
{
    //xaddress_domain is    enum_xaddress_domain_xip(0) or  enum_xaddress_domain_xip2(1)
    //xaddress_type   is    enum_xip_type
    //xnetwork_type         refer enum_xnetwork_type
    -[enum_xaddress_domain_xip:1bit | enum_xip_type:2bit | xnetwork_type:5bit]
    -[xnetwork_version#:3bit] //elect round# at Chain
    -[xnetwork-id: 7-7-7 bit] //A-Class,B-Class,C-Class,D-Class,E-Class Network...
    -[zone-id:7bit|cluster-id:7bit|group-id:8bit|node-id:10bit]
}
//XIP2 is 128bit address like IPv6 design on top of XIP
XIP2 = [high 64bit:label data][low 64bit:XIP]
{
    high 64bit for enum_xnetwork_type_xchain
        -[xgroup_node_count:10bit]
        -[xnetwork_round/height:54bit]

    low  64bit:
        -[XIP: 64bit]
}
*/

uint64_t xvnode_house_t::get_group_key(const xvip2_t & target_group) const {
    uint64_t group_key = ((target_group.low_addr << 11) >> 21) | ((target_group.high_addr & 0x1FFFFF) << 43);
    return group_key;
}

std::string xvnode_house_t::get_elect_address(const xvip2_t & target_group) const {
    uint8_t zone_id = get_zone_id_from_xip2(target_group);
    uint8_t cluster_id = get_cluster_id_from_xip2(target_group);
    uint8_t group_id = get_group_id_from_xip2(target_group);

    std::string elect_address{};
    if (zone_id == 0) {
        if (cluster_id == 1) {
            if (group_id>=1 && group_id<127)
                elect_address = sys_contract_zec_elect_consensus_addr;
        }
    } else if (zone_id == 1) {
        if (cluster_id==0 && group_id==0)
            elect_address = sys_contract_rec_elect_rec_addr;
    } else if (zone_id == 2) {
        if (cluster_id==0 && group_id==0)
            elect_address = sys_contract_rec_elect_zec_addr;
    }

    return elect_address;
}

void xvnode_house_t::notify_lack_elect_info(uint64_t group_key, const std::string &elect_address, uint64_t elect_height) const {

    if (!elect_address.empty()) {

        xkinfo("get node or group failed %s %u %lu", elect_address.c_str(), (uint32_t)elect_height, group_key);

        std::string elect_table_addr = account_address_to_block_address(top::common::xaccount_address_t{elect_address});
        mbus::xevent_behind_ptr_t e = make_object_ptr<mbus::xevent_behind_check_t>(
                    elect_table_addr, "vnode house");

        e->err = mbus::xevent_t::succ;
        m_bus->push_event(e);
    } else {
        //xkinfo("get node or group failed (%u %u %u)", zone_id, cluster_id, group_id);
    }
}

xauto_ptr<xvnodegroup_t> xvnode_house_t::get_group_internal(const xvip2_t & target_group) const {
    base::xauto_ptr<base::xvnodegroup_t> group1 = get_group_pure(target_group);
    if (group1 != nullptr)
        return group1;

    const_cast<xvnode_house_t *>(this)->load_group_from_store(target_group);

    base::xauto_ptr<base::xvnodegroup_t> group2 = get_group_pure(target_group);
    if (group2 != nullptr)
        return group2;

    return nullptr;
}

xauto_ptr<xvnodegroup_t> xvnode_house_t::get_group_pure(const xvip2_t & target_group) const {

    const uint64_t group_key = get_group_key(target_group);

    std::lock_guard<std::mutex> locker(m_lock);
    base::xvnodegroup_t* group = nullptr;
    bool ret = m_vgroups.get(group_key, group);
    if (ret)
    {
        if (group->get_network_height() == get_network_height_from_xip2(target_group)) //double check exactly height
        {
            group->add_ref();
            return group;
        }
    }

    return nullptr;
}

void xvnode_house_t::load_group_from_store(const xvip2_t & target_node) {
    std::string elect_address = get_elect_address(target_node);
    uint64_t elect_height = get_network_height_from_xip2(target_node);

    // TODO check flag, use committed block?
    base::xvaccount_t _vaddress(elect_address);
    XMETRICS_GAUGE(metrics::blockstore_access_from_vnodesrv, 1);
    xauto_ptr<xvblock_t> blk_ptr = m_blockstore->load_block_object(_vaddress, elect_height, base::enum_xvblock_flag_committed, false);
    if (blk_ptr == nullptr)
        return;

    base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(blk_ptr.get(), metrics::statestore_access_from_vnodesrv_load_state);
    if (bstate == nullptr) {
        xwarn("xvnode_house_t::load_group_from_store fail-load state.block=%s", blk_ptr->dump().c_str());
        return;
    }
    xaccount_ptr_t state = std::make_shared<xunit_bstate_t>(bstate.get());

    std::string result;
    auto property_names = data::election::get_property_name_by_addr(common::xaccount_address_t{blk_ptr->get_account()});
    using top::data::election::xelection_result_store_t;
    for (auto const & property : property_names) {
        state->string_get(property, result);
        if (result.empty()) {
            xwarn("[xvnode_house_t::load_group_from_store] string get null. %s, height=%" PRIu64 ",property=%s",
                blk_ptr->get_account().c_str(), blk_ptr->get_height(), property.c_str());
            continue;
        }
        auto const & election_result_store = codec::msgpack_decode<xelection_result_store_t>({std::begin(result), std::end(result)});
        if (election_result_store.empty()) {
            xwarn("[xvnode_house_t::load_group_from_store] elect result is empty! %s, %" PRIu64, blk_ptr->get_account().c_str(), blk_ptr->get_height());
            continue;
        }
        xdbg("[xvnode_house_t::load_group_from_store] add_group succ. %s, %" PRIu64 ",property=%s,value_size=%zu",
            elect_address.c_str(), elect_height, property.c_str(), result.size());
        common::xnetwork_id_t nid{uint32_t(get_network_id_from_xip2(target_node))};
        add_group(elect_address, elect_height, election_result_store, nid);
    }
    return;
}

void xvnode_house_t::add_group(const std::string &elect_address, uint64_t elect_height,
        data::election::xelection_result_store_t const & election_result_store, const common::xnetwork_id_t &nid) {
    std::map<xvip2_t, std::vector<base::xvnode_t *>, xvip2_compare> group_map;
    std::map<xvip2_t, uint64_t, xvip2_compare>                      group_enable_map;

    for (auto const & election_result_info : election_result_store) {
        auto const &network_id = top::get<common::xnetwork_id_t const>(election_result_info);
        if (network_id != nid) {
            continue;
        }

        auto const& election_type_results = top::get<data::election::xelection_network_result_t>(election_result_info);
        for (auto const & election_type_result : election_type_results) {

            common::xzone_id_t zone_id;

            auto const node_type = top::get<common::xnode_type_t const>(election_type_result);
            if (common::has<common::xnode_type_t::consensus>(node_type) ||
                common::has<common::xnode_type_t::consensus_validator>(node_type) ||
                common::has<common::xnode_type_t::consensus_auditor>(node_type)) {

                zone_id = common::xconsensus_zone_id;

            } else if (common::has<common::xnode_type_t::zec>(node_type)) {

                zone_id = common::xzec_zone_id;
            } else if (common::has<common::xnode_type_t::committee>(node_type)) {

                zone_id = common::xcommittee_zone_id;

            } else {
                continue;
            }

            auto const & election_result = top::get<data::election::xelection_result_t>(election_type_result);

            for (auto const & cluster_result_info : election_result) {

                auto const &cluster_id = top::get<common::xcluster_id_t const>(cluster_result_info);
                auto const & cluster_result = top::get<data::election::xelection_cluster_result_t>(cluster_result_info);

                for (auto const & group_result_info : cluster_result) {

                    auto const &group_id = top::get<common::xgroup_id_t const>(group_result_info);
                    auto const &election_group_result = top::get<data::election::xelection_group_result_t>(group_result_info);

                    std::vector<base::xvnode_t *> nodes;
                    common::xip2_t xip2{network_id,
                            zone_id,
                            cluster_id,
                            group_id,
                            // common::xnetwork_version_t{static_cast<common::xnetwork_version_t::value_type>(election_group_result.group_version().value())},
                            static_cast<uint16_t>(election_group_result.size()),
                            elect_height};

                    auto const & group_result = top::get<data::election::xelection_group_result_t>(group_result_info);
                    for (auto const & node_info : group_result) {
                        auto const slot_id = top::get<common::xslot_id_t const>(node_info);
                        auto const & bundle = top::get<data::election::xelection_info_bundle_t>(node_info);
                        assert(bundle.node_id().has_value());

                        common::xip2_t node_xip2{
                            xip2.network_id(),
                            xip2.zone_id(),
                            xip2.cluster_id(),
                            xip2.group_id(),
                            slot_id,
                            // xip2.network_version(),
                            xip2.size(),
                            xip2.height()
                        };
                        std::string pri_key{""};
                        if (m_node_id == bundle.node_id()) {
                            pri_key = DecodePrivateString(m_sign_key);
                        }
                        auto pub_key = base::xstring_utl::base64_decode(bundle.election_info().consensus_public_key.to_string());
#if 0
                        xdbg("[add_group222][add_node] account %s publickey:%s",
                            bundle.node_id().value().c_str(),
                            bundle.election_info().public_key.to_string().c_str());
#endif
                        auto * node = new xvnode_wrap_t{bundle.node_id().value(), node_xip2.value(), pub_key, pri_key};
#if 0
                        xdbg("[add_group222][add_node] %lu, %lu -> %lu, %lu",
                            xip2.value().high_addr,
                            xip2.value().low_addr,
                            node_xip2.value().high_addr,
                            node_xip2.value().low_addr);
#endif
                        assert(nodes.size()==slot_id.value());
                        nodes.push_back(node);
                    }

                    group_map[xip2.value()] = std::move(nodes);
                    group_enable_map[xip2.value()] = group_result.start_time();
                }
            }
        }
    }

    {
        std::lock_guard<std::mutex> locker(m_lock);

        for (auto & pair : group_map) {

            const xvip2_t &xip2 = pair.first;

            const uint64_t group_key = get_group_key(xip2);

            base::xvnodegroup_t* group = nullptr;
            bool ret = m_vgroups.get(group_key, group);
            if (ret) {
                // TODO pair.second->release_ref();
                continue;
            }

#ifdef DEBUG
            uint8_t zone_id = group_key>>15 & 0x7f;
            uint8_t cluster_id = group_key>>8 & 0x7f;
            uint8_t group_id = group_key & 0xff;

            xdbg("[vnode_house_add_group][overview] %s %lu %lu (%u %u %u)", elect_address.c_str(), elect_height, group_key, zone_id, cluster_id, group_id);
            for (auto &it: pair.second) {

                const xvip2_t &xvip2 = it->get_xip2_addr();

                xdbg("[vnode_house_add_group][node] %s %lu %lu", it->get_account().c_str(),
                                xip2.high_addr,
                                xip2.low_addr);
            }
#endif
            base::xauto_ptr<base::xvnodegroup_t> _consensus_group(new xvnode_group_wrap_t(pair.first, group_enable_map[pair.first], pair.second));
            _consensus_group->add_ref();
            m_vgroups.put(group_key, _consensus_group.get());
        }
    }

    for(auto &pair : group_map) {
        for(auto n : pair.second) {
            n->release_ref();
        }
    }
}


NS_END2
