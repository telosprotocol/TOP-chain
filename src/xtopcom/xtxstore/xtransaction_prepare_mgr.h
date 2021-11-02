#pragma once
#include "xbase/xbase.h"
#include "xbase/xns_macro.h"
#include "xbase/xobject.h"
#include "xbase/xthread.h"
#include "xbase/xtimer.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xtimer_driver.h"
#include "xdata/xblock.h"
#include "xdata/xtransaction_cache.h"
#include "xmbus/xmessage_bus.h"

#include <map>

NS_BEG2(top, txexecutor)

class xtransaction_prepare_mgr
  : public std::enable_shared_from_this<xtransaction_prepare_mgr>
  , public xbasic_runnable_t<xtransaction_prepare_mgr> {
public:
    xtransaction_prepare_mgr(observer_ptr<mbus::xmessage_bus_face_t> const & mbus, observer_ptr<xbase_timer_driver_t> const & timer_driver);
    void start() override;
    void stop() override;
    std::string tx_exec_status_to_str(uint8_t exec_status);
    std::shared_ptr<data::xtransaction_cache_t> transaction_cache();

private:
    xtransaction_prepare_mgr();
    xtransaction_prepare_mgr(const xtransaction_prepare_mgr &);
    xtransaction_prepare_mgr & operator=(const xtransaction_prepare_mgr &);
    void on_block_to_db_event(mbus::xevent_ptr_t e);
    int update_prepare_cache(const data::xblock_ptr_t block_ptr);
    void do_check_tx();

private:
    observer_ptr<mbus::xmessage_bus_face_t> m_mbus;
    observer_ptr<xbase_timer_driver_t> m_timer_driver;
    std::shared_ptr<data::xtransaction_cache_t> m_transaction_cache;
    uint32_t m_listener;
};

NS_END2