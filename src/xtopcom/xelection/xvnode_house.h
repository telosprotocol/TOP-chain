// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvcnode.h"
#include "xvledger/xvblockstore.h"
#include "xmbus/xmessage_bus.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xlru_cache.h"
#include "xcommon/xnode_id.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xmetrics/xmetrics.h"

NS_BEG2(top, election)

class xvnode_wrap_t : public base::xvnode_t {
public:
    xvnode_wrap_t(const std::string & account, const xvip2_t & xip2_addr,const std::string & sign_pub_key):
    base::xvnode_t(account, xip2_addr, sign_pub_key) {
        XMETRICS_GAUGE(metrics::vnode_election_house_vnode_count, 1);
    }

    virtual ~xvnode_wrap_t() {
        XMETRICS_GAUGE(metrics::vnode_election_house_vnode_count, -1);
    }
};

class xvnode_group_wrap_t : public base::xvnodegroup_t {
public:
    xvnode_group_wrap_t(const xvip2_t & group_address, const uint64_t effect_clock_height, std::vector<base::xvnode_t*> & nodes):
    base::xvnodegroup_t(group_address, effect_clock_height, nodes) {
        XMETRICS_GAUGE(metrics::vnode_election_house_vnode_group_count, 1);
    }

    virtual ~xvnode_group_wrap_t() {
        XMETRICS_GAUGE(metrics::vnode_election_house_vnode_group_count, 1);
    }
};


class xvnode_house_t : public base::xvnodesrv_t {
public:
    /**
     * @brief Construct a new xvnode house t object
     *
     * @param node_id node id
     * @param blockstore block store
     * @param bus message bus
     */
    xvnode_house_t(common::xnode_id_t const & node_id,
                   xobject_ptr_t<base::xvblockstore_t> const & blockstore,
                   observer_ptr<mbus::xmessage_bus_face_t> const & bus);
    /**
     * @brief Destroy the xvnode house t object
     *
     */
    ~xvnode_house_t() override = default;

    /**
     * @brief Get the node object
     *
     * @param target_node target node xvip2
     * @return base::xauto_ptr<base::xvnode_t>
     */
    virtual base::xauto_ptr<base::xvnode_t> get_node(const xvip2_t & target_node) const override;
    /**
     * @brief Get the group object
     *
     * @param target_group target group xvip2
     * @return base::xauto_ptr<base::xvnodegroup_t>
     */
    virtual base::xauto_ptr<base::xvnodegroup_t> get_group(const xvip2_t & target_group) const override;
    /**
     * @brief add group
     *
     * @param group_ptr group ptr
     * @return true
     * @return false
     */
    virtual bool                       add_group(const base::xvnodegroup_t* group_ptr)   override {return false;}
    virtual bool                       remove_group(const xvip2_t & target_group)  override {return false;}

protected:
    /**
     * @brief Get the group object internally
     *
     * @param target_group target group xvip2
     * @return base::xauto_ptr<base::xvnodegroup_t>
     */
    base::xauto_ptr<base::xvnodegroup_t> get_group_internal(const xvip2_t & target_group) const;
    /**
     * @brief Get the group object
     *
     * @param target_group target group xvip2
     * @return base::xauto_ptr<base::xvnodegroup_t>
     */
    base::xauto_ptr<base::xvnodegroup_t> get_group_pure(const xvip2_t & target_group) const;
    /**
     * @brief load group from store
     *
     * @param target_node target group xvip2
     */
    void load_group_from_store(const xvip2_t & target_node);

    /**
     * @brief Get the group key
     *
     * @param target_group target group xvip2
     * @return uint64_t key
     */
    uint64_t get_group_key(const xvip2_t & target_group) const;
    /**
     * @brief Get the elect address
     *
     * @param target_group target group xvip2
     * @return std::string
     */
    std::string get_elect_address(const xvip2_t & target_group) const;
    /**
     * @brief notity to load elect info
     *
     * @param group_key group key
     * @param elect_address elect address
     * @param elect_height elect height
     */
    void notify_lack_elect_info(uint64_t group_key, const std::string &elect_address, uint64_t elect_height) const;
    /**
     * @brief add group
     *
     * @param elect_address elect address
     * @param elect_height elect height
     * @param election_result_store election result store
     * @param nid network id
     */
    void add_group(const std::string &elect_address, uint64_t elect_height,
            data::election::xelection_result_store_t const & election_result_store, const common::xnetwork_id_t &nid);

    common::xnode_id_t m_node_id;
    xobject_ptr_t<base::xvblockstore_t> m_blockstore;
    observer_ptr<mbus::xmessage_bus_face_t> m_bus;
    mutable std::mutex                         m_lock;
    //uint64_t                           m_vnetwork_id; //network id,refer definition of xip2 at xbase.h
    //uint64_t                           m_vnet_version;//version is same concept as round of election
    mutable basic::xlru_cache<uint64_t, base::xvnodegroup_t*, basic::xref_deleter_t<base::xvnodegroup_t>> m_vgroups{512};     //mapping <version/round --> group>
};

NS_END2
