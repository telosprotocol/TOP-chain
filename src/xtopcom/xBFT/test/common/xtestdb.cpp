// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtestdb.hpp"

//#define __delete_db_data__

namespace top
{
    namespace test
    {
        xstoredb_t::xstoredb_t(const std::string & store_path)
        {
            m_total_keys = 0;
            m_cur_clock_store = 0;
            m_cur_data_store  = 0;
            m_store_path = store_path;
        }
        xstoredb_t::~xstoredb_t()
        {
        }
        
        bool             xstoredb_t::set_vblock(const std::string & store_path,base::xvblock_t* block_ptr)
        {
            if(NULL == block_ptr)
                return false;
            
            const std::string _input_path    = store_path + block_ptr->get_input_path();
            set_value(_input_path,block_ptr->get_input()->get_resources_data());
            
            const std::string _output_path    = store_path + block_ptr->get_output_path();
            set_value(_output_path,block_ptr->get_output()->get_resources_data());
            
            std::string       _header_cert_bin;
            const std::string _header_cert_path  = store_path + block_ptr->get_header_path();
            block_ptr->serialize_to_string(_header_cert_bin);
            return set_value(_header_cert_path,_header_cert_bin);
        }
    
        bool            xstoredb_t::set_vblock_header(const std::string & store_path,base::xvblock_t* block_ptr)
        {
            std::string       _header_cert_bin;
            const std::string _header_cert_path  = store_path + block_ptr->get_header_path();
            block_ptr->serialize_to_string(_header_cert_bin);
            return set_value(_header_cert_path,_header_cert_bin);
        }
        
        base::xvblock_t* xstoredb_t::get_vblock(const std::string & store_path,const std::string & account, const uint64_t height) const
        {
            const std::string _header_cert_path = store_path + base::xvblock_t::create_header_path(account, height);
            const std::string _header_cert_bin  = get_value(_header_cert_path);
            if(_header_cert_bin.empty())
            {
                if(height != 0)
                    xwarn("xstoredb_t::get_vblock,not found block'header&cert object at %s",_header_cert_bin.c_str());
                return NULL;
            }
            base::xvblock_t* block_ptr = base::xvblock_t::create_block_object(_header_cert_bin);
            if(block_ptr != NULL)
            {
                if(block_ptr->get_block_class() != base::enum_xvblock_class_nil)
                {
                    const std::string full_input_path = store_path + block_ptr->get_input_path();
                    const std::string _input_content = get_value(full_input_path);
                    if(false == _input_content.empty())
                    {
                        block_ptr->set_input_resources(_input_content);
                    }
                    const std::string full_output_path = store_path + block_ptr->get_output_path();
                    const std::string _output_content = get_value(full_output_path);
                    if(false == _output_content.empty())
                    {
                        block_ptr->set_output_resources(_output_content);
                    }
                }
            }
            return block_ptr;
        }
  
        base::xvblock_t* xstoredb_t::get_vblock_header(const std::string & store_path,const std::string & account,const uint64_t height) const
        {
            const std::string _header_cert_path = store_path + base::xvblock_t::create_header_path(account, height);
            const std::string _header_cert_bin  = get_value(_header_cert_path);
            if(_header_cert_bin.empty())
            {
                if(height != 0)
                    xwarn("xstoredb_t::get_vblock_header,not found block'header&cert object at %s",_header_cert_bin.c_str());
                return NULL;
            }
            return base::xvblock_t::create_block_object(_header_cert_bin);
        }
        bool             xstoredb_t::get_vblock_input(const std::string & store_path,base::xvblock_t* block_ptr)  const
        {
            if(NULL == block_ptr)
                return false;
            
            if(block_ptr->get_block_class() != base::enum_xvblock_class_nil)
            {
                const std::string full_input_path = store_path + block_ptr->get_input_path();
                const std::string _input_content = get_value(full_input_path);
                if(false == _input_content.empty())
                {
                    block_ptr->set_input_resources(_input_content);
                }
            }
            return true;
        }
        bool             xstoredb_t::get_vblock_output(const std::string & store_path,base::xvblock_t* block_ptr) const
        {
            if(NULL == block_ptr)
                return false;
            
            if(block_ptr->get_block_class() != base::enum_xvblock_class_nil)
            {
                const std::string full_output_path = store_path + block_ptr->get_output_path();
                const std::string _output_content = get_value(full_output_path);
                if(false == _output_content.empty())
                {
                    block_ptr->set_output_resources(_output_content);
                }
            }
            return true;
        }
        
        bool             xstoredb_t::execute_block(base::xvblock_t* block)
        {
            return true;
        }
        
        bool             xstoredb_t::set_value(const std::string & key, const std::string& value)
        {
            ++m_total_keys;
        
            if(key.find("/meta") != std::string::npos)
            {
                m_meta_store[key] = value;
            }
            else if( (key.find("Tt00013axZ3Gy8nzi7oNYhTBDb9XMb8KHdqYhw4Kx") != std::string::npos)
               || (key.find("3axZ3Gy8nzi7oNYhTBDb9XMb8KHdqYhw4Kx") != std::string::npos)
               || (key.find("71c216d000013404") != std::string::npos)
               || (key.find("71c216d000010404") != std::string::npos) )
            {
                m_clock_store[key] = value;
                xdbg("xstoredb_t::store clock key(%s)",key.c_str());
            }
            else
            {
                //xdbg("xstoredb_t::store block key(%s)",key.c_str());
                m_dumy_store[key] = value;
            }
            
            return true;
        }
        bool             xstoredb_t::delete_value(const std::string & key)
        {
            if( (key.find("Tt00013axZ3Gy8nzi7oNYhTBDb9XMb8KHdqYhw4Kx") != std::string::npos)
               || (key.find("3axZ3Gy8nzi7oNYhTBDb9XMb8KHdqYhw4Kx") != std::string::npos)
               || (key.find("71c216d000013404") != std::string::npos)
               || (key.find("71c216d000010404") != std::string::npos) )
            {
                auto it = m_clock_store.find(key);
                if(it != m_clock_store.end())
                {
                    m_clock_store.erase(it);
                    xdbg("xstoredb_t::delete clock key(%s) at store#1",key.c_str());
                    return true;
                }
            }
            else
            {
                auto it = m_dumy_store.find(key);
                if(it != m_dumy_store.end())
                {
                    m_dumy_store.erase(it);
                    xdbg("xstoredb_t::delete raw key(%s) at store#1",key.c_str());
                    return true;
                }
            }
            return true;
        }
        const std::string  xstoredb_t::get_value(const std::string & key) const
        {
            if(key.empty())
            {
                return std::string();
            }
            if(key.find("/meta") != std::string::npos)
            {
                auto it = m_meta_store.find(key);
                if(it != m_meta_store.end())
                    return it->second;
            }
            else if( (key.find("Tt00013axZ3Gy8nzi7oNYhTBDb9XMb8KHdqYhw4Kx") != std::string::npos)
               || (key.find("3axZ3Gy8nzi7oNYhTBDb9XMb8KHdqYhw4Kx") != std::string::npos)
               || (key.find("71c216d000013404") != std::string::npos)
               || (key.find("71c216d000010404") != std::string::npos) )
            {
                auto it = m_clock_store.find(key);
                if(it != m_clock_store.end())
                {
                    xdbg("xstoredb_t::get_value clock key(%s) at store#1",key.c_str());
                    return it->second;
                }
                xdbg("xstoredb_t::get_value faild to find clock key(%s)",key.c_str());
            }
            else
            {
                auto it = m_dumy_store.find(key);
                if(it != m_dumy_store.end())
                    return it->second;
            }
            
            return std::string();
        }
    
        bool  xstoredb_t::delete_values(std::vector<std::string> & to_deleted_keys)
        {
            for(auto & key : to_deleted_keys)
            {
                delete_value(key);
            }
            return true;
        }
        
        //prefix must start from first char of key
        bool  xstoredb_t::read_range(const std::string& prefix, std::vector<std::string>& values)
        {
            for(auto it = m_dumy_store.begin(); it != m_dumy_store.end(); ++it)
            {
                if(it->first.find(prefix) != std::string::npos)
                {
                    values.push_back(it->second);
                }
            }
            return (values.empty() == false);
        }
 
        //note:begin_key and end_key must has same style(first char of key)
        bool  xstoredb_t::delete_range(const std::string & begin_key,const std::string & end_key)
        {
            auto start_pos = m_dumy_store.end();
            auto end_pos   = m_dumy_store.end();
            for(auto it = m_dumy_store.begin(); it != m_dumy_store.end(); ++it)
            {
                if(start_pos == m_dumy_store.end())
                {
                    if(it->first.find(begin_key) != std::string::npos)
                    {
                        start_pos = it;
                    }
                }
                else if(end_pos == m_dumy_store.end())
                {
                    if(it->first.find(end_key) != std::string::npos)
                    {
                        end_pos = it;
                    }
                }
                else
                {
                    break;
                }

            }
            if(start_pos == m_dumy_store.end())
            {
                xwarn("xstoredb_t::delete_range faild to find key(%s)",begin_key.c_str());
                return false;
            }
            
            if(end_pos == m_dumy_store.end())
            {
                xwarn("xstoredb_t::delete_range faild to find key(%s)",end_key.c_str());
                return false;
            }
            
            m_dumy_store.erase(start_pos, end_pos);//[first,last)
            xinfo("xstoredb_t::delete_range,now size=%d after erase[%s - %s)",(int)m_dumy_store.size(),begin_key.c_str(),end_key.c_str());
            return true;
        }
        
        bool  xstoredb_t::single_delete(const std::string & target_key)//key must be readonly(never update after PUT),otherwise the behavior is undefined
        {
            return delete_value(target_key);
        }
    
        void   xveventbus_impl::push_event(const mbus::xevent_ptr_t& e)
        {
        }
        
        mbus::xevent_ptr_t  xveventbus_impl::create_event_for_store_index_to_db(base::xvbindex_t * target_block)
        {
            //return mbus::xevent_ptr_t(new mbus::xevent_t(0));
            return nullptr;
        }
        
        mbus::xevent_ptr_t  xveventbus_impl::create_event_for_revoke_index_to_db(base::xvbindex_t * target_index)
        {
            return nullptr;
        }
        
        mbus::xevent_ptr_t  xveventbus_impl::create_event_for_store_block_to_db(base::xvblock_t * target_block)
        {
            //return mbus::xevent_ptr_t(new mbus::xevent_t(0));
            return nullptr;
        }
        mbus::xevent_ptr_t  xveventbus_impl::create_event_for_store_committed_block(base::xvbindex_t * target_index)
        {
            return nullptr;
        }
    };
};
