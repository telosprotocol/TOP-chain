// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

const char * config_content = R"({
    "ip": "127.0.0.1",
    "node_id": "T_adv1",
    "http_port": 19081,
    "grpc_port": 19082,
    "dht_port": 19083,
    "msg_port": 19084,
    "ws_port": 19085,
    "seeds": "T_rec1:127.0.0.1;T_rec2:127.0.0.2",
    "seed_node_id": "T_rec1:xxxx",
    "log_level": 0,
    "log_path": "/tmp/adv1",
    "db_path": "/tmp/adv1",
    "node_type": "advance",
    "public_key": "AqlqnzLxVyy7sT6/GYk1oc3RhbBGAqcI57pvQfy7cds3",
    "sign_key": "8wEE4qlZplj1gjp1Pndl/Jm2ESp/M2QUQvD3vG1EpMo=",
    "platform":{
        "http_test" : false,
        "http_port" : 8101,
        "business_port" : 9101,
        "local_xid" : "",
        "log_path" : "/tmp/adv1/log.bitvpn.log",
        "max_broadcast_number" : 3,
        "public_endpoints" : "127.0.0.1:9000",
        "url_endpoints"    : "https://wwww.github.com",
        "service_list" : "vpn",
        "show_cmd" : false,
        "zone_id" : 1,
        "db_path" : "/tmp/adv1/db",
        "country" : "US",
        "cons_new_edge" : false,
        "business" : "EDGE",
        "client_mode":false
    },
    "address" : "hz",
    "job" : null,
    "fruit" : ["apple", "pearl", "banana"],
    "like" : [
    {
        "book" : "c++",
        "food" : "apple",
        "music" : "ddx",
        "sport" : "game"
    },
    {
        "hate" : "game"
    }
    ],
   "name" : "swduan",
   "phone" : "10086"
})";

const char * staticec_config_content = R"(
{
    "staticec" : {
        "custom" : {
            "rec" : {
                "nodes" : ["T_rec1", "T_rec2", "T_rec3"],
                "arc" : ["T_arc1"]
            },
            "zone1" : {
                "zec" : ["T_zec1", "T_zec2", "T_zec3"],
                "clus1" : {
                    "adv1" : ["T_adv1", "T_adv2", "T_adv3"],
                    "con1" : ["T_con1", "T_con2", "T_con3"],
                    "con2" : ["T_con10", "T_con11", "T_con12"]
                },
                "clus2" : {
                    "adv1" : ["T_adv4", "T_adv5", "T_adv6"],
                    "con1" : ["T_con4", "T_con5", "T_con6"],
                    "con2" : ["T_con7", "T_con8", "T_con9"]
                },
                "arc" : ["T_arc1"]
            },
            "edge" : ["T_edge"]
        }
    }
}
)";
