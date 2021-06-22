// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xthread.h"
#include "xbasic/xrunnable.h"
#include "xcommon/xlogic_time.h"

#include <functional>
#include <string>

NS_BEG2(top, time)

using xchain_time_watcher = std::function<void(common::xlogic_time_t)>;

enum class xenum_logic_timer_update_strategy { discard_old_value, force };
using xlogic_timer_update_strategy_t = xenum_logic_timer_update_strategy;

/**
 * @brief Logic chain timer
 *
 */
class xchain_time_face_t : public base::xobject_t, public xbasic_runnable_t<xchain_time_face_t> {
protected:
    ~xchain_time_face_t() override = default;
public:

    xchain_time_face_t() = default;
    xchain_time_face_t(xchain_time_face_t const &) = delete;
    xchain_time_face_t & operator=(xchain_time_face_t const &) = delete;
    xchain_time_face_t(xchain_time_face_t &&) = default;
    xchain_time_face_t & operator=(xchain_time_face_t &&) = default;

    /// @brief Update logic chain timer.
    /// @param time Value used to update.
    /// @param update_strategry Update strategry.
    virtual void update_time(common::xlogic_time_t time, xlogic_timer_update_strategy_t update_strategry) = 0;

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
    // virtual bool watch_one(uint64_t interval, xchain_time_watcher cb) = 0;

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
