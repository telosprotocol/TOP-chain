// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#if 0
#pragma once

#include <string>
#include "xbase/xthread.h"
#include "xbase/xns_macro.h"
#include "xbasic/xobject_ptr.h"
#include "xmetrics/xmetrics.h"


NS_BEG2(top, metrics)

#ifdef ENABLE_METRICS

class xmetrics_default_appender : public xmetrics_appender {
public:
   virtual void dump(const std::vector<std::string>& tag_values);
};

class xcounter_timer : public base::xxtimer_t {
public:
   explicit xcounter_timer(base::xcontext_t & _context, //NOLINT
                            int32_t timer_thread_id,
                            xmetrics_appender * appender);

protected:
   virtual ~xcounter_timer() {}

protected:
   bool on_timer_fire(const int32_t thread_id,
                                const int64_t timer_id,
                                const int64_t current_time_ms,
                                const int32_t start_timeout_ms,
                                int32_t & in_out_cur_interval_ms) override;
private:
   xmetrics_appender * m_default_appender;
   xmetrics_appender * m_ext_appender;
};

class simple_counter {
public:
   simple_counter(uint64_t milliseconds = 180000) : m_interval(milliseconds) {}
   ~simple_counter() {
      if (m_timer != nullptr) {
         m_timer->stop();
      }
      if (m_timer_thread != nullptr) {
         m_timer_thread->close();
         m_timer_thread->release_ref();
      }
   };

public:
   void init(xmetrics_appender * appeder = nullptr);
   virtual void incr(xmetrics metrics, int32_t val);

   void update_metrics();
   void set_interval(uint64_t interval) {m_interval = interval;}
   std::vector<std::string> values();
private:
   top::base::xiothread_t* m_timer_thread{nullptr};
   xobject_ptr_t<xcounter_timer> m_timer{nullptr};
   xmetrics m_metrics{};
   uint64_t m_interval{};
   long m_delta_metrics[xmetrics::e_max_tags+1]{};
   long m_current_metrics[xmetrics::e_max_tags+1]{};
};

#endif

NS_END2

#endif