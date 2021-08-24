// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
 
#include <string>
#include "xvaccount.h"
#include "xvcanvas.h"

namespace top
{
    namespace base
    {
        class xvexestate_t;//just decare in advance
        class xvexecontxt_t : public xobject_t
        {
            friend class xvexecute_t;
        public:
            xvexecontxt_t(const uint64_t new_max_tags,const uint64_t new_used_tags,xvcanvas_t * input_canvas,xvcanvas_t * output_canvas);
        protected:
            virtual ~xvexecontxt_t();
        private:
            xvexecontxt_t();
            xvexecontxt_t(xvexecontxt_t &&);
            xvexecontxt_t(const xvexecontxt_t &);
            xvexecontxt_t & operator = (xvexecontxt_t &&);
            xvexecontxt_t & operator = (const xvexecontxt_t &);
            
        public: //state related api is independly for variouse mode(block-based state or account-stated state)
            //especially Contract rely this interface to load execute state
            virtual xauto_ptr<xvexestate_t> get_state(const xvaccount_t & account) = 0;
            virtual xauto_ptr<xvexestate_t> get_state(const std::string & account_addr) = 0;
            
            virtual bool                    snapshot() = 0; //clone first then do actual contract execution
            virtual bool                    restore() = 0; //restore to last state if have snapshot
        public:
            xvcanvas_t*     get_input_canvas()  const {return m_input_canvas;}
            xvcanvas_t*     get_output_canvas() const {return m_output_canvas;}
 
            bool                       withdraw_tgas(const uint64_t tgas); //return false if not have enough tags left
            inline const uint64_t      get_used_tgas() const {return m_used_tgas;}
            inline const uint64_t      get_max_tgas()  const {return m_max_tgas;}
 
        protected:
            void            reset_input_canvas(xvcanvas_t * new_ptr);
            void            reset_output_canvas(xvcanvas_t * new_ptr);
            
            void            reset_prev_context(xvexecontxt_t * new_context);
            inline xvexecontxt_t*  get_prev_context() const {return m_prev_context;}
        private:
            xvcanvas_t *    m_input_canvas;   //recording actions if present
            xvcanvas_t *    m_output_canvas;  //recording propety'bin log
            xvexecontxt_t*  m_prev_context;   //snapshot for last full state
        private:
            uint64_t        m_max_tgas;       //max tgas allow used for this method
            uint64_t        m_used_tgas;      //return how many tgas(virtual cost) used for caller
        };
    
        class xvblock_t;
        class xvheader_t;
        xauto_ptr<xvexecontxt_t> create_block_context(const xvblock_t & target_block,const uint64_t new_max_tags);
        xauto_ptr<xvexecontxt_t> create_block_context(const xvheader_t & target_header,const uint64_t new_max_tags);
    
    }//end of namespace of base

}//end of namespace top
