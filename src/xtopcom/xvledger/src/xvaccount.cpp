// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../xvaccount.h"
#include "xbase/xcontext.h"
#include "xmetrics/xmetrics.h"

namespace top
{
    namespace base
    {
        xvaccount_t::xvaccount_t()
        {
            XMETRICS_GAUGE(metrics::dataobject_xvaccount, 1);
            m_account_xid = 0;
        }
    
        xvaccount_t::xvaccount_t(const std::string & account_address)
        {
            XMETRICS_GAUGE(metrics::dataobject_xvaccount, 1);
            m_account_addr  = account_address;
            m_account_xid   = get_xid_from_account(account_address);
            m_account_xid_str = xstring_utl::uint642hex(m_account_xid);
        }
        
        xvaccount_t::xvaccount_t(const xvaccount_t & obj)
        {
            XMETRICS_GAUGE(metrics::dataobject_xvaccount, 1);
            m_account_addr = obj.m_account_addr;
            m_account_xid  = obj.m_account_xid;
            m_account_xid_str = obj.m_account_xid_str;
        }
    
        xvaccount_t & xvaccount_t::operator = (const xvaccount_t & obj)
        {
            m_account_addr = obj.m_account_addr;
            m_account_xid  = obj.m_account_xid;
            m_account_xid_str = obj.m_account_xid_str;
            return *this;
        }
    
        xvaccount_t & xvaccount_t::operator = (const std::string & new_account_addr)
        {
            m_account_addr  = new_account_addr;
            m_account_xid   = get_xid_from_account(new_account_addr);
            m_account_xid_str = xstring_utl::uint642hex(m_account_xid);
            return *this;
        }
    
        xvaccount_t::~xvaccount_t()
        {
            XMETRICS_GAUGE(metrics::dataobject_xvaccount, -1);
        }
        
        xvactmeta_t*  xvactmeta_t::load(const std::string & meta_serialized_data)
        {
            if(meta_serialized_data.empty()) //check first
                return NULL;
            
            xvactmeta_t* meta_ptr = new xvactmeta_t();
            if(meta_ptr->serialize_from_string(meta_serialized_data) <= 0)
            {
                xerror("xacctmeta_t::load,bad meta_serialized_data that not follow spec");
                meta_ptr->release_ref();
                return NULL;
            }
            return meta_ptr;
        }
        
        xvactmeta_t::xvactmeta_t()
            :base::xdataobj_t(base::xdataunit_t::enum_xdata_type_vaccountmeta)
        {
    #if defined(ENABLE_METRICS)
            XMETRICS_GAUGE(metrics::dataobject_xacctmeta_t, 1);
    #endif
            _reserved_u16 = 0;
            _block_level  = (uint8_t)-1; //init to 255(that ensure is not allocated)
            _meta_spec_version = 1;     //version #1 now
            _highest_cert_block_height     = 0;
            _highest_lock_block_height     = 0;
            _highest_commit_block_height   = 0;
            _highest_execute_block_height  = 0;
            _highest_connect_block_height  = 0;
            _highest_full_block_height     = 0;
            _highest_genesis_connect_height= 0;
            _highest_sync_height           = 0;
        }
    
        xvactmeta_t::xvactmeta_t(const xvactmeta_t & obj)
            :base::xdataobj_t(base::xdataunit_t::enum_xdata_type_vaccountmeta)
        {
            *this = obj;
        }
    
        xvactmeta_t & xvactmeta_t::operator = (const xvactmeta_t & obj)
        {
            _highest_cert_block_height = obj._highest_cert_block_height;    //latest certificated block but not changed to lock/commit status
            _highest_lock_block_height = obj._highest_lock_block_height;    //latest locked block that not allow fork
            _highest_commit_block_height = obj._highest_commit_block_height;  //latest commited block to allow change state of account,like balance.
            _highest_execute_block_height = obj._highest_execute_block_height; //latest executed block that has executed and change state of account
            _highest_full_block_height = obj._highest_full_block_height;    //latest full-block height for this account
            _highest_connect_block_height = obj._highest_connect_block_height; //indicated the last block who is connected all the way to last full-block
            _highest_connect_block_hash = obj._highest_connect_block_hash;
            _highest_execute_block_hash = obj._highest_execute_block_hash;
            //reserved _lowest_genesis_connect_height to trune block
            _lowest_genesis_connect_height = obj._lowest_genesis_connect_height;  //[_lowest_genesis_connect_height,_highest_genesis_connect_height]
            _highest_genesis_connect_height = obj._highest_genesis_connect_height;//indicated the last block who is connected to genesis block
            _highest_genesis_connect_hash = obj._highest_genesis_connect_hash;
            _highest_sync_height = obj._highest_sync_height;           // higest continous block started from highest full table block
            
            _reserved_u16 = obj._reserved_u16;      //reserved for future
            _block_level = obj._block_level;       //set per block 'enum_xvblock_level,each account has unique level
            _meta_spec_version = obj._meta_spec_version; //add version control for compatible case
            
            return *this;
        }
    
        
        xvactmeta_t::~xvactmeta_t()
        {
    #if defined(ENABLE_METRICS)
            XMETRICS_GAUGE(metrics::dataobject_xacctmeta_t, -1);
    #endif
        }
        
        std::string xvactmeta_t::dump() const
        {
            char local_param_buf[256];
            return std::string(local_param_buf);
        }
        
        //caller respond to cast (void*) to related  interface ptr
        void*   xvactmeta_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == base::xdataunit_t::enum_xdata_type_vaccountmeta)
                return this;
            
            return base::xdataobj_t::query_interface(_enum_xobject_type_);
        }
        
        int32_t   xvactmeta_t::do_write(base::xstream_t & stream)//serialize whole object to binary
        {
            const int32_t begin_size = stream.size();
            
            stream << _highest_cert_block_height;
            stream << _highest_lock_block_height;
            stream << _highest_commit_block_height;
            stream << _highest_execute_block_height;
            stream << _highest_full_block_height;
            stream << _highest_connect_block_height;
            stream.write_tiny_string(_highest_connect_block_hash);
            stream.write_tiny_string(_highest_execute_block_hash);
            stream << _highest_genesis_connect_height;
            stream.write_tiny_string(_highest_genesis_connect_hash);
            stream << _highest_sync_height;
            
            //from here we introduce version control for meta
            stream << _meta_spec_version;
            stream << _block_level;
            stream << _reserved_u16;
            stream << _lowest_genesis_connect_height;
            
            return (stream.size() - begin_size);
        }
        
        int32_t   xvactmeta_t::do_read(base::xstream_t & stream)//serialize from binary and regeneate content
        {
            const int32_t begin_size = stream.size();
            
            stream >> _highest_cert_block_height;
            stream >> _highest_lock_block_height;
            stream >> _highest_commit_block_height;
            stream >> _highest_execute_block_height;
            stream >> _highest_full_block_height;
            stream >> _highest_connect_block_height;
            stream.read_tiny_string(_highest_connect_block_hash);
            stream.read_tiny_string(_highest_execute_block_hash);
            stream >> _highest_genesis_connect_height;
            stream.read_tiny_string(_highest_genesis_connect_hash);
            stream >> _highest_sync_height;
            
            stream >> _meta_spec_version;
            stream >> _block_level;
            stream >> _reserved_u16;
            stream >> _lowest_genesis_connect_height;
            
            if(stream.size() > 0) //still have data to read
            {
            }
            
            return (begin_size - stream.size());
        }
        
        //serialize vheader and certificaiton,return how many bytes is writed/read
        int32_t   xvactmeta_t::serialize_to_string(std::string & bin_data)   //wrap function fo serialize_to(stream)
        {
            base::xautostream_t<1024> _stream(base::xcontext_t::instance());
            const int result = xdataobj_t::serialize_to(_stream);
            if(result > 0)
                bin_data.assign((const char*)_stream.data(),_stream.size());
            return result;
        }
        
        int32_t   xvactmeta_t::serialize_from_string(const std::string & bin_data) //wrap function fo serialize_from(stream)
        {
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)bin_data.data(),(uint32_t)bin_data.size());
            const int result = xdataobj_t::serialize_from(_stream);
            return result;
        }
        
        std::string  xvactmeta_t::get_meta_path(base::xvaccount_t & _account)
        {
            std::string meta_path;
            meta_path.reserve(256);
            meta_path += base::xstring_utl::tostring(_account.get_chainid());
            meta_path += "/";
            meta_path += _account.get_account();
            meta_path += "/meta";
            
            return meta_path;
        }
    
    };//end of namespace of base
};//end of namespace of top
