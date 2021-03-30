// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xnetwork/tests/xio_context_manager.hpp"

#include <gtest/gtest.h>

#include <cstddef>

NS_BEG3(top, network, tests)

template <std::size_t IoContextCount>
class xtop_io_context_fixture : public testing::Test
{
protected:
    xio_context_manager_t<IoContextCount> m_io_manager{};

public:
    xtop_io_context_fixture()                                            = default;
    xtop_io_context_fixture(xtop_io_context_fixture const &)             = delete;
    xtop_io_context_fixture & operator=(xtop_io_context_fixture const &) = delete;
    xtop_io_context_fixture(xtop_io_context_fixture &&)                  = default;
    xtop_io_context_fixture & operator=(xtop_io_context_fixture &&)      = default;
    ~xtop_io_context_fixture() override                                  = default;

protected:
    void
    SetUp() override {
        Test::SetUp();
        m_io_manager.start();
    }

    void
    TearDown() override {
        m_io_manager.stop();
        Test::TearDown();
    }
};

template <std::size_t IoContextCount>
using xio_context_fixture_t = xtop_io_context_fixture<IoContextCount>;

NS_END3
