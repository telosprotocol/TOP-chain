//// Copyright (c) 2017-2018 Telos Foundation & contributors
//// Distributed under the MIT software license, see the accompanying
//// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
//#include "xbase/xlog.h"
//#include "xutility/xhash.h"
//#include "xvnetwork/xcodec/xmsgpack/xmessage_codec.hpp"
//#include "xvnetwork/xcodec/xmsgpack/xvnetwork_message_codec.hpp"
//#include "xvnetwork/xnode_delegate.h"
//#include "xvnetwork/xvnetwork_message.h"
//
//#include <cassert>
//#include <cinttypes>
//
//NS_BEG2(top, vnetwork)
//
//xtop_node_delegate::xtop_node_delegate(network::xnetwork_driver_face_t * network_driver)
//    : m_network_driver{ make_observer(network_driver) }
//{
//}
//
//void
//xtop_node_delegate::start() {
//    assert(m_network_driver);
//    m_network_driver->register_message_ready_notify(std::bind(&xtop_node_delegate::on_network_data_ready,
//                                                              shared_from_this(),
//                                                              std::placeholders::_1,
//                                                              std::placeholders::_2));
//
//    assert(!running());
//    running(true);
//    assert(running());
//}
//
//void
//xtop_node_delegate::stop() {
//    assert(running());
//    running(false);
//    assert(!running());
//
//    assert(m_network_driver);
//    m_network_driver->unregister_message_ready_notify();
//}
//
//common::xnode_id_t const &
//xtop_node_delegate::host_node_id() const noexcept {
//    assert(m_network_driver);
//    return m_network_driver->host_node_id();
//}
//
//void
//xtop_node_delegate::on_network_data_ready(common::xnode_id_t const & sender,
//                                          xbyte_buffer_t const & bytes_message) {
//#if !defined NDEBUG
//    if (m_dedicated_thread_id == std::thread::id{}) {
//        m_dedicated_thread_id = std::this_thread::get_id();
//    }
//
//    assert(m_dedicated_thread_id == std::this_thread::get_id());
//#endif
//
//    if (!running()) {
//        xwarn("[node delegate] not run");
//        return;
//    }
//
//    xdbg("[node delegate] %s receives bytes msg %" PRIx64 " delivered by %s",
//         host_node_id().to_string().c_str(),
//         utl::xxh64_t::digest(bytes_message.data(), bytes_message.size()),
//         sender.to_string().c_str());
//
//    auto vnetwork_message = codec::msgpack_decode<xvnetwork_message_t>(bytes);
//    auto const & message = vnetwork_message.message();
//
//    xdbg("[node delegate] %s receives message %" PRIx64 " from %s msg id %u",
//         host_node_id().to_string().c_str(),
//         vnetwork_message.hash(),
//         vnetwork_message.sender().account_address().to_string().c_str(),
//         static_cast<std::uint32_t>(vnetwork_message.message().id()));
//
//    auto const & sender_address = vnetwork_message.sender();
//    auto const & receiver_address = vnetwork_message.receiver();
//    if (sender_address.version() != receiver_address.version()) {
//        xerror("[node delegate] message sender (version:%s) & receiver(version:%s) are not in the same round. discard",
//               sender_address.version().to_string().c_str(),
//               receiver_address.version().to_string().c_str());
//        return;
//    }
//
//    for (auto const & vswitch_info : m_switches) {
//        auto const & version = std::get<xversion_t>(vswitch_info);
//        auto const & vswitch = std::get<std::shared_ptr<xvswitch_t>>(vswitch_info);
//
//        if (version == receiver_address.version()) {
//            // vswitch->handle_message(vnetwork_message);
//            return;
//        }
//    }
//}
//
//
//NS_END2
