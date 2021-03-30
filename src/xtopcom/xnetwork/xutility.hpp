// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xasio_config.h"
#include "xbasic/xutility.h"
#include "xnetwork/xendpoint.h"

#include <asio/ip/udp.hpp>

#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

NS_BEG2(top, network)


template <typename ToType>
struct convert_to;

template <>
struct convert_to<xendpoint_t>
{
    static
    xendpoint_t
    from(std::pair<::sockaddr_storage, socklen_t> const & addr)
    {
        auto const & ss = top::get<::sockaddr_storage>(addr);
        auto const & sslen = top::get<socklen_t>(addr);

        char host[NI_MAXHOST] = { 0 };
        char serv[NI_MAXSERV] = { 0 };
        if (getnameinfo(reinterpret_cast<sockaddr const *>(&ss),
                        sslen,
                        host,
                        NI_MAXHOST,
                        serv,
                        NI_MAXSERV,
                        NI_NUMERICHOST | NI_NUMERICSERV) == 0) {
            return xendpoint_t{ host, static_cast<std::uint16_t>(std::stoi(serv)) };
        }

        throw std::runtime_error{ "convert sockaddr_storage to xendpoint_t failed" };
    }

    static
    xendpoint_t
    from(asio::ip::udp::endpoint const & endpoint) {
        return xendpoint_t{ endpoint.address().to_string(), endpoint.port() };
    }
};

template <>
struct convert_to<asio::ip::udp::endpoint>
{
    static
    asio::ip::udp::endpoint
    from(std::pair<::sockaddr_storage, socklen_t> const & addr) {
        auto const & ss = top::get<::sockaddr_storage>(addr);
        auto const & sslen = top::get<socklen_t>(addr);

        char host[NI_MAXHOST] = { 0 };
        char serv[NI_MAXSERV] = { 0 };
        if (auto const err = getnameinfo(reinterpret_cast<sockaddr const *>(&ss),
                                         sslen,
                                         host,
                                         NI_MAXHOST,
                                         serv,
                                         NI_MAXSERV,
                                         NI_NUMERICHOST | NI_NUMERICSERV)) {
            std::string what{ __FILE__ ": getnameinfo failed: "};
            what += gai_strerror(err);

            throw std::runtime_error{ what };
        }

        return { asio::ip::make_address(host), static_cast<std::uint16_t>(std::stoi(serv)) };
    }

    static
    asio::ip::udp::endpoint
    from(xendpoint_t const & endpoint) {
        return { asio::ip::make_address(endpoint.address()), endpoint.port() };
    }
};

template <>
struct convert_to<std::pair<::sockaddr_storage, socklen_t>>
{
    static
    std::pair<::sockaddr_storage, socklen_t>
    from(xendpoint_t const & endpoint) {
        ::addrinfo hints{};
        std::memset(&hints, 0, sizeof(hints));

        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_family = AF_UNSPEC;
        hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV;

        ::addrinfo * result{ nullptr };
        if (auto const err = getaddrinfo(endpoint.address().c_str(),
                                         std::to_string(endpoint.port()).c_str(),
                                         &hints,
                                         &result)) {
            ::freeaddrinfo(result);

            std::string what{ __FILE__ ": getaddrinfo failed: " };
            what += gai_strerror(err);

            throw std::runtime_error{ what };
        }

        ::sockaddr_storage ss{};
        memcpy(&ss, result->ai_addr, result->ai_addrlen);

        auto sslen = result->ai_addrlen;

        ::freeaddrinfo(result);

        return { std::move(ss), sslen };
    }

    static
    std::pair<::sockaddr_storage, socklen_t>
    from(asio::ip::udp::endpoint const & endpoint) {
        return from(xendpoint_t{ endpoint.address().to_string(), endpoint.port() });
    }
};


NS_END2
