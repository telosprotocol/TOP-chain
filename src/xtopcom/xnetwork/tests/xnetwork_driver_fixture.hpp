// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xthreading/xutility.h"
#include "xbasic/xtimer_driver.h"
#include "xbasic/xutility.h"
#include "xnetwork/tests/xnetwork_driver_manager.hpp"
#include "xnetwork/tests/xtimer_driver_fixture.hpp"
#include "xnetwork/xnetwork_driver.h"
#include "xnetwork/xp2p/xdht_host.h"
#include "xnetwork/xudp_socket.h"

#include <array>
#include <memory>
#include <mutex>
#include <sstream>

NS_BEG3(top, network, tests)

template
<
    std::size_t NetworkDriverCount,
    std::size_t TimerDriverCount,
    std::size_t IoContextCount,
    typename AppSocketT,
    typename DhtSocketT
>
class xtop_network_driver_fixture : public xtimer_driver_fixture_t<TimerDriverCount, IoContextCount>
{
    using base_t = xtimer_driver_fixture_t<TimerDriverCount, IoContextCount>;
protected:
    xnetwork_driver_manager_t<NetworkDriverCount, AppSocketT, DhtSocketT> m_network_driver_manager
    {
        static_cast<xobject_manager_t<xasio_io_context_wrapper_t> *>(std::addressof(this->m_io_manager)),
        static_cast<xobject_manager_t<xtimer_driver_t> *>(std::addressof(this->m_timer_driver_manager))
    };

public:
#if defined XCXX11
    xtop_network_driver_fixture() : m_received_msg_count{ 0 }
    {
    }
#else
    xtop_network_driver_fixture()                                                = default;
#endif
    xtop_network_driver_fixture(xtop_network_driver_fixture const &)             = delete;
    xtop_network_driver_fixture & operator=(xtop_network_driver_fixture const &) = delete;
    xtop_network_driver_fixture(xtop_network_driver_fixture &&)                  = default;
    xtop_network_driver_fixture & operator=(xtop_network_driver_fixture &&)      = default;
    ~xtop_network_driver_fixture() override                                      = default;

protected:
    std::mutex m_messages_mutex{};
    std::unordered_map<common::xnode_id_t, common::xnode_id_t> m_messages{};
    std::atomic<std::size_t> m_received_msg_count{ 0 };
    void
    SetUp() override {
        base_t::SetUp();
        m_received_msg_count.store(0, std::memory_order_release);

        for (auto i = 0u; i < NetworkDriverCount; ++i) {
            m_network_driver_manager.object(i).register_message_ready_notify(std::bind(&xtop_network_driver_fixture::on_message,
                                                                                       this,
                                                                                       std::addressof(m_network_driver_manager.object(i)),
                                                                                       std::placeholders::_1,
                                                                                       std::placeholders::_2));
        }

        m_network_driver_manager.start();
    }

    void
    TearDown() override {
        XLOCK_GUARD(m_messages_mutex) {
            m_messages.clear();
        }

        m_network_driver_manager.stop();
        for (auto i = 0u; i < NetworkDriverCount; ++i) {
            m_network_driver_manager.object(i).unregister_message_ready_notify();
        }

        base_t::TearDown();
    }

    void
    on_message(xnetwork_driver_face_t const * network_driver,
               common::xnode_id_t const & sender,
               xbyte_buffer_t const & bytes) {
        m_received_msg_count.fetch_add(1, std::memory_order_acq_rel);
        do_on_message(network_driver, sender, bytes);
    }

protected:
    virtual
    void
    do_on_message(xnetwork_driver_face_t const *,
                  common::xnode_id_t const & sender,
                  xbyte_buffer_t const & bytes) {
        std::string const message{ std::begin(bytes), std::end(bytes) };
        std::istringstream iss{ message };

        std::string sender_id_string;
        iss >> sender_id_string;

        std::string receiver_id_string;
        iss >> receiver_id_string;

        EXPECT_EQ(sender.to_string(), sender_id_string);

        XLOCK_GUARD(m_messages_mutex) {
            ASSERT_TRUE(top::get<bool>(m_messages.insert({ common::xnode_id_t{ receiver_id_string }, common::xnode_id_t{ sender_id_string } })));
        }
    }
};

template
<
    std::size_t NetworkDriverCount,
    std::size_t TimerDriverCount,
    std::size_t IoContextCount,
    typename AppSocketT = xudp_socket_t,
    typename DhtSocketT = xudp_socket_t
>
using xnetwork_driver_fixture_t = xtop_network_driver_fixture<NetworkDriverCount, TimerDriverCount, IoContextCount, AppSocketT, DhtSocketT>;

NS_END3
