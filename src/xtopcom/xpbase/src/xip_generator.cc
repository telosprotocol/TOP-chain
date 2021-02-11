#include "xpbase/base/xip_generator.h"

namespace top {

namespace base {

bool XipGenerator::CreateXip(
        uint32_t network_id,
        uint8_t zone_id,
        uint32_t role,
        const std::string& local_ip,
        XipParser& xip) {
    xip.set_xnetwork_id(network_id);
    xip.set_zone_id(zone_id);
    xip.set_xinterface_id(CreateInterfaceId());
    xip.set_network_type(role);

    if (local_ip.empty()) {
        return false;
    }

    struct in_addr addr;
    if (inet_pton(AF_INET, local_ip.c_str(), &addr.s_addr) != 1) {
        return false;
    }

    uint32_t ip_32 = addr.s_addr;
    xip.set_cluster_id(ip_32 & 0x7F);
    xip.set_group_id(ip_32 >> 8 & 0xFF);  // 111.222.333.444 -> 222
    xip.set_node_id((ip_32 >> 16 & 0xFFFF) % 255);  // 111.222.333.444 -> 333.444
    return true;
}

uint32_t XipGenerator::CreateInterfaceId() {
    return RandomUint32() % std::numeric_limits<uint32_t>::max();
}

}  // namespace base

}  // namespace top
