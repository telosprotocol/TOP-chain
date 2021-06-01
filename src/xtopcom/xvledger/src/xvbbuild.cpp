// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xbase/xutl.h"
#include "../xvexecontxt.h"
#include "../xvbbuild.h"
#include "../xvledger.h"
 
namespace top
{
    namespace base
    {
        xvbmaker_t::xvbmaker_t(const xvblock_t & target_frame)
        {
            m_input_ptr  = NULL;
            m_output_ptr = NULL;
            m_base_frame = NULL;
            m_exe_context= NULL;
            
            m_base_frame = new xvblock_t(target_frame);
        }
    
        xvbmaker_t::~xvbmaker_t()
        {
            if(m_input_ptr != NULL)
                m_input_ptr->release_ref();
            
            if(m_output_ptr != NULL)
                m_output_ptr->release_ref();
            
            if(m_base_frame != NULL)
                m_base_frame->release_ref();
            
            if(m_exe_context != NULL)
                m_exe_context->release_ref();
        }
    
        void   xvbmaker_t::reset_input(xvinput_t * new_ptr)
        {
            if(new_ptr != NULL)
                new_ptr->add_ref();
            
            xvinput_t * old_ptr = xatomic_t::xexchange(m_input_ptr, new_ptr);
            if(old_ptr != NULL)
                old_ptr->release_ref();
        }
        
        void   xvbmaker_t::reset_output(xvoutput_t * new_ptr)
        {
            if(new_ptr != NULL)
                new_ptr->add_ref();
            
            xvoutput_t * old_ptr = xatomic_t::xexchange(m_output_ptr, new_ptr);;
            if(old_ptr != NULL)
                old_ptr->release_ref();
        }
    
        xauto_ptr<xvblock_t>   xvbmaker_t::build_block()
        {
            xassert(m_input_ptr != NULL);
            if(NULL == m_input_ptr)
                return nullptr;
 
            xassert(m_output_ptr != NULL);
            if(NULL == m_output_ptr)
                return nullptr;
            
            xauto_ptr<xvheader_t> clone_header(get_block_frame()->get_header()->clone());
            clone_header->set_input_hash(std::string()); //reset to nil
            clone_header->set_output_hash(std::string());//reset to nil
            
            xvblock_t * new_block = new xvblock_t(*clone_header,*get_block_frame()->get_cert(),m_input_ptr,m_output_ptr);
            return new_block;
        }
    
        xvunitmaker_t::xvunitmaker_t(const xvblock_t & target_frame)
            :xvbmaker_t(target_frame)
        {
            auto contxt(create_block_context(target_frame,0));
            contxt->add_ref();
            m_exe_context = contxt();
        }

        xvunitmaker_t::~xvunitmaker_t()
        {
        }
    
        bool  xvunitmaker_t::build_input(const std::vector<xvtransact_t*> & input_txs)
        {
            if(get_block_frame()->get_block_class() == enum_xvblock_class_nil)
            {
                xerror("xvunitmaker_t::build_input(),not allow have input for nil block(%s)",get_block_frame()->dump().c_str());
                return false;
            }
            if(input_txs.empty())
            {
                xerror("xvunitmaker_t::build_input(),nil input for block(%s)",get_block_frame()->dump().c_str());
                return false;
            }
            
            xauto_ptr<xstrmap_t> raw_tx(new xstrmap_t());
            std::vector<xvaction_t> input_actions;
            for(auto & tx : input_txs)
            {
                if(NULL == tx)
                {
                    xerror("xvunitmaker_t::build_input(),found nil transaction for block(%s)",tx->dump().c_str(),get_block_frame()->dump().c_str());
                    return false;
                }
                if(tx->is_valid() == false)
                {
                    xerror("xvunitmaker_t::build_input(),found bad transaction(%s) for block(%s)",tx->dump().c_str(),get_block_frame()->dump().c_str());
                    return false;
                }
                const xvaction_t & action(tx->get_action());
                if(action.get_contract_uri().find(get_block_frame()->get_account()) != 0)
                {
                    xerror("xvunitmaker_t::build_input(),found bad action(%s) that not belong this block(%s)",action.dump().c_str(),get_block_frame()->dump().c_str());
                    return false;
                }
                
                input_actions.emplace_back(action);
                if(raw_tx->find(tx->get_hash()) == false)
                {
                    std::string trans_bin;
                    tx->serialize_to_string(trans_bin);
                    raw_tx->set(tx->get_hash(),trans_bin);
                }
            }
            
            std::vector<xventity_t*>  _entities;
            xvinentity_t * input_entity = new xvinentity_t(std::move(input_actions));
            _entities.emplace_back(input_entity);//transfer owner to _entities
            
            xauto_ptr<xvinput_t>input_obj(new xvinput_t(std::move(_entities),*raw_tx));
            reset_input(input_obj());
            return true;
        }
        
        bool  xvunitmaker_t::build_output() //execute
        {
            if(get_block_input() == NULL)
            {
                xerror("xvunitmaker_t::build_output(),Please build input first for block(%s)",get_block_frame()->dump().c_str());
                return false;
            }

            xauto_ptr<xvoutput_t> output(execute_input(*get_block_input(), *m_exe_context));
            if(!output)
            {
                xerror("xvunitmaker_t::build_output(),failed build output for block(%s)",get_block_frame()->dump().c_str());
                return false;
            }
            
            reset_output(output());
            return true;
        }
    
    };//end of namespace of base
};//end of namespace of top
