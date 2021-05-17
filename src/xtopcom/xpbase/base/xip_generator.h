//
//  xip_generator.h
//
//  Created by @Charlie Xie on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#ifdef _WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

#include <stdint.h>
#include <limits>
#include <string>
#include <memory>

#include "xpbase/base/top_utils.h"
#include "xpbase/base/xip_parser.h"
#include "xpbase/base/rand_util.h"

namespace top {

namespace base {
class XipParser;

class XipGenerator {
public:
    XipGenerator() {}
    ~XipGenerator() {}
    bool CreateXip(
            uint32_t network_id,
            uint8_t zone_id,
            uint32_t role,
            const std::string& local_ip,
            XipParser& xip);

    uint32_t CreateInterfaceId();

private:
    DISALLOW_COPY_AND_ASSIGN(XipGenerator);
};

typedef std::shared_ptr<XipGenerator> XipGeneratorPtr;

}  // namespace base

}  // namespace top
