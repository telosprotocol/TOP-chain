// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xkeymigrate.h"
#include "xvledger/xvbindex.h"
#include "xvledger/xvtxindex.h"
#include "xdata/xblocktool.h"

namespace top
{
    namespace base
    {
        xkeymigrate_t::xkeymigrate_t()
        {
            xkinfo("xkeymigrate_t::xkeymigrate_t");
        }
    
        xkeymigrate_t:: ~xkeymigrate_t()
        {
            xkinfo("xkeymigrate_t::destroyed");
        }
        
        bool  xkeymigrate_t::is_valid(const uint32_t obj_ver) //check version
        {
            return xkeyvfilter_t::is_valid(obj_ver);
        }
    
        int   xkeymigrate_t::init(const xvconfig_t & config_obj)
        {
            xkinfo("xkeymigrate_t::init");
            std::vector<std::string> all_table_addrs = data::xblocktool_t::make_all_table_addresses();

            for (auto & addr : all_table_addrs) {
                base::xvaccount_t _vaddr(addr);
                m_tableaddr_vids[_vaddr.get_xvid_str()] = addr;
            }
            
            return enum_xcode_successful;
        }

        std::string xkeymigrate_t::get_addr_by_xvid(const std::string & xvid) const {
            auto iter = m_tableaddr_vids.find(xvid);
            if (iter != m_tableaddr_vids.end()) {
                return iter->second;
            }
            return std::string();
        }
    
        bool  xkeymigrate_t::close(bool force_async)//close filter
        {
            xkinfo("xkeymigrate_t::close");
            if(is_close() == false)
            {
                xkeyvfilter_t::close(force_async); //mark closed flag first
                //XTODO,add own clean logic
            }
            xkinfo("xkeymigrate_t::close,finished");
            return true;
        }
    
        //caller respond to cast (void*) to related  interface ptr
        void*  xkeymigrate_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_sys_object_type_filter)
                return this;
            
            return xkeyvfilter_t::query_interface(_enum_xobject_type_);
        }
    
        enum_xfilter_handle_code  xkeymigrate_t::fire_event(const xvevent_t & event,xvfilter_t* last_filter)
        {
            xdbg("xkeymigrate_t::fire_event for event(0x%x)",event.get_type());
            if(is_close())
            {
                xwarn("xkeymigrate_t::fire_event,closed");
                return enum_xfilter_handle_code_closed;
            }
            
            if(event.get_event_category() != enum_xevent_category_db)
            {
                xerror("xkeymigrate_t::fire_event,bad event category for event(0x%x)",event.get_type());
                return enum_xfilter_handle_code_bad_type;
            }
            
            //logicly split full_event_type = [8bit:Event_Category][8bit:Event_KEY]
            //Event_KEY = [4bit:code][4bit:key_type]
            uint8_t event_key = event.get_event_key();
            if( (event_key & enum_xdbevent_code_transfer) != 0)
            {
                //force to convert key_type to enum_xdbkey_type_keyvalue,while migrating
                event_key = (event_key & 0xF0) | enum_xdbkey_type_keyvalue;
            }
            if(event_key >= enum_max_event_keys_count)
            {
                xerror("xkeymigrate_t::fire_event,bad event id for event(0x%x)",event.get_type());
                return enum_xfilter_handle_code_bad_type;
            }
            
            xevent_handler * target_handler = get_event_handlers()[event_key];
            if(target_handler != nullptr)
                return (*target_handler)(event,last_filter);
            
            return enum_xfilter_handle_code_ignore;
        }
    
        enum_xfilter_handle_code xkeymigrate_t::transfer_keyvalue(xdbevent_t & event,xvfilter_t* last_filter)
        {
            xdbg("xkeymigrate_t::transfer_keyvalue for event(0x%x)",event.get_type());
            if(is_close())
            {
                xwarn("xkeymigrate_t::transfer_keyvalue,closed");
                return enum_xfilter_handle_code_closed;
            }
            
            if(get_object_version() > 0)
            {
                return transfer_db_v2_to_v3(event, last_filter);
            }
            
            return enum_xfilter_handle_code_success;
        }

        enum_xfilter_handle_code xkeymigrate_t::transfer_db_v2_to_v3_transaction(xdbevent_t & event,xvfilter_t* last_filter) const
        {
            const auto & db_value = event.get_db_value();
            base::xauto_ptr<base::xvtxindex_t> txindex = new base::xvtxindex_t();
            int32_t ret = txindex->serialize_from_string(db_value);
            if (ret <= 0) {
                xerror("xkeymigrate_t::transfer_tx_v2_to_v3,fail txindex serialize");
                return enum_xfilter_handle_code_interrupt;
            }

            // load unit block from tx index
            base::xvaccount_t _unitvaccount(txindex->get_block_addr());

            std::string table_addr = base::xvaccount_t::make_table_account_address(_unitvaccount);
            if (table_addr == txindex->get_block_addr()) {
                xerror("xkeymigrate_t::transfer_tx_v2_to_v3,fail txindex addr already is table addr");
                return enum_xfilter_handle_code_interrupt;
            }

            uint64_t _unitheight = txindex->get_block_height();
            // XTODO only load main-entry bindex key, almost all nodes should can migrate successfully
            std::string unitbindex_key = xvdbkey_t::create_block_index_key(_unitvaccount, _unitheight);
            std::string unitbindex_bin = event.get_source_store()->get_value(unitbindex_key);
            if (unitbindex_bin.empty()) {
                xerror("xkeymigrate_t::transfer_tx_v2_to_v3,fail load unitbindex");
                return enum_xfilter_handle_code_interrupt;
            }
            base::xauto_ptr<base::xvbindex_t> unitbindex = new base::xvbindex_t();
            ret = unitbindex->serialize_from(unitbindex_bin);
            if (ret <= 0) {
                xerror("xkeymigrate_t::transfer_tx_v2_to_v3,fail unitbindex serialize");
                return enum_xfilter_handle_code_interrupt;
            }
            
            txindex->set_block_addr(table_addr);
            txindex->set_block_height(unitbindex->get_parent_block_height());
            if (unitbindex->get_height() > 0) {
                xassert(unitbindex->get_parent_block_height() > 0);
            }

            std::string new_txindex_bin;
            txindex->serialize_to_string(new_txindex_bin);
            event.get_target_store()->set_value(event.get_db_key(), new_txindex_bin);
            xdbg("xkeymigrate_t::transfer_tx_v2_to_v3 set_value_to_dst_db with new value.unit_addr=%s,unit_height=%ld,table_addr=%s,table_height=%ld",
                _unitvaccount.get_address().c_str(), _unitheight, table_addr.c_str(), unitbindex->get_parent_block_height());
            return enum_xfilter_handle_code_finish;  // process finish
        }

        enum_xfilter_handle_code xkeymigrate_t::transfer_db_v2_to_v3_bindex(xdbevent_t & event,xvfilter_t* last_filter) const
        {
            const auto & db_value = event.get_db_value();
            base::xauto_ptr<base::xvbindex_t> bindex = new base::xvbindex_t();
            int32_t ret = bindex->serialize_from(db_value);
            if (ret <= 0) {
                xerror("xkeymigrate_t::transfer_db_v2_to_v3_bindex,fail bindex serialize");
                return enum_xfilter_handle_code_interrupt;
            }            

            if (bindex->get_address().empty()) {
                xerror("xkeymigrate_t::transfer_db_v2_to_v3_bindex,fail bindex address empty");
                return enum_xfilter_handle_code_interrupt;
            }
            base::xvaccount_t _account(bindex->get_address());

            // transfer block bindex
            std::string new_bindex_key;
            if(bindex->check_store_flag(base::enum_index_store_flag_main_entry)) {
                new_bindex_key = xvdbkey_t::create_prunable_block_index_key(_account, bindex->get_height());
            } else {
                new_bindex_key = xvdbkey_t::create_prunable_block_index_key(_account, bindex->get_height(), bindex->get_viewid());
            }
            if (false == event.get_target_store()->set_value(new_bindex_key, db_value)) {
                xerror("xkeymigrate_t::transfer_db_v2_to_v3_bindex,fail set block index");
                return enum_xfilter_handle_code_interrupt;
            } else {
                xdbg("xkeymigrate_t::transfer_db_v2_to_v3_bindex set_value_to_dst_db block index with new value.bindex=%s,is_main_entry=%d,key=%s", bindex->dump().c_str(),bindex->check_store_flag(base::enum_index_store_flag_main_entry),new_bindex_key.c_str());
            }

            std::string _blockhash;
            // transfer block object key
            std::string old_object_key = xvdbkey_t::create_block_object_key(_account, bindex->get_block_hash());
            std::string object_value = event.get_source_store()->get_value(old_object_key);
            if (object_value.empty()) {
                xerror("xkeymigrate_t::transfer_db_v2_to_v3_bindex,fail load block object");
                return enum_xfilter_handle_code_interrupt;
            }
            std::string new_object_key = xvdbkey_t::create_prunable_block_object_key(_account, bindex->get_height(), bindex->get_viewid());
            if (false == event.get_target_store()->set_value(new_object_key, object_value)) {
                xerror("xkeymigrate_t::transfer_db_v2_to_v3_bindex,fail set block object");
                return enum_xfilter_handle_code_interrupt;
            } else {
                xdbg("xkeymigrate_t::transfer_db_v2_to_v3_bindex set_value_to_dst_db block object with new value.bindex=%s", bindex->dump().c_str());
            }

            // transfer block input/output key
            if (bindex->get_block_class() != base::enum_xvblock_class_nil) {
                std::string old_input_key = xvdbkey_t::create_block_input_resource_key(_account, bindex->get_block_hash());
                std::string input_value = event.get_source_store()->get_value(old_input_key);
                if (!input_value.empty()) {
                    std::string new_input_key = xvdbkey_t::create_prunable_block_input_resource_key(_account, bindex->get_height(), bindex->get_viewid());
                    if (false == event.get_target_store()->set_value(new_input_key, input_value)) {
                        xerror("xkeymigrate_t::transfer_db_v2_to_v3_bindex,fail set block input");
                        return enum_xfilter_handle_code_interrupt;
                    } else {
                        xdbg("xkeymigrate_t::transfer_db_v2_to_v3_bindex set_value_to_dst_db block input with new value.bindex=%s", bindex->dump().c_str());
                    }
                }
                std::string old_output_key = xvdbkey_t::create_block_output_resource_key(_account, bindex->get_block_hash());
                std::string output_value = event.get_source_store()->get_value(old_output_key);
                if (!output_value.empty()) {
                    std::string new_output_key = xvdbkey_t::create_prunable_block_output_resource_key(_account, bindex->get_height(), bindex->get_viewid());
                    if (false == event.get_target_store()->set_value(new_output_key, output_value)) {
                        xerror("xkeymigrate_t::transfer_db_v2_to_v3_bindex,fail set block output");
                        return enum_xfilter_handle_code_interrupt;
                    } else {
                        xdbg("xkeymigrate_t::transfer_db_v2_to_v3_bindex set_value_to_dst_db block output with new value.bindex=%s", bindex->dump().c_str());
                    }
                }
            }

            // try to transfer block state
            std::string old_state_key = xvdbkey_t::create_block_state_key(_account, bindex->get_block_hash());
            std::string state_value = event.get_source_store()->get_value(old_state_key);
            if (!state_value.empty()) {
                std::string new_state_key = xvdbkey_t::create_prunable_state_key(_account, bindex->get_height(), bindex->get_block_hash());
                if (false == event.get_target_store()->set_value(new_state_key, state_value)) {
                    xerror("xkeymigrate_t::transfer_db_v2_to_v3_bindex,fail set block state");
                    return enum_xfilter_handle_code_interrupt;
                } else {
                    xdbg("xkeymigrate_t::transfer_db_v2_to_v3_bindex set_value_to_dst_db block state with new value.bindex=%s", bindex->dump().c_str());
                }
            }
 
            return enum_xfilter_handle_code_finish;  // process finish
        }        

        enum_xfilter_handle_code xkeymigrate_t::transfer_db_v2_to_v3_meta(xdbevent_t & event,xvfilter_t* last_filter) const
        {
            const auto & db_key = event.get_db_key();

            std::vector<std::string> values;
            base::xstring_utl::split_string(db_key, '/', values);
            if (values.size() != 3) {
                xerror("xkeymigrate_t::transfer_db_v2_to_v3_meta,fail invalid meta key");
                return enum_xfilter_handle_code_interrupt;
            }
            std::string address = values[1];
            if (address.empty()) {
                xerror("xkeymigrate_t::transfer_db_v2_to_v3_meta,fail invalid address");
                return enum_xfilter_handle_code_interrupt;
            }

            base::xvaccount_t _vaddr(address);
            std::string new_meta_key = base::xvdbkey_t::create_account_meta_key(_vaddr);
            if (false == event.get_target_store()->set_value(new_meta_key, event.get_db_value())) {
                xerror("xkeymigrate_t::transfer_db_v2_to_v3_meta,fail set value");
                return enum_xfilter_handle_code_interrupt;
            } else {
                xdbg("xkeymigrate_t::transfer_db_v2_to_v3_meta set_value_to_dst_db meta with new value.addr=%s",address.c_str());
            }
            return enum_xfilter_handle_code_finish;  // process finish
        }

        enum_xfilter_handle_code xkeymigrate_t::transfer_db_v2_to_v3_span_height(xdbevent_t & event,xvfilter_t* last_filter) const
        {
            const auto & db_key = event.get_db_key();
            std::vector<std::string> values;
            base::xstring_utl::split_string(db_key, '/', values);
            if (values.size() != 2) {
                xerror("xkeymigrate_t::transfer_db_v2_to_v3_span_height,fail invalid span key");
                return enum_xfilter_handle_code_interrupt;
            }
            std::string xvid_str = values[1];
            std::string table_addr = get_addr_by_xvid(xvid_str);
            if (table_addr.empty()) {
                xerror("xkeymigrate_t::transfer_db_v2_to_v3_span_height,fail get addr by xvid");
                return enum_xfilter_handle_code_interrupt;
            }
            base::xvaccount_t _vaddr(table_addr);
            std::string new_key = base::xvdbkey_t::create_account_span_genesis_height_key(_vaddr);
            if (false == event.get_target_store()->set_value(new_key, event.get_db_value())) {
                xerror("xkeymigrate_t::transfer_db_v2_to_v3_span_height,fail set value");
                return enum_xfilter_handle_code_interrupt;
            } else {
                xdbg("xkeymigrate_t::transfer_db_v2_to_v3_span_height set_value_to_dst_db span height with new value.addr=%s",table_addr.c_str());
            }
            return enum_xfilter_handle_code_finish;  // process finish
        }

        enum_xfilter_handle_code xkeymigrate_t::transfer_db_v2_to_v3_span(xdbevent_t & event,xvfilter_t* last_filter) const
        {
            const auto & db_key = event.get_db_key();
            std::vector<std::string> values;
            base::xstring_utl::split_string(db_key, '/', values);
            if (values.size() != 4) {
                xerror("xkeymigrate_t::transfer_db_v2_to_v3_span,fail invalid span key");
                return enum_xfilter_handle_code_interrupt;
            }
            std::string xvid_str = values[1];
            std::string table_addr = get_addr_by_xvid(xvid_str);
            if (table_addr.empty()) {
                xerror("xkeymigrate_t::transfer_db_v2_to_v3_span,fail get addr by xvid");
                return enum_xfilter_handle_code_interrupt;
            }
            uint64_t height = base::xstring_utl::hex2uint64(values[3]);

            base::xvaccount_t _vaddr(table_addr);
            std::string new_key = base::xvdbkey_t::create_account_span_key(_vaddr, height);
            if (false == event.get_target_store()->set_value(new_key, event.get_db_value())) {
                xerror("xkeymigrate_t::transfer_db_v2_to_v3_span,fail set value");
                return enum_xfilter_handle_code_interrupt;
            } else {
                xdbg("xkeymigrate_t::transfer_db_v2_to_v3_span set_value_to_dst_db span with new value.addr=%s,height=%ld",table_addr.c_str(),height);
            }
            return enum_xfilter_handle_code_finish;  // process finish
        }

        enum_xfilter_handle_code xkeymigrate_t::transfer_db_v2_to_v3(xdbevent_t & event,xvfilter_t* last_filter) const
        {
            enum_xfilter_handle_code _code = enum_xfilter_handle_code_finish;
            auto key_type = event.get_db_key_type();
            if (key_type == base::enum_xdbkey_type_block_index) {
                _code = transfer_db_v2_to_v3_bindex(event, last_filter);
            } else if (key_type == base::enum_xdbkey_type_block_object 
                    || key_type == enum_xdbkey_type_block_input_resource 
                    || key_type == enum_xdbkey_type_block_output_resource
                    || key_type == enum_xdbkey_type_state_object) {
                // drop old block object/input/output/state
                _code = enum_xfilter_handle_code_finish;  // drop object
            } else if (key_type == base::enum_xdbkey_type_transaction) {
                _code = transfer_db_v2_to_v3_transaction(event, last_filter);
            } else if (key_type == base::enum_xdbkey_type_account_meta) {
                _code = transfer_db_v2_to_v3_meta(event, last_filter);
            } else if (key_type == base::enum_xdbkey_type_account_span) {
                _code = transfer_db_v2_to_v3_span(event, last_filter);
            } else if (key_type == base::enum_xdbkey_type_account_span_height) {
                _code = transfer_db_v2_to_v3_span_height(event, last_filter);
            } else {
                // others copy directly
                if (false == event.get_target_store()->set_value(event.get_db_key(), event.get_db_value())) {
                    xerror("xkeymigrate_t::transfer_db_v2_to_v3,fail set value");
                    _code = enum_xfilter_handle_code_interrupt;
                } else {
                    xdbg("xkeymigrate_t::transfer_db_v2_to_v3_bindex set_value_to_dst_db other with new value.");
                }
            }
            return _code;
        }
            
    }//end of namespace of base
}//end of namespace top
