// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

// #include "xbasic/xthreading/xutility.h"
// #include "xbasic/xtimer_driver.h"
#include "xnetwork/tests/xnetwork_driver_fixture.hpp"
// #include "xnetwork/xnetwork_driver.h"
// #include "xnetwork/xp2p/xdht_host.h"
// #include "xnetwork/xudp_socket.h"

#include <cassert>

NS_BEG3(top, network, tests)

template
<
    std::size_t NetworkDriverCount,
    std::size_t TimerDriverCount,
    std::size_t IoContextCount,
    typename AppSocketT,
    typename DhtSocketT
>
class xtop_rumor_fixture : public xnetwork_driver_fixture_t<NetworkDriverCount, TimerDriverCount, IoContextCount, AppSocketT, DhtSocketT>
{
public:
    xtop_rumor_fixture()                                       = default;
    xtop_rumor_fixture(xtop_rumor_fixture const &)             = delete;
    xtop_rumor_fixture & operator=(xtop_rumor_fixture const &) = delete;
    xtop_rumor_fixture(xtop_rumor_fixture &&)                  = default;
    xtop_rumor_fixture & operator=(xtop_rumor_fixture &&)      = default;
    ~xtop_rumor_fixture() override                             = default;

    std::string const rumor_base{ "rumor" };
    std::vector<std::string> rumors;

protected:
    void
    do_on_message(xnetwork_driver_face_t const *,
                  common::xnode_id_t const &,
                  xbyte_buffer_t const & bytes) override {
        std::string const actual{ std::begin(bytes), std::begin(bytes) + rumor_base.size() };
        EXPECT_EQ(rumor_base, actual);

        // xdbg("[aries hit]%ld %zu\n", std::time(nullptr), this->m_received_msg_count.load(std::memory_order_relaxed));
        std::printf("%ld %zu\n", std::time(nullptr), this->m_received_msg_count.load(std::memory_order_relaxed));
        std::fflush(stdout);
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
using xrumor_fixture_t = xtop_rumor_fixture<NetworkDriverCount, TimerDriverCount, IoContextCount, AppSocketT, DhtSocketT>;

NS_END3
