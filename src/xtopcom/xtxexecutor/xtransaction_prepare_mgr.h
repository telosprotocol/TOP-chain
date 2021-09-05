#pragma once
#include <map>
#include "xbase/xbase.h"
#include "xbase/xns_macro.h"
#include "xbase/xobject.h"
#include "xbase/xthread.h"
#include "xbase/xtimer.h"
#include "xbasic/xmemory.hpp"
#include "xmbus/xmessage_bus.h"
#include "xdata/xblock.h"
#include "xbasic/xtimer_driver.h"
#include "xdata/xtransaction_cache.h"

NS_BEG2(top, txexecutor)

class xtransaction_prepare_mgr
//: public base::xxtimer_t {
: public std::enable_shared_from_this<xtransaction_prepare_mgr>
, public xbasic_runnable_t<xtransaction_prepare_mgr> {
public:
    xtransaction_prepare_mgr(observer_ptr<mbus::xmessage_bus_face_t> const & mbus,
        observer_ptr<xbase_timer_driver_t> const & timer_driver,
        observer_ptr<data::xtransaction_cache_t> const & transaction_cache) :
        m_mbus(mbus), m_timer_driver(timer_driver),
        m_transaction_cache(transaction_cache) {}         
/*    xtransaction_prepare_mgr(observer_ptr<mbus::xmessage_bus_face_t> const & mbus,
        base::xcontext_t & _context,
        int32_t timer_thread_id,
        observer_ptr<data::xtransaction_cache_t> const & transaction_cache)
        :base::xxtimer_t(_context,timer_thread_id), m_mbus(mbus),
        //m_mbus(mbus), m_timer_driver{std::make_shared<xbase_timer_driver_t>(std::make_shared<xbase_io_context_wrapper_t>())}
 */
    void start() override;
    void stop() override;
//    bool running();
    std::string tx_exec_status_to_str(uint8_t exec_status);
    void set_timer_driver(const observer_ptr<xbase_timer_driver_t> timer_driver);
private:
    xtransaction_prepare_mgr();
    xtransaction_prepare_mgr(const xtransaction_prepare_mgr &);
    xtransaction_prepare_mgr & operator = (const xtransaction_prepare_mgr &);
    void on_block_to_db_event(mbus::xevent_ptr_t e);
    int update_prepare_cache(const data::xblock_ptr_t block_ptr);
    void do_check_tx();
//protected:
//    virtual bool        on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) override;
private:
    observer_ptr<mbus::xmessage_bus_face_t> m_mbus;
    observer_ptr<xbase_timer_driver_t> m_timer_driver;
    observer_ptr<data::xtransaction_cache_t> m_transaction_cache;
    uint32_t m_listener;
//    bool m_running{false};
};

NS_END2