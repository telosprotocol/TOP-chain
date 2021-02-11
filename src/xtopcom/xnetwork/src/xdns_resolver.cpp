// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//

#include "xnetwork/xdns_resolver.h"
#include "xnetwork/xutility.hpp"

NS_BEG2(top, network)

xtop_dns_resolver::xtop_dns_resolver(std::shared_ptr<xasio_io_context_wrapper_t> const & io_wrapper)
    : m_resolver{ io_wrapper->create<asio::ip::udp::resolver>() } {
}

xendpoint_t
xtop_dns_resolver::resolve(std::string const & address, std::uint16_t const port, std::error_code & ec) {
    auto const result = m_resolver.resolve(address, std::to_string(port), ec);
    if (!ec) {
        return convert_to<xendpoint_t>::from(result->endpoint());
    }

    return xendpoint_t{};
}

NS_END2

