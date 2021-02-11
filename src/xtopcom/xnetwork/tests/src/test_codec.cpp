// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcodec/xmsgpack_codec.hpp"
#include "xnetwork/xcodec/xmsgpack/xnode_address_codec.hpp"
#include "xnetwork/xcodec/xmsgpack/xdiscover_datagram_codec.hpp"
#include "xnetwork/xcodec/xmsgpack/xdiscover_message_codec.hpp"
#include "xnetwork/xnode_endpoint.h"
#include "xcommon/xnode_id.h"
#include "xnetwork/xp2p/xdiscover_datagram.h"
#include "xnetwork/xp2p/xdiscover_message.h"

#include <gtest/gtest.h>


using namespace top;

TEST(xnetwork, node_address_codec) {
    network::xnode_endpoint_t const endpoint{ "1.2.3.4", 1000 };
    auto const bytes = codec::xmsgpack_codec_t<network::xnode_endpoint_t>::encode(endpoint);
    auto const decoded_endpoint = codec::xmsgpack_codec_t<network::xnode_endpoint_t>::decode(bytes);
    EXPECT_EQ(endpoint, decoded_endpoint);
    EXPECT_EQ(decoded_endpoint.empty(), false);
    EXPECT_EQ(decoded_endpoint.address(), endpoint.address());
    EXPECT_EQ(decoded_endpoint.port(), endpoint.port());
}

TEST(xnetwork, node_id_codec) {
    std::string const node_id_value{ "im the node id" };
    common::xnode_id_t const node_id{ node_id_value };
    EXPECT_EQ(node_id.value(), node_id_value);

    auto const node_id_bytes = codec::xmsgpack_codec_t<common::xnode_id_t>::encode(node_id);
    auto const decoded_node_id = codec::xmsgpack_codec_t<common::xnode_id_t>::decode(node_id_bytes);
    EXPECT_EQ(node_id, decoded_node_id);
    EXPECT_EQ(decoded_node_id.value(), node_id.value());
    EXPECT_EQ(decoded_node_id.value(), node_id_value);
}

TEST(xnetwork, ping_node_codec) {
    network::xnode_endpoint_t const endpoint{ "1.2.3.4", 1000 };
    common::xnode_id_t const node_id{ "im the node id" };
    network::xdht_node_t const target_node{ node_id, endpoint };

    network::p2p::xping_node_t const ping_node{ target_node };
    auto const ping_node_bytes = codec::xmsgpack_codec_t<network::p2p::xping_node_t>::encode(ping_node);
    auto const decoded_ping_node = codec::xmsgpack_codec_t<network::p2p::xping_node_t>::decode(ping_node_bytes);
    EXPECT_EQ(decoded_ping_node.empty(), false);
    EXPECT_EQ(decoded_ping_node.target_node(), target_node);
    EXPECT_EQ(decoded_ping_node.target_node().empty(), false);
    EXPECT_EQ(decoded_ping_node.target_node().endpoint(), endpoint);
    EXPECT_EQ(decoded_ping_node.target_node().id(), node_id);
}

TEST(xnetwork, pong_codec) {
    network::xnode_endpoint_t const endpoint{ "1.2.3.4", 1000 };
    network::p2p::xpong_t const pong{ endpoint, top::xhash256_t{} };
    auto const pong_bytes = codec::xmsgpack_codec_t<network::p2p::xpong_t>::encode(pong);
    auto const decoded_pong = codec::xmsgpack_codec_t<network::p2p::xpong_t>::decode(pong_bytes);
    EXPECT_EQ(false, decoded_pong.empty());
    EXPECT_EQ(endpoint, decoded_pong.ping_endpoint());
    EXPECT_EQ(false, decoded_pong.ping_endpoint().empty());
    EXPECT_EQ(endpoint.address(), decoded_pong.ping_endpoint().address());
    EXPECT_EQ(endpoint.port(), decoded_pong.ping_endpoint().port());
    // EXPECT_EQ(10000, decoded_pong.app_port());
}

TEST(xnetwork, find_node_codec) {
    common::xnode_id_t const node_id{ "im the node id" };
    network::p2p::xfind_node_t const find_node{ node_id };
    auto const find_node_bytes = codec::xmsgpack_codec_t<network::p2p::xfind_node_t>::encode(find_node);
    auto const decoded_find_node = codec::xmsgpack_codec_t<network::p2p::xfind_node_t>::decode(find_node_bytes);
    EXPECT_EQ(decoded_find_node.empty(), false);
    EXPECT_EQ(decoded_find_node.target_id(), node_id);
}

TEST(xnetwork, neighbors_codec) {
    network::xnode_endpoint_t const endpoint{ "1.2.3.4", 1000 };
    common::xnode_id_t const node_id{ "im the node id" };

    network::p2p::xneighbors_t neighbors{};
    neighbors.add_neighbor({ node_id, endpoint });
    auto const neighbors_bytes = codec::xmsgpack_codec_t<network::p2p::xneighbors_t>::encode(neighbors);
    auto const decoded_neighbors = codec::xmsgpack_codec_t<network::p2p::xneighbors_t>::decode(neighbors_bytes);
    EXPECT_EQ(decoded_neighbors.neighbors().size(), 1);

    // auto const & neighbor = *std::begin(neighbors.neighbors());
    // EXPECT_EQ(10000, neighbor.second);
}

TEST(xnetwork, ping_node_message_codec) {
    network::xnode_endpoint_t const endpoint{ "1.2.3.4", 1000 };
    common::xnode_id_t const node_id{ "im the node id" };
    network::xdht_node_t const node{ node_id, endpoint };

    network::p2p::xping_node_t const ping_node{ node };
    network::p2p::xdiscover_message_t const ping_node_message
    {
        codec::xmsgpack_codec_t<network::p2p::xping_node_t>::encode(ping_node),
        node_id,
        network::p2p::xdiscover_message_type_t::ping_node
    };
    auto const ping_message_bytes = codec::xmsgpack_codec_t<network::p2p::xdiscover_message_t>::encode(ping_node_message);
    auto const decoded_ping_message = codec::xmsgpack_codec_t<network::p2p::xdiscover_message_t>::decode(ping_message_bytes);
    EXPECT_EQ(decoded_ping_message.empty(), false);
    EXPECT_EQ(decoded_ping_message.payload(), codec::xmsgpack_codec_t<network::p2p::xping_node_t>::encode(ping_node));
    EXPECT_EQ(decoded_ping_message.sender_node_id(), node_id);
    EXPECT_EQ(decoded_ping_message.type(), network::p2p::xdiscover_message_type_t::ping_node);
}

TEST(xnetwork, pong_message_codec) {
    network::xnode_endpoint_t const endpoint{ "1.2.3.4", 1000 };
    network::p2p::xpong_t const pong{ endpoint, top::xhash256_t{} };
    common::xnode_id_t const node_id{ "im the node id" };
    auto const pong_bytes = codec::xmsgpack_codec_t<network::p2p::xpong_t>::encode(pong);

    network::p2p::xdiscover_message_t const pong_message
    {
        pong_bytes,
        node_id,
        network::p2p::xdiscover_message_type_t::pong
    };
    auto const pong_message_bytes = codec::xmsgpack_codec_t<network::p2p::xdiscover_message_t>::encode(pong_message);
    auto const decoded_pong_message = codec::xmsgpack_codec_t<network::p2p::xdiscover_message_t>::decode(pong_message_bytes);
    EXPECT_EQ(false, decoded_pong_message.empty());
    EXPECT_EQ(pong_bytes, decoded_pong_message.payload());
    EXPECT_EQ(node_id, decoded_pong_message.sender_node_id());
    EXPECT_EQ(network::p2p::xdiscover_message_type_t::pong, decoded_pong_message.type());
}
