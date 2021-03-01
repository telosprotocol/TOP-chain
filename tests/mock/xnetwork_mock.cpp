#include "xnetwork_mock.h"
namespace top {
namespace mock {
    network_mock::nodes network_mock::s_nodes;
    std::recursive_mutex network_mock::s_mutex{};
}
}
