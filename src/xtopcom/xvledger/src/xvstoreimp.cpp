// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "../xvdbstore.h"
#include "../xvblockstore.h"
#include "../xvstatestore.h"
#include "../xveventbus.h"
#include "../xvledger.h"
 
namespace top
{
    namespace base
    {
        xvdbstore_t::xvdbstore_t()
            :xobject_t((enum_xobject_type)enum_xobject_type_vxdbstore)
        {
        }
        
        xvdbstore_t::~xvdbstore_t()
        {
        };
        
        //caller need to cast (void*) to related ptr
        void*   xvdbstore_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_xobject_type_vxdbstore)
                return this;
            
            return xobject_t::query_interface(_enum_xobject_type_);
        }
        
        xvblockstore_t::xvblockstore_t(base::xcontext_t & _context,const int32_t target_thread_id)
            :xiobject_t(_context,target_thread_id,(enum_xobject_type)enum_xobject_type_vblockstore)
        {
            xvblock_t::register_object(xcontext_t::instance()); //should only have one xvblockstore_t per process
        }
        
        xvblockstore_t::~xvblockstore_t()
        {
        };
        
        //caller need to cast (void*) to related ptr
        void*   xvblockstore_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_xobject_type_vblockstore)
                return this;
            
            return xiobject_t::query_interface(_enum_xobject_type_);
        }
        //only allow remove flag within xvblockstore_t
        void  xvblockstore_t::remove_block_flag(xvblock_t* to_block, enum_xvblock_flag flag)
        {
            if(to_block != NULL)
                to_block->remove_block_flag(flag);
        }
    
        xveventbus_t::xveventbus_t()
            :xobject_t(enum_xevent_route_path_by_mbus)
        {
        }
        xveventbus_t::~xveventbus_t()
        {
        }
    
         void*   xveventbus_t::query_interface(const int32_t type)
         {
             if(type == enum_xobject_type_veventbus)
                 return this;
         
             return xobject_t::query_interface(type);
         }
        
        bool   xveventbus_t::handle_event(const xvevent_t & ev)
        {
            if(ev.get_type() == enum_xevent_route_path_by_mbus)
            {
                auto * ev_ptr = const_cast<xvevent_t *>(&ev);
                ev_ptr->add_ref();
                mbus::xevent_ptr_t mbus_ev_ptr;
                mbus_ev_ptr.attach(dynamic_cast<mbus::xevent_t *>(ev_ptr));

                push_event(mbus_ev_ptr);
                return true;
            }
            return false;
        }
    
        xvstatestore_t::xvstatestore_t()
            :xobject_t(enum_xobject_type_vstatestore)
        {
            
        }
    
        xvstatestore_t::~xvstatestore_t()
        {
        }
    
        void*   xvstatestore_t::query_interface(const int32_t type)
        {
            if(type == enum_xobject_type_vstatestore)
                return this;
            
            return xobject_t::query_interface(type);
        }
    
        const std::string  xvstatestore_t::create_state_db_key(xvaccount_t & account,const uint64_t block_height,const std::string & hashkey)
        {
            const std::string key_path = "/state/" + base::xstring_utl::tostring(account.get_xvid()) + "/" + base::xstring_utl::tostring(block_height) + "/" + hashkey;
            return key_path;
        }
    
        bool   xvstatestore_t::write_state_to_db(xvblock_t * block_ptr)
        {
            if(NULL == block_ptr)
                return false;
            
            xvaccount_t target_account(block_ptr->get_account());
            return write_state_to_db(target_account,block_ptr);
        }
        
        bool   xvstatestore_t::write_state_to_db(xvaccount_t & target_account,xvblock_t * block_ptr)
        {
            if(NULL == block_ptr)
                return false;
            
            if(block_ptr->get_state() == NULL)
                return false;
            
            const std::string state_db_key = create_state_db_key(target_account,block_ptr->get_height(),block_ptr->get_block_hash());
            
            std::string state_db_bin;
            if(block_ptr->get_state()->serialize_to_string(state_db_bin))
            {
                if(base::xvchain_t::instance().get_xdbstore()->set_value(state_db_key,state_db_bin))
                    return true;
                
                xerror("xvstatestore::write_state_to_db,fail to write into db for path(%s)",state_db_key.c_str());
            }
            xerror("xvstatestore::write_state_to_db,fail to serialize for state of block(%s)",block_ptr->dump().c_str());
            return false;
        }
        
        xvbstate_t*     xvstatestore_t::read_state_from_db(xvblock_t * for_block)
        {
            if(NULL == for_block)
                return NULL;
            
            xvaccount_t target_account(for_block->get_account());
            return read_state_from_db(target_account,for_block);
        }
        xvbstate_t*     xvstatestore_t::read_state_from_db(xvaccount_t & target_account,xvblock_t * for_block)
        {
            if(NULL == for_block)
                return NULL;
            
            const std::string state_db_key = create_state_db_key(target_account,for_block->get_height(),for_block->get_block_hash());
            
            const std::string state_db_bin = base::xvchain_t::instance().get_xdbstore()->get_value(state_db_key);
            if(state_db_bin.empty())
            {
                xwarn("xvstatestore::read_state_from_db,fail to read from db for path(%s)",state_db_key.c_str());
                return NULL;
            }
            xvbstate_t* state_ptr = xvblock_t::create_state_object(state_db_bin);
            if(NULL == state_ptr)//remove the error data for invalid data
            {
                base::xvchain_t::instance().get_xdbstore()->delete_value(state_db_key);
                xerror("xvstatestore::read_state_from_db,invalid data at db for path(%s)",state_db_key.c_str());
            }
            return state_ptr;
        }
    
        bool   xvstatestore_t::rebuild_state_for_block(xvblock_t & target_block)
        {
            if(target_block.get_state() == NULL)
            {
                xauto_ptr<xvbstate_t> init_state(new xvbstate_t(target_block.get_account(),target_block.get_height(),std::vector<xvproperty_t*>()));
                target_block.reset_block_state(init_state.get());//must successful
            }
          
            if(target_block.get_block_class() == enum_xvblock_class_nil)
            {
                xinfo("xvstatestore::rebuild_state_for_block,nothing chanage for state of nil block(%s)",target_block.dump().c_str());
                return true;
            }
            else //build state from instruction code
            {
                if(NULL == target_block.get_output())
                {
                    xvaccount_t target_account(target_block.get_account());
                    if(false == xvchain_t::instance().get_xblockstore()->load_block_output(target_account, &target_block))
                    {
                        xerror("xvstatestore::rebuild_state_for_block,fail to load output for block(%s)",target_block.dump().c_str());
                        return false;
                    }
                }
                
                const std::vector<xventity_t*> & entities = target_block.get_output()->get_entitys();
                for(auto & ent : entities)
                {
                    const std::string binlog(ent->query_value("xbinlog"));
                    if(binlog.empty() == false)
                    {
                        if(false == target_block.get_state()->apply_changes_of_binlog(binlog))
                        {
                            xerror("xvstatestore::rebuild_state_for_block,invalid binlog and abort it for block(%s)",target_block.dump().c_str());
                            return false;
                        }
                    }
                }
                xinfo("xvstatestore::get_block_state,successful rebuilt state for block(%s)",target_block.dump().c_str());
                return true;
            }
        }
    
        bool   xvstatestore_t::get_block_state(xvblock_t * current_block) //once successful,assign xvbstate_t into block
        {
            if(NULL == current_block)
                return false;
            
            if(current_block->get_state() != NULL)//check whether has cached already
                return true;
                        
            //step#1:try load from db for target state
            xvaccount_t target_account(current_block->get_account());
            {
                xauto_ptr<xvbstate_t> current_block_state_ptr(read_state_from_db(target_account,current_block));
                if(current_block_state_ptr)
                {
                    if(current_block->reset_block_state(current_block_state_ptr.get()))
                    {
                        xdbg("xvstatestore::get_block_state,found state(%s) for block(%s)",current_block_state_ptr->dump().c_str(),current_block->dump().c_str());
                        return true;
                    }
                }
            }
            //step#2:try rebuild state completely for full-block or genesis block
            if(current_block->get_height() == 0) //direct rebuild state from geneis block
            {
                current_block->reset_block_state(NULL); //force to reset now
                bool result = rebuild_state_for_block(*current_block);//then rebuild it completely
                if(result)//persist full state into db
                   write_state_to_db(current_block);
                return result;
            }
            else if(current_block->get_block_class() == enum_xvblock_class_full)//direct rebuild state for full-block
            {
                current_block->reset_block_state(NULL); //force to reset now
                bool result = rebuild_state_for_block(*current_block);//then rebuild it completely
                if(result)//persist full state into db
                    write_state_to_db(current_block);
                return result;
            }
    
            //step#3:rebuild current state based on prev block 'state
            if(current_block->get_prev_block() == NULL)
            {
                xauto_ptr<xvblock_t> prev_block(xvchain_t::instance().get_xblockstore()->load_block_object(target_account,current_block->get_height() - 1,current_block->get_last_block_hash(),true));
                if(!prev_block)
                {
                    xwarn("xvstatestore::get_block_state,fail to load prev-block from current(%s)",current_block->dump().c_str());
                    return false;
                }
                if(get_block_state(prev_block.get()))//reenter call prev of prev ... until full-block or genesis block
                {
                    xauto_ptr<xvbstate_t> new_current_state_ptr(prev_block->get_state()->clone(current_block->get_height()));
                    current_block->reset_block_state(new_current_state_ptr.get());//setup first
                    return rebuild_state_for_block(*current_block); //then reexecute instruction based on last-state
                }
            }
            else if(get_block_state(current_block->get_prev_block()))
            {
                xauto_ptr<xvbstate_t> new_current_state_ptr(current_block->get_prev_block()->get_state()->clone(current_block->get_height()));
                current_block->reset_block_state(new_current_state_ptr.get());//setup first
                return rebuild_state_for_block(*current_block); //then reexecute instruction based on last-state
            }
            xwarn("xvstatestore::get_block_state,fail to load state for block(%s) as prev-one build state fail",current_block->dump().c_str());
            return false;
        }
    
        xauto_ptr<xvbstate_t> xvstatestore_t::get_block_state(xvaccount_t & account,const uint64_t height,const uint64_t view_id)
        {
            xauto_ptr<xvblock_t> target_block(xvchain_t::instance().get_xblockstore()->load_block_object(account,height,view_id,true));
            
            if(get_block_state(target_block.get()))
            {
                target_block->get_state()->add_ref();
                return target_block->get_state();
            }
            xwarn("xvstatestore::get_block_state,fail get state for block of account(%s)->height(%" PRIu64 ")->viewid(%" PRIu64 ")",account.get_address().c_str(),height,view_id);
            return nullptr;
        }
    
        xauto_ptr<xvbstate_t> xvstatestore_t::get_block_state(xvaccount_t & account,const uint64_t height,const std::string& block_hash)
        {
            xauto_ptr<xvblock_t> target_block(xvchain_t::instance().get_xblockstore()->load_block_object(account,height,block_hash,true));
            if(get_block_state(target_block.get()))
            {
                target_block->get_state()->add_ref();
                return target_block->get_state();
            }
            xwarn("xvstatestore::get_block_state,fail get state for block of account(%s)->height(%" PRIu64 ")->hash(%s)",account.get_address().c_str(),height,block_hash.c_str());
            return nullptr;
        }
        
    };//end of namespace of base
};//end of namespace of top
