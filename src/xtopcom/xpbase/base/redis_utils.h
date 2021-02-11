//
//  top_utils.h
//
//  Created by @author on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include <cstdarg>

#include <string>
#include <iostream>


#include "xtransport/proto/transport.pb.h"



void redis_push_stability(const top::transport::protobuf::RoutingMessage& message);
void redis_push_stability_after_filter(const top::transport::protobuf::RoutingMessage& message);
void redis_push_qps(top::transport::protobuf::RoutingMessage message, ...);
void redis_push_hop_num(const top::transport::protobuf::RoutingMessage& message);
void redis_push_recv_count(const top::transport::protobuf::RoutingMessage& message);
void redis_push_ctd_begin(const top::transport::protobuf::RoutingMessage& message);
void redis_push_hset(top::transport::protobuf::RoutingMessage message, ...);
void redis_push_sendtime(const top::transport::protobuf::RoutingMessage& message);

#ifdef REDIS_PUSH
#define TOP_NETWORK_DEBUG_FOR_REDIS(message, operation, ...) \
    do { \
        if (std::string(operation).compare("stability") == 0) { \
            redis_push_stability(message); \
        } else if (std::string(operation).compare("stability_af") == 0) { \
            redis_push_stability_after_filter(message); \
        } else if (std::string(operation).compare("netqps") == 0) { \
            redis_push_qps(message, ## __VA_ARGS__); \
        } else if (std::string(operation).compare("hop_num") == 0) { \
            redis_push_hop_num(message); \
        } else if (std::string(operation).compare("recv_count") == 0) { \
            redis_push_recv_count(message); \
        } else if (std::string(operation).compare("ctdbegin") == 0) { \
            redis_push_ctd_begin(message); \
        } else if (std::string(operation).compare("sendtime") == 0) { \
            redis_push_sendtime(message); \
        } else if (std::string(operation).compare("hset") == 0) { \
            redis_push_hset(message, ## __VA_ARGS__); \
        } \
    } while (0)
#else
#define TOP_NETWORK_DEBUG_FOR_REDIS(message, operation, ...)
#endif


