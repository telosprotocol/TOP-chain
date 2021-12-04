// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxsmigrate.h"
#include "xvledger/xvbindex.h"

namespace top
{
    namespace base
    {
        xtxsmigrate_t::xtxsmigrate_t()
        {
            xkinfo("xtxsmigrate_t::xtxsmigrate_t");
        }
    
        xtxsmigrate_t:: ~xtxsmigrate_t()
        {
            xkinfo("xtxsmigrate_t::destroyed");
        }
        
        bool  xtxsmigrate_t::is_valid(const uint32_t obj_ver) //check version
        {
            return xtxsfilter_t::is_valid(obj_ver);
        }
    
        int   xtxsmigrate_t::init(const xvconfig_t & config_obj)
        {
            return enum_xcode_successful;
        }
    
        bool  xtxsmigrate_t::close(bool force_async)//close filter
        {
            xkinfo("xtxsmigrate_t::close");
            if(is_close() == false)
            {
                xtxsfilter_t::close(force_async); //mark closed flag first
                //XTODO,add own clean logic
            }
            xkinfo("xtxsmigrate_t::close,finished");
            return true;
        }
        
        //caller respond to cast (void*) to related  interface ptr
        void*  xtxsmigrate_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_sys_object_type_filter)
                return this;
            
            return xtxsfilter_t::query_interface(_enum_xobject_type_);
        }
    
        enum_xfilter_handle_code xtxsmigrate_t::transfer_keyvalue(xdbevent_t & event,xvfilter_t* last_filter)
        {
            xdbg("xtxsmigrate_t::transfer_keyvalue for event(0x%x)",event.get_type());
            if(is_close())
            {
                xwarn("xtxsmigrate_t::transfer_keyvalue,closed");
                return enum_xfilter_handle_code_closed;
            }
                
            if(get_object_version() > 0)
            {
                //XTODO add code for specific version
            }

            if(event.check_event_flag(xdbevent_t::enum_dbevent_flag_key_stored) == false)
            {
                if(event.get_target_store() != nullptr)
                {
                    xdbg("xtxsmigrate_t::transfer_keyvalue set_value_to_dst_db with no changed. key=%s,for event(0x%x)",event.get_db_key().c_str(), event.get_type());
                    if(event.get_target_store()->set_value(event.get_db_key(), event.get_db_value()))
                        event.set_event_flag(xdbevent_t::enum_dbevent_flag_key_stored);
                }
            }
            return enum_xfilter_handle_code_ignore;
        }
    
        enum_xfilter_handle_code xtxsmigrate_t::transfer_tx_v2_to_v3(xdbevent_t & event,xvfilter_t* last_filter)
        {
            const auto & db_value = event.get_db_value();
            base::xauto_ptr<base::xvtxindex_t> txindex = new base::xvtxindex_t();
            int32_t ret = txindex->serialize_from_string(db_value);
            if (ret <= 0) {
                xerror("xtxsmigrate_t::transfer_tx_v2_to_v3,fail txindex serialize");
                return enum_xfilter_handle_code_finish;
            }

            // load unit block from tx index
            base::xvaccount_t _unitvaccount(txindex->get_block_addr());

            std::string table_addr = base::xvaccount_t::make_table_account_address(_unitvaccount);
            if (table_addr == txindex->get_block_addr()) {
                xerror("xtxsmigrate_t::transfer_tx_v2_to_v3,fail txindex addr already is table addr");
                return enum_xfilter_handle_code_finish;
            }

            uint64_t _unitheight = txindex->get_block_height();
            // XTODO only load main-entry bindex key, almost all nodes should can migrate successfully
            std::string unitbindex_key = xvdbkey_t::create_block_index_key(_unitvaccount, _unitheight);
            std::string unitbindex_bin = event.get_source_store()->get_value(unitbindex_key);
            if (unitbindex_bin.empty()) {
                xerror("xtxsmigrate_t::transfer_tx_v2_to_v3,fail load unitbindex");
                return enum_xfilter_handle_code_finish;
            }
            base::xauto_ptr<base::xvbindex_t> unitbindex = new base::xvbindex_t();
            ret = unitbindex->serialize_from(unitbindex_bin);
            if (ret <= 0) {
                xerror("xtxsmigrate_t::transfer_tx_v2_to_v3,fail unitbindex serialize");
                return enum_xfilter_handle_code_finish;
            }
            
            txindex->set_block_addr(table_addr);
            txindex->set_block_height(unitbindex->get_parent_block_height());
            if (unitbindex->get_height() > 0) {
                xassert(unitbindex->get_parent_block_height() > 0);
            }

            std::string new_txindex_bin;
            txindex->serialize_to_string(new_txindex_bin);
            event.get_target_store()->set_value(event.get_db_key(), new_txindex_bin);
            xdbg("xtxsmigrate_t::transfer_tx_v2_to_v3 set_value_to_dst_db with new value.unit_addr=%s,unit_height=%ld,table_addr=%s,table_height=%ld",
                _unitvaccount.get_address().c_str(), _unitheight, table_addr.c_str(), unitbindex->get_parent_block_height());
            return enum_xfilter_handle_code_finish;  // process finish
        }

        enum_xfilter_handle_code xtxsmigrate_t::transfer_tx(xdbevent_t & event,xvfilter_t* last_filter)
        {
            xdbg("xtxsmigrate_t::transfer_tx for event(0x%x)",event.get_type());
            if(is_close())
            {
                xwarn("xtxsmigrate_t::transfer_tx,closed");
                return enum_xfilter_handle_code_closed;
            }

            transfer_tx_v2_to_v3(event, last_filter);
            // XTODO always return enum_xfilter_handle_code_finish
            return enum_xfilter_handle_code_finish;
        }
            
    }//end of namespace of base
}//end of namespace top
