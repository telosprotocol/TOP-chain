// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xhashable.hpp"
#include "xdata/xelection/xelection_info.h"
#include "xelection/xcache/xbasic_element.h"
#include "xelection/xcache/xelement_fwd.h"

#include <memory>

NS_BEG3(top, election, cache)

/**
 * @brief cache object holds a node data
 */
class xtop_node_element
  : public xbasic_element_t
  , public xhashable_t<xtop_node_element> {
public:
    using hash_result_type = xhashable_t<xtop_node_element>::hash_result_type;

private:
    using xbase_t = xbasic_element_t;

protected:
    std::weak_ptr<xgroup_element_t> m_group_element;
    data::election::xelection_info_t m_election_info;

public:
    xtop_node_element(xtop_node_element const &) = delete;
    xtop_node_element & operator=(xtop_node_element const &) = delete;
    xtop_node_element(xtop_node_element &&) = default;
    xtop_node_element & operator=(xtop_node_element &&) = default;
    ~xtop_node_element() override = default;

    xtop_node_element(common::xnode_id_t const & node_id,
                      common::xslot_id_t const & slot_id,
                      data::election::xelection_info_t const & election_info,
                      std::shared_ptr<xgroup_element_t> const & group_element);

    std::shared_ptr<xgroup_element_t> group_element() const noexcept;

    data::election::xelection_info_t const & election_info() const noexcept;

    common::xversion_t const & joined_version() const noexcept;

    std::uint64_t staking() const noexcept;

    hash_result_type hash() const override;

    void swap(xtop_node_element & other) noexcept;

    bool operator==(xtop_node_element const & other) const noexcept;

    bool operator!=(xtop_node_element const & other) const noexcept;
};

NS_END3
