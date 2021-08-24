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
        //---------------------------------xvbbuild_t---------------------------------//
        //xvbbuild_t responsible to combine input,output and header -> a new block
        xvbbuild_t::xvbbuild_t(const xvheader_t & target_header)
        {
            m_block_ptr       = NULL;
            m_header_ptr      = NULL;
            m_input_ptr       = NULL;
            m_output_ptr      = NULL;
         
            m_header_ptr = new xvheader_t(target_header);
            m_header_ptr->set_input_hash(std::string()); //reset to nil first
            m_header_ptr->set_output_hash(std::string()); //reset to nil first
        }
    
        xvbbuild_t::~xvbbuild_t()
        {
            if(m_input_ptr != NULL)
                m_input_ptr->release_ref();
            
            if(m_output_ptr != NULL)
                m_output_ptr->release_ref();
            
            if(m_header_ptr != NULL)
                m_header_ptr->release_ref();
       
            if(m_block_ptr != NULL)
                m_block_ptr->release_ref();
        }
    
        void   xvbbuild_t::reset_block(xvblock_t* new_ptr)
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            if(new_ptr != NULL)
                new_ptr->add_ref();
            
            xvblock_t * old_ptr = m_block_ptr;
            m_block_ptr = new_ptr;
            if(old_ptr != NULL)
                old_ptr->release_ref();
        }
    
        bool   xvbbuild_t::build_input(xvinput_t * new_ptr)
        {
            if(NULL == new_ptr)
                return false;
            
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            new_ptr->add_ref();
            
            xvinput_t * old_ptr = m_input_ptr;
            m_input_ptr = new_ptr;
            if(old_ptr != NULL)
                old_ptr->release_ref();
            
            return true;
        }
        
        bool   xvbbuild_t::build_output(xvoutput_t * new_ptr)
        {
            if(NULL == new_ptr)
                return false;
            
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            new_ptr->add_ref();
            
            xvoutput_t * old_ptr = m_output_ptr;
            m_output_ptr = new_ptr;
            if(old_ptr != NULL)
                old_ptr->release_ref();
            
            return true;
        }

        bool   xvbbuild_t::build_block(xvqcert_t * qcert_ptr)
        {
            xassert(qcert_ptr != NULL);
            if(NULL == qcert_ptr)
                return false;
            
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            if(m_block_ptr != NULL) //just allow build once
                return true;
            
            xvblock_t * new_block = NULL;
            if(get_block_header()->get_block_class() != enum_xvblock_class_nil)
            {
                xassert(m_input_ptr != NULL);
                if(NULL == m_input_ptr)
                    return false;
                
                xassert(m_output_ptr != NULL);
                if(NULL == m_output_ptr)
                    return false;
                
                //setup input/output root hash first
                qcert_ptr->set_input_root_hash(get_block_input()->get_root_hash());
                qcert_ptr->set_output_root_hash(get_block_output()->get_root_hash());
                
                new_block = new xvblock_t(*m_header_ptr,*qcert_ptr,m_input_ptr,m_output_ptr);
                if( (new_block->get_input() == NULL) || (new_block->get_output() == NULL) ) //check by input/output
                {
                    xerror("xvbbuild_t::build_block,fail to construct block by qcert(%s) vs header(%s)",qcert_ptr->dump().c_str(),m_header_ptr->dump().c_str());
                    
                    new_block->close();
                    new_block->release_ref();
                    return false;
                }
            }
            else
            {
                qcert_ptr->set_input_root_hash(std::string());
                qcert_ptr->set_output_root_hash(std::string());
                new_block = new xvblock_t(*m_header_ptr,*qcert_ptr,NULL,NULL);
            }
            m_block_ptr = new_block; //transfer owner to  m_block_ptr
            
            xinfo("xvbbuild_t::build_block,generated block(%s)",new_block->dump().c_str());
            return true;
        }
    
        const std::string  xvbbuild_t::build_mpt_root(const std::vector<std::string> & elements)
        {
            //xtodo,add implementation of mpt
            std::string root;
            for(auto & str : elements)
            {
                root += str;
            }
            return root;
        }
    
        xvblock_t*   xvbbuild_t::new_block_ptr(xvqcert_t & qcert_ptr)
        {
            return  new xvblock_t(*m_header_ptr,qcert_ptr,m_input_ptr,m_output_ptr);
        }
    
        void xvbbuild_t::set_crypto_hash_type_of_cert(xvqcert_t * qcert_ptr,enum_xhash_type new_type)
        {
            if(qcert_ptr != NULL)
                qcert_ptr->set_crypto_hash_type(new_type);
        }
        
        void xvbbuild_t::set_input_root_hash_of_cert(xvqcert_t * qcert_ptr,const std::string & root_hash)
        {
            if(qcert_ptr != NULL)
                qcert_ptr->set_input_root_hash(root_hash);
        }
        
        void xvbbuild_t::set_output_root_hash_of_cert(xvqcert_t * qcert_ptr,const std::string & root_hash)
        {
            if(qcert_ptr != NULL)
                qcert_ptr->set_output_root_hash(root_hash);
        }

        void xvbbuild_t::set_extend_data_of_cert(xvqcert_t * qcert_ptr,const std::string & ext_data)
        {
            if(qcert_ptr != NULL)
                qcert_ptr->set_extend_data(ext_data);
        }
    
        //---------------------------------xvbmaker_t---------------------------------//
        //xvbmaker_t responsible to excute action->execute entity->generate input & output -> then build a new block
        xvbmaker_t::xvbmaker_t(const xvheader_t & target_header,xvexecontxt_t * exe_contxt)
            :xvbbuild_t(target_header)
        {
            m_exe_context     = NULL;
            m_input_resource  = NULL;
            m_output_resource = NULL;
            
            m_primary_input_entity  = NULL;
            m_primary_output_entity = NULL;
            
            if(exe_contxt != NULL)
            {
                m_exe_context = exe_contxt;
                m_exe_context->add_ref();
            }
            else
            {
                auto unit_contxt(create_block_context(target_header,0));
                m_exe_context = unit_contxt();
                m_exe_context->add_ref();
            }
            
            //create it first
            m_input_resource  = new xstrmap_t();
            m_output_resource = new xstrmap_t();
            m_primary_input_entity = new xvinentity_t(std::vector<xvaction_t>());
            
            m_hash_type = enum_xhash_type_sha2_256;//default as sha2 256
        }
        
        xvbmaker_t::~xvbmaker_t()
        {
            if(m_primary_input_entity != NULL)
                m_primary_input_entity->release_ref();
            
            if(m_primary_output_entity != NULL)
                m_primary_output_entity->release_ref();
            
            if(m_input_resource != NULL)
                m_input_resource->release_ref();
            
            if(m_output_resource != NULL)
                m_output_resource->release_ref();
            
            if(m_exe_context != NULL)
                m_exe_context->release_ref();
        }

        bool  xvbmaker_t::build_entity(const std::vector<xvtransact_t*> & input_txs)
        {
            if(input_txs.empty())
                return true;
            
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            if(get_block_header()->get_block_class() == enum_xvblock_class_nil)
            {
                xerror("xvbmaker_t::build_entity,try rebuild input txs for nil block(%s)",get_block_header()->dump().c_str());
                return false;
            }
            if(get_block_input() != NULL)//just only build once
            {
                xerror("xvbmaker_t::build_entity,try rebuild input txs for block(%s)",get_block_header()->dump().c_str());
                return false;
            }
            
            if(verify_input_txs(*get_block_header(), input_txs))
            {
                std::vector<xvaction_t> input_actions;
                for(auto & tx : input_txs)
                {
                    const xvaction_t & action(tx->get_action());
                    input_actions.emplace_back(action);
                    if(get_input_resource()->find(tx->get_hash()) == false)
                    {
                        std::string trans_bin;
                        tx->serialize_to_string(trans_bin);
                        get_input_resource()->set(tx->get_hash(),trans_bin);
                    }
                }
                if(NULL == m_primary_input_entity)
                {
                    m_primary_input_entity = new xvinentity_t(std::move(input_actions));
                }
                else
                {
                    std::vector<xvaction_t> full_actions = m_primary_input_entity->get_actions();
                    for(auto & act : input_actions)
                        full_actions.emplace_back(act);
                    
                    xvinentity_t * new_entity = new xvinentity_t(std::move(full_actions));
                    m_primary_input_entity->release_ref();
                    m_primary_input_entity = new_entity; //assign to new one
                }
                xinfo("xvbmaker_t::build_entity,finish primay entity of block(%s)",get_block_header()->dump().c_str());
                return true;
            }
            xerror("xvbmaker_t::build_entity,invalid input primary txs for block(%s)",get_block_header()->dump().c_str());
            return false;
        }
        
        bool  xvbmaker_t::make_input()
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            if(get_block_input() != NULL) //just only allow build once
                return true;
            
            std::vector<xventity_t*>  _entities;
            _entities.emplace_back(get_input_entity());//copy ptr to entity,later reference by xvinput_t
            xauto_ptr<xvinput_t>input_obj(new xvinput_t(_entities,*get_input_resource()));
            build_input(input_obj());
            
            xinfo("xvbmaker_t::make_input,done for block(%s)",get_block_header()->dump().c_str());
            return true;
        }
        
        bool  xvbmaker_t::make_output() //execute
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            if(get_block_output() != NULL) //just only build once
                return true;
            
            if(get_output_entity() != NULL)
                return true;
            
            if( (get_input_entity() == NULL) || (get_block_input() == NULL) )
            {
                xerror("xvbmaker_t::make_output(),Please build input first for block(%s)",get_block_header()->dump().c_str());
                return false;
            }
            
            if(get_input_entity()->get_actions().empty() == false) //not empty block
            {
                m_primary_output_entity = execute_entity(get_input_entity(),*get_exe_context());
                if(NULL == m_primary_output_entity) //added rerfernce by execute_entity
                {
                    xerror("xvbmaker_t::make_output(),failed build output for block(%s)",get_block_header()->dump().c_str());
                    return false;
                }
                //note: actions may be modified by execute_entity,so build root_hash after execute_entity
                std::vector<std::string> actions_hash_vector;//build action' mpt tree,and assign mpt root as input root hash
                {
                    const std::vector<xvaction_t> & actions = get_input_entity()->get_actions();
                    for(auto & it : actions)
                    {
                        std::string action_bin;
                        it.serialize_to(action_bin);
                        actions_hash_vector.emplace_back(xcontext_t::instance().hash(action_bin, get_crypto_hash_type()));
                    }
                    ((xvinput_t*)get_block_input())->set_root_hash(build_mpt_root(actions_hash_vector));
                }
                
                //build output
                std::vector<xventity_t*> output_entities;
                output_entities.emplace_back(m_primary_output_entity);//copy ptr to entity,later reference by xvoutput_t
                xvoutput_t *  output_ptr = new xvoutput_t(output_entities,*get_output_resource());//reference it
                
                //build output root hash
                {
                    auto vbstate = get_exe_context()->get_state(get_block_header()->get_account());
                    std::string full_state_bin;
                    vbstate->take_snapshot(full_state_bin);
                    output_ptr->set_root_hash(xcontext_t::instance().hash(full_state_bin,get_crypto_hash_type()));
                }
                
                //assign to maker
                build_output(output_ptr);
                output_ptr->release_ref();
            }
            else
            {
                m_primary_output_entity = new xvoutentity_t(std::string()); //create empty output
                ((xvinput_t*)get_block_input())->set_root_hash(std::string());
                
                std::vector<xventity_t*> output_entities;
                output_entities.emplace_back(m_primary_output_entity);//copy ptr to entity,later reference by xvoutput_t
                xvoutput_t *  output_ptr = new xvoutput_t(output_entities,std::string());//reference it
                
                //assign to maker
                build_output(output_ptr);
                output_ptr->release_ref();
            }
            xinfo("xvbmaker_t::build_output,done for block(%s)",get_block_header()->dump().c_str());
            return true;
        }
        
        const bool  xvbmaker_t::verify_input_txs(const xvheader_t & target,const std::vector<xvtransact_t*> & input_txs)
        {
            if(input_txs.empty())
            {
                if(target.get_block_class() == enum_xvblock_class_nil)
                    return true;
                
                xerror("xvbmaker_t::verify_input_txs(),empty input for block(%s)",target.dump().c_str());
                return false;
            }
            else if(target.get_block_class() == enum_xvblock_class_nil)
            {
                xerror("xvbmaker_t::verify_input_txs(),not allow have input for nil block(%s)",target.dump().c_str());
                return false;
            }
            
            std::vector<xvaction_t> input_actions;
            for(auto & tx : input_txs)
            {
                if(NULL == tx)
                {
                    xerror("xvbmaker_t::verify_input_txs(),found nil transaction for block(%s)",target.dump().c_str());
                    return false;
                }
                if(tx->is_valid() == false)
                {
                    xerror("xvbmaker_t::verify_input_txs(),found bad transaction(%s) for block(%s)",tx->dump().c_str(),target.dump().c_str());
                    return false;
                }
                //input tx must apply for target block of target account
                const xvaction_t & action(tx->get_action());
                if(action.get_contract_uri().find(target.get_account()) != 0)
                {
                    xerror("xvbmaker_t::verify_input_txs(),found bad action(%s) that not belong this block(%s)",action.dump().c_str(),target.dump().c_str());
                    return false;
                }
            }
            return true;
        }
      
        bool  xvbmaker_t::merge_input_resource(const xstrmap_t * src_map)
        {
            if(NULL == src_map)
                return false;
            
            //combine resource of input
            get_input_resource()->merge(*src_map);
            return true;
        }
        
        bool  xvbmaker_t::merge_output_resource(const xstrmap_t * src_map)
        {
            if(NULL == src_map)
                return false;
            
            //combine resource of output
            get_output_resource()->merge(*src_map);
            return true;
        }
        
        //---------------------------------xvtablemaker_t---------------------------------//
        xvtablemaker_t::xvtablemaker_t(const xvheader_t & table_header,xvexecontxt_t * exe_contxt)
            :xvbmaker_t(table_header,exe_contxt)
        {
        }
        
        xvtablemaker_t::~xvtablemaker_t()
        {
            for(auto unit_ptr : m_sub_units)
                delete unit_ptr;
            
            m_sub_units.clear();
        }
    
        bool  xvtablemaker_t::build_entity(const xvheader_t & unit_header,const std::vector<xvtransact_t*> & input_unit_txs)
        {
            if(input_unit_txs.empty())
                return true;
            
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            if(get_block_header()->get_block_class() == enum_xvblock_class_nil)
            {
                xerror("xvtablemaker_t::build_entity,try build txs for nil block(%s)",get_block_header()->dump().c_str());
                return false;
            }
            
            for(auto unit : m_sub_units)
            {
                //each table only allow hold one entry for each unit
                if(unit->get_block_account() == unit_header.get_account())
                {
                    if(unit->get_block_header()->is_equal(unit_header))//found same header/unit
                        return unit->build_entity(input_unit_txs); //do increased-build

                    xerror("xvtablemaker_t::build_entity,un-mached unit(%s) vs  existing(%s)",unit_header.dump().c_str(),unit->get_block_header()->dump().c_str());
                    return false;
                }
            }
            
            auto unit_contxt(create_block_context(unit_header,0));
            if( (!unit_contxt) || (unit_contxt->is_close()))
            {
                xwarn("xvtablemaker_t::build_entity,failed to load exe-state for block(%s)",unit_header.dump().c_str());
                return false;
            }
            
            xvbmaker_t * new_unit = new xvbmaker_t(unit_header,unit_contxt());
            xassert(new_unit != NULL);
            if(new_unit->build_entity(input_unit_txs))
            {
                m_sub_units.emplace_back(new_unit);
                xdbg("xvtablemaker_t::build_entity,done for unit block(%s) as txs(%d)",unit_header.dump().c_str(),(int)input_unit_txs.size());
                return true;
            }
            delete new_unit;
            xerror("xvtablemaker_t::build_entity,invalid input unit txs for block(%s) as  txs(%d)",unit_header.dump().c_str(),(int)input_unit_txs.size());
            return false;
        }
    
        bool  xvtablemaker_t::make_input()
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            xinfo("xvtablemaker_t::make_input,start for block(%s)",get_block_header()->dump().c_str());
            xvbmaker_t::make_input(); //build primary entity of table
            for(auto unit : m_sub_units)
            {
                if(unit->make_input())
                {
                    merge_input_resource(unit->get_block_input()->get_resources());//combine resource of input
                }
            }
   
            xinfo("xvtablemaker_t::make_input,done for block(%s)",get_block_header()->dump().c_str());
            return true;
        }
        
        bool  xvtablemaker_t::make_output() //execute
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            xinfo("xvtablemaker_t::make_output,start for block(%s)",get_block_header()->dump().c_str());
            xvbmaker_t::make_output();
            for(auto unit : m_sub_units)
            {
                if(unit->make_output())
                {
                    merge_output_resource(unit->get_block_output()->get_resources());//combine resource of output
                }
            }
 
            xinfo("xvtablemaker_t::make_output,done for block(%s)",get_block_header()->dump().c_str());
            return true;
        }
    
        bool  xvtablemaker_t::build_block(xvqcert_t * qcert_ptr) //step#4
        {
            if(NULL == qcert_ptr)
            {
                xassert(qcert_ptr != NULL);
                return false;
            }
            
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            if(get_block() != NULL) //just allow build once
                return true;
        
            //must has done build_input()
            if(NULL == get_block_input())
            {
                xassert(get_block_input() != NULL);
                return false;
            }
            //must has done build_output()
            if(NULL == get_block_output())
            {
                xassert(get_block_output() != NULL);
                return false;
            }
            
            //align setting related qcert of maker, to qcert_ptr
            set_crypto_hash_type_of_cert(qcert_ptr, get_crypto_hash_type());
 
            if(get_block_header()->get_block_class() != enum_xvblock_class_nil) //nil block go default implement
            {
                xinfo("xvtablemaker_t::build_block,start for block(%s)",get_block_header()->dump().c_str());
                
                std::vector<std::string> units_input_root_hash_vector;
                std::vector<std::string> units_output_root_hash_vector;
                std::vector<std::string> units_header_hash_vector;
                for(auto unit : m_sub_units)
                {
                    //clone new cert and do build seperately
                    xauto_ptr<xvqcert_t> new_unit_cert(new xvqcert_t(*qcert_ptr));
                    if(unit->build_block(qcert_ptr))
                    {
                        units_input_root_hash_vector.emplace_back(unit->get_block_input()->get_root_hash());
                        units_output_root_hash_vector.emplace_back(unit->get_block_output()->get_root_hash());
                        units_header_hash_vector.emplace_back(unit->get_block()->get_header_hash());
                    }
                }
                
                //rebuild input of table
                {
                    std::vector<base::xventity_t*> all_input_entities;
                    get_input_entity()->add_ref(); //add refer before copy to vector
                    all_input_entities.push_back(get_input_entity());//always occupy #0 location for primary item
                    for(auto unit : m_sub_units)
                    {
                        if(unit->get_block() != NULL) //successful build unit
                        {
                            auto unit_entitys = unit->get_block_input()->get_entitys();
                            for(auto ent : unit_entitys)//actually only have one entity for unit,but here just handle as general
                            {
                                xvintable_ent * new_entity = new xvintable_ent(*unit->get_block_header(),((xvinentity_t*)ent)->get_actions());
                                all_input_entities.emplace_back(new_entity);//transfered owner of ptr to vector
                            }
                        }
                    }
                    //note:here through std::move() all ptr has been transfer to xvinput, so it is no-need release manually
                    xauto_ptr<xvinput_t>input_obj(new xvinput_t(std::move(all_input_entities),*get_input_resource()));
                    build_input(input_obj());
                }
                
                //rebuild output of table
                {
                    std::vector<base::xventity_t*> all_output_entities;
                    all_output_entities.emplace_back(get_output_entity());//always occupy #0 location
                    for(auto unit : m_sub_units)
                    {
                        if(unit->get_block() != NULL) //successful build unit
                        {
                            auto unit_entitys = unit->get_block_output()->get_entitys();
                            for(auto ent : unit_entitys)
                            {
                                all_output_entities.emplace_back(ent);
                            }
                        }
                    }
                    //build output of table
                    xauto_ptr<xvoutput_t> output_obj(new xvoutput_t(all_output_entities,*get_output_resource()));
                    build_output(output_obj());
                }
                
                //build input root hash of table
                ((xvinput_t*)get_block_input())->set_root_hash(build_mpt_root(units_input_root_hash_vector));
                //build output root hash of table
                ((xvoutput_t*)get_block_output())->set_root_hash(build_mpt_root(units_output_root_hash_vector));
                
                //build certification root hash of table
                set_extend_data_of_cert(qcert_ptr,build_mpt_root(units_header_hash_vector));
                
                xinfo("xvtablemaker_t::build_block,done for block(%s)",get_block_header()->dump().c_str());
            }
            else
            {
                ((xvinput_t*)get_block_input())->set_root_hash(std::string());
                ((xvoutput_t*)get_block_output())->set_root_hash(std::string());
                set_extend_data_of_cert(qcert_ptr,std::string()); //assign nil
            }
            return xvbmaker_t::build_block(qcert_ptr);
        }

    };//end of namespace of base
};//end of namespace of top
