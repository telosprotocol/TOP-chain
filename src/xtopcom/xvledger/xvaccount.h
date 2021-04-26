// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbase/xhash.h"
#include "xbase/xns_macro.h"
#include "xbase/xobject.h"
#include "xbase/xutl.h"

namespace top
{
    namespace base
    {
        // account space is divided into netid#->zone#(aka bucket#)->book#->table#->account#
        /* each network has unique ledger
         enum enum_vledger_const
         {
            enum_vledger_has_buckets_count       = 16,     //4bit: max 16 buckets(aka zones) of each ledger
            enum_vledger_has_zones_count         = enum_vledger_has_buckets_count
         
            enum_vbucket_has_books_count         = 128,    //7bit: each bucket has max 128 books
            enum_vbook_has_tables_count          = 8,      //3bit: each book has max 8 tables
            enum_vbucket_has_tables_count        = 1024,   //total 1024 = 128 * 8 under one bucket/zone
         };
         */
        
        enum enum_vaccount_addr_type
        {
            enum_vaccount_addr_type_invalid                     =  0,
            
            enum_vaccount_addr_type_root_account                = '$',
            enum_vaccount_addr_type_black_hole                  = '!',
            enum_vaccount_addr_type_timer                       = 't',
            enum_vaccount_addr_type_clock                       = 't', //clock cert
            enum_vaccount_addr_type_drand                       = 'r', //drand cert
            
            
            ////////////////////Edward25519 generated key->accoun///////////////////////////////////////////
            enum_vaccount_addr_type_ed25519_user_account        = 'A',  //Edward25519 generated key->account
            enum_vaccount_addr_type_ed25519_user_sub_account    = 'B',  //Edward25519 generated key->account
            
            //note: reserve 'C'--'Z' here
            enum_vaccount_addr_type_ed25519_reserved_start      = 'C',
            enum_vaccount_addr_type_ed25519_reserved_end        = 'Z',
            
            ////////////////////secp256k1 generated key->accoun/////////////////////////////////////////////
            enum_vaccount_addr_type_secp256k1_user_account      = '0',  //secp256k1 generated key->account
            enum_vaccount_addr_type_secp256k1_user_sub_account  = '1',  //secp256k1 generated key->account
            enum_vaccount_addr_type_native_contract             = '2',  //secp256k1 generated key->account
            enum_vaccount_addr_type_custom_contract             = '3',  //secp256k1 generated key->account
            
            enum_vaccount_addr_type_block_contract              = 'a',  //secp256k1 generated key->account
        };
        
        //each chain has max 16 zones/buckets, define as below
        enum enum_xchain_zone_index
        {
            enum_chain_zone_consensus_index   = 0,  //for consesnus
            enum_chain_zone_beacon_index      = 1,  //for beacon
            enum_chain_zone_zec_index         = 2,  //for election
            
            enum_chain_zone_archive_index     = 14, //for archive nodes
            enum_chain_zone_edge_index        = 15, //for edge nodes
        };
        //define common rules for chain_id
        enum enum_xchain_id
        {
            enum_main_chain_id          = 0,      //main chain for TOP asset
            enum_rootbeacon_chain_id    = 128,    //root beacon of TOP platform
            
            enum_test_chain_id          = 255,    //for test purpose
            
            //service_chain_id defined as below ,that must >= 256
            enum_service_chain_id_start_reserved = 256,
        };
        
        using xtable_shortid_t = uint16_t;//note: short table_id = [zone_index][book_index][table_index]
        using xtable_longid_t = uint32_t;//note: long table_id = [chain_id][zone_index][book_index][table_index]
        
        class xvaccount_t : virtual public xrefcount_t
        {
        public:
            enum enum_vaccount_address_size
            {
                enum_vaccount_address_min_size  = 18, //(>20,<256)
                enum_vaccount_address_max_size  = 256,//(>20,<256)
            };
        public: //create account address of blockchain
            //account format as = T-[type|ledger_id]-public_key_address-subaddr_of_ledger(book# and table#,optional)
            //note: zone_index:range of [0,15], book_index: range of [0,127], table_index: range of [0,7]
            //and chain_id same as net_id of definition as XIP,but valid range for xchain is limited as [0,4095]
            static const std::string  make_account_address(enum_vaccount_addr_type addr_type,enum_xchain_id chain_id, enum_xchain_zone_index zone_index,const uint8_t book_index,const uint8_t table_index,const std::string & public_key_address)//subaddr_of_zone is optional
            {
                const uint16_t ledger_id = make_ledger_id(chain_id, zone_index);
                return make_account_address(addr_type,ledger_id,book_index,table_index,public_key_address);
            }
            static const std::string  make_account_address(enum_vaccount_addr_type addr_type,const uint16_t ledger_id,const uint8_t book_index,const uint8_t table_index,const std::string & public_key_address)//subaddr_of_zone is optional
            {
                const uint16_t subaddr_of_ledger = make_subaddr_of_ledger(book_index,table_index);
                return make_account_address(addr_type,ledger_id,public_key_address,subaddr_of_ledger);
            }
            //format T[Type:1char][ledger-id:4chars][public address][sub_address]
            static const std::string  make_account_address(enum_vaccount_addr_type addr_type,const uint16_t ledger_id,const std::string & public_key_address,uint16_t subaddr_of_ledger = (uint16_t)-1)//subaddr_of_ledger is optional
            {
                if((int)addr_type <= 0)//not allow empty
                {
                    xassert(0);//should not happen
                    return std::string();
                }
                char prefix_chars[32] = {0};
                snprintf(prefix_chars, sizeof(prefix_chars), "%c", (const char)addr_type);
                std::string prefix_string(prefix_chars);
                
                const std::string szledgerid = xstring_utl::uint642hex(ledger_id);//must be 1-4 hex char since ledger_id must <= 65535(0xFFFF)
                if(szledgerid.size() < 4)
                {
                    prefix_string.append(4-szledgerid.size(), '0');
                }
                prefix_string.append(szledgerid);

                std::string final_account_address;
                if(subaddr_of_ledger >= enum_vbucket_has_tables_count) //regular account or case for invalid subaddr_of_ledger
                {
                    final_account_address = (std::string("T") + prefix_string + public_key_address);
                }
                else //for system account that has pre-defined subaddress for some contracts
                {
                    final_account_address = (std::string("T") + prefix_string + public_key_address + "@" + xstring_utl::tostring(subaddr_of_ledger));//@ indicate that not have verfication function
                }
                const int account_address_size = (int)final_account_address.size();
                xassert(account_address_size > enum_vaccount_address_min_size);
                xassert(account_address_size < enum_vaccount_address_max_size);
                return final_account_address;
            }
            //just for make native address and block address with '@'
            static const std::string  make_account_address(const std::string & prefix, uint16_t subaddr)
            {
                xassert(subaddr < enum_vbucket_has_tables_count);
                std::string final_account_address = prefix + "@" + xstring_utl::tostring(subaddr);
                xassert(get_addrtype_from_account(final_account_address) == enum_vaccount_addr_type_native_contract || get_addrtype_from_account(final_account_address) == enum_vaccount_addr_type_block_contract);
                return final_account_address;
            }
            //just for make native address and block address with '@'
            static bool get_prefix_subaddr_from_account(const std::string & account_addr, std::string & prefix, uint16_t & subaddr)
            {
                std::vector<std::string> parts;
                if(xstring_utl::split_string(account_addr,'@',parts) >= 2)//valid account address
                {
                    prefix = parts[0];
                    subaddr = (uint16_t)(xstring_utl::toint32(parts[1]) & enum_vbucket_has_tables_count_mask);
                    return true;
                }
                xassert(0);
                return false;
            }
            //
            static bool get_public_address_from_account(const std::string & account_addr, std::string & public_address)
            {
                std::string prefix;
                std::vector<std::string> parts;
                if(xstring_utl::split_string(account_addr,'@',parts) >= 2)//valid account address
                {
                    prefix = parts[0];
                }
                else
                {
                    prefix = account_addr;
                }
                public_address = prefix.substr(6);//always 6 hex chars
                return true;
            }
            //account format as = T[Type:1char][ledger-id:4chars][public address][sub_address(optional)]
            static enum_vaccount_addr_type get_addrtype_from_account(const std::string & account_addr)
            {
                char _addr_type = 0; //0 is invalid
                if(account_addr.size() > enum_vaccount_address_min_size) //at least 24 cahrs
                    _addr_type = account_addr.at(1);

                xassert(_addr_type != 0);
                return (enum_vaccount_addr_type)_addr_type;
            }
            //account format as = T[Type:1char][ledger-id:4chars][public address][sub_address(optional)]
            static const uint16_t get_ledgerid_from_account(const std::string & account_addr)
            {
                uint16_t  ledger_id = 0;//0 is valid and default value
                const int account_address_size = (int)account_addr.size();
                if( (account_address_size > enum_vaccount_address_min_size) && (account_address_size < enum_vaccount_address_max_size) )
                {
                    const std::string string_ledger_id = account_addr.substr(2,4);//always 4 hex chars
                    ledger_id = (uint16_t)xstring_utl::hex2uint64(string_ledger_id);

                    return ledger_id;
                }
                xassert(0);
                return 0;
            }
            static bool  get_type_and_ledgerid_from_account(uint8_t & addr_type,uint16_t & ledger_id,const std::string & account_addr)
            {
                addr_type  = 0; //0 is invalid
                ledger_id   = 0; //0 is valid and default value
                
                const int account_address_size = (int)account_addr.size();
                if( (account_address_size > enum_vaccount_address_min_size) && (account_address_size < enum_vaccount_address_max_size) )
                {
                    addr_type = get_addrtype_from_account(account_addr);
                    ledger_id = get_ledgerid_from_account(account_addr);
                    return true;
                }
                xassert(0);
                return false;
            }
            
            //ledger_id= [chain_id:12bit][zone_index:4bit]
            static const uint16_t  make_ledger_id(enum_xchain_id chain_id,enum_xchain_zone_index zone_index)
            {
                uint16_t ledger_id = chain_id << 4; //max is 12bit
                ledger_id |= (uint16_t)(zone_index & 0xF);
                return ledger_id;
            }
            //ledger_id= [chain_id:12bit][zone_index:4bit]
            static const uint16_t get_chainid_from_ledgerid(const uint16_t ledger_id)
            {
                return (ledger_id >> 4);
            }
            static const uint16_t get_netid_from_ledgerid(const uint16_t ledger_id)
            {
                return (ledger_id >> 4);
            }
            //ledger_id= [chain_id:12bit][zone_index:4bit]
            static const uint8_t  get_zoneindex_from_ledgerid(const uint16_t ledger_id)
            {
                return (uint8_t)(ledger_id & 0xF);
            }
            //[10bit:subaddr_of_ledger] = [7 bit:book-index]-[3 bit:table-index]
            static const uint16_t make_subaddr_of_ledger(const uint8_t book_index,const uint8_t table_index)
            {
                const uint16_t subaddr_of_ledger = (((uint16_t)book_index & enum_vbucket_has_books_count_mask) << 3) | (table_index & 0x07);
                return subaddr_of_ledger;
            }
            static uint16_t       get_ledgersubaddr_from_account(const std::string & account_addr)
            {
                uint32_t account_index        = 0;
                uint32_t ledger_id            = 0;
                uint16_t ledger_subaddr       = 0; //0 is valid and default value
                get_ledger_fulladdr_from_account(account_addr,ledger_id,ledger_subaddr,account_index);
                return ledger_subaddr;
            }
            static const uint8_t get_book_index_from_subaddr(const uint16_t subaddr_of_zone)
            {
                return (uint8_t)((subaddr_of_zone >> 3) & enum_vbucket_has_books_count_mask);
            }
            static const uint8_t get_table_index_from_subaddr(const uint16_t subaddr_of_zone)
            {
                return (uint8_t)(subaddr_of_zone & 0x07);
            }
            
        protected:
            static bool get_ledger_fulladdr_from_account(const std::string & account_addr,uint32_t & ledger_id,uint16_t & ledger_sub_addr,uint32_t & account_index)
            {
                ledger_id       = 0;//0 is valid and default value
                ledger_sub_addr = 0;//0 is valid and default value
                account_index   = get_index_from_account(account_addr);  //hash whole account address
                const int account_address_size = (int)account_addr.size();
                if( (account_address_size < enum_vaccount_address_max_size) && (account_address_size > enum_vaccount_address_min_size) )
                {
                    ledger_id = get_ledgerid_from_account(account_addr);
                    
                    std::string::size_type _pos_of_subaddr = account_addr.find_last_of('@');
                    if(_pos_of_subaddr != std::string::npos)//system account
                    {
                        const std::string _string_subaddr = account_addr.substr(_pos_of_subaddr + 1);
                        ledger_sub_addr = (uint16_t)xstring_utl::toint32(_string_subaddr);
                        xassert(ledger_sub_addr < enum_vbucket_has_tables_count);
                    }
                    else //regular account as = T"type|ledger_id"["public_key_address"]
                    {
                        ledger_sub_addr = (uint16_t)(account_index);
                    }
                    ledger_sub_addr = (ledger_sub_addr & enum_vbucket_has_tables_count_mask); //force to trim others,to ensure less than 1024
                    
                    return true;
                }
                return false;
            }
            
        public: //convert account_address to xid_t
            /*
             //XID : ID of the logic & virtual account/user# at overlay network
             XID  definition as total 64bit =
             {
                -[32bit]    //index(could be as hash(account_address)
                -[26bit]    //prefix  e.g. xledger defined as below
                    -[16bit]:ledger_id
                        -[12bit:net#]
                        -[4 bit:zone#/bucket-index]
                    -[10bit]:sub_addr_of_ledger
                        -[7 bit:book-index]
                        -[3 bit:table-index]
                -[enum_xid_level :3bit]
                -[enum_xid_type  :3bit]
             }
             */
            static const uint32_t get_index_from_account(const std::string & account_addr)
            {
                return (uint32_t)xhash64_t::digest(account_addr);//hash64 better performance than hash32
            }
            static const xvid_t  get_xid_from_account(const std::string & account_addr)
            {
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

        public:
            xvaccount_t(const std::string & account_address);
            virtual ~xvaccount_t();
        protected:
            xvaccount_t(const xvaccount_t & obj);
        private:
            xvaccount_t();
            xvaccount_t & operator = (const xvaccount_t &);
        public:
            inline const int            get_ledger_id()   const {return get_vledger_ledger_id(m_account_xid);}
            inline const int            get_chainid()     const {return get_vledger_chain_id(m_account_xid);}
            inline const int            get_zone_index()  const {return get_vledger_zone_index(m_account_xid);}
            inline const int            get_bucket_index()const {return get_zone_index();}
            inline const int            get_net_id()      const {return get_chainid();}
            
            inline const int            get_ledger_subaddr() const {return get_vledger_subaddr(m_account_xid);}
            inline const int            get_book_index()     const {return get_vledger_book_index(m_account_xid);}
            inline const int            get_table_index()    const {return get_vledger_table_index(m_account_xid);}
            
            inline const xtable_shortid_t       get_short_table_id()//note: short table_id = [zone_index][book_index][table_index]
            {
                  return (xtable_shortid_t)((get_zone_index() << 10) | get_ledger_subaddr());
            }
            inline const xtable_longid_t       get_long_table_id()//note: long table_id = [chain_id][zone_index][book_index][table_index]
            {
                return xtable_longid_t((get_ledger_id() << 10) | get_ledger_subaddr());
            }
            
            inline const xvid_t         get_xvid()    const {return m_account_xid;}
            inline const std::string&   get_xvid_str()const {return m_account_xid_str;}
            inline const std::string&   get_address() const {return m_account_addr;}
            inline const std::string&   get_account() const {return m_account_addr;}
            inline const uint32_t       get_account_index() const {return get_xid_index(m_account_xid);}
            
            enum_vaccount_addr_type     get_addr_type()const{return get_addrtype_from_account(m_account_addr);}
        private:
            xvid_t                      m_account_xid;
            std::string                 m_account_xid_str;//tostring(m_account_xid),cache it as performance improve
            std::string                 m_account_addr;
        };
    }//end of namespace of base
}//end of namespace top
