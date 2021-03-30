// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvnetwork/xmessage.h"
#include "xvnetwork/xaddress.h"

#include <functional>


NS_BEG2(top, vnetwork)

// /**
//  * @brief Message ready callback.
//  *        void callback(xvnode_address_t const & src, xvnode_address_t const & dst, xmessage_t const & msg);
//  */
// using xmessage_ready_callback_t = std::function<void(xvnode_address_t const &, xvnode_address_t const &, xmessage_t const &)>;

/**
 * @brief Message ready callback.
 *        void callback(xvnode_address_t const & src, xmessage_t const & msg);
 */
using xmessage_ready_callback_t = std::function<void(xvnode_address_t const &, xmessage_t const &, std::uint64_t const)>;

/**
 * @brief virtual network message ready callback.
 *        void callback(xvnode_address_t const & src, xmessage_t const & msg);
 */
using xvnetwork_message_ready_callback_t = std::function<void(xvnode_address_t const &, xmessage_t const &, std::uint64_t const)>;

NS_END2
