// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <cstdint>
#include <memory>

#include "xpbase/base/contain_template.h"
#include "xtransport/proto/transport.pb.h"
#include "xkad/proto/kadmlia.pb.h"
#include "xkad/gossip/rumor_def.h"

namespace top {
namespace transport {
namespace protobuf {
class RoutingMessage;
}  // namespace protobuf
}  // namespace transport

namespace gossip {

class RumorFilter :
    public ContainTemplate<
    uint32_t,
    kDefaultFilterSize,
    kRepeatedValueNum>{
public:
    static RumorFilter* Instance();
    ~RumorFilter() {}
    bool FiltMessage(
        transport::protobuf::RoutingMessage&);
private:
    RumorFilter() {}
};

typedef std::shared_ptr<RumorFilter> RumorFilterPtr;

}
}
