// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xrunnable.h"
#include "xnetwork/xendpoint.h"

#include <asio/ip/udp.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <system_error>

NS_BEG2(top, network)

class xtop_dns_resolver final
{
private:
    asio::ip::udp::resolver m_resolver;

public:
    xtop_dns_resolver(xtop_dns_resolver const &)             = delete;
    xtop_dns_resolver & operator=(xtop_dns_resolver const &) = delete;
    xtop_dns_resolver(xtop_dns_resolver &&)                  = default;
    xtop_dns_resolver & operator=(xtop_dns_resolver &&)      = default;
    ~xtop_dns_resolver()                                     = default;

    explicit
    xtop_dns_resolver(std::shared_ptr<xasio_io_context_wrapper_t> const & io_wrapper);

    xendpoint_t
    resolve(std::string const & address, std::uint16_t const port, std::error_code & ec);
};
using xdns_resolver_t = xtop_dns_resolver;

NS_END2
