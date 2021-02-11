// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcodec/xconverter.h"
#include "xcommon/xnode_id.h"
#include "xprotobuf/xnode_id.pb.h"

NS_BEG2(top, codec)

template <>
struct xtop_converter<common::xnode_id_t, top::network::protobuf::NodeId> final
{
    static
    common::xnode_id_t
    convert_from(top::network::protobuf::NodeId const & proto_node_id);

    static
    top::network::protobuf::NodeId
    convert_from(common::xnode_id_t const & node_id);
};


NS_END2

