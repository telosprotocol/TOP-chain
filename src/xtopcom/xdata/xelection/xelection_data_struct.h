// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcodec/xmsgpack/xsimple_map_result_codec.hpp"
#include "xbasic/xsimple_map_result.hpp"
#include "xcommon/xip.h"
#include "xcommon/xnode_type.h"
#include "xdata/xcodec/xmsgpack/xelection_group_result_codec.hpp"
#include "xdata/xelection/xelection_group_result.h"

NS_BEG3(top, data, election)

using xelection_cluster_result_t = top::xsimple_map_result_t<common::xgroup_id_t, xelection_group_result_t>;

using xelection_result_t = top::xsimple_map_result_t<common::xcluster_id_t, xelection_cluster_result_t>;

using xelection_network_result_t = top::xsimple_map_result_t<common::xnode_type_t, xelection_result_t>;

using xelection_result_store_t = top::xsimple_map_result_t<common::xnetwork_id_t, xelection_network_result_t>;

NS_END3