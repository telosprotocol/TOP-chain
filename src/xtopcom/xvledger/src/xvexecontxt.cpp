// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//#include <cinttypes>
#include "../xvstate.h"
#include "../xvexecontxt.h"
#include "../xvledger.h"
#include "xmetrics/xmetrics.h"
namespace top
{
    namespace base
    {
        xvexecontxt_t::xvexecontxt_t(const uint64_t new_max_tags,const uint64_t new_used_tags,xvcanvas_t * input_canvas,xvcanvas_t * output_canvas)
            :xobject_t(enum_xobject_type_exe_contxt)
        {
            m_input_canvas = NULL;
            m_output_canvas= NULL;
            
            m_used_tgas = new_used_tags;
            m_max_tgas = new_max_tags;

            if(input_canvas != NULL)
                reset_input_canvas(input_canvas);
            if(output_canvas != NULL)
                reset_output_canvas(output_canvas);
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvexecontxt, 1);
        }
    
        xvexecontxt_t::~xvexecontxt_t()
        {
            reset_input_canvas(NULL);
            reset_output_canvas(NULL);
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvexecontxt, -1);
        }
    
        void   xvexecontxt_t::reset_input_canvas(xvcanvas_t * new_ptr)
        {
            if(new_ptr != NULL)
                new_ptr->add_ref();
            
            xvcanvas_t * old_tpr = xatomic_t::xexchange(m_input_canvas, new_ptr);
            if(old_tpr != NULL)
                old_tpr->release_ref();
        }
    
        void   xvexecontxt_t::reset_output_canvas(xvcanvas_t * new_ptr)
        {
            if(new_ptr != NULL)
                new_ptr->add_ref();
            
            xvcanvas_t * old_tpr = xatomic_t::xexchange(m_output_canvas, new_ptr);
            if(old_tpr != NULL)
                old_tpr->release_ref();
        }
        
        bool   xvexecontxt_t::withdraw_tgas(const uint64_t tgas) //return false if not have enough tags left
        {
            if( (m_used_tgas + tgas) <= m_max_tgas)
            {
                xatomic_t::xadd(m_used_tgas,tgas);
                return true;
            }
            return false;
        }

        class xvblkcontext_t : public xvexecontxt_t
        {
        public:
            xvblkcontext_t(const xvheader_t & target_header,const uint64_t new_max_tags);
            xvblkcontext_t(const xvblock_t & target_block,const uint64_t new_max_tags);
            xvblkcontext_t(const xvblkcontext_t & obj);
        protected:
            ~xvblkcontext_t();
        public:
            virtual xauto_ptr<xvexestate_t> get_state(const xvaccount_t & account) override;
            virtual xauto_ptr<xvexestate_t> get_state(const std::string & account_addr) override;
            
            virtual bool                    snapshot() override; //clone first then do actual contract execution
            virtual bool                    restore() override; //restore to last state if have snapshot
        private:
            xvbstate_t*     m_block_state; //block-based state object
        };
    
        xvblkcontext_t::xvblkcontext_t(const xvheader_t & target_header,const uint64_t new_max_tags)
            :xvexecontxt_t(new_max_tags,0,NULL,NULL)
        {
            m_block_state = NULL;
            
            xvaccount_t target_account(target_header.get_account());
            // xauto_ptr<xvbstate_t> target_state(xvchain_t::instance().get_xstatestore()->get_block_state(target_account,target_header.get_height() - 1, target_header.get_last_block_hash(), metrics::statestore_access_from_blk_ctx));
            xauto_ptr<xvbstate_t> target_state(nullptr);
            if(target_state)
            {
                m_block_state = target_state.get();
                target_state->add_ref();
            }
            else
            {
                close(); //force to close and mark it is unavaiable
            }
        }
    
        xvblkcontext_t::xvblkcontext_t(const xvblock_t & target_block,const uint64_t new_max_tags)
            :xvexecontxt_t(new_max_tags,0,NULL,NULL)
        {
            m_block_state = NULL;
            
            // xauto_ptr<xvbstate_t> target_state(xvchain_t::instance().get_xstatestore()->get_block_state((xvblock_t*)&target_block, metrics::statestore_access_from_blk_ctx));
            xauto_ptr<xvbstate_t> target_state(nullptr);
            if(target_state)
            {
                m_block_state = target_state.get();
                target_state->add_ref();
            }
            else
            {
                close(); //force to close and mark it is unavaiable
            }
        }
    
        xvblkcontext_t::xvblkcontext_t(const xvblkcontext_t & obj)
            :xvexecontxt_t(obj.get_max_tgas(),obj.get_used_tgas(),NULL,NULL)
        {
            m_block_state = NULL;
            if(obj.m_block_state != NULL)
            {
                m_block_state = (xvbstate_t*)obj.m_block_state->clone();
                if(obj.get_input_canvas() != NULL)
                {
                    xauto_ptr<xvcanvas_t> new_input_canvas(new xvcanvas_t(*obj.get_input_canvas()));
                    reset_input_canvas(new_input_canvas());
                }
                
                if(obj.get_output_canvas() != NULL)
                {
                    xauto_ptr<xvcanvas_t> new_output_canvas(new xvcanvas_t(*obj.get_output_canvas()));
                    reset_output_canvas(new_output_canvas());
                }
            }
            else
            {
                close(); //force to close and mark it is unavaiable
            }
        }
        
        xvblkcontext_t::~xvblkcontext_t()
        {
            if(m_block_state != NULL)
                m_block_state->release_ref();
        }
        
        xauto_ptr<xvexestate_t> xvblkcontext_t::get_state(const xvaccount_t & account)
        {
            if(NULL == m_block_state)
                return nullptr;
            
            if(account.get_address() != m_block_state->get_address())
                return nullptr;
            
            m_block_state->add_ref();
            return m_block_state;
        }
        
        xauto_ptr<xvexestate_t> xvblkcontext_t::get_state(const std::string & account_addr)
        {
            if(NULL == m_block_state)
                return nullptr;
            
            if(account_addr != m_block_state->get_address())
                return nullptr;
            
            m_block_state->add_ref();
            return m_block_state;
        }
    
        bool xvblkcontext_t::snapshot()
        {
            xauto_ptr<xvexecontxt_t> clone_context(new xvblkcontext_t(*this));
            //reset_prev_context(clone_context());
            return true;
        }
    
        bool   xvblkcontext_t::restore() //restore to snapshot
        {
            return true;
        }
    
        
        //chain managed account 'state by a MPT tree(or likely) according state-hash,return xvactstate_t
        //traditional block that directly manage txs and change states of all account
        class xvactcontext_t : public xvexecontxt_t
        {
        protected:
            xvactcontext_t(const std::string & root_state_hash,const uint64_t new_max_tags)
                :xvexecontxt_t(new_max_tags,0,NULL,NULL)
            {
                m_root_state_hash = root_state_hash;
            };
            virtual ~xvactcontext_t(){};
        private:
            xvactcontext_t(xvactcontext_t &&);
            xvactcontext_t(const xvactcontext_t &);
            xvactcontext_t & operator = (xvactcontext_t &&);
            xvactcontext_t & operator = (const xvactcontext_t &);
            
        public:
            //construct new xvactstate_t based on last state of account(through last_state_root_hash)
            //virtual xauto_ptr<xvactstate_t> get_account_state(const xvaccount_t & account,const std::string last_state_root_hash);
            xauto_ptr<xvexestate_t> get_state(const xvaccount_t & account)
            {
                //XTODO
                return nullptr;
            }
            
            xauto_ptr<xvexestate_t> get_state(const std::string & account_addr)
            {
                //XTODO
                return nullptr;
            }
        private:
            std::string     m_root_state_hash; //root hash for chain' accounts
        };
        
        xauto_ptr<xvexecontxt_t> create_block_context(const xvblock_t & target_block,const uint64_t new_max_tags)
        {
            return new xvblkcontext_t(target_block,new_max_tags);
        }
    
        xauto_ptr<xvexecontxt_t> create_block_context(const xvheader_t & target_header,const uint64_t new_max_tags)
        {
            return new xvblkcontext_t(target_header,new_max_tags);
        }
    
    };//end of namespace of base
};//end of namespace of top
