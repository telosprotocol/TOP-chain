// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xaddress.h"

#include <cstdint>
#include <utility>
#include <vector>

NS_BEG2(top, data)

std::vector<uint16_t> get_table_ids(common::xzone_id_t const & zone_id,
                                    common::xcluster_id_t const & cluster_id,
                                    common::xgroup_id_t const & group_id,
                                    common::xgroup_id_t const & associated_parent_group_id);

std::vector<uint16_t> get_table_ids(common::xgroup_address_t const & group_address,
                                    common::xgroup_id_t const & associated_parent_group_id);


NS_END2
