// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "../xvaccount.h"
#include "../xvledger.h"
#include "xbase/xcontext.h"
#include "xmetrics/xmetrics.h"

namespace top
{
    namespace base
    {
        //convert to binary/bytes address with compact mode as for DB 'key
        const std::string  xvaccount_t::get_storage_key(const xvaccount_t & _account)
        {
            //sample of full address as "Tx0000[raw_public_addr]@[subledger]"
            const std::string& account_full_adress = _account.get_address();
            if (account_full_adress.size() < enum_vaccount_address_prefix_size) {
                xerror("xvaccount_t::get_storage_key fail,address=%s", account_full_adress.c_str());
                return std::string();
            }
            
            //step#1: extract the raw address of public key from full-address
            std::string raw_public_addr;
            const std::string::size_type end_raw_public_addr_pos = account_full_adress.find_last_of('@');
            if(end_raw_public_addr_pos == std::string::npos)//most cases
            {
                raw_public_addr = account_full_adress.substr(enum_vaccount_address_prefix_size);
            }
            else if(end_raw_public_addr_pos > enum_vaccount_address_prefix_size)
            {
                //note: string substr (size_t pos = 0, size_t len = npos); //[)
                raw_public_addr = account_full_adress.substr(enum_vaccount_address_prefix_size,end_raw_public_addr_pos - enum_vaccount_address_prefix_size);
            }
            else //exception case
            {
                raw_public_addr = account_full_adress.substr(enum_vaccount_address_prefix_size);
            }
            
            //step#2: decode hex to byte address for eth address
            enum_vaccount_addr_type addr_type = _account.get_addr_type();
            if (addr_type == enum_vaccount_addr_type_secp256k1_eth_user_account)
            {
                raw_public_addr = base::xstring_utl::from_hex(raw_public_addr);
            }
            
            //step#3: combine with ledger_id/subsubleder/raw_public_addr
            const std::string final_storage_key = xstring_utl::tostring(_account.get_ledger_id()) + "/" + xstring_utl::tostring(_account.get_ledger_subaddr()) + "/" + raw_public_addr;
            return final_storage_key;
        }
    
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
            //storage key
            m_account_store_key = xvaccount_t::get_storage_key(*this);
        }
        
        xvaccount_t::xvaccount_t(const xvaccount_t & obj)
        {
            XMETRICS_GAUGE(metrics::dataobject_xvaccount, 1);
            m_account_addr = obj.m_account_addr;
            m_account_xid  = obj.m_account_xid;
            m_account_xid_str = obj.m_account_xid_str;
            m_account_store_key  = obj.m_account_store_key;
        }
    
        xvaccount_t & xvaccount_t::operator = (const xvaccount_t & obj)
        {
            m_account_addr = obj.m_account_addr;
            m_account_xid  = obj.m_account_xid;
            m_account_xid_str = obj.m_account_xid_str;
            m_account_store_key  = obj.m_account_store_key;
            return *this;
        }
    
        xvaccount_t & xvaccount_t::operator = (const std::string & new_account_addr)
        {
            m_account_addr  = new_account_addr;
            m_account_xid   = get_xid_from_account(new_account_addr);
            m_account_xid_str = xstring_utl::uint642hex(m_account_xid);
            
            m_account_store_key = xvaccount_t::get_storage_key(*this);
            return *this;
        }
    
        xvaccount_t::~xvaccount_t()
        {
            XMETRICS_GAUGE(metrics::dataobject_xvaccount, -1);
        }

        std::string xvaccount_t::compact_address_to(const std::string & account_addr)
        {
            std::string compact_addr;
            enum_vaccount_addr_type addr_type = get_addrtype_from_account(account_addr);
            if (addr_type == enum_vaccount_addr_type_secp256k1_eth_user_account)
            {
                char compact_type = enum_vaccount_compact_type_eth_main_chain;
                xassert(ADDRESS_PREFIX_ETH_TYPE_IN_MAIN_CHAIN == account_addr.substr(0, enum_vaccount_address_prefix_size));
                compact_addr = account_addr.substr(enum_vaccount_address_prefix_size);
                compact_addr = base::xstring_utl::from_hex(compact_addr);
                return compact_type + compact_addr;
            }
            else
            {
                return account_addr;
            }
        }
        std::string xvaccount_t::compact_address_from(const std::string & data)
        {
            // the first byte is compact type
            enum_vaccount_compact_type compact_type = (enum_vaccount_compact_type)data[0];

            if (compact_type == enum_vaccount_compact_type_no_compact)
            {
                return data;
            }
            else if (compact_type == enum_vaccount_compact_type_eth_main_chain)
            {
                std::string compact_addr = data.substr(1);
                std::string account_addr;
                compact_addr = base::xstring_utl::to_hex(compact_addr);
                account_addr = ADDRESS_PREFIX_ETH_TYPE_IN_MAIN_CHAIN + compact_addr;
                return account_addr;
            }
            else
            {
                xassert(false);
                return {};
            }
        }
    
        bool  xvaccount_t::is_unit_address() const
        {
            if(   (get_addr_type() == enum_vaccount_addr_type_secp256k1_eth_user_account)
               || (get_addr_type() == enum_vaccount_addr_type_secp256k1_user_account) )
            {
                return true;
            }
            return false;
        }
        
        bool  xvaccount_t::is_table_address() const
        {
            //table address like "Ta0000@255"
            if(get_addr_type() == enum_vaccount_addr_type_block_contract) //table address
            {
                return true;
            }
            return false;
        }
        
        bool  xvaccount_t::is_contract_address() const
        {
            if(get_addr_type() == enum_vaccount_addr_type_native_contract)
            {
                return true;
            }
            return false;
        }

        //------------------------------------account meta-------------------------------------//
        xblockmeta_t::xblockmeta_t()
        {
            _lowest_vkey2_block_height     = (uint64_t)-1;//init as max value
            _highest_deleted_block_height  = 0;
            _highest_cert_block_height     = 0;
            _highest_lock_block_height     = 0;
            _highest_commit_block_height   = 0;
            _highest_connect_block_height  = 0;
            _highest_full_block_height     = 0;
            _block_level  = (uint8_t)-1; //init to 255(that ensure is not allocated)
        }
        
        xblockmeta_t::xblockmeta_t(const xblockmeta_t & obj)
        {
            *this = obj;
        }
        
        xblockmeta_t::xblockmeta_t(xblockmeta_t && move_obj)
        {
            *this = move_obj;
        }
        
        xblockmeta_t & xblockmeta_t::operator = (const xblockmeta_t & obj)
        {
            _lowest_vkey2_block_height      = obj._lowest_vkey2_block_height;
            _highest_deleted_block_height   = obj._highest_deleted_block_height;
            _highest_cert_block_height      = obj._highest_cert_block_height;
            _highest_lock_block_height      = obj._highest_lock_block_height;
            _highest_commit_block_height    = obj._highest_commit_block_height;
            _highest_full_block_height      = obj._highest_full_block_height;
            _highest_connect_block_height   = obj._highest_connect_block_height;
            _highest_connect_block_hash     = obj._highest_connect_block_hash;
            _block_level                    = obj._block_level;
            return *this;
        }
    
        bool    xblockmeta_t::operator == (const xblockmeta_t & obj) const
        {
            if(  (_highest_cert_block_height    != obj._highest_cert_block_height)
               ||(_highest_lock_block_height    != obj._highest_lock_block_height)
               ||(_highest_commit_block_height  != obj._highest_commit_block_height)
               ||(_highest_full_block_height    != obj._highest_full_block_height)
               ||(_highest_connect_block_height != obj._highest_connect_block_height)
               ||(_highest_deleted_block_height != obj._highest_deleted_block_height)
               ||(_lowest_vkey2_block_height    != obj._lowest_vkey2_block_height)
               ||(_highest_connect_block_hash   != obj._highest_connect_block_hash)
               ||(_block_level != obj._block_level) )
            {
                return false;
            }
            return true;
        }
        
        xblockmeta_t::~xblockmeta_t()
        {
        }
    
        const std::string xblockmeta_t::ddump() const
        {
            char local_param_buf[256];
            xprintf(local_param_buf,sizeof(local_param_buf),"{meta:height for cert=%" PRIu64 ",lock=%" PRIu64 ",commit=%" PRIu64 " ,connected=%" PRIu64 ",full=%" PRIu64 "}",(int64_t)_highest_cert_block_height,(int64_t)_highest_lock_block_height,(int64_t)_highest_commit_block_height,(int64_t)_highest_connect_block_height,_highest_full_block_height);
            
            return std::string(local_param_buf);
        }
    
        xsyncmeta_t::xsyncmeta_t()
        {
            _highest_genesis_connect_height = 0;
            _highest_sync_height            = 0;
        }
    
        xsyncmeta_t::xsyncmeta_t(const xsyncmeta_t & obj)
        {
            *this = obj;
        }
    
        xsyncmeta_t::xsyncmeta_t(xsyncmeta_t && move_obj)
        {
            *this = move_obj;
        }
    
        xsyncmeta_t & xsyncmeta_t::operator = (const xsyncmeta_t & obj)
        {
            _highest_genesis_connect_height = obj._highest_genesis_connect_height;
            _highest_genesis_connect_hash   = obj._highest_genesis_connect_hash;
            _highest_sync_height            = obj._highest_sync_height;
            return *this;
        }
    
        bool    xsyncmeta_t::operator == (const xsyncmeta_t & obj) const
        {
            if(  (_highest_sync_height            != obj._highest_sync_height)
               ||(_highest_genesis_connect_height != obj._highest_genesis_connect_height)
               ||(_highest_genesis_connect_hash   != obj._highest_genesis_connect_hash) )
            {
                return false;
            }
            return true;
        }
    
        xsyncmeta_t::~xsyncmeta_t()
        {
        }
    
        const std::string xsyncmeta_t::ddump() const
        {
            return std::string();
        }
    
        xstatemeta_t::xstatemeta_t()
        {
            _lowest_execute_block_height  = 0;
            _highest_execute_block_height = 0;
        }
    
        xstatemeta_t::xstatemeta_t(xstatemeta_t && move_obj)
        {
            _lowest_execute_block_height  = 0;
            _highest_execute_block_height = 0;
            *this = move_obj;
        }
        
        xstatemeta_t::xstatemeta_t(const xstatemeta_t & obj)
        {
            _lowest_execute_block_height  = 0;
            _highest_execute_block_height = 0;
            *this = obj;
        }
    
        xstatemeta_t & xstatemeta_t::operator = (const xstatemeta_t & obj)
        {
            _lowest_execute_block_height  = obj._lowest_execute_block_height;
            _highest_execute_block_height = obj._highest_execute_block_height;
            _highest_execute_block_hash   = obj._highest_execute_block_hash;
            return *this;
        }
    
        bool    xstatemeta_t::operator == (const xstatemeta_t & obj) const
        {
            if(  (_lowest_execute_block_height  != obj._lowest_execute_block_height)
               ||(_highest_execute_block_height != obj._highest_execute_block_height)
               ||(_highest_execute_block_hash   != obj._highest_execute_block_hash) )
            {
                return false;
            }
            return true;
        }
    
        xstatemeta_t::~xstatemeta_t()
        {
        }
    
        const std::string xstatemeta_t::ddump() const
        {
            return std::string();
        }
    
        xindxmeta_t::xindxmeta_t()
        {
            m_latest_unit_height = 0;
            m_latest_unit_viewid = 0;
            m_latest_tx_nonce    = 0;
            m_account_flag       = 0;
        }
        
        xindxmeta_t::xindxmeta_t(xindxmeta_t && move_obj)
        {
            m_latest_unit_height = 0;
            m_latest_unit_viewid = 0;
            m_latest_tx_nonce    = 0;
            m_account_flag       = 0;
            *this = move_obj;
        }
        
        xindxmeta_t::xindxmeta_t(const xindxmeta_t & obj)
        {
            m_latest_unit_height = 0;
            m_latest_unit_viewid = 0;
            m_latest_tx_nonce    = 0;
            m_account_flag       = 0;
            *this = obj;
        }
        
        xindxmeta_t & xindxmeta_t::operator = (const xindxmeta_t & obj)
        {
            m_latest_unit_height = obj.m_latest_unit_height;
            m_latest_unit_viewid = obj.m_latest_unit_viewid;
            m_latest_tx_nonce = obj.m_latest_tx_nonce;
            m_account_flag    = obj.m_account_flag;
            return *this;
        }
    
        bool    xindxmeta_t::operator == (const xindxmeta_t & obj) const
        {
            if(  (m_latest_unit_height != obj.m_latest_unit_height)
               ||(m_latest_unit_viewid != obj.m_latest_unit_viewid)
               ||(m_latest_tx_nonce    != obj.m_latest_tx_nonce)
               ||(m_account_flag       != obj.m_account_flag) )
            {
                return false;
            }
            return true;
        }
        
        xindxmeta_t::~xindxmeta_t()
        {
        }
        
        const std::string xindxmeta_t::ddump() const
        {
            return std::string();
        }
    
        xvactmeta_t*  xvactmeta_t::load(xvaccount_t & _account,const std::string & meta_serialized_data)
        {
            if(meta_serialized_data.empty()) //check first
                return new xvactmeta_t(_account); //return empty meta
            
            xvactmeta_t* meta_ptr = new xvactmeta_t(_account);
            if(meta_ptr->serialize_from_string(meta_serialized_data) <= 0)
            {
                xerror("xvactmeta_t::load,bad meta_serialized_data that not follow spec");
                meta_ptr->release_ref();//release bad object
                return new xvactmeta_t(_account); //return empty meta
            }
            return meta_ptr;
        }
        
        const std::string  xvactmeta_t::get_meta_path(xvaccount_t & _account)
        {
            std::string meta_path;
            meta_path.reserve(256);
            meta_path += xstring_utl::tostring(_account.get_chainid());
            meta_path += "/";
            meta_path += _account.get_account();
            meta_path += "/meta";
            
            return meta_path;
        }
        
        xvactmeta_t::xvactmeta_t(xvaccount_t & _account)
            :xdataobj_t(xdataunit_t::enum_xdata_type_vaccountmeta)
        {
            _meta_process_id = base::xvchain_t::instance().get_current_process_id();
            _meta_spec_version = 2;     //version #2 now

            #ifdef DEBUG
            m_account_address = _account.get_address();
            #else
            m_account_address = _account.get_xvid_str();
            #endif
        }
        
        xvactmeta_t::xvactmeta_t(xvactmeta_t && obj)
            :xdataobj_t(xdataunit_t::enum_xdata_type_vaccountmeta)
        {
            _meta_process_id = base::xvchain_t::instance().get_current_process_id();
            _meta_spec_version = 2;     //version #2 now
            
            *this = obj;
        }
    
        xvactmeta_t::xvactmeta_t(const xvactmeta_t & obj)
            :xdataobj_t(xdataunit_t::enum_xdata_type_vaccountmeta)
        {
            _meta_process_id = base::xvchain_t::instance().get_current_process_id();
            _meta_spec_version = 2;     //version #2 now
            
            *this = obj;
        }
    
        xvactmeta_t::~xvactmeta_t()
        {
        }
    
        std::string  xvactmeta_t::dump() const
        {
            return m_account_address + xblockmeta_t::ddump();
        }
 
        xvactmeta_t& xvactmeta_t::operator = (const xvactmeta_t & obj)
        {
            _meta_process_id = obj._meta_process_id;      //reserved for future
            _meta_spec_version = obj._meta_spec_version; //add version control for compatible case
            m_account_address = obj.m_account_address;
            
            xblockmeta_t::operator=(obj);
            xstatemeta_t::operator=(obj);
            xsyncmeta_t::operator=(obj);
            xindxmeta_t::operator=(obj);
            
            return *this;
        }
    
        void   xvactmeta_t::update_meta_process_id(const uint16_t _process_id)
        {
            if(_meta_process_id != _process_id)
                _meta_process_id = _process_id;
        }
    
        //APIs only open for  xvaccountobj_t object
        bool   xvactmeta_t::set_block_meta(const xblockmeta_t & new_meta)
        {
            if(  (new_meta._highest_cert_block_height    < _highest_cert_block_height)
               ||(new_meta._highest_lock_block_height    < _highest_lock_block_height)
               ||(new_meta._highest_commit_block_height  < _highest_commit_block_height)
               ||(new_meta._highest_full_block_height    < _highest_full_block_height)
               ||(new_meta._highest_connect_block_height < _highest_connect_block_height)
               ||(new_meta._highest_deleted_block_height < _highest_deleted_block_height) )
            {
                xerror("xvactmeta_t::set_block_meta,try overwrited newer_meta(%s) with old_meta( %s)",xblockmeta_t::ddump().c_str(),new_meta.ddump().c_str());
                return false; //try to overwrited newer version
            }
            else if(xblockmeta_t::operator==(new_meta))
            {
                return false;
            }
            
            xblockmeta_t::operator=(new_meta);
            add_modified_count();
            return true;
        }
    
        bool   xvactmeta_t::set_state_meta(const xstatemeta_t & new_meta)
        {
            if(   (new_meta._highest_execute_block_height < _highest_execute_block_height)
               || (new_meta._lowest_execute_block_height  < _lowest_execute_block_height) )
            {
                xerror("xvactmeta_t::set_state_meta,try overwrited newer_meta(%s) with old_meta( %s)",xstatemeta_t::ddump().c_str(),new_meta.ddump().c_str());
                return false;
            }
            else if(xstatemeta_t::operator==(new_meta))
            {
                return false;
            }

            xstatemeta_t::operator=(new_meta);
            add_modified_count();
            return true;
        }
    
        bool   xvactmeta_t::set_index_meta(const xindxmeta_t & new_meta)
        {
            if(  (new_meta.m_latest_unit_height  < m_latest_unit_height)
               ||(new_meta.m_latest_unit_viewid  < m_latest_unit_viewid)
               ||(new_meta.m_latest_tx_nonce     < m_latest_tx_nonce) )
            {
                xerror("xvactmeta_t::set_index_meta,try overwrited newer_meta(%s) with old_meta( %s)",xindxmeta_t::ddump().c_str(),new_meta.ddump().c_str());
                return false;
            }
            else if(xindxmeta_t::operator==(new_meta))
            {
                return false;
            }
            
            xindxmeta_t::operator=(new_meta);
            add_modified_count();
            return true;
        }
    
        bool   xvactmeta_t::set_sync_meta(const xsyncmeta_t & new_meta)
        {
            if(  (new_meta._highest_genesis_connect_height < _highest_genesis_connect_height)
               ||(new_meta._highest_sync_height            < _highest_sync_height) )
            {
                xerror("xvactmeta_t::set_sync_meta,try overwrited newer_meta(%s) with old_meta( %s)",xsyncmeta_t::ddump().c_str(),new_meta.ddump().c_str());
                return false;
            }
            else if(xsyncmeta_t::operator==(new_meta))
            {
                return false;
            }
                
            xsyncmeta_t::operator=(new_meta);
            add_modified_count();
            return true;
        }
    
        bool xvactmeta_t::set_latest_executed_block(const uint64_t height, const std::string & blockhash)
        {
            if(height < _highest_execute_block_height)
            {
                // TODO(jimmy) it may happen when set executed height from statestore with multi-threads
                xwarn("xvactmeta_t::set_latest_executed_block,try overwrited _highest_execute_block_height(%llu) with old_meta(%llu)",_highest_execute_block_height,height);
                return false;
            }
            
            if( (height != _highest_execute_block_height) || (blockhash != _highest_execute_block_hash) )
            {
                _highest_execute_block_height = height;
                _highest_execute_block_hash   = blockhash;
                add_modified_count();
                return true;
            }
            return false;
        }
    
        bool   xvactmeta_t::set_latest_deleted_block(const uint64_t height)
        {
            if(height <= _highest_deleted_block_height)
            {
                if(height < _highest_deleted_block_height)
                    xwarn("xvactmeta_t::set_latest_deleted_block,try overwrited _highest_deleted_block_height(%llu) with old_meta(%llu)",_highest_deleted_block_height,height);
                return false;
            }
            base::xatomic_t::xstore(_highest_deleted_block_height,height);
            add_modified_count();
            return true;
        }
    
        const xblockmeta_t   xvactmeta_t::clone_block_meta() const
        {
            return *this;
        }
        xblockmeta_t& xvactmeta_t::get_block_meta()
        {
            return *this;
        }
    
        const xstatemeta_t   xvactmeta_t::clone_state_meta() const
        {
            return *this;
        }
        xstatemeta_t& xvactmeta_t::get_state_meta()
        {
            return *this;
        }

        const xindxmeta_t    xvactmeta_t::clone_index_meta() const
        {
            return *this;
        }
        xindxmeta_t&  xvactmeta_t::get_index_meta()
        {
            return *this;
        }

        const xsyncmeta_t   xvactmeta_t::clone_sync_meta()  const
        {
            return *this;
        }
        xsyncmeta_t&  xvactmeta_t::get_sync_meta()
        {
            return *this;
        }
         
        //caller respond to cast (void*) to related  interface ptr
        void*   xvactmeta_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == xdataunit_t::enum_xdata_type_vaccountmeta)
                return this;
            
            return xdataobj_t::query_interface(_enum_xobject_type_);
        }

        int32_t   xvactmeta_t::do_write(xstream_t & stream)//serialize whole object to binary
        {
            const int32_t begin_size = stream.size();
            
            const uint16_t cur_process_id = (uint16_t)base::xvchain_t::instance().get_current_process_id();
 
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
            stream << cur_process_id;
            stream << _highest_deleted_block_height;
            
            //keep above unchanged and compatible with old format
            
            //added since version#2 of _meta_spec_version
            if(_meta_spec_version >= 2)
            {
                stream.write_compact_var(m_latest_unit_height);
                stream.write_compact_var(m_latest_unit_viewid);
                stream.write_compact_var(m_latest_tx_nonce);
                stream.write_compact_var(m_account_flag);
                
                stream.write_compact_var(_lowest_execute_block_height);
                stream.write_compact_var(_lowest_vkey2_block_height);
            }
            
            return (stream.size() - begin_size);
        }
    
        int32_t   xvactmeta_t::do_read(xstream_t & stream)//serialize from binary and regeneate content
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
            stream >> _meta_process_id;
            stream >> _highest_deleted_block_height;
            
            //keep above unchanged and compatible with old format
            
            if(_meta_spec_version >= 2)//since version#2
            {
                stream.read_compact_var(m_latest_unit_height);
                stream.read_compact_var(m_latest_unit_viewid);
                stream.read_compact_var(m_latest_tx_nonce);
                stream.read_compact_var(m_account_flag);
                
                stream.read_compact_var(_lowest_execute_block_height);
                stream.read_compact_var(_lowest_vkey2_block_height);
            }
 
            return (begin_size - stream.size());
        }
        
    };//end of namespace of base
};//end of namespace of top
