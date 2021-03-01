// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <vector>
#include <random>
#include "xunit_service/xworkpool_dispatcher.h"
#include "xchain_timer/xchain_timer.h"
#include "xunit_service/xtimer_dispatcher.h"
#include "xbase/xtimer.h"
#include "xbase/xcontext.h"
#include "xtestclock.hpp"

namespace top {
namespace mock {
using namespace xunit_service;
using xunit_service::xworkpool_dispatcher;
using base::xxtimer_t;
using xunit_service::xcons_service_para_face;

class xtimer_work_pool_dispatcher : public xworkpool_dispatcher {
 public:
    xtimer_work_pool_dispatcher(std::shared_ptr<xcons_service_para_face> const& p_para,
                            std::shared_ptr<xblock_maker_face> const& block_maker,
                            const std::string & account_prefix)
    :xworkpool_dispatcher(nullptr, p_para, block_maker) { }

 public:
    virtual bool on_timer_fire(base::xvblock_t* clock_block) {
        auto pool = m_para->get_resources()->get_workpool();
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto packer_iter : m_packers) {
            auto packer = packer_iter.second;

            auto async_reset = [this](base::xcall_t &call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
                auto packer = dynamic_cast<xbatch_packer*>(call.get_param1().get_object());
                auto clock = dynamic_cast<base::xvblock_t*>(call.get_param2().get_object());
                packer->fire_clock(*clock, 0, 0);
                return true;
            };
            base::xcall_t asyn_call(async_reset, packer.get(), clock_block);
            packer->send_call(asyn_call);
        }
        return true;
    }

    virtual xcons_service_para_face * get_para() {
        return m_para.get();
    }
};
using dispatcher_ptr_t = std::shared_ptr<xcons_dispatcher>;

class xdispatcher_builder_mock : public xunit_service::xcons_dispatcher_builder_face {
public:
    xunit_service::xcons_dispatcher_ptr build(observer_ptr<mbus::xmessage_bus_face_t> const &mb, xcons_service_para_ptr const &, e_cons_type cons_type) override {
        return nullptr;
        // if (cons_type == xunit_service::e_table) {
        //     auto block_maker = p_srv_para->get_consensus_para()->get_block_maker(cons_type);
        //     // TODO(justin): use real table address
        //     std::string table_publick_addr = "table1234567890abcdef";
        //     // auto        ptr = new xtimer_work_pool_dispatcher(p_srv_para, block_maker, table_publick_addr);
        //     auto ptr = std::make_shared<xtimer_work_pool_dispatcher>(p_srv_para, block_maker, table_publick_addr);
        //     m_dispatchers.push_back(ptr);
        //     return ptr;
        // } else {
        //     auto block_maker = p_srv_para->get_consensus_para()->get_block_maker(cons_type);
        //     if (block_maker == nullptr) {
        //         return nullptr;
        //     }
        //     return std::make_shared<xunit_service::xtimer_dispatcher_t>(p_srv_para, block_maker, 10);
        // }
    }
public:
    std::vector<xunit_service::xcons_dispatcher_ptr> m_dispatchers;
};

class xlocal_time : public xxtimer_t {
 public:
    explicit xlocal_time(const std::vector<xcons_dispatcher_builder_ptr> & builders,
    base::xcontext_t &_context, int32_t timer_thread_id) : xxtimer_t(_context, timer_thread_id) {
       m_builders.insert(m_builders.begin(), builders.begin(), builders.end());
        start(0, 10000);
        m_clocker = new xtestclocker_t();
    }
    virtual ~xlocal_time() {}

 protected:
    inline virtual bool discard() {return false;}
    virtual bool on_timer_fire(const int32_t thread_id, const int64_t timer_id,
                        const int64_t current_time_ms, const int32_t start_timeout_ms,
                        int32_t & in_out_cur_interval_ms) {
        m_curtime++;
        auto block = m_clocker->on_clock_fire();
        for (auto builder : m_builders) {
            auto build_mock = dynamic_cast<xdispatcher_builder_mock*>(builder.get());
            auto dispatchers = build_mock->m_dispatchers;
            for (auto dispatcher : dispatchers) {
            if (!discard()) {
                auto time_dispatcher = dynamic_cast<xtimer_work_pool_dispatcher*>(dispatcher.get());
                if (time_dispatcher != nullptr) {
                    time_dispatcher->get_para()->get_resources()->get_vblockstore()->store_block(block);
                    time_dispatcher->on_timer_fire(block);
                }
            }
        }
        }
        m_last_block.attach(block);
        return true;
    }

    size_t get_total_nodes() {
        return m_dispachers.size();
    }

    // base::xvblock_t* create_block(uint64_t curtime) {
    //     const std::string global_clock_account("T-clock-xxx");
    //     std::vector<base::xvblock_t*> parents_blocks;
    //     std::string empty_txs;
    //     if (m_last_block == nullptr) {
    //         m_last_block = xemptyblock_t::create_genesis_emptyunit(1, global_clock_account);
    //     }
    //     auto clock_block = xemptyblock_t::create_next_emptyblock(m_last_block->get_viewid() + 1, m_last_block->get_clock() + 1, m_last_block);
    //     return clock_block;
    // }

 private:
    std::vector<xcons_dispatcher_builder_ptr> m_builders;
    std::vector<dispatcher_ptr_t> m_dispachers;
    observer_ptr<time::xchain_timer_t> m_timer;
    volatile uint64_t m_curtime {0};
    xobject_ptr_t<base::xvblock_t> m_last_block;
    xtestclocker_t * m_clocker;
};

using std::default_random_engine;
class xrandom_discard_timer_dispatcher : public xtimer_work_pool_dispatcher {
public:
    xrandom_discard_timer_dispatcher(std::shared_ptr<xcons_service_para_face> const& p_para,
                            std::shared_ptr<xblock_maker_face> const& block_maker,
                            const std::string & account_prefix, uint rate) :
    xtimer_work_pool_dispatcher(p_para, block_maker, account_prefix), m_rate(rate) {}
protected:
    inline virtual bool discard() {
        auto random = m_random_engine();
        return (random % 100) <= m_rate;
    }
private:
    uint m_rate;
    default_random_engine m_random_engine;
};
}  // namespace mock
}  // namespace top
