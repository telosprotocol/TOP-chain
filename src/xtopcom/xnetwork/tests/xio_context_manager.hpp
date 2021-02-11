// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xthreading/xbackend_thread.hpp"
#include "xnetwork/tests/xobject_manager.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <memory>

NS_BEG3(top, network, tests)

template <std::size_t IoContextCount>
class xtop_io_context_manager final : public xobject_manager_t<xasio_io_context_wrapper_t>
{
private:
    std::array<std::shared_ptr<xasio_io_context_wrapper_t>, IoContextCount> m_io_context_wrappers{};

public:
    xtop_io_context_manager(xtop_io_context_manager const &)             = delete;
    xtop_io_context_manager & operator=(xtop_io_context_manager const &) = delete;
    xtop_io_context_manager(xtop_io_context_manager &&)                  = default;
    xtop_io_context_manager & operator=(xtop_io_context_manager &&)      = default;

    xtop_io_context_manager() {
        for (auto i = 0u; i < IoContextCount; ++i) {
            m_io_context_wrappers[i] = std::make_shared<xasio_io_context_wrapper_t>();
        }
    }

    ~xtop_io_context_manager() {
        for (auto i = 0u; i < IoContextCount; ++i) {
            m_io_context_wrappers[i].reset();
        }
    }

    void
    start() override {

        for (auto i = 0u; i < IoContextCount; ++i) {
            m_io_context_wrappers[i]->async_start();
        }
    }

    void
    stop() override {
        for (auto i = 0u; i < IoContextCount; ++i) {
            assert(m_io_context_wrappers[i]);
            m_io_context_wrappers[i]->stop();
        }
    }

    std::size_t
    object_count() const noexcept override {
        return IoContextCount;
    }

    xasio_io_context_wrapper_t &
    object(std::size_t const index) override {
        return *m_io_context_wrappers.at(index);
    }

    xasio_io_context_wrapper_t const &
    object(std::size_t const index) const override {
        return *m_io_context_wrappers.at(index);
    }
};

template <std::size_t IoContextCount>
using xio_context_manager_t = xtop_io_context_manager<IoContextCount>;

NS_END3
