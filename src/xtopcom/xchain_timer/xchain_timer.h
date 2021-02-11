// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.#pragma once

#pragma once

#include "xbase/xthread.h"
#include "xchain_timer/xchain_timer_face.h"

#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>

NS_BEG2(top, time)

class xchain_timer_t final : public xchain_time_face_t {
protected:
    virtual ~xchain_timer_t() {}
public:
    virtual void                init() override;
    virtual bool                update_time(data::xblock_t* timer_block, bool force = false) override;
    virtual uint64_t            logic_time() const noexcept override;

    // note: interval is 10s/round, not second!!
    virtual bool                watch(const std::string & key, uint64_t interval, xchain_time_watcher cb) override;
    virtual bool                unwatch(const std::string & key) override;
    virtual bool                watch_one(uint64_t interval, xchain_time_watcher cb) override;
    virtual void                close() override;
    virtual base::xiothread_t * get_iothread() const noexcept override;

protected:
    void process(data::xblock_t* timer_block, int64_t recv_ms);

private:
    struct time_watcher_item {
        uint64_t            interval;
        xchain_time_watcher watcher;
    };
    std::mutex                                 m_mutex;
    std::mutex                                 m_one_timer_mutex;
    std::map<std::string, time_watcher_item>   m_watch_map;
    std::map<std::uint64_t, time_watcher_item> m_watch_one_map;  // only run one time
    base::xiothread_t *                        m_timer_thread{nullptr};
    std::atomic<uint64_t>                      m_latest;
};

NS_END2
