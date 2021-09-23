// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcodec/xmsgpack/xid_codec.hpp"
#include "xbasic/xcodec/xmsgpack/xsimple_map_result_codec.hpp"
#include "xcommon/xip.h"
#include "xcommon/xnode_id.h"
#include "xdata/xcodec/xmsgpack/xstandby_node_info2_codec.hpp"
#include "xdata/xstandby/xstandby_node_info2.h"

NS_BEG3(top, data, standby)

using xrec_standby_chain_result_t = top::xsimple_map_result_t<common::xnode_id_t, xrec_standby_node_info_t>;

using xrec_standby_result_store_t = top::xsimple_map_result_t<common::xnetwork_id_t, xrec_standby_chain_result_t>;

using xzec_standby_result_t = top::xsimple_map_result_t<common::xnode_id_t, xzec_standby_node_info_t>;

NS_END3