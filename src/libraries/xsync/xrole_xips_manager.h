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
#include "xdata/xgenesis_data.h"
#include "xrouter/xrouter.h"
#include "xvnetwork/xvhost_face.h"

NS_BEG2(top, sync)

using xip_vector_ptr = std::shared_ptr<std::vector<common::xnode_address_t>>;

struct xrole_xips_t {
    common::xnode_address_t self_xip;
    xip_vector_ptr neighbour_xips{};
    xip_vector_ptr parent_xips{};
    xip_vector_ptr orig_neighbour_xips{}; // for special cases
    std::set<uint16_t> set_table_ids;
    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> m_vnetwork_driver;

    xrole_xips_t(const common::xnode_address_t& _self_xip,
                 const xip_vector_ptr& neighbour_xips_ptr,
                 const xip_vector_ptr& parent_xips_ptr,
                 const xip_vector_ptr& orig_neighbour_xips_ptr,
                 const std::set<uint16_t> &_set_table_ids,
                 const std::shared_ptr<vnetwork::xvnetwork_driver_face_t>& network_driver) :
    self_xip(_self_xip),
    neighbour_xips(neighbour_xips_ptr),
    parent_xips(parent_xips_ptr),
    orig_neighbour_xips(orig_neighbour_xips_ptr),
    set_table_ids(_set_table_ids),
    m_vnetwork_driver(network_driver) {}

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
                                            [&](const common::xnode_address_t& addr) {
                                                return addr.node_id() == id;
                                            }),
                            list_ptr->end());
        }
    }
};

class xrole_xips_manager_t {
public:

    virtual ~xrole_xips_manager_t();

    xrole_xips_manager_t(std::string vnode_id);

    /**
     * add new role : self xip, neighbours' xips and parents' xips.
     *
     * @param self_xip self xip
     * @param neighbours neighbours' xips
     * @param parents parents' xips
     * @param archives archives' xips for this turn
     */
    void add_role(const common::xnode_address_t& self_xip, 
                  const std::vector<common::xnode_address_t>& neighbours,
                  const std::vector<common::xnode_address_t>& parents, 
                  const std::shared_ptr<vnetwork::xvnetwork_driver_face_t>& network_driver,
                  const std::set<uint16_t> &table_ids);

    /**
     * remove role by self xip
     *
     * @param self_xip self xip
     */
    void remove_role(const common::xnode_address_t& self_xip);

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
    const common::xnode_address_t get_static_xip();

    /**
     * get random neighbours peers for special role.
     *
     * @param self_xip self xip
     * @param max_peers max peers
     * @return random peers, can be empty if no such role
     */
    std::vector<common::xnode_address_t> get_rand_neighbors(const common::xnode_address_t& self_xip, uint32_t max_peers);

    std::vector<common::xnode_address_t> get_rand_parents(const common::xnode_address_t& self_xip, uint32_t max_peers);

    std::vector<common::xnode_address_t> get_rand_archives(uint32_t max_peers);

    std::vector<common::xnode_address_t> get_archive_list();

    std::vector<common::xnode_address_t> get_rand_full_nodes(uint32_t max_peers);
    std::vector<common::xnode_address_t> get_full_nodes();
    std::vector<common::xnode_address_t> get_edge_archive_list();
    std::vector<common::xnode_address_t> get_relay_list() const;

    bool get_self_addr(common::xnode_address_t& self_addr) const;

    bool vrf_gossip_with_archive(base::xvblock_t *time_vblock, common::xnode_type_t role_type);

    std::vector<common::xnode_address_t> get_all_neighbors(const common::xnode_address_t& self_addr);

    bool get_self_addr(common::xnode_type_t node_type, uint16_t table_id, common::xnode_address_t& self_addr) const;
    void set_miner(common::xminer_type_t miner_type, bool genesis);
    bool genesis() {return m_genesis;}
    common::xminer_type_t miner_type() { return m_miner_type; }
protected:

    xip_vector_ptr create_xip_vector_ptr(const std::vector<common::xnode_address_t>& list, const common::xnode_address_t& self_xip);
    xip_vector_ptr create_archive_xip_vector_ptr(const std::vector<common::xnode_address_t>& list, const common::xnode_address_t& self_xip) const;

    std::vector<common::xnode_address_t> get_rand_peers(const xip_vector_ptr& list_ptr, uint32_t max_peers);

protected:
    std::string m_vnode_id;
    mutable std::mutex m_lock;
    std::unordered_map<common::xnode_address_t, xrole_xips_t> m_map;
    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> m_vnetwork_driver;
    common::xnode_address_t m_self_xip;
    common::xminer_type_t m_miner_type;
    bool m_genesis{false};
};

NS_END2
