// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <unordered_map>
#include <algorithm>
#include <vector>
#include "xdata/xgenesis_data.h"
#include "xvnetwork/xaddress.h"
#include "xpbase/base/top_utils.h"
#include "xsync/xdeceit_node_manager.h"
#include "xdata/xgenesis_data.h"
#include "xrouter/xrouter.h"
#include "xvnetwork/xvhost_face.h"
#include "xbase/xvledger.h"

NS_BEG2(top, sync)

using xip_vector_ptr = std::shared_ptr<std::vector<vnetwork::xvnode_address_t>>;

struct xrole_xips_t {
    vnetwork::xvnode_address_t self_xip;
    xip_vector_ptr neighbour_xips{};
    xip_vector_ptr parent_xips{};
    xip_vector_ptr orig_neighbour_xips{}; // for special cases

    xrole_xips_t(const vnetwork::xvnode_address_t& _self_xip,
                 const xip_vector_ptr& neighbour_xips_ptr,
                 const xip_vector_ptr& parent_xips_ptr,
                 const xip_vector_ptr& orig_neighbour_xips_ptr) :
    self_xip(_self_xip),
    neighbour_xips(neighbour_xips_ptr),
    parent_xips(parent_xips_ptr),
    orig_neighbour_xips(orig_neighbour_xips_ptr) {}

    xrole_xips_t() = default;
    xrole_xips_t(const xrole_xips_t& other) = default;

    void remove_xips_by_id(const common::xnode_id_t& id) {
        remove_xips_by_id(neighbour_xips, id);
        remove_xips_by_id(parent_xips, id);
    }

    static void remove_xips_by_id(xip_vector_ptr& list_ptr, const common::xnode_id_t& id) {
        if(list_ptr != nullptr) {
            list_ptr->erase(std::remove_if( list_ptr->begin(),
                                            list_ptr->end(),
                                            [&](const vnetwork::xvnode_address_t& addr) {
                                                return addr.node_id() == id;
                                            }),
                            list_ptr->end());
        }
    }
};

class xrole_xips_manager_t {
public:

    virtual ~xrole_xips_manager_t();

    xrole_xips_manager_t(std::string vnode_id, const observer_ptr<base::xvnodesrv_t> &nodesrv, xdeceit_node_manager_t *blacklist);

    /**
     * add new role : self xip, neighbours' xips and parents' xips.
     *
     * @param self_xip self xip
     * @param neighbours neighbours' xips
     * @param parents parents' xips
     * @param archives archives' xips for this turn
     */
    void add_role(const vnetwork::xvnode_address_t& self_xip, const std::vector<vnetwork::xvnode_address_t>& neighbours,
                  const std::vector<vnetwork::xvnode_address_t>& parents, const std::vector<vnetwork::xvnode_address_t>& archives);

    /**
     * remove role by self xip
     *
     * @param self_xip self xip
     */
    void remove_role(const vnetwork::xvnode_address_t& self_xip);

    /**
     * remove all xips which belong to special node id.
     *
     * @param id node id
     */
    void remove_xips_by_id(const common::xnode_id_t& id);

    /**
     * get frozen role xip.
     *
     */
    const vnetwork::xvnode_address_t get_static_xip();

    /**
     * get random neighbours peers for special role.
     *
     * @param self_xip self xip
     * @param max_peers max peers
     * @return random peers, can be empty if no such role
     */
    std::vector<vnetwork::xvnode_address_t> get_rand_neighbors(const vnetwork::xvnode_address_t& self_xip, uint32_t max_peers);

    std::vector<vnetwork::xvnode_address_t> get_rand_parents(const vnetwork::xvnode_address_t& self_xip, uint32_t max_peers);

    std::vector<vnetwork::xvnode_address_t> get_rand_archives(const vnetwork::xvnode_address_t& self_xip, uint32_t max_peers);

    std::vector<vnetwork::xvnode_address_t> get_vrf_sqrt_archive(const std::string& hash);

    std::vector<vnetwork::xvnode_address_t> get_archive_list();

    bool is_consensus_role_exist(base::xvblock_t *vblock) const;

    bool get_rand_neighbor_by_block(base::xvblock_t *vblock, vnetwork::xvnode_address_t& self_xip, vnetwork::xvnode_address_t& target_xip);

    bool vrf_gossip_with_archive(base::xvblock_t *time_vblock, common::xnode_type_t role_type);

    /**
     * vrf sende new block check.
     *
     * @param vblock
     * @param OUTPUT self_xip self xip
     * @return true if self_xip should be a sender; otherwise false
     */
    bool vrf_send_newblock(base::xvblock_t *vblock, vnetwork::xvnode_address_t& self_xip, uint32_t &self_position, uint32_t &deliver_node_count);

protected:

    xip_vector_ptr create_xip_vector_ptr(const std::vector<vnetwork::xvnode_address_t>& list, const vnetwork::xvnode_address_t& self_xip);
    xip_vector_ptr create_archive_xip_vector_ptr(const std::vector<vnetwork::xvnode_address_t>& list, const vnetwork::xvnode_address_t& self_xip);

    std::vector<vnetwork::xvnode_address_t> get_rand_peers(const xip_vector_ptr& list_ptr, uint32_t max_peers);

    uint32_t vrf_value(const std::string& hash);

    vnetwork::xvnode_address_t build_address_from_vnode(const xvip2_t &group_xip2, const std::vector<base::xvnode_t*> &nodes, int32_t slot_id);

protected:
    std::string m_vnode_id;
    mutable std::mutex m_lock;
    xip_vector_ptr m_archive_xips{};
    std::unordered_map<vnetwork::xvnode_address_t, xrole_xips_t> m_map;

    observer_ptr<base::xvnodesrv_t> m_nodesrv;
    xdeceit_node_manager_t *m_blacklist;
};

NS_END2
