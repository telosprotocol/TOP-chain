// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <functional>
#include <string>

#include "xbase/xthread.h"
#include "xcommon/xlogic_time.h"
#include "xdata/xblock.h"

NS_BEG2(top, time)

struct xchain_time_st {
    constexpr xchain_time_st()                         = default;

    xchain_time_st (data::xblock_t* _timer_block, int64_t recv_ms)
    : timer_block{_timer_block}, local_update_time{recv_ms} {
        if(timer_block != nullptr) {
            timer_block->add_ref();
            xtime_round = timer_block->get_height();
        }
    }

    virtual ~xchain_time_st() {
        if(timer_block != nullptr) {
            timer_block->release_ref();
        }
    }

    xchain_time_st & operator=(xchain_time_st const & time) {
        local_update_time = time.local_update_time;
        xtime_round = time.xtime_round;
        if(timer_block != nullptr) {
            timer_block->release_ref();
        }
        timer_block = time.timer_block;
        if(timer_block != nullptr) {
            timer_block->add_ref();
        }
        return *this;
    }

    data::xblock_t*       timer_block{};
    common::xlogic_time_t xtime_round{ 0 };         // chain timestamp periodically updating round number (or version number)
    int64_t               local_update_time{ 0 };    // local receive time msg time
};

using xchain_time_watcher = std::function<void(const xchain_time_st &)>;

/**
 * @brief Logic chain timer
 *
 */
class xchain_time_face_t : public base::xobject_t {
 protected:
    virtual ~xchain_time_face_t() {}
 public:

    /**
     * @brief Update logic chain timer. The last logic chain clock is from the input timer_block.
     *
     * @param timer_block Timer block, contains the last chain time.
     * @param force       Force update or not if the input value is different from the local value.
     * @return true       Update successful.
     * @return false      Update fails.
     */
    virtual bool update_time(data::xblock_t* timer_block, bool force = false) = 0;

    /**
     * @brief Watch the logic chain timer pluse by specifing the interval. If time pluse matches the interval, callback cb will be invoked.
     *
     * @param key       Watch key, used for unwatch. Must be unique, UB if duplicated.
     * @param interval  The interval for watching in logic time unit.
     * @param cb        Callback object invoked when timeouts.
     * @return true     Watch successful.
     * @return false    Watch fails.
     */
    virtual bool watch(const std::string & key, uint64_t interval, xchain_time_watcher cb) = 0;

    /**
     * @brief Watch the logic chain timer pluse once. If time pluse matches the interval, callback cb will be invoked.
     *
     * @param interval  The interval for watching in logic time unit.
     * @param cb        Callback object invoked when timeouts.
     * @return true     Watch successful.
     * @return false    Watch fails.
     */
    virtual bool watch_one(uint64_t interval, xchain_time_watcher cb) = 0;

    /**
     * @brief Unsubscribe the watch.
     *
     * @param key       The watch key.
     * @return true     Unsubscribe successful.
     * @return false    unsubscribe fails.
     */
    virtual bool unwatch(const std::string & key) = 0;

    /**
     * @brief Initialize the logic chain timer.
     *
     */
    virtual void init() = 0;

    /**
     * @brief Get the last logic time.
     *
     * @return common::xlogic_time_t    The last lgic time.
     */
    virtual common::xlogic_time_t logic_time() const noexcept = 0;

    /**
     * @brief Close
     *
     */
    virtual void close() = 0;

    /**
     * @brief Get the iothread object. For internal use only.
     *
     * @return base::xiothread_t* Points to the iothread object.
     */
    virtual base::xiothread_t* get_iothread() const noexcept = 0;
};

NS_END2
