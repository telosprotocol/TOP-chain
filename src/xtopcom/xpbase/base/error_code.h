#pragma once

namespace top {
namespace kadmlia {

enum RoutingReturnCode {
    kKadSuccess = 0,
    kKadFailed = 1,
    kKadTimeout = 2,
    kKadNodeHasAdded = 3,
};

} // namespace kadmlia
} // namespace top
