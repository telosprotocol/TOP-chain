// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xthreading/xbackend_thread.hpp"
#include "xbasic/xtimer_driver.h"

#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <random>
#include <system_error>
#include <array>

#if 0
TEST(xbasic, timer_driver) {
    auto io_context_wrapper = std::make_shared<top::xasio_io_context_wrapper_t>();
    io_context_wrapper->async_start();

    auto timer_driver = std::make_shared<top::xtimer_driver_t>(io_context_wrapper);
    timer_driver->start();

    constexpr std::size_t loop_count = 10;
    std::size_t counter{ 0 };
    for (auto i = 0u; i < loop_count; ++i) {
        timer_driver->schedule(std::chrono::milliseconds{ 1000 * (i + 1) }, [&counter, i](std::error_code const & ec)
        {
            if (ec) {
                EXPECT_EQ(asio::error::operation_aborted, ec.value());
            } else {
                EXPECT_EQ(i, counter);
                ++counter;
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds{ 30 });

    EXPECT_EQ(loop_count, counter);

    timer_driver->stop();
    timer_driver.reset();
    io_context_wrapper->stop();
    io_context_wrapper.reset();
}
#endif
#if defined(TEST_TIMER_DRIVER_PERF)
TEST(xbasic, base_timer_driver_normal) {
    auto io_thread = top::make_object_ptr<top::base::xiothread_t>();
    auto io_context_wrapper = std::make_shared<top::xbase_io_context_wrapper_t>(io_thread);
    io_context_wrapper->start();

    auto timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_context_wrapper);
    timer_driver->start();

    constexpr std::size_t loop_count = 10;
    std::size_t counter{0};
    for (auto i = 0u; i < loop_count; ++i) {
        timer_driver->schedule(std::chrono::milliseconds{1000 * (i + 1)}, [&counter, i](std::chrono::milliseconds) {
            EXPECT_EQ(i, counter);
            ++counter;
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds{30});

    EXPECT_EQ(loop_count, counter);

    timer_driver->stop();
    timer_driver.reset();
    io_context_wrapper->stop();
    io_context_wrapper.reset();
    io_thread->close();
    io_thread.reset();

    std::this_thread::sleep_for(std::chrono::seconds(2));
}

TEST(xbasic, base_timer_driver_record) {
    auto io_thread = top::make_object_ptr<top::base::xiothread_t>();
    auto io_context_wrapper = std::make_shared<top::xbase_io_context_wrapper_t>(io_thread);
    io_context_wrapper->start();

    auto timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_context_wrapper);
    timer_driver->start();

    constexpr std::size_t loop_count = 100;
    std::array<std::chrono::steady_clock::time_point, loop_count> executed_time_points;
    auto start_time_point = std::chrono::steady_clock::now();
    for (auto i = 0u; i < loop_count; ++i) {
        timer_driver->schedule(std::chrono::milliseconds{1000 * (i + 1)},
                               [&executed_time_points, i](std::chrono::milliseconds) { executed_time_points[i] = std::chrono::steady_clock::now();
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds{120});

    for (auto i = 0u; i < loop_count; ++i) {
        auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(executed_time_points[i] - start_time_point);
        auto excpected_diff = std::chrono::milliseconds{1000 * (i + 1)};
        EXPECT_TRUE((excpected_diff - std::chrono::milliseconds{100}) < time_diff && time_diff < (excpected_diff + std::chrono::milliseconds{100}));
    }

    timer_driver->stop();
    timer_driver.reset();
    io_context_wrapper->stop();
    io_context_wrapper.reset();
    io_thread->close();
    io_thread.reset();

    std::this_thread::sleep_for(std::chrono::seconds{2});
}


TEST(xbasic, base_timer_driver_perf) {
    auto io_thread = top::make_object_ptr<top::base::xiothread_t>();
    auto io_context_wrapper = std::make_shared<top::xbase_io_context_wrapper_t>(io_thread);
    io_context_wrapper->start();

    auto timer_driver = std::make_shared<top::xbase_timer_driver>(io_context_wrapper);
    timer_driver->start();

    constexpr std::size_t loop_count = std::numeric_limits<std::size_t>::max();
    std::size_t counter{0};
    std::random_device rd;   // Will be used to obtain a seed for the random number engine
    std::mt19937_64 gen(rd());  // Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> distrib(1, 1000);
    for (auto i = 0u; i < loop_count; ++i) {
        auto ms = distrib(gen);
        std::this_thread::sleep_for(std::chrono::milliseconds{ms});
        timer_driver->schedule(std::chrono::milliseconds{ms}, [&counter, i](std::chrono::milliseconds) {
            // EXPECT_EQ(i, counter);
            ++counter;
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds{300});

    EXPECT_EQ(loop_count, counter);

    timer_driver->stop();
    timer_driver.reset();
    io_context_wrapper->stop();
    io_context_wrapper.reset();
    io_thread->close();
    io_thread.reset();

    std::this_thread::sleep_for(std::chrono::seconds{2});
}
#endif
