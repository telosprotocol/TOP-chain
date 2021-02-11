//
//  top_utils.h
//
//  Created by @author on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//


#include "xpbase/base/redis_utils.h"

#include <cstdarg>

#include <string>
#include <iostream>


#include "xpbase/base/redis_client.h"
#include "xbase/xpacket.h"
#include "xpbase/base/top_log.h"
#include "xpbase/base/top_utils.h"
#include "xpbase/base/kad_key/kadmlia_key.h"


using namespace top;

void redis_push_stability(const top::transport::protobuf::RoutingMessage& message) {
    TOP_DEBUG("redis push stability");

    auto redis_cli = base::RedisClient::Instance()->redis_cli();
    if(!redis_cli->is_connected()) {
        TOP_WARN("redis not connected");
        return;
    }

    uint32_t id = message.id();
    // every message can be received by several nodes(otherwise not received)
    std::string base_key = std::to_string(id) + "_" + HexEncode(message.xid());
    redis_cli->sadd(
            base_key + "_recvnodes",
            {HexEncode(global_xid->Get())});

    // received times in all of every message
    redis_cli->incrby(base_key + "_allrecv", 1);

    // update time when receive the message(received time of the last received node)
    uint64_t recv_update_time = GetCurrentTimeMicSec();
    redis_cli->hset(base_key + "time", "recv_update", std::to_string(recv_update_time));

    // how many kind of message received by every nodes
    //redis_cli->sadd(HexEncode(global_xid->Get()) + "_recvallids", {std::to_string(id)});

    // node id list
    redis_cli->sadd("allnodes_set", {HexEncode(global_xid->Get())});
    redis_cli->commit();
    return;
}


void redis_push_qps(top::transport::protobuf::RoutingMessage message, ...) {
    auto redis_cli = base::RedisClient::Instance()->redis_cli();
    if(!redis_cli->is_connected()) {
        TOP_WARN("redis not connected");
        return;
    }
	va_list args;
	va_start(args, message);
    int qps = va_arg(args, int);
	va_end(args);

    redis_cli->lpush("smaug_recv_qps_" + HexEncode(global_xid->Get()), {std::to_string(qps)});
    return;
}

void redis_push_stability_after_filter(const top::transport::protobuf::RoutingMessage& message) {
    TOP_DEBUG("redis push stability");

    auto redis_cli = base::RedisClient::Instance()->redis_cli();
    if(!redis_cli->is_connected()) {
        TOP_WARN("redis not connected");
        return;
    }

    uint32_t id = message.id();
    // every message can be received by several nodes(otherwise not received)
    std::string base_key = std::to_string(id) + "_" + HexEncode(message.xid());
    redis_cli->sadd(
            base_key + "_recvnodes_af",
            {HexEncode(global_xid->Get())});

    // received times in all of every message
    redis_cli->incrby(base_key + "_allrecv_af", 1);

    // update time when receive the message(received time of the last received node)
    uint64_t recv_update_time = GetCurrentTimeMicSec();
    redis_cli->hset(base_key + "time", "recv_update_af", std::to_string(recv_update_time));

    // how many kind of message received by every nodes
    //redis_cli->sadd(HexEncode(global_xid->Get()) + "_recvallids", {std::to_string(id)});
    redis_cli->lpush(base_key , {std::to_string(message.hop_num())});

    // node id list
    redis_cli->sadd("allnodes_set_af", {HexEncode(global_xid->Get())});
    redis_cli->commit();
    return;
}




void redis_push_hop_num(const top::transport::protobuf::RoutingMessage& message) {
    /*
    auto redis_cli = base::RedisClient::Instance()->redis_cli();
    if (!redis_cli->is_connected()) {
        TOP_WARN("redis not connected");
        return;
    }
    redis_cli->hset(
            "bloomfilter_each_node_receive_hop_num",
            HexSubstr(global_xid->Get()),
            std::to_string(message.hop_num()));
    redis_cli->commit();
    */
    return;
}

void redis_push_recv_count(const top::transport::protobuf::RoutingMessage& message) {
    auto redis_cli = base::RedisClient::Instance()->redis_cli();
    if (!redis_cli->is_connected()) {
        TOP_WARN("redis not connected");
        return;
    }
    redis_cli->hincrby("bloomfilter_each_node_receive_count", HexSubstr(global_xid->Get()), 1);
    redis_cli->incrby("bloomfilter_all_node_receive_count", 1);
    redis_cli->commit();
    return;
}


void redis_push_ctd_begin(const top::transport::protobuf::RoutingMessage& message) {
    auto redis_cli = base::RedisClient::Instance()->redis_cli();
    if (!redis_cli->is_connected()) {
        TOP_WARN("redis not connected");
        return;
    }
    auto reply = redis_cli->exists({"ctd_begin_id"});
    redis_cli->commit();
    auto g = reply.get();
    if (g.is_null() || g.as_integer() == 0) {
        redis_cli->set("ctd_begin_id", std::to_string(message.id() + 1));
        redis_cli->commit();
    }
    return;
}

void redis_push_hset(top::transport::protobuf::RoutingMessage message, ...) {
    auto redis_cli = base::RedisClient::Instance()->redis_cli();
    if(!redis_cli->is_connected()) {
        TOP_WARN("redis not connected");
        return;
    }
	va_list args;
	va_start(args, message);
    const char* key = va_arg(args, char*);
    const char* field = va_arg(args, char*);
    const char* value = va_arg(args, char*);
	va_end(args);
    redis_cli->hset(key, field, value);
    redis_cli->commit();
    return;
}


void redis_push_sendtime(const top::transport::protobuf::RoutingMessage& message) {
    auto redis_cli = base::RedisClient::Instance()->redis_cli();
    if(!redis_cli->is_connected()) {
        TOP_WARN("redis not connected");
        return;
    }

    std::string base_key = std::to_string(message.id()) + "_" + HexEncode(global_xid->Get());
    redis_cli->hset(base_key  + "time" , "send_time", std::to_string(GetCurrentTimeMicSec()));
	
    redis_cli->commit();
    return;
}
