// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"
#include "xcommon/xnode_type.h"
#include "xcommon/xversion.h"

NS_BEG3(top, election, cache)

class xtop_basic_element {
    common::xnode_address_t m_address;

protected:
    /**
     * @brief Construct a network data element
     */
    explicit xtop_basic_element(common::xnetwork_id_t const & network_id);

    /**
     * @brief Construct a zone data element
     */
    xtop_basic_element(common::xnetwork_id_t const & network_id, common::xzone_id_t const & zone_id);

    /**
     * @brief Construct a cluster data element
     */
    xtop_basic_element(common::xnetwork_id_t const & network_id, common::xzone_id_t const & zone_id, common::xcluster_id_t const & cluster_id);

    xtop_basic_element(common::xversion_t const & version,
                       common::xnetwork_id_t const & network_id,
                       common::xzone_id_t const & zone_id,
                       common::xcluster_id_t const & cluster_id,
                       common::xgroup_id_t const & group_id,
                       uint16_t const sharding_size,
                       uint64_t const associated_blk_height);

    xtop_basic_element(common::xversion_t const & version,
                       common::xnetwork_id_t const & network_id,
                       common::xzone_id_t const & zone_id,
                       common::xcluster_id_t const & cluster_id,
                       common::xgroup_id_t const & group_id,
                       common::xnode_id_t const & node_id,
                       common::xslot_id_t const & slot_id,
                       uint16_t const sharding_size,
                       uint64_t const associated_blk_height);

public:
    xtop_basic_element(xtop_basic_element const &) = delete;
    xtop_basic_element & operator=(xtop_basic_element const &) = delete;
    xtop_basic_element(xtop_basic_element &&) = default;
    xtop_basic_element & operator=(xtop_basic_element &&) = default;
    virtual ~xtop_basic_element() = default;

    common::xnode_type_t type() const noexcept;

    common::xversion_t const & version() const noexcept;

    common::xnetwork_id_t network_id() const noexcept;

    common::xzone_id_t zone_id() const noexcept;

    common::xcluster_id_t cluster_id() const noexcept;

    common::xgroup_id_t group_id() const noexcept;

    common::xslot_id_t slot_id() const noexcept;

    uint16_t sharding_size() const noexcept;

    uint64_t associated_blk_height() const noexcept;

    common::xnode_id_t const & node_id() const noexcept;

    common::xnode_address_t const & address() const noexcept;

protected:
    void swap(xtop_basic_element & other) noexcept;

    bool operator==(xtop_basic_element const & other) const noexcept;
};
using xbasic_element_t = xtop_basic_element;

NS_END3
