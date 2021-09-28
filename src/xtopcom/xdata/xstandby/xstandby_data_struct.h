// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcodec/xmsgpack/xid_codec.hpp"
#include "xbasic/xcodec/xmsgpack/xsimple_map_result_codec.hpp"
#include "xbasic/xutility.h"
#include "xcommon/xip.h"
#include "xcommon/xnode_id.h"
#include "xdata/xcodec/xmsgpack/xstandby_node_info2_codec.hpp"
#include "xdata/xelection/xstandby_network_result.h"
#include "xdata/xstandby/xstandby_node_info2.h"

NS_BEG3(top, data, standby)

using xsimple_standby_result_t = top::xsimple_map_result_t<common::xnode_id_t, xsimple_standby_node_info_t>;

using xrec_standby_result_store_t = top::xsimple_map_result_t<common::xnetwork_id_t, xsimple_standby_result_t>;

using xzec_standby_result_t = top::xsimple_map_result_t<common::xnode_id_t, xzec_standby_node_info_t>;

// convert new zec_standby_result_t into old xstandby_network_result_t.
// so no need to update election algroithm for now.
// temporarily function. lack of mainnet activatied judgment logical.
// data::election::xstandby_network_result_t to_standby_network_result(data::standby::xzec_standby_result_t const & zec_standby_result);


/**
 * @brief select nodes from zec_standby with certain node_type
 * 
 * @param zec_standby_result input params
 * @param node_type node_type that you need
 * @param genesis_nodes_only if only need genesis_node. set is True, defaultly False
 * @return data::standby::xsimple_standby_result_t = simple_map<node_id, simple_standby_node_info>
 */
data::standby::xsimple_standby_result_t select_standby_nodes(data::standby::xzec_standby_result_t const & zec_standby_result,
                                                             common::xnode_type_t const & node_type,
                                                             bool genesis_nodes_only = false);

NS_END3