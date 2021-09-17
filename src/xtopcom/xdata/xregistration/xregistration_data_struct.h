// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcodec/xmsgpack/xid_codec.hpp"
#include "xbasic/xcodec/xmsgpack/xsimple_map_result_codec.hpp"
#include "xdata/xcodec/xmsgpack/xrec_registration_node_info_codec.hpp"
#include "xdata/xcodec/xmsgpack/xzec_registration_node_info_codec.hpp"
#include "xdata/xregistration/xregistration_node_info.h"

#include <string>
#include <vector>

NS_BEG3(top, data, registration)

using xregistration_chain_result_t = xsimple_map_result_t<common::xnode_id_t, xrec_registration_node_info_t>;

using xregistration_result_store_t = xsimple_map_result_t<common::xnetwork_id_t, xregistration_chain_result_t>;

using xzec_registration_result_t = xsimple_map_result_t<common::xnode_id_t, xzec_registration_node_info_t>;

NS_END3