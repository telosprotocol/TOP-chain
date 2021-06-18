// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xpbase/base/top_string_util.h"
#include "xtransport/proto/transport.pb.h"

namespace top {
namespace transport {

inline std::string FormatMsgid(const protobuf::RoutingMessage & message) {
    return base::StringUtil::str_fmt("msgid(%u)", message.id());
}

}  // namespace transport
}  // namespace top

#ifndef NDEBUG
   // reduce hot log
#    define TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE(pre, message)                                                                                                                       \
        do {                                                                                                                                                                       \
            xdbg("%s %s: %s [hop_num:%d] [type:%d]",                                                                                                                               \
                 top::transport::FormatMsgid(message).c_str(),                                                                                                                     \
                 std::string(pre).c_str(),                                                                                                                                         \
                 message.debug().c_str(),                                                                                                                                          \
                 message.hop_num(),                                                                                                                                                \
                 message.type());                                                                                                                                                  \
        } while (0)

/*
#define TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE2(pre, message, thread_id) \
    do { \
        TOP_DEBUG("%s %s: %s [hop_num:%d] [type:%d] at thread(%d)", \
                top::transport::FormatMsgid(message).c_str(), \
                std::string(pre).c_str(), message.debug().c_str(), message.hop_num(), message.type(),thread_id); \
    } while (0)
    */

//#define TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE(pre, message)
#    define TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE2(pre, message, thread_id)

#else
#    define TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE(pre, message)
#    define TOP_NETWORK_DEBUG_FOR_PROTOMESSAGE2(pre, message, thread_id)
#endif
