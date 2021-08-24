// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvcnode.h"

namespace top { namespace mock {

class xmock_nodesrv_t : public base::xvnodesrv_t {
public:
    virtual ~xmock_nodesrv_t() {}
    base::xauto_ptr<base::xvnode_t> get_node(const xvip2_t & target_node) const override {

        uint64_t group_key = get_group_key(target_node);

        auto it = m_vgroups.find(group_key);
        if (it == m_vgroups.end())
            return nullptr;

        base::xvnodegroup_t *group = it->second;

        base::xvnode_t* node = group->get_node(target_node);
        if (node != nullptr) {
            node->add_ref();
            return node;
        }

        return nullptr;
    }

    base::xauto_ptr<base::xvnodegroup_t> get_group(const xvip2_t & target_group) const override {

        uint64_t group_key = get_group_key(target_group);
        auto it = m_vgroups.find(group_key);
        if (it == m_vgroups.end())
            return nullptr;

        it->second->add_ref();
        return it->second;
    }

    bool add_group(const base::xvnodegroup_t* group_ptr) override {
        uint64_t group_key = get_group_key(group_ptr->get_xip2_addr());
        auto it = m_vgroups.find(group_key);
        if (it != m_vgroups.end())
            return false;

        xvip2_t group_addr = group_ptr->get_xip2_addr();
        reset_node_id_to_xip2(group_addr);

        std::vector<base::xvnode_t*> new_nodes;
        const std::vector<base::xvnode_t*> &nodes = group_ptr->get_nodes();
        for (auto &it: nodes) {
            it->add_ref();
            new_nodes.push_back(it);
        }

        base::xvnodegroup_t *group = new base::xvnodegroup_t(group_addr, 0, new_nodes);
        m_vgroups[group_key] = group;

        return true;
    }

    bool remove_group(const xvip2_t & target_group) override {

        assert(0);
        return true;
    }

private:
    uint64_t get_group_key(const xvip2_t & target_group) const {
        uint64_t group_key = ((target_group.low_addr << 11) >> 21) | ((target_group.high_addr & 0x1FFFFF) << 43);
        return group_key;
    }

private:
    std::map<uint64_t, base::xvnodegroup_t*> m_vgroups;
};

}
}
