// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include <sstream>
#include "../xvaccount.h"
#include "../xvledger.h"
#include "xbase/xcontext.h"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xmetrics/xmetrics.h"

namespace top
{
    namespace base
    {
        //full_ledger = (ledger_id & 0xFFFF) + (subaddr_of_ledger & 0xFF)
            //ledger_id= [chain_id:12bit][zone_index:4bit]
            //[8bit:subaddr_of_ledger] = [5 bit:book-index]-[3 bit:table-index]
        const std::string  fullledger_to_storageid(const xvid_t xvid_addr)//pick fixed 24bit as storage_id
        {
            char szBuff[32] = {0};
            const int inBufLen = sizeof(szBuff);
            const uint64_t storage_id = (get_vledger_ledger_id(xvid_addr) << 8) | get_vledger_subaddr(xvid_addr);
            snprintf(szBuff,inBufLen,"%6llx", (long long unsigned int)storage_id);
            return std::string(szBuff);//align 24bit as fixed size
        }
    
        //convert to binary/bytes address with compact mode as for DB 'key
        const std::string  xvaccount_t::get_storage_key(const xvaccount_t & _account)
        {            
            xassert(!_account.get_address().empty());
            //sample of full address as "Tx0000[raw_public_addr]@[subledger]"
         
            //ledger_id= [chain_id:12bit][zone_index:4bit]
            //[10bit:subaddr_of_ledger] = [7 bit:book-index]-[3 bit:table-index]
            //step#3: combine with ledger_id/subsubleder/raw_public_addr
            const std::string final_storage_key = fullledger_to_storageid(_account.get_xvid()) + "/" + xvaccount_t::compact_address_to(_account.get_address());
            return final_storage_key;
        }
    
        xvaccount_t::xvaccount_t()
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvaccount, 1);
            m_account_xid = 0;
        }
    
        xvaccount_t::xvaccount_t(const std::string & account_address)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvaccount, 1);
            m_account_addr  = account_address;
            m_account_xid   = get_xid_from_account(account_address);
            //storage key
            m_account_store_key = xvaccount_t::get_storage_key(*this);
        }
        
        xvaccount_t::xvaccount_t(const xvaccount_t & obj)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvaccount, 1);
            m_account_addr = obj.m_account_addr;
            m_account_xid  = obj.m_account_xid;
            m_account_store_key  = obj.m_account_store_key;
        }
    
        xvaccount_t & xvaccount_t::operator = (const xvaccount_t & obj)
        {
            m_account_addr = obj.m_account_addr;
            m_account_xid  = obj.m_account_xid;
            m_account_store_key  = obj.m_account_store_key;
            return *this;
        }
    
        xvaccount_t & xvaccount_t::operator = (const std::string & new_account_addr)
        {
            m_account_addr  = new_account_addr;
            m_account_xid   = get_xid_from_account(new_account_addr);
            
            m_account_store_key = xvaccount_t::get_storage_key(*this);
            return *this;
        }
    
        xvaccount_t::~xvaccount_t()
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvaccount, -1);
        }

        std::string xvaccount_t::get_xvid_str()const {
            // XTODO xvid str is used for old db key, should be removed later
            return xstring_utl::uint642hex(m_account_xid);
        }

        std::string xvaccount_t::compact_address_to(const std::string & account_addr)
        {
            std::string compact_addr;
            enum_vaccount_addr_type addr_type = get_addrtype_from_account(account_addr);
            if (base::xvaccount_t::is_eth_address_type(addr_type))
            {
                char compact_type;
                if (addr_type == enum_vaccount_addr_type_secp256k1_eth_user_account) {
                    compact_type = enum_vaccount_compact_type_eth_main_chain;
                    xassert(ADDRESS_PREFIX_ETH_TYPE_IN_MAIN_CHAIN == account_addr.substr(0, enum_vaccount_address_prefix_size));
                } else {
                    compact_type = enum_vaccount_compact_type_evm_main_chain;
                    xassert(ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN == account_addr.substr(0, enum_vaccount_address_prefix_size));
                }
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
            } else if (compact_type == enum_vaccount_compact_type_evm_main_chain) {
                std::string compact_addr = data.substr(1);
                std::string account_addr;
                compact_addr = base::xstring_utl::to_hex(compact_addr);
                account_addr = ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN + compact_addr;
                return account_addr;
            }
            else
            {
                xassert(false);
                return {};
            }
        }

        const uint32_t xvaccount_t::get_index_from_account(const std::string & account_addr) {
            XMETRICS_GAUGE(metrics::cpu_hash_64_calc, 1);
            return (uint32_t)xhash64_t::digest(account_addr);//hash64 better performance than hash32
        }

        const xvid_t  xvaccount_t::get_xid_from_account(const std::string & account_addr) {
            uint32_t account_index        = 0;
            uint32_t ledger_id            = 0;
            uint16_t ledger_subaddr       = 0;
            if(get_ledger_fulladdr_from_account(account_addr,ledger_id,ledger_subaddr,account_index))
            {
                xvid_t _xid_from_addr = (ledger_id << 16) | ((ledger_subaddr & enum_vbucket_has_tables_count_mask) << 6) | enum_xid_type_xledger;
                _xid_from_addr |= (((uint64_t)account_index) << 32);
                return _xid_from_addr; //as default not include account'hash index as performance consideration
            }
            return 0; //invalid account
        }
        std::string xvaccount_t::to_evm_address(const std::string& account) {
            if (account.size() < 2)
                return "";
            std::string value;
            value.resize(account.size());
            std::transform(account.begin(), account.end(), value.begin(), ::tolower);
            value = std::string(base::ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN) + value.substr(2);
            return value;
        }

        bool xvaccount_t::is_user_address_type(enum_vaccount_addr_type addr_type) {
            return (addr_type == enum_vaccount_addr_type_secp256k1_eth_user_account || addr_type == enum_vaccount_addr_type_secp256k1_user_account ||
                    addr_type == enum_vaccount_addr_type_secp256k1_evm_user_account);
        }
        bool xvaccount_t::is_eth_address_type(enum_vaccount_addr_type addr_type) {
            return (addr_type == enum_vaccount_addr_type_secp256k1_eth_user_account || addr_type == enum_vaccount_addr_type_secp256k1_evm_user_account);
        }
        bool xvaccount_t::is_table_address_type(enum_vaccount_addr_type addr_type) {
            return (addr_type == enum_vaccount_addr_type_block_contract);
        }
        bool xvaccount_t::is_contract_address_type(enum_vaccount_addr_type addr_type) {
            return (addr_type == enum_vaccount_addr_type_native_contract);
        }
        bool xvaccount_t::is_unit_address_type(enum_vaccount_addr_type addr_type) {
            return is_user_address_type(addr_type) || is_contract_address_type(addr_type);
        }        
        bool xvaccount_t::is_drand_address_type(enum_vaccount_addr_type addr_type) {
            return (addr_type == enum_vaccount_addr_type_drand);
        }
        bool xvaccount_t::is_timer_address_type(enum_vaccount_addr_type addr_type) {
            return (addr_type == enum_vaccount_addr_type_timer);
        }

        bool xvaccount_t::check_address(const std::string & account_addr, bool isTransaction)
        {
            // check address min-length and max-length
            if(account_addr.size() <= enum_vaccount_address_prefix_size || account_addr.size() > enum_vaccount_address_max_size) {
                xwarn("xvaccount_t::check_address fail-too short address,size=%zu", account_addr.size());
                return false;
            }

            // check if first-char is T
            char first_type = account_addr[0];
            if (first_type != 'T') {
                xwarn("xvaccount_t::check_address fail-not top addr");
                return false;
            }

            // check parts number
            std::vector<std::string> parts;
            int parts_num = base::xstring_utl::split_string(account_addr,'@',parts);
            if (parts_num > 2) {
                xwarn("xvaccount_t::check_address fail-too many parts %d", parts_num);
                return false;
            }

            // check addr type
            enum_vaccount_addr_type addr_type = get_addrtype_from_account(account_addr);
            switch (addr_type) {
                case enum_vaccount_addr_type_root_account:
                case enum_vaccount_addr_type_black_hole:
                case enum_vaccount_addr_type_timer:
                case enum_vaccount_addr_type_drand:
                case enum_vaccount_addr_type_secp256k1_user_account:
                case enum_vaccount_addr_type_secp256k1_eth_user_account:
                case enum_vaccount_addr_type_secp256k1_evm_user_account:
                    if (parts_num != 1) {
                        xwarn("xvaccount_t::check_address fail-addr type and parts num mismatch. type=%d,parts_num=%d", addr_type, parts_num);
                        return false;
                    }
                    break;
                case enum_vaccount_addr_type_native_contract:
                    if (isTransaction) {
                        std::string strLedgrid = account_addr.substr(2, 4);
                        if (strLedgrid == std::string("0000")) { // shard constracts
                            if (parts_num != 2 && parts_num != 1) {
                                xwarn("xvaccount_t::check_address fail-addr type and parts num mismatch. isTransaction:%d type=%d,parts_num=%d,ledgrid:%s", isTransaction, addr_type, parts_num, strLedgrid.c_str());
                                return false;
                            }
                        } else {
                            if (parts_num != 2) {
                                xwarn("xvaccount_t::check_address fail-addr type and parts num mismatch. isTransaction:%d type=%d,parts_num=%d,ledgrid:%s", isTransaction, addr_type, parts_num, strLedgrid.c_str());
                                return false;
                            }
                        }
                    } else {
                        if (parts_num != 2) {
                            xwarn("xvaccount_t::check_address fail-addr type and parts num mismatch. isTransaction:%d type=%d,parts_num=%d", isTransaction, addr_type, parts_num);
                            return false;
                        }
                    }
                    break;
                case enum_vaccount_addr_type_block_contract:
                    if (parts_num != 2) {
                        xwarn("xvaccount_t::check_address fail-addr type and parts num mismatch. type=%d,parts_num=%d", addr_type, parts_num);
                        return false;
                    }
                    break;
                default:
                    xwarn("xvaccount_t::check_address fail-invalid type. type=%d", addr_type);
                    return false;
            }

            // check subaddr
            if (parts_num > 1) {
                std::string subaddr_str = parts[1];
                if (false == base::xstring_utl::digital_string(subaddr_str)) {
                    xwarn("xvaccount_t::check_address fail-subaddr not digital.");
                    return false;
                }
                int32_t subaddr_int32 = base::xstring_utl::toint32(subaddr_str);

                std::string strLedgrid = account_addr.substr(2, 4);

                for (size_t i = 0; i < strLedgrid.size(); i++) {
                    if (('0' > strLedgrid[i]) || (('9' < strLedgrid[i]) && ('a' > strLedgrid[i])) || ('f' < strLedgrid[i])) {
                        xwarn("xvaccount_t::check_address ledger id address not all hex char");
                        return false;
                    }
                }

                uint16_t ledger_id = base::xvaccount_t::get_ledgerid_from_account(account_addr);
                // table addr judge subaddr size
                if (addr_type == enum_vaccount_addr_type_block_contract || addr_type == enum_vaccount_addr_type_native_contract) {
                    if (!valid_zone_and_subaddr((enum_xchain_zone_index)ledger_id, subaddr_int32)){
                         xwarn("xvaccount_t::check_address fail-subaddr scope invalid.addr_type=%d,subaddr=%d", addr_type, subaddr_int32);
                         return false;
                    }
                }
            }

            // check T8 eth address
            if (addr_type == enum_vaccount_addr_type_secp256k1_eth_user_account ||
                addr_type == enum_vaccount_addr_type_secp256k1_evm_user_account) {
                std::string addrtemp = account_addr.substr(enum_vaccount_address_prefix_size);
                // check if all lower char
                std::string addrtemp2 = addrtemp;
                base::xstring_utl::tolower_string(addrtemp2);
                if (addrtemp != addrtemp2) {
                    xwarn("xvaccount_t::check_address fail-has upper char");
                    return false;
                }

                // check if all hex char
                for (size_t i = 0; i < addrtemp2.size(); i++) {
                    if (('0' > addrtemp2[i]) || (('9' < addrtemp2[i]) && ('a' > addrtemp2[i])) || ('f' < addrtemp2[i])) {
                        xwarn("xvaccount_t::check_address T8 eth address not all hex char");
                        return false;
                    }
                }

                // check ledger_id
                std::string strLedgrid = account_addr.substr(2, 4);

                for (size_t i = 0; i < strLedgrid.size(); i++) {
                    if (('0' > strLedgrid[i]) || (('9' < strLedgrid[i]) && ('a' > strLedgrid[i])) || ('f' < strLedgrid[i])) {
                        xwarn("xvaccount_t::check_address ledger id address not all hex char");
                        return false;
                    }
                }
                uint16_t ledger_id = base::xvaccount_t::get_ledgerid_from_account(account_addr);
                if (addr_type == enum_vaccount_addr_type_secp256k1_eth_user_account) {
                    if (ledger_id != enum_chain_zone_consensus_index && ledger_id != enum_chain_zone_beacon_index && ledger_id != enum_chain_zone_zec_index &&
                        ledger_id != enum_chain_zone_relay_index) {
                        xwarn("xvaccount_t::check_address T8 eth address ledger id error");
                        return false;
                    }
                } else {
                    if (ledger_id != enum_chain_zone_evm_index) {
                        xwarn("xvaccount_t::check_address T6 eth address ledger id error");
                        return false;
                    }
                }
            }
            return true;
        }

        bool xvaccount_t::valid_zone_and_subaddr(enum_xchain_zone_index zone_index, uint16_t subaddr) {
            if (zone_index == enum_chain_zone_consensus_index) {
                if (subaddr < 0 || subaddr >= enum_vbucket_has_tables_count) {
                    return false;
                }
            // root beacon contracts addr judge subaddr size 
            } else if (zone_index == enum_chain_zone_beacon_index) {
                if (subaddr < 0 || static_cast<uint32_t>(subaddr) >= MAIN_CHAIN_REC_TABLE_USED_NUM) {
                    return false;
                }
            // sub beacon contracts addr judge subaddr size 
            } else if (zone_index == enum_chain_zone_zec_index) {
                if (subaddr < 0 || static_cast<uint32_t>(subaddr) >= MAIN_CHAIN_ZEC_TABLE_USED_NUM) {
                    return false;
                }
            } else if (zone_index == enum_chain_zone_evm_index) {
                if (subaddr < 0 || static_cast<uint32_t>(subaddr) >= MAIN_CHAIN_EVM_TABLE_USED_NUM) {
                    return false;
                }
            } else if (zone_index == enum_chain_zone_relay_index) {
                if (subaddr < 0 || static_cast<uint32_t>(subaddr) >= MAIN_CHAIN_RELAY_TABLE_USED_NUM) {
                    return false;
                }
            } else {
                assert(false);
                return false;
            }
            return true;
        }
    
        bool  xvaccount_t::is_unit_address() const
        {
            return is_unit_address_type(get_addr_type());
        }
        bool  xvaccount_t::is_user_address() const
        {
            return is_user_address_type(get_addr_type());
        }
        bool xvaccount_t::is_eth_address() const
        {
            return is_eth_address_type(get_addr_type());
        }
        
        bool  xvaccount_t::is_table_address() const
        {
            //table address like "Ta0000@255"
            return is_table_address_type(get_addr_type());
        }
        
        bool  xvaccount_t::is_contract_address() const
        {
            return is_contract_address_type(get_addr_type());
        }

        bool  xvaccount_t::is_timer_address() const
        {
            return is_timer_address_type(get_addr_type());
        }
        
        bool  xvaccount_t::is_drand_address() const
        {
            return is_drand_address_type(get_addr_type());
        }
        bool  xvaccount_t::is_relay_address() const
        {
            return (get_addr_type() == enum_vaccount_addr_type_relay_block);
        }
        bool  xvaccount_t::has_valid_table_addr() const {
            return valid_zone_and_subaddr((enum_xchain_zone_index)get_zone_index(), (uint16_t)get_ledger_subaddr());
        }

        //------------------------------------account meta-------------------------------------//
        xblockmeta_t::xblockmeta_t()
        {
            _lowest_vkey2_block_height     = 0;
            _highest_deleted_block_height  = 0;
            _highest_cert_block_height     = 0;
            _highest_lock_block_height     = 0;
            _highest_commit_block_height   = 0;
            _highest_connect_block_height  = 0;
            _highest_cp_connect_block_height = 0;
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
            _highest_cp_connect_block_height= obj._highest_cp_connect_block_height;
            _highest_cp_connect_block_hash  = obj._highest_cp_connect_block_hash;
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
               ||(_highest_cp_connect_block_height != obj._highest_cp_connect_block_height)
               ||(_highest_cp_connect_block_hash   != obj._highest_cp_connect_block_hash)
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
            xprintf(local_param_buf,sizeof(local_param_buf),"{meta:height for cert=%" PRIu64 ",lock=%" PRIu64 ",commit=%" PRIu64 " ,connected=%" PRIu64 ",cp_connected=%" PRIu64 ",full=%" PRIu64 ",deleted=%" PRIu64 ",vkey2=%" PRIu64 "}",
            (int64_t)_highest_cert_block_height,(int64_t)_highest_lock_block_height,(int64_t)_highest_commit_block_height,(int64_t)_highest_connect_block_height,(int64_t)_highest_cp_connect_block_height,(int64_t)_highest_full_block_height,(int64_t)_highest_deleted_block_height,(int64_t)_lowest_vkey2_block_height);
            
            return std::string(local_param_buf);
        }
    
        xsyncmeta_t::xsyncmeta_t()
        {
            _highest_genesis_connect_height = 0;
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
            return *this;
        }
    
        bool    xsyncmeta_t::operator == (const xsyncmeta_t & obj) const
        {
            if( (_highest_genesis_connect_height != obj._highest_genesis_connect_height) )
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
            return *this;
        }
    
        bool    xstatemeta_t::operator == (const xstatemeta_t & obj) const
        {
            if(  (_lowest_execute_block_height  != obj._lowest_execute_block_height)
               ||(_highest_execute_block_height != obj._highest_execute_block_height) )
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
        
        xvactmeta_t::xvactmeta_t(const xvaccount_t & _account)
            :xdataobj_t(xdataunit_t::enum_xdata_type_vaccountmeta)
        {
            init_version_control();
            _meta_process_id = base::xvchain_t::instance().get_current_process_id();
            _highest_saved_block_height = 0;
            //XTODO,remove below assert when related xbase checked in main-branch
            xassert(__XBASE_MAIN_VERSION_CODE__ >= 1);
            xassert(__XBASE_FEATURE_VERSION_CODE__ >= 3);
            xassert(__XBASE_MINOR_VERSION_CODE__ >= 8);
        }
        
        xvactmeta_t::xvactmeta_t(xvactmeta_t && obj)
            :xdataobj_t(xdataunit_t::enum_xdata_type_vaccountmeta)
        {
            init_version_control();
            _meta_process_id = base::xvchain_t::instance().get_current_process_id();
            _highest_saved_block_height = 0;
            *this = obj;
        }
    
        xvactmeta_t::xvactmeta_t(const xvactmeta_t & obj)
            :xdataobj_t(xdataunit_t::enum_xdata_type_vaccountmeta)
        {
            init_version_control();
            _meta_process_id = base::xvchain_t::instance().get_current_process_id();
            _highest_saved_block_height = 0;
            *this = obj;
        }
    
        xvactmeta_t::~xvactmeta_t()
        {
        }
    
        std::string  xvactmeta_t::dump() const
        {
            std::stringstream ss;
            ss << ",cert=" << xblockmeta_t::_highest_cert_block_height;
            ss << ",lock=" << xblockmeta_t::_highest_lock_block_height;
            ss << ",commit=" << xblockmeta_t::_highest_commit_block_height;
            ss << ",connect=" << xblockmeta_t::_highest_connect_block_height;
            ss << ",cp=" << xblockmeta_t::_highest_cp_connect_block_height;
            ss << ",full=" << xblockmeta_t::_highest_full_block_height;
            ss << ",delete=" << xblockmeta_t::_highest_deleted_block_height;
            ss << ",vkey2=" << xblockmeta_t::_lowest_vkey2_block_height;
            ss << ",h_exec=" << xstatemeta_t::_highest_execute_block_height;
            ss << ",l_exec=" << xstatemeta_t::_lowest_execute_block_height;
            return ss.str();

            // return m_account_address + xblockmeta_t::ddump();
        }
 
        xvactmeta_t& xvactmeta_t::operator = (const xvactmeta_t & obj)
        {
            _meta_process_id = obj._meta_process_id;      //reserved for future
            _meta_spec_version = obj._meta_spec_version; //add version control for compatible case
            _highest_saved_block_height = obj._highest_saved_block_height;
            
            xblockmeta_t::operator=(obj);
            xstatemeta_t::operator=(obj);
            xsyncmeta_t::operator=(obj);
            
            return *this;
        }
    
        void   xvactmeta_t::update_highest_saved_block_height(const uint64_t new_height)
        {
            if(new_height > _highest_saved_block_height)
                _highest_saved_block_height = new_height;
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
               ||(new_meta._highest_cp_connect_block_height < _highest_cp_connect_block_height)
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
    
        bool   xvactmeta_t::set_sync_meta(const xsyncmeta_t & new_meta)
        {
            if(  (new_meta._highest_genesis_connect_height < _highest_genesis_connect_height) )
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
    
        bool xvactmeta_t::set_latest_executed_block(const uint64_t height)
        {
            if(height < _highest_execute_block_height)
            {
                // TODO(jimmy) it may happen when set executed height from statestore with multi-threads
                xwarn("xvactmeta_t::set_latest_executed_block,try overwrited _highest_execute_block_height(%llu) with old_meta(%llu)",_highest_execute_block_height,height);
                return false;
            }
            
            if(height != _highest_execute_block_height)
            {
                _highest_execute_block_height = height;
                // _highest_execute_block_hash   = blockhash; // XTODO no need set execute block hash

                // _lowest_execute_block_height rewirted by set_lowest_executed_block only. after we use unit state prune.
                // const uint32_t safe_distance = XGET_CONFIG(fulltable_interval_block_num) * 10;
                // if (_highest_execute_block_height > safe_distance) {
                //     _lowest_execute_block_height = _highest_execute_block_height - safe_distance;
                // }
                add_modified_count();
                return true;
            }
            return false;
        }

        bool xvactmeta_t::set_lowest_executed_block(const uint64_t height)
        {
            if(height < _lowest_execute_block_height)
            {
                // TODO(jimmy) it may happen when set executed height from statestore with multi-threads
                xwarn("xvactmeta_t::set_latest_executed_block,try overwrited _lowest_execute_block_height(%llu) with old_meta(%llu)",_lowest_execute_block_height,height);
                return false;
            }
            
            if(height != _lowest_execute_block_height)
            {
                _lowest_execute_block_height = height;
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

        int32_t  xvactmeta_t::serialize_from(xstream_t & stream)
        {
            // override serialize_from for set enum_xdata_flag_fragment flag, otherwise it may be modified by xdataobj_t deserialize
            int32_t ret = xdataobj_t::serialize_from(stream);
            init_version_control();
            return ret;
        }

        void   xvactmeta_t::init_version_control()
        {
            //borrow enum_xdata_flag_fragment to tell wheher using compact mode to serialization
            set_unit_flag(enum_xdata_flag_fragment);
            _meta_spec_version = 3;     //version #3 now
        }

        int32_t   xvactmeta_t::do_write(xstream_t & stream)//serialize whole object to binary
        {
            const int32_t begin_size = stream.size();
            const uint16_t cur_process_id = (uint16_t)base::xvchain_t::instance().get_current_process_id();

            xassert(check_unit_flag(enum_xdata_flag_fragment) == true);
            xassert(_meta_spec_version == 3);

            //borrow enum_xdata_flag_fragment to tell wheher using compact mode to serialization
            // if(check_unit_flag(enum_xdata_flag_fragment) == false)//old format
            // {
            //     stream << _highest_cert_block_height;
            //     stream << _highest_lock_block_height;
            //     stream << _highest_commit_block_height;
            //     stream << _highest_execute_block_height;
            //     stream << _highest_full_block_height;
            //     stream << _highest_connect_block_height;
            //     stream.write_tiny_string(_highest_connect_block_hash);
            //     stream.write_tiny_string(_highest_execute_block_hash);
            //     stream << _highest_genesis_connect_height;
            //     stream.write_tiny_string(_highest_genesis_connect_hash);
            //     stream << _highest_sync_height;

            //     //from here we introduce version control for meta
            //     stream << _meta_spec_version;
            //     stream << _block_level;
            //     stream << cur_process_id;
            //     stream << _highest_deleted_block_height;
                
            //     //keep above unchanged and compatible with old format
                
            //     //added since version#2 of _meta_spec_version
            //     if(_meta_spec_version >= 2)
            //     {
            //         stream.write_compact_var(m_latest_unit_height);
            //         stream.write_compact_var(m_latest_unit_viewid);
            //         stream.write_compact_var(m_latest_tx_nonce);
            //         stream.write_compact_var(m_account_flag);
                    
            //         stream.write_compact_var(_lowest_execute_block_height);
            //         stream.write_compact_var(_lowest_vkey2_block_height);
            //     }
            // }
            // else //new compact mode
            // {
                stream << _meta_spec_version;
                stream << _block_level;
                stream << cur_process_id;
                
                stream.write_compact_var(_highest_cert_block_height);
                stream.write_compact_var(_highest_lock_block_height);
                stream.write_compact_var(_highest_commit_block_height);
                stream.write_compact_var(_highest_full_block_height);
                stream.write_compact_var(_highest_deleted_block_height);
                stream.write_compact_var(_lowest_vkey2_block_height);
                stream.write_compact_var(uint64_t(0));//XTODO no use members _highest_sync_height
                
                stream.write_compact_var(_lowest_execute_block_height);
                stream.write_compact_var(_highest_execute_block_height);
                stream.write_compact_var(std::string());//XTODO no use members,_highest_execute_block_hash

                stream.write_compact_var(_highest_connect_block_height);
                stream.write_compact_var(_highest_connect_block_hash);

                stream.write_compact_var(_highest_genesis_connect_height);
                stream.write_compact_var(std::string());//XTODO no use members,_highest_genesis_connect_hash
                
                stream.write_compact_var(uint64_t(0));//XTODO no use members m_latest_unit_height
                stream.write_compact_var(uint64_t(0));//XTODO no use members m_latest_unit_viewid
                stream.write_compact_var(uint64_t(0));//XTODO no use members m_latest_tx_nonce
                stream.write_compact_var(uint16_t(0));//XTODO no use members m_account_flag

                stream.write_compact_var(_highest_cp_connect_block_height);
                stream.write_compact_var(_highest_cp_connect_block_hash);
            // }
        
            return (stream.size() - begin_size);
        }

        void xvactmeta_t::update_cp_connect(const uint64_t cp_connect_height, const std::string & cp_connect_hash)
        {
            if (cp_connect_height > _highest_cp_connect_block_height) {
                _highest_cp_connect_block_height = cp_connect_height;
                _highest_cp_connect_block_hash = cp_connect_hash;
            }
            else if (cp_connect_height == _highest_cp_connect_block_height)
            {
                if (_highest_cp_connect_block_height == 0 && _highest_cp_connect_block_hash.empty() && !cp_connect_hash.empty())
                {
                    _highest_cp_connect_block_hash = cp_connect_hash;
                }
                else if (_highest_cp_connect_block_hash != cp_connect_hash)
                {
                    xerror("update_cp_connect hash mismatch height:%llu,db:%s,cp:%s",
                            cp_connect_height, base::xstring_utl::to_hex(cp_connect_hash).c_str(), base::xstring_utl::to_hex(_highest_cp_connect_block_hash).c_str());
                }
            }
            else
            {
                xinfo("update_cp_connect checkpoint:%llu fresher than meta:%llu", _highest_cp_connect_block_height, cp_connect_height);
            }
        }

        int32_t   xvactmeta_t::do_read(xstream_t & stream)//serialize from binary and regeneate content
        {
            const int32_t begin_size = stream.size();
            uint64_t temp64;
            uint16_t temp16;
            std::string tempstr;
            //borrow enum_xdata_flag_fragment to tell wheher using compact mode to serialization
            if(check_unit_flag(enum_xdata_flag_fragment) == false)//old format
            {
                stream >> _highest_cert_block_height;
                stream >> _highest_lock_block_height;
                stream >> _highest_commit_block_height;
                stream >> _highest_execute_block_height;
                stream >> _highest_full_block_height;
                stream >> _highest_connect_block_height;
                stream.read_tiny_string(_highest_connect_block_hash);
                stream.read_tiny_string(tempstr);//XTODO no use members _highest_execute_block_hash
                stream >> _highest_genesis_connect_height;
                stream.read_tiny_string(tempstr);//XTODO no use members _highest_genesis_connect_hash
                stream >> temp64;//XTODO no use members _highest_sync_height
                
                stream >> _meta_spec_version;
                stream >> _block_level;
                stream >> _meta_process_id;
                stream >> _highest_deleted_block_height;
                
                //keep above unchanged and compatible with old format
                
                if(_meta_spec_version >= 2)//since version#2
                {
                    stream.read_compact_var(temp64);//m_latest_unit_height
                    stream.read_compact_var(temp64);//m_latest_unit_viewid
                    stream.read_compact_var(temp64);//m_latest_tx_nonce
                    stream.read_compact_var(temp16);//m_account_flag
                    
                    stream.read_compact_var(_lowest_execute_block_height);
                    stream.read_compact_var(_lowest_vkey2_block_height);
                }

                // XTODO restore to default value for old db
                if ( (0 != _lowest_vkey2_block_height) || (0 != _highest_deleted_block_height) )
                {
                    xwarn("xvactmeta_t::do_read restore default value._lowest_vkey2_block_height=%ld,_highest_deleted_block_height=%ld",_lowest_vkey2_block_height,_highest_deleted_block_height);
                    _lowest_vkey2_block_height     = 0;
                    _highest_deleted_block_height  = 0;
                    add_modified_count();
                }
            }
            else //new compact mode
            {
                stream >> _meta_spec_version;
                stream >> _block_level;
                stream >> _meta_process_id;
                
                stream.read_compact_var(_highest_cert_block_height);
                stream.read_compact_var(_highest_lock_block_height);
                stream.read_compact_var(_highest_commit_block_height);
                stream.read_compact_var(_highest_full_block_height);
                stream.read_compact_var(_highest_deleted_block_height);
                stream.read_compact_var(_lowest_vkey2_block_height);
                stream.read_compact_var(temp64);//XTODO no use members _highest_sync_height
                
                stream.read_compact_var(_lowest_execute_block_height);
                stream.read_compact_var(_highest_execute_block_height);                
                stream.read_compact_var(tempstr);//XTODO no use members _highest_execute_block_hash
                
                stream.read_compact_var(_highest_connect_block_height);
                stream.read_compact_var(_highest_connect_block_hash);

                stream.read_compact_var(_highest_genesis_connect_height);
                stream.read_compact_var(tempstr);//XTODO no use members _highest_genesis_connect_hash
                
                stream.read_compact_var(temp64);//m_latest_unit_height
                stream.read_compact_var(temp64);//m_latest_unit_viewid
                stream.read_compact_var(temp64);//m_latest_tx_nonce
                stream.read_compact_var(temp16);//m_account_flag

                if(_meta_spec_version >= 3)
                {
                    uint64_t cp_connect_height = 0;
                    stream.read_compact_var(cp_connect_height);
                    std::string cp_connect_hash;
                    stream.read_compact_var(cp_connect_hash);
                    update_cp_connect(cp_connect_height, cp_connect_hash);
                }
            }
            
            //init by saved meta
            if(_highest_saved_block_height < _highest_cert_block_height)
                _highest_saved_block_height = _highest_cert_block_height;

            init_version_control();  // reinit version control

            return (begin_size - stream.size());
        }
        
    };//end of namespace of base
};//end of namespace of top
