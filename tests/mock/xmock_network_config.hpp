// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xdata_common.h"
#include <json/json.h>

namespace top { namespace mock {

// 1shard(2node)
static Json::Value xsync_validator_behind() {

    Json::Value v = Json::objectValue;

    v["group"]["zone0"]["type"] = "zone";

    v["group"]["adv0"]["type"] = "advance";
    v["group"]["adv0"]["parent"] = "zone0";

    v["group"]["shard0"]["type"] = "validator";
    v["group"]["shard0"]["parent"] = "adv0";

    v["node"]["node0"]["parent"] = "shard0";
    v["node"]["node1"]["parent"] = "shard0";

    return v;
}

// 1shard(2node) + 1 archive(6node)
static Json::Value xsync_broadcast() {

    Json::Value v = Json::objectValue;

    v["group"]["zone0"]["type"] = "zone";
    v["group"]["arc0"]["type"] = "archive";
    v["group"]["arc0"]["parent"] = "zone0";

    v["group"]["adv0"]["type"] = "advance";
    v["group"]["adv0"]["parent"] = "zone0";

    v["group"]["shard0"]["type"] = "validator";
    v["group"]["shard0"]["parent"] = "adv0";

    v["node"]["node0"]["parent"] = "shard0";
    v["node"]["node1"]["parent"] = "shard0";

    v["node"]["node2"]["parent"] = "arc0";
    v["node"]["node3"]["parent"] = "arc0";
    v["node"]["node4"]["parent"] = "arc0";
    v["node"]["node5"]["parent"] = "arc0";
    v["node"]["node6"]["parent"] = "arc0";
    v["node"]["node7"]["parent"] = "arc0";

    return v;
}

// beacon(4node)
static Json::Value timercert() {

    Json::Value v = Json::objectValue;

    v["group"]["zone0"]["type"] = "zone";

    v["group"]["beacon"]["type"] = "beacon";
    v["group"]["beacon"]["parent"] = "zone0";

    v["node"]["node0"]["parent"] = "beacon";
    v["node"]["node1"]["parent"] = "beacon";
    v["node"]["node2"]["parent"] = "beacon";
    v["node"]["node3"]["parent"] = "beacon";

    return v;
}

// 1rec(3node) + 1zec(3node) + 1adv(3node) + 1shard(3node) + 1archive(3node)
static Json::Value xsync_latest() {

    Json::Value v = Json::objectValue;

    v["group"]["zone0"]["type"] = "zone";

    v["group"]["rec0"]["type"] = "beacon";
    v["group"]["rec0"]["parent"] = "zone0";

    v["group"]["zec0"]["type"] = "zec";
    v["group"]["zec0"]["parent"] = "zone0";

    //v["group"]["edge0"]["type"] = "edge";
    //v["group"]["edge0"]["parent"] = "zone0";

    v["group"]["adv0"]["type"] = "advance";
    v["group"]["adv0"]["parent"] = "zone0";

    v["group"]["shard0"]["type"] = "validator";
    v["group"]["shard0"]["parent"] = "adv0";

    v["group"]["arc0"]["type"] = "archive";
    v["group"]["arc0"]["parent"] = "zone0";

    v["node"]["node0"]["parent"] = "rec0";
    v["node"]["node1"]["parent"] = "rec0";
    v["node"]["node2"]["parent"] = "rec0";

    v["node"]["node3"]["parent"] = "zec0";
    v["node"]["node4"]["parent"] = "zec0";
    v["node"]["node5"]["parent"] = "zec0";

    v["node"]["node6"]["parent"] = "adv0";
    v["node"]["node7"]["parent"] = "adv0";
    v["node"]["node8"]["parent"] = "adv0";

    v["node"]["node9"]["parent"] = "shard0";
    v["node"]["node10"]["parent"] = "shard0";
    v["node"]["node11"]["parent"] = "shard0";

    v["node"]["node12"]["parent"] = "arc0";
    v["node"]["node13"]["parent"] = "arc0";
    v["node"]["node14"]["parent"] = "arc0";

    return v;
}

enum enum_mock_role {
    enum_mock_role_none,
    enum_mock_role_zone,
    enum_mock_role_beacon,
    enum_mock_role_zec,
    enum_mock_role_advance,
    enum_mock_role_validator,
    enum_mock_role_archive,
};

class config_node_t {
public:
    void get_node_info(std::string &info) {
        info = m_name;
    }
    std::string m_name;
    std::string m_path_name;
    std::string m_parent;
};

class config_group_t;

class config_group_t {
public:
    void dump(int depth) {
        for (int i=0; i<depth; i++)
            printf("\t");

        std::string node_list = "[ ";
        for (auto it: m_map_node) {
            std::string info;
            it.second.get_node_info(info);
            node_list += info;
            node_list += " ";
        }
        node_list += "]";
        printf("%s %s\n", m_name.c_str(), node_list.c_str());

        for (auto it: m_map_group) {
            config_group_t g = it.second;
            g.dump(depth+1);
        }
    }
    enum_mock_role m_type;
    std::string m_name;
    std::string m_path_name;
    std::string m_parent;
    std::map<std::string, config_group_t> m_map_group;
    std::map<std::string, config_node_t> m_map_node;
};

class xmock_network_config_t {
public:

    xmock_network_config_t(Json::Value &cfg_system_topology) {
        extract_system_topology(cfg_system_topology, m_system_topology);
    }

    std::map<std::string, config_group_t>& get_system_topology() {
        return m_system_topology;
    }

private:
    void extract_system_topology(Json::Value &system_topology, std::map<std::string, config_group_t> &map_group) {

        printf("%s\n", system_topology.toStyledString().c_str());

        std::map<std::string, config_node_t> map_node;

        // get group
        if (system_topology.isMember("group")) {
            Json::Value &group_list = system_topology["group"];
            Json::Value::Members members = group_list.getMemberNames();
            for (auto member: members) {

                config_group_t tmp;
                tmp.m_name = member;
                enum_mock_role role = enum_mock_role_none;

                std::string type = group_list[member]["type"].asString();
                if (type == "zone")
                    role = enum_mock_role_zone;
                else if (type == "beacon")
                    role = enum_mock_role_beacon;
                else if (type == "zec")
                    role = enum_mock_role_zec;
                else if (type == "advance")
                    role = enum_mock_role_advance;
                else if (type == "validator")
                    role = enum_mock_role_validator;
                else if (type == "archive")
                    role = enum_mock_role_archive;
                else
                    assert(0);

                tmp.m_type = role;

                tmp.m_parent = group_list[member]["parent"].asString();

                //printf("get group %s %s %s\n", tmp.m_name.c_str(), tmp.m_type.c_str(), tmp.m_parent.c_str());

                map_group[tmp.m_name] = tmp;;
            }
        }

        // get node
        if (system_topology.isMember("node")) {

            // find group
            Json::Value &node_list = system_topology["node"];
            Json::Value::Members members = node_list.getMemberNames();

            for (auto member: members) {
                config_node_t tmp;
                tmp.m_name = member;
                tmp.m_parent = node_list[member]["parent"].asString();

                //printf("get node %s %s\n", tmp.m_name.c_str(), tmp.m_parent.c_str());
                map_node[tmp.m_name] = tmp;
            }
        }

        // move node into various group
        for (auto it: map_node) {

            config_node_t node = it.second;
            auto it2 = map_group.find(node.m_parent);
            if (it2 == map_group.end())
                continue;

            config_group_t &group = it2->second;
            group.m_map_node[node.m_name] = node;
        }

        // move shard into advance
        std::map<std::string, config_group_t>::iterator it = map_group.begin();
        for (; it!=map_group.end(); ) {
            config_group_t src = it->second;
            if (src.m_type == enum_mock_role_validator) {
                auto it2 = map_group.find(src.m_parent);
                if (it2 != map_group.end()) {
                    config_group_t &group = it2->second;
                    group.m_map_group[src.m_name] = src;
                    map_group.erase(it++);
                    continue;
                }
            }
            it++;
        }

        // move advance into zone
        it = map_group.begin();
        for (; it!=map_group.end(); ) {
            config_group_t src = it->second;
            if (src.m_type == enum_mock_role_advance) {
                auto it2 = map_group.find(src.m_parent);
                if (it2 != map_group.end()) {
                    config_group_t &group = it2->second;
                    group.m_map_group[src.m_name] = src;
                    map_group.erase(it++);
                    continue;
                }
            }
            it++;
        }

        // move archive into zone
        it = map_group.begin();
        for (; it!=map_group.end(); ) {
            config_group_t src = it->second;
            if (src.m_type == enum_mock_role_archive) {
                auto it2 = map_group.find(src.m_parent);
                if (it2 != map_group.end()) {
                    config_group_t &group = it2->second;
                    group.m_map_group[src.m_name] = src;
                    map_group.erase(it++);
                    continue;
                }
            }
            it++;
        }

        // move beacon into zone
        it = map_group.begin();
        for (; it!=map_group.end(); ) {
            config_group_t src = it->second;
            if (src.m_type == enum_mock_role_beacon) {
                auto it2 = map_group.find(src.m_parent);
                if (it2 != map_group.end()) {
                    config_group_t &group = it2->second;
                    group.m_map_group[src.m_name] = src;
                    map_group.erase(it++);
                    continue;
                }
            }
            it++;
        }

        // move zec into zone
        it = map_group.begin();
        for (; it!=map_group.end(); ) {
            config_group_t src = it->second;
            if (src.m_type == enum_mock_role_zec) {
                auto it2 = map_group.find(src.m_parent);
                if (it2 != map_group.end()) {
                    config_group_t &group = it2->second;
                    group.m_map_group[src.m_name] = src;
                    map_group.erase(it++);
                    continue;
                }
            }
            it++;
        }

        // only reserve zone
        it = map_group.begin();
        for (; it!=map_group.end(); ) {
            config_group_t src = it->second;
            if (src.m_type != enum_mock_role_zone) {
                map_group.erase(it++);
                continue;
            }
            it++;
        }

        for (auto it: map_group) {
            config_group_t &group = it.second;
            group.dump(0);
        }
    }

private:
    std::map<std::string, config_group_t> m_system_topology;
};

}
}
