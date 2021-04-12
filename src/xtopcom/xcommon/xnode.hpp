// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <utility>

NS_BEG2(top, common)

template <typename EndpointOrAddressT, typename IdT>
class xtop_node
{
public:
    using endpoint_type = EndpointOrAddressT;
    using address_type = EndpointOrAddressT;
    using id_type = IdT;

protected:
    endpoint_type m_endpoint{};
    id_type      m_id{};

public:
    xtop_node()                              = default;
    xtop_node(xtop_node const &)             = default;
    xtop_node & operator=(xtop_node const &) = default;
    xtop_node(xtop_node &&)                  = default;
    xtop_node & operator=(xtop_node &&)      = default;
    virtual ~xtop_node()                     = default;

    xtop_node(id_type i, endpoint_type ep) noexcept
        : m_endpoint{ std::move(ep) }, m_id{ std::move(i) }
    {
    }

    void
    swap(xtop_node & other) noexcept {
        std::swap(m_endpoint, other.m_endpoint);
        std::swap(m_id, other.m_id);
    }

    bool
    operator==(xtop_node const & other) const noexcept {
        return m_endpoint == other.m_endpoint && m_id == other.m_id;
    }

    endpoint_type &
    endpoint() & noexcept {
        return m_endpoint;
    }

    endpoint_type const &
    endpoint() const & noexcept {
        return m_endpoint;
    }

    endpoint_type &&
    endpoint() && noexcept {
        return std::move(m_endpoint);
    }

    endpoint_type const &&
    endpoint() const &&  noexcept {
        return std::move(m_endpoint);
    }

    address_type &
    address() & noexcept {
        return m_endpoint;
    }

    address_type const &
    address() const & noexcept {
        return m_endpoint;
    }

    address_type &&
    address() && noexcept {
        return std::move(m_endpoint);
    }

    address_type const &&
    address() const &&  noexcept {
        return std::move(m_endpoint);
    }

    id_type &
    id() & noexcept {
        return m_id;
    }

    id_type const &
    id() const & noexcept {
        return m_id;
    }

    id_type &&
    id() && noexcept {
        return std::move(m_id);
    }

    id_type const &&
    id() const && noexcept {
        return std::move(m_id);
    }

    bool
    empty() const noexcept {
        return m_endpoint.empty() || m_id.empty();
    }
};

template <typename EndpointT, typename IdT>
using xnode_t = xtop_node<EndpointT, IdT>;

NS_END2
