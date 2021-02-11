// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtestnet.hpp"
#include "xbase/xthread.h"

namespace top
{
    namespace test
    {
        xnetsimulator_t::xnetsimulator_t(base::xcontext_t & _context,const int32_t target_thread_id)
            :xconsensus::xcsobject_t(_context,target_thread_id,base::enum_xobject_app_type_undefine)
        {
        }
        
        xnetsimulator_t::~xnetsimulator_t()
        {
        }
        
        xloss_simulator::xloss_simulator(base::xcontext_t & _context,const int32_t target_thread_id,enum_net_lossrate_type _type)
            :xnetsimulator_t(_context,target_thread_id),
        m_gen_random_seed(0,100),
        m_gen_random_lossrate(0,100)
        {
            m_lossrate_type = _type;
            m_change_frequence = m_gen_random_seed(m_random_engine);
            regen_lossrate();
        }
        
        xloss_simulator::~xloss_simulator()
        {
        }
        
        int32_t xloss_simulator::regen_lossrate()
        {
            m_lossrate_value      = m_gen_random_lossrate(m_random_engine); //regenerate lossrate
            if(get_lossrate_type() == enum_net_lossrate_type_small)
            {
                m_lossrate_value = (m_lossrate_value % 3) + 1; //1-3%
            }
            else if(get_lossrate_type() == enum_net_lossrate_type_medium)
            {
                m_lossrate_value = (m_lossrate_value % 5) + 3; //3-8%
            }
            else if(get_lossrate_type() == enum_net_lossrate_type_high)
            {
                m_lossrate_value = (m_lossrate_value % 7) + 8; //8-15%
            }
            else if(get_lossrate_type() == enum_net_lossrate_type_extreamhigh)
            {
                m_lossrate_value = (m_lossrate_value % 20) + 15; //15-30%)
            }
            else if(get_lossrate_type() == enum_net_lossrate_type_unreachable)
            {
                m_lossrate_value = (m_lossrate_value % 20) + 30; //30-50%
            }
            else if(get_lossrate_type() == enum_net_lossrate_type_zero) //nothing loss
            {
                m_lossrate_value = 0;
            }
            return m_lossrate_value;
        }
 
        //note: to return false may call child'push_event_down,or stop further routing when return true
        bool  xloss_simulator::on_event_down(const base::xvevent_t & event,base::xionode_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            if(event.get_type() == base::enum_xevent_core_type_pdu)
            {
                --m_change_frequence;
                if(m_change_frequence <= 0)
                {
                    m_change_frequence = m_gen_random_seed(m_random_engine) + 36;//(36,135)
                    regen_lossrate();
                }
                const int32_t  random_value  = m_gen_random_seed(m_random_engine);
                if(random_value <= m_lossrate_value) //match condistion
                {
                    xdbg("xloss_simulator::on_event_down,network loss packet,from=%llu to %llu at node:%llu as lossrate=%d",event.get_from_xip().low_addr,event.get_to_xip().low_addr,get_xip2_low_addr(),m_lossrate_value);
                    return true; //drop packet
                }
            }
            return false;
        }
        
        xrtt_simulator::xrtt_simulator(base::xcontext_t & _context,const int32_t target_thread_id,enum_net_rtt_type _type)
            :xnetsimulator_t(_context,target_thread_id),
        m_gen_random_seed(0,100)
        {
            m_rtt_type = _type;
            m_rtt_value        = 0;
            m_change_frequence = 0;
 
            m_raw_timer = NULL;
            m_raw_timer = get_thread()->create_timer((xtimersink_t*)this);
            m_raw_timer->start(1000, 50); //every 50ms
        }
        
        xrtt_simulator::~xrtt_simulator()
        {
            for(auto it : m_event_out_queue)
            {
                if(it != nullptr)
                    it->release_ref();
            }
            m_event_out_queue.clear();
            for(auto it : m_event_in_queue)
            {
                if(it != nullptr)
                    it->release_ref();
            }
            m_event_in_queue.clear();
 
            m_raw_timer->close();
            m_raw_timer->release_ref();
        }
        
        int32_t xrtt_simulator::regen_rtt()
        {
            m_rtt_value  = m_gen_random_rtt(m_random_engine); //regenerate lossrate
            if(get_rtt_type() == enum_net_rtt_type_small)
            {
                m_rtt_value = (m_rtt_value % 70) + 10; //(10-80ms)
            }
            else if(get_rtt_type() == enum_net_rtt_type_medium)
            {
                m_rtt_value = (m_rtt_value % 120) + 80; //(80-200ms)
            }
            else if(get_rtt_type() == enum_net_rtt_type_high)
            {
                m_rtt_value = (m_rtt_value % 300) + 200; //(200-500ms)
            }
            else if(get_rtt_type() == enum_net_rtt_type_extreamhigh)
            {
                m_rtt_value = (m_rtt_value % 1500) + 500; //(500-2000ms)
            }
            else if(get_rtt_type() == enum_net_rtt_type_unreachable)
            {
                m_rtt_value = (m_rtt_value % 8000) + 2000; //2000-10000ms
            }
            else if(get_rtt_type() == enum_net_rtt_type_random)
            {
                m_rtt_value = m_rtt_value % 1000;
            }
            else if(get_rtt_type() == enum_net_rtt_type_zero)
            {
                m_rtt_value = 0;
            }
            return m_rtt_value;
        }
        
        //note: to return false may call parent'push_event_up,or stop further routing when return true
        bool   xrtt_simulator::on_event_up(const base::xvevent_t & event,base::xionode_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            if(event.get_type() == base::enum_xevent_core_type_pdu)
            {
                --m_change_frequence;
                if(m_change_frequence <= 0)
                {
                    m_change_frequence = m_gen_random_seed(m_random_engine) + 36;//(36,135)
                    regen_rtt();
                }
                while(false == m_event_out_queue.empty())
                {
                    base::xvevent_t* _frontevent = m_event_out_queue.front();
                    if(timenow_ms > _frontevent->get_cookie())
                    {
                        m_event_out_queue.pop_front(); //pop first
                        
                        push_event_up(*_frontevent, this, cur_thread_id, timenow_ms);
                        _frontevent->release_ref();
                    }
                    else
                    {
                        break;
                    }
                }
                if(m_rtt_value > 0)
                {
                    base::xvevent_t * _event_obj = (base::xvevent_t*)&event;
                    _event_obj->add_ref();
                    _event_obj->set_cookie(timenow_ms + m_rtt_value/2);//set timeout
                    m_event_out_queue.push_back(_event_obj);
                    return true;
                }
            }
            return false;
        }
        
        //note: to return false may call child'push_event_down,or stop further routing when return true
        bool  xrtt_simulator::on_event_down(const base::xvevent_t & event,base::xionode_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            if(event.get_type() == base::enum_xevent_core_type_pdu)
            {
                --m_change_frequence;
                if(m_change_frequence <= 0)
                {
                    m_change_frequence = m_gen_random_seed(m_random_engine) + 36;//(36,135)
                    regen_rtt();
                }
                while(false == m_event_in_queue.empty())
                {
                    base::xvevent_t* _frontevent = m_event_in_queue.front();
                    if(timenow_ms > _frontevent->get_cookie())
                    {
                        m_event_in_queue.pop_front(); //pop first
                        
                        push_event_down(*_frontevent, this, cur_thread_id, timenow_ms);
                        _frontevent->release_ref();
                    }
                    else
                    {
                        break;
                    }
                }
                if(m_rtt_value > 0)
                {
                    base::xvevent_t * _event_obj = (base::xvevent_t*)&event;
                    _event_obj->add_ref();
                    _event_obj->set_cookie(timenow_ms + m_rtt_value/2);//set timeout
                    m_event_in_queue.push_back(_event_obj);
                    return true;
                }
            }
            return false;
        }
        
        bool    xrtt_simulator::on_timer_start(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms)   //attached into io-thread
        {
            return true;
        }
        bool    xrtt_simulator::on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms)
        {
            return true;
        }
        
        bool   xrtt_simulator::on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms)
        {
            //leverage on_pdu_event_down to find out which one packet should deliver now
            while(false == m_event_out_queue.empty())
            {
                base::xvevent_t* _frontevent = m_event_out_queue.front();
                if(current_time_ms > _frontevent->get_cookie() )
                {
                    m_event_out_queue.pop_front(); //pop first
                    
                    push_event_up(*_frontevent, this, thread_id, current_time_ms);
                    _frontevent->release_ref();
                }
                else
                {
                    break;
                }
            }
            while(false == m_event_in_queue.empty())
            {
                base::xvevent_t* _frontevent = m_event_in_queue.front();
                if(current_time_ms > _frontevent->get_cookie())
                {
                    m_event_in_queue.pop_front(); //pop first
                    
                    push_event_down(*_frontevent, this, thread_id, current_time_ms);
                    _frontevent->release_ref();
                }
                else
                {
                    break;
                }
            }
            return true;
        }
        
        xoutoforder_simulator::xoutoforder_simulator(base::xcontext_t & _context,const int32_t target_thread_id,enum_net_outoforder_type _type)
            :xnetsimulator_t(_context,target_thread_id),
             m_gen_random_seed(0,100)
        {
            m_outoforder_type  = _type;
            m_outoforder_value = 0;
            m_change_frequence = 0;
        }
        
        xoutoforder_simulator::~xoutoforder_simulator()
        {
            for(auto it : m_event_out_queue)
            {
                if(it != nullptr)
                    it->release_ref();
            }
            m_event_out_queue.clear();
        }
        
        int32_t xoutoforder_simulator::regen_rate()
        {
            m_outoforder_value      = m_gen_random_rate(m_random_engine); //regenerate lossrate
            if(get_outoforder_type() == enum_net_outoforder_type_small)
            {
                m_outoforder_value = (m_outoforder_value % 3) + 1; //1-3%
            }
            else if(get_outoforder_type() == enum_net_outoforder_type_medium)
            {
                m_outoforder_value = (m_outoforder_value % 5) + 3; //3-8%
            }
            else if(get_outoforder_type() == enum_net_outoforder_type_high)
            {
                m_outoforder_value = (m_outoforder_value % 7) + 8; //8-15%
            }
            else if(get_outoforder_type() == enum_net_outoforder_type_extreamhigh)
            {
                m_outoforder_value = (m_outoforder_value % 20) + 15; //15-30%)
            }
            else if(get_outoforder_type() == enum_net_outoforder_type_unreachable)
            {
                m_outoforder_value = (m_outoforder_value % 20) + 30; //30-50% packet go unorder
            }
            else if(get_outoforder_type() == enum_net_outoforder_type_zero) //nothing disorder
            {
                m_outoforder_value = 0;
            }
            return m_outoforder_value;
        }
        
        //note: to return false may call parent'push_event_up,or stop further routing when return true
        bool  xoutoforder_simulator::on_event_up(const base::xvevent_t & event,base::xionode_t* from_child,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            if(event.get_type() == base::enum_xevent_core_type_pdu)
            {
                --m_change_frequence;
                if(m_change_frequence <= 0)
                {
                    m_change_frequence = m_gen_random_seed(m_random_engine) + 36;//(36,135)
                    regen_rate();
                }
                if(false == m_event_out_queue.empty())
                {
                    std::function<void(void*)> _reset_send_job = [this](void* )->void{
                        while(false == m_event_out_queue.empty())
                        {
                            base::xvevent_t* _frontevent = m_event_out_queue.front();
                            m_event_out_queue.pop_front(); //pop first
                            
                            push_event_up(*_frontevent, this, 0, 0);
                            _frontevent->release_ref();
                        }
                    };
                    send_call(_reset_send_job,0);
                }
                
                bool is_stop_event_up = false;
                const int32_t  random_value  = m_gen_random_seed(m_random_engine);
                if(random_value <= m_outoforder_value) //match condistion
                {
                    base::xvevent_t * _event_obj = (base::xvevent_t*)&event;
                    _event_obj->add_ref();
                    m_event_out_queue.push_front(_event_obj);
                    
                    xdbg("xoutoforder_simulator::on_event_up,network cache packet,from=%llu to %llu at node:%llu as unorder_rate=%d",event.get_from_xip().low_addr,event.get_to_xip().low_addr,get_xip2_low_addr(),m_outoforder_value);
                    
                    is_stop_event_up = true;
                }
                return is_stop_event_up;
            }
            return false;
        }
        //note: to return false may call child'push_event_down,or stop further routing when return true
        bool  xoutoforder_simulator::on_event_down(const base::xvevent_t & event,base::xionode_t* from_parent,const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            while(m_event_out_queue.size() > 1)
            {
                base::xvevent_t* _frontevent = m_event_out_queue.front();
                m_event_out_queue.pop_front(); //pop first
                
                push_event_up(*_frontevent, this, cur_thread_id, timenow_ms);
                _frontevent->release_ref();
            }
            return false;
        }
         
        bool    xoutoforder_simulator::on_timer_start(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms)   //attached into io-thread
        {
            return true;
        }
        bool    xoutoforder_simulator::on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms)
        {
            return true;
        }
        
        bool   xoutoforder_simulator::on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms)
        {
            return true;
        }
    };
};
