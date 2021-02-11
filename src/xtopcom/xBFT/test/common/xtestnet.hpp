// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <random>
#include <queue>
#include "xbase/xobject.h"
#include "xbase/xtimer.h"
#include "xBFT/xconsobj.h"

namespace top
{
    namespace test
    {
        enum enum_net_rtt_type
        {
            enum_net_rtt_type_mask              = 0x00F0, //check whether has rtt setting
            enum_net_rtt_type_zero              = 0x0010, //immediately reach
            enum_net_rtt_type_random            = 0x0020, //[0- 10000ms)
            enum_net_rtt_type_small             = 0x0030, //(10-80ms), most national-wide network
            enum_net_rtt_type_medium            = 0x0040, //(80-200ms),most internation-wide network
            enum_net_rtt_type_high              = 0x0050, //(200-500ms),bad internation network for some countries
            enum_net_rtt_type_extreamhigh       = 0x0060, //(500-2000ms),not connectable network
            enum_net_rtt_type_unreachable       = 0x0070, //(2000-10000ms),not reachable
        };
        enum enum_net_lossrate_type
        {
            enum_net_lossrate_type_mask         = 0x0F00, //check whether has loss rate setting
            enum_net_lossrate_type_zero         = 0x0100, //nothing lost
            enum_net_lossrate_type_random       = 0x0200, //random packets drops(0-100%)
            enum_net_lossrate_type_small        = 0x0300, //(1-3%), normal network
            enum_net_lossrate_type_medium       = 0x0400, //(3-8%), bad network
            enum_net_lossrate_type_high         = 0x0500, //(8-15%),difficult to connect and rare load webpage
            enum_net_lossrate_type_extreamhigh  = 0x0600, //(15-30%) lossrate is very high at real world
            enum_net_lossrate_type_unreachable  = 0x0700, //(30-50%)
        };
        enum enum_net_outoforder_type
        {
            enum_net_outoforder_type_mark       = 0xF000,
            enum_net_outoforder_type_zero       = 0x1000,
            enum_net_outoforder_type_small      = 0x2000,
            enum_net_outoforder_type_medium     = 0x3000,
            enum_net_outoforder_type_high       = 0x4000,
            enum_net_outoforder_type_extreamhigh= 0x5000,
            enum_net_outoforder_type_unreachable= 0x6000,
        };
        
        class xnetsimulator_t : public xconsensus::xcsobject_t
        {
        protected:
            xnetsimulator_t(base::xcontext_t & _context,const int32_t target_thread_id);
            virtual ~xnetsimulator_t();
        private:
            xnetsimulator_t();
            xnetsimulator_t(const xnetsimulator_t &);
            xnetsimulator_t & operator = (const xnetsimulator_t &);
        };
        
        class xloss_simulator: public xnetsimulator_t
        {
        public:
            xloss_simulator(base::xcontext_t & _context,const int32_t target_thread_id,enum_net_lossrate_type _type);
        protected:
            virtual ~xloss_simulator();
        private:
            xloss_simulator();
            xloss_simulator(const xloss_simulator &);
            xloss_simulator & operator = (const xloss_simulator &);
        public:
            enum_net_lossrate_type  get_lossrate_type() const {return m_lossrate_type;}
        protected:
            //note: to return false may call child'push_event_down,or stop further routing when return true
            virtual bool            on_event_down(const base::xvevent_t & event,base::xionode_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
        private:
            int32_t       regen_lossrate();
        private:
            enum_net_lossrate_type          m_lossrate_type;
            int32_t                         m_lossrate_value;
            int32_t                         m_change_frequence;
        private:
            std::default_random_engine              m_random_engine;
            std::uniform_int_distribution<int32_t>  m_gen_random_lossrate;
            std::uniform_int_distribution<int32_t>  m_gen_random_seed;
        };
        
        class xrtt_simulator : public xnetsimulator_t,public base::xtimersink_t
        {
        public:
            xrtt_simulator(base::xcontext_t & _context,const int32_t target_thread_id,enum_net_rtt_type _type);
        protected:
            virtual ~xrtt_simulator();
        private:
            xrtt_simulator();
            xrtt_simulator(const xrtt_simulator &);
            xrtt_simulator & operator = (const xrtt_simulator &);
            
        public:
            enum_net_rtt_type   get_rtt_type() const {return m_rtt_type;}
            
        protected:
            //note: to return false may call parent'push_event_up,or stop further routing when return true
            virtual bool            on_event_up(const base::xvevent_t & event,base::xionode_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            //note: to return false may call child'push_event_down,or stop further routing when return true
            virtual bool            on_event_down(const base::xvevent_t & event,base::xionode_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
            virtual bool    on_timer_start(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms) override;   //attached into io-thread
            virtual bool    on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms) override;   //detach means it detach
            virtual bool    on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) override;
        private:
            int32_t             regen_rtt();
        private:
            enum_net_rtt_type             m_rtt_type;
            int32_t                       m_rtt_value;
            int32_t                       m_change_frequence;
            std::default_random_engine              m_random_engine;
            std::uniform_int_distribution<uint32_t> m_gen_random_rtt;
            std::uniform_int_distribution<int32_t>  m_gen_random_seed;
        private:
            std::deque<base::xvevent_t*>  m_event_in_queue;
            std::deque<base::xvevent_t*>  m_event_out_queue;
            base::xtimer_t*               m_raw_timer;
        };
        
        class xoutoforder_simulator : public xnetsimulator_t,public base::xtimersink_t
        {
        public:
            xoutoforder_simulator(base::xcontext_t & _context,const int32_t target_thread_id,enum_net_outoforder_type _type);
        protected:
            virtual ~xoutoforder_simulator();
        private:
            xoutoforder_simulator();
            xoutoforder_simulator(const xoutoforder_simulator &);
            xoutoforder_simulator & operator = (const xoutoforder_simulator &);
            
        public:
            enum_net_outoforder_type      get_outoforder_type() const {return m_outoforder_type;}
            
        protected:
            //note: to return false may call parent'push_event_up,or stop further routing when return true
            virtual bool            on_event_up(const base::xvevent_t & event,base::xionode_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            //note: to return false may call child'push_event_down,or stop further routing when return true
            virtual bool            on_event_down(const base::xvevent_t & event,base::xionode_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms) override;
            
            virtual bool    on_timer_start(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms) override;   //attached into io-thread
            virtual bool    on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms) override;   //detach means it detach
            virtual bool    on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) override;
        private:
            int32_t     regen_rate();
        private:
            enum_net_outoforder_type      m_outoforder_type;
            int32_t                       m_outoforder_value;
            int32_t                       m_change_frequence;
            
            std::default_random_engine              m_random_engine;
            std::uniform_int_distribution<uint32_t> m_gen_random_rate;
            std::uniform_int_distribution<int32_t>  m_gen_random_seed;
        private:
            std::deque<base::xvevent_t*>  m_event_out_queue;
        };
    };
};
