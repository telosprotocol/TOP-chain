// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xbase.h"
#include "xmock_addr_generator.hpp"
#include "xmock_network_config.hpp"
#include "xmock_nodesrv.hpp"
#include "xcertauth/xcertauth_face.h"
#include "xcrypto/xckey.h"

namespace top { namespace mock {

class xmock_node_info_t;

class xmock_node_info_t {
public:
    std::string m_vnode_id; //all path name
    std::string m_group_name; // belong to a group
    std::string m_node_name; // unique name from config
    vnetwork::xvnode_address_t m_addr;
    xvip2_t m_xip;
    std::string m_public_key;
    std::string m_private_key;
    std::vector<xmock_node_info_t*> m_neighbor_nodes;
    std::vector<xmock_node_info_t*> m_parent_nodes;
    std::vector<xmock_node_info_t*> m_child_nodes;
    std::vector<xmock_node_info_t*> m_archive_nodes;
    std::vector<xmock_node_info_t*> m_all_nodes;
    xobject_ptr_t<base::xvnodesrv_t> m_nodesvr{};
    xobject_ptr_t<base::xvcertauth_t> m_certauth{};
};

class network_group_t;

class network_group_t {
public:
    enum_mock_role m_type;
    std::string m_name;
    std::string m_path_name;
    std::map<std::string, network_group_t> m_map_group;
    std::vector<std::shared_ptr<xmock_node_info_t>> m_vector_node;
};

class xmock_network_t {
public:
    xmock_network_t(xmock_network_config_t &cfg_network) {

        std::map<std::string, config_group_t> &system_topology = cfg_network.get_system_topology();
        construct_network(system_topology, m_group);
    }

    std::vector<std::shared_ptr<xmock_node_info_t>> get_all_nodes() {
        return m_all_nodes;
    }

    std::map<std::string, network_group_t> get_structed_network() {
        return m_group;
    }

private:
    void construct_network(std::map<std::string, config_group_t> &system_topology, std::map<std::string, network_group_t> &runnable) {

        // create basic info
        for (auto it: system_topology) {
            config_group_t &g = it.second;
            if (g.m_type == enum_mock_role_zone) {
                network_group_t network_group;
                network_group.m_type = g.m_type;
                std::pair<std::map<std::string, network_group_t>::iterator, bool> ret = runnable.insert(std::make_pair(g.m_name, network_group));
                create_zone(g, ret.first->second);
            }
        }

        // fill neighbor ...
        for (auto &it: m_group) {
            network_group_t &g = it.second;
            if (g.m_type == enum_mock_role_zone) {
                fill_zone(g);
            }
        }
    }

    void create_zone(config_group_t &config_group, network_group_t &network_group) {

        const std::string &prefix = config_group.m_name;

        for (auto it: config_group.m_map_group) {
            config_group_t &g = it.second;

            network_group_t new_network_group;
            new_network_group.m_type = g.m_type;
            new_network_group.m_name = g.m_name;

            if (g.m_type == enum_mock_role_beacon) {
                std::pair<std::map<std::string, network_group_t>::iterator, bool> ret = network_group.m_map_group.insert(std::make_pair(g.m_name, new_network_group));
                create_beacon(prefix, g, ret.first->second);
            } else if (g.m_type == enum_mock_role_zec) {
                std::pair<std::map<std::string, network_group_t>::iterator, bool> ret = network_group.m_map_group.insert(std::make_pair(g.m_name, new_network_group));
                create_zec(prefix, g, ret.first->second);
            } else if (g.m_type == enum_mock_role_archive) {
                std::pair<std::map<std::string, network_group_t>::iterator, bool> ret = network_group.m_map_group.insert(std::make_pair(g.m_name, new_network_group));
                create_archive(prefix, g, ret.first->second);
            } else if (g.m_type == enum_mock_role_advance) {
                std::pair<std::map<std::string, network_group_t>::iterator, bool> ret = network_group.m_map_group.insert(std::make_pair(g.m_name, new_network_group));
                create_advance(prefix, g, ret.first->second);
            }
        }
    }

    void create_beacon(const std::string &prefix, config_group_t &config_group, network_group_t &network_group) {

        std::string path = prefix + "_" + config_group.m_name;

        uint16_t slot_id = 0;
        uint16_t sharding_size = config_group.m_map_node.size();
        for (auto it: config_group.m_map_node) {
            config_node_t &n = it.second;
            std::string name = path + "_" + n.m_name;

            top::common::xnode_address_t addr = m_addr_generator.create_beacon_addr(slot_id++, sharding_size, name);
            create_node(network_group, config_group.m_name, n.m_name, addr);
        }
    }

    void create_zec(const std::string &prefix, config_group_t &config_group, network_group_t &network_group) {

        std::string path = prefix + "_" + config_group.m_name;

        uint16_t slot_id = 0;
        uint16_t sharding_size = config_group.m_map_node.size();
        for (auto it: config_group.m_map_node) {
            config_node_t &n = it.second;
            std::string name = path + "_" + n.m_name;

            top::common::xnode_address_t addr = m_addr_generator.create_zec_addr(slot_id++, sharding_size, name);
            create_node(network_group, config_group.m_name, n.m_name, addr);
        }
    }

    void create_archive(const std::string &prefix, config_group_t &config_group, network_group_t &network_group) {

        std::string path = prefix + "_" + config_group.m_name;

        uint16_t slot_id = 0;
        uint16_t sharding_size = config_group.m_map_node.size();
        for (auto it: config_group.m_map_node) {
            config_node_t &n = it.second;
            std::string name = n.m_name;

            top::common::xnode_address_t addr = m_addr_generator.create_archive_addr(slot_id++, sharding_size, name);
            create_node(network_group, config_group.m_name, n.m_name, addr);
        }
    }

    void create_advance(const std::string &prefix, config_group_t &config_group, network_group_t &network_group) {

        std::string path = prefix + "_" + config_group.m_name;

        uint16_t slot_id = 0;
        uint16_t sharding_size = config_group.m_map_node.size();
        for (auto it: config_group.m_map_node) {
            config_node_t &n = it.second;
            std::string name = path + "_" + n.m_name;

            top::common::xnode_address_t addr = m_addr_generator.create_advance_addr(slot_id++, sharding_size, name);
            create_node(network_group, config_group.m_name, n.m_name, addr);
        }

        for (auto it: config_group.m_map_group) {
            config_group_t &g = it.second;
            if (g.m_type == enum_mock_role_validator) {
                network_group_t new_network_group;
                new_network_group.m_type = g.m_type;
                new_network_group.m_name = g.m_name;
                std::pair<std::map<std::string, network_group_t>::iterator, bool> ret = network_group.m_map_group.insert(std::make_pair(g.m_name, new_network_group));
                create_validator(path, g, ret.first->second);
            }
        }
    }

    void create_validator(const std::string &prefix, config_group_t &config_group, network_group_t &network_group) {

        std::string path = prefix + "_" + config_group.m_name;

        uint16_t slot_id = 0;
        uint16_t sharding_size = config_group.m_map_node.size();
        for (auto it: config_group.m_map_node) {
            config_node_t &n = it.second;
            std::string name = n.m_name;

            top::common::xnode_address_t addr = m_addr_generator.create_validator_addr(slot_id++, sharding_size, name);
            create_node(network_group, config_group.m_name, n.m_name, addr);
        }
    }

    void create_node(network_group_t &network_group, const std::string &group_name, const std::string &node_name, const vnetwork::xvnode_address_t &addr) {

        //printf("create node=%s\n", addr.to_string().c_str());

        std::shared_ptr<xmock_node_info_t> node_ptr = std::make_shared<xmock_node_info_t>();
        node_ptr->m_vnode_id = group_name + "_" + node_name;
        node_ptr->m_group_name = group_name;
        node_ptr->m_node_name = node_name;
        node_ptr->m_addr = addr;
        node_ptr->m_xip = addr.xip2();

        utl::xecprikey_t node_prv_key;
        std::string _node_prv_key_str((const char*)node_prv_key.data(),node_prv_key.size());
        std::string _node_pub_key_str = node_prv_key.get_compress_public_key();
        //const std::string node_account  = node_prv_key.to_account_address('0', 0);

        node_ptr->m_public_key = _node_pub_key_str;
        node_ptr->m_private_key = _node_prv_key_str;

        network_group.m_vector_node.push_back(node_ptr);

        m_all_nodes.push_back(node_ptr);

        if (common::has<common::xnode_type_t::storage_archive>(addr.type()))
            m_archive_nodes.push_back(node_ptr);
    }

    void fill_zone(network_group_t &network_group) {

        for (auto it: network_group.m_map_group) {
            network_group_t &g = it.second;
            if (g.m_type == enum_mock_role_beacon || g.m_type == enum_mock_role_zec || g.m_type == enum_mock_role_archive) {
                fill_common(g);
            } else if (g.m_type == enum_mock_role_advance) {
                fill_advance(g);
            }
        }
    }

    void fill_common(network_group_t &network_group) {

        std::vector<xmock_node_info_t*> vector_neighbor_info;
        std::vector<xmock_node_info_t*> vector_parent_info;
        std::vector<xmock_node_info_t*> vector_child_info;

        get_neighbor_nodes(network_group, vector_neighbor_info);

        for (auto it: network_group.m_vector_node) {
            fill_node(it, vector_neighbor_info, vector_parent_info, vector_child_info);
        }
    }

    void fill_advance(network_group_t &network_group) {

        std::vector<xmock_node_info_t*> vector_parent_info;
        std::vector<xmock_node_info_t*> vector_neighbor_info;
        std::vector<xmock_node_info_t*> vector_child_info;

        get_neighbor_nodes(network_group, vector_neighbor_info);

        for (auto it: network_group.m_map_group) {
            network_group_t &g = it.second;
            if (g.m_type == enum_mock_role_validator) {
                for (auto it2: g.m_vector_node) {
                    std::shared_ptr<xmock_node_info_t> &node = it2;
                    vector_child_info.push_back(node.get());
                }
            }
        }

        for (auto it: network_group.m_vector_node) {
            fill_node(it, vector_neighbor_info, vector_parent_info, vector_child_info);
        }

        for (auto it: network_group.m_map_group) {
            network_group_t &g = it.second;
            if (g.m_type == enum_mock_role_validator) {
                network_group_t new_network_group;
                //std::pair<std::map<std::string, network_group_t>::iterator, bool> ret = network_group.m_map_group.insert(std::make_pair(g.m_name, new_network_group));
                network_group.m_map_group.insert(std::make_pair(g.m_name, new_network_group));
                fill_shard(g, vector_neighbor_info);
            }
        }
    }

    void fill_shard(network_group_t &network_group, const std::vector<xmock_node_info_t*> &vector_parent_info) {

        std::vector<xmock_node_info_t*> vector_child_info;
        std::vector<xmock_node_info_t*> vector_neighbor_info;

        get_neighbor_nodes(network_group, vector_neighbor_info);

        for (auto it: network_group.m_vector_node) {
            fill_node(it, vector_neighbor_info, vector_parent_info, vector_child_info);
        }
    }

    void get_neighbor_nodes(network_group_t &network_group, std::vector<xmock_node_info_t*> &vector_neighbor_info) {
        for (auto it: network_group.m_vector_node) {
            std::shared_ptr<xmock_node_info_t> &node = it;
            vector_neighbor_info.push_back(node.get());
        }
    }

    void fill_node(std::shared_ptr<xmock_node_info_t> &node_ptr, const std::vector<xmock_node_info_t*> &vector_neighbor_info, const std::vector<xmock_node_info_t*> &vector_parent_info,
                const std::vector<xmock_node_info_t*> &vector_child_info) {

        std::vector<xmock_node_info_t*> vector_archive_nodes;
        std::vector<xmock_node_info_t*> vector_all_nodes;
        xobject_ptr_t<base::xvnodesrv_t> nodesvr = make_object_ptr<xmock_nodesrv_t>();

        for (auto &it: m_archive_nodes) {
            std::shared_ptr<xmock_node_info_t> &node = it;
            vector_archive_nodes.push_back(node.get());
        }

        for (auto &it: m_all_nodes) {
            std::shared_ptr<xmock_node_info_t> &node = it;
            vector_all_nodes.push_back(node.get());
        }

        std::map<uint64_t, std::vector<base::xvnode_t*>> node_groups;
        for (auto &it: m_all_nodes) {

            xvip2_t node_xip = it->m_xip;
            vnetwork::xvnode_address_t &addr = it->m_addr;
            std::string node_account = addr.account_address().to_string();

            uint64_t group_key = get_group_key(node_xip);
            std::string pri_key{""};
            if (it->m_addr == node_ptr->m_addr)
                pri_key = node_ptr->m_private_key;
            base::xvnode_t *n = new base::xvnode_t{node_account, node_xip, it->m_public_key};

            //const uint32_t node_index = get_node_id_from_xip2(n->get_xip2_addr());
            //printf("node=%s %s %s nodeidx=%u\n", it->m_node_name.c_str(), it->m_vnode_id.c_str(), it->m_addr.to_string().c_str(), node_index);

            auto it2 = node_groups.find(group_key);
            if (it2 == node_groups.end()) {
                std::vector<base::xvnode_t*> g;
                g.push_back(n);
                node_groups[group_key] = g;
            } else {
                it2->second.push_back(n);
            }
        }
        for (auto &it: node_groups) {

            std::vector<base::xvnode_t*> &group = it.second;
            xvip2_t group_addr = group[0]->get_xip2_addr();
            reset_node_id_to_xip2(group_addr);

            //printf("groupsize=%u count=%u\n", (uint32_t)group.size(), get_group_nodes_count_from_xip2(group_addr));

            base::xvnodegroup_t *g = new base::xvnodegroup_t(group_addr, 0, it.second);
            nodesvr->add_group(g);
        }

        //xobject_ptr_t<base::xvcertauth_t> cert_ptr = make_object_ptr<xmock_auth_t>((uint32_t)1);

        node_ptr->m_neighbor_nodes = vector_neighbor_info;
        node_ptr->m_parent_nodes = vector_parent_info;
        node_ptr->m_child_nodes = vector_child_info;
        node_ptr->m_archive_nodes = vector_archive_nodes;
        node_ptr->m_all_nodes = vector_all_nodes;
        node_ptr->m_nodesvr = nodesvr;
        node_ptr->m_certauth = auth::xauthcontext_t::create(*nodesvr.get());;
    }

    uint64_t get_group_key(const xvip2_t & target_group) {
        uint64_t group_key = ((target_group.low_addr << 11) >> 21) | ((target_group.high_addr & 0x1FFFFF) << 43);
        return group_key;
    }

private:
    xmock_addr_generator_t m_addr_generator;
    std::map<std::string, network_group_t> m_group;
    std::vector<std::shared_ptr<xmock_node_info_t>> m_archive_nodes;
    std::vector<std::shared_ptr<xmock_node_info_t>> m_all_nodes;
};

}
}
