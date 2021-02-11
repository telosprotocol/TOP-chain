// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xhashable.hpp"
#include "xbasic/xsimple_message.hpp"
#include "xcommon/xaddress.h"
#include "xcommon/xlogic_time.h"
#include "xvnetwork/xmessage.h"

#include <cstdint>

NS_BEG2(top, vnetwork)

enum class xenum_vnetwork_message_type : std::uint8_t
{
    invalid = 0,
    forward = 1,
    normal  = 2
};
using xvnetwork_message_type_t = xenum_vnetwork_message_type;

class xtop_vnetwork_message final : public xhashable_t<xtop_vnetwork_message>
{
    common::xnode_address_t m_sender{};
    common::xnode_address_t m_receiver{};
    xmessage_t m_message{};
    common::xlogic_time_t m_logic_time{};

public:
    using hash_result_type = xhashable_t<xtop_vnetwork_message>::hash_result_type;

    xtop_vnetwork_message()                                          = default;
    xtop_vnetwork_message(xtop_vnetwork_message const &)             = default;
    xtop_vnetwork_message & operator=(xtop_vnetwork_message const &) = default;
    xtop_vnetwork_message(xtop_vnetwork_message &&)                  = default;
    xtop_vnetwork_message & operator=(xtop_vnetwork_message &&)      = default;
    ~xtop_vnetwork_message()                                         = default;

    xtop_vnetwork_message(common::xnode_address_t const & sender,
                          common::xnode_address_t const & receiver,
                          xmessage_t message,
                          common::xlogic_time_t const logic_time);

    common::xnode_address_t const &
    sender() const noexcept;

    common::xnode_address_t const &
    receiver() const noexcept;

    void 
    receiver(common::xnode_address_t new_recv) noexcept;

    xmessage_t const &
    message() const noexcept;

    xbyte_buffer_t const &
    message_payload() const noexcept;

    common::xmessage_id_t
    message_id() const noexcept;

    common::xlogic_time_t
    logic_time() const noexcept;

    bool
    empty() const noexcept;

    hash_result_type
    hash() const override;
};
using xvnetwork_message_t = xtop_vnetwork_message;

NS_END2
