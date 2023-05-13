// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wunused-value"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-value"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xbase/xhash.h"
#include "xbase/xns_macro.h"
#include "xbase/xobject.h"
#include "xbase/xutl.h"
#include "xbase/xdata.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

namespace top
{
    XINLINE_CONSTEXPR uint32_t MAIN_CHAIN_REC_TABLE_USED_NUM{1};
    XINLINE_CONSTEXPR uint32_t MAIN_CHAIN_ZEC_TABLE_USED_NUM{3};
    XINLINE_CONSTEXPR uint32_t MAIN_CHAIN_EVM_TABLE_USED_NUM{1};
    XINLINE_CONSTEXPR uint32_t MAIN_CHAIN_RELAY_TABLE_USED_NUM{1};
    XINLINE_CONSTEXPR uint32_t TOTAL_TABLE_NUM{MAIN_CHAIN_REC_TABLE_USED_NUM+MAIN_CHAIN_ZEC_TABLE_USED_NUM+MAIN_CHAIN_EVM_TABLE_USED_NUM+MAIN_CHAIN_RELAY_TABLE_USED_NUM+enum_vbucket_has_tables_count};
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
            // enum_vaccount_addr_type_secp256k1_user_sub_account  = '1',  //secp256k1 generated key->account
            enum_vaccount_addr_type_native_contract             = '2',  //secp256k1 generated key->account
            // enum_vaccount_addr_type_custom_contract             = '3',  //secp256k1 generated key->account
            enum_vaccount_addr_type_secp256k1_evm_user_account  = '6',
            enum_vaccount_addr_type_secp256k1_eth_user_account  = '8',
            enum_vaccount_addr_type_block_contract              = 'a',  //secp256k1 generated key->account
            enum_vaccount_addr_type_relay_block                 = 'b',  //secp256k1 generated key->account
        };
        
        //each chain has max 16 zones/buckets, define as below
        enum enum_xchain_zone_index
        {
            enum_chain_zone_consensus_index   = 0,  //for consesnus
            enum_chain_zone_beacon_index      = 1,  //for beacon
            enum_chain_zone_zec_index         = 2,  //for election
            enum_chain_zone_frozen_index      = 3,  // for sync
            enum_chain_zone_evm_index         = 4,  // for eth
            enum_chain_zone_relay_index       = 5,

            enum_chain_zone_fullnode_index    = 13,
            enum_chain_zone_storage_index     = 14, //for archive nodes
            enum_chain_zone_edge_index        = 15, //for edge nodes
        };
        //define common rules for chain_id
        enum enum_xchain_id
        {
            enum_main_chain_id          = 0,      //main chain for TOP asset
            enum_rootbeacon_chain_id    = 128,    //root beacon of TOP platform
            enum_consortium_id          = 129,     //for consortium
            enum_test_chain_id          = 255,    //for test purpose
            
            //service_chain_id defined as below ,that must >= 256
            enum_service_chain_id_start_reserved = 256,
        };
        
        class xvledger_config_t {
        public:
            static std::map<base::enum_xchain_zone_index, uint16_t> const&  get_all_consensus_zone_subaddr_paris() {
                static std::map<base::enum_xchain_zone_index, uint16_t> _pairs = {
                    {enum_chain_zone_beacon_index, MAIN_CHAIN_REC_TABLE_USED_NUM},
                    {enum_chain_zone_zec_index, MAIN_CHAIN_ZEC_TABLE_USED_NUM},
                    {enum_chain_zone_consensus_index, enum_vbucket_has_tables_count},
                    {enum_chain_zone_evm_index, MAIN_CHAIN_EVM_TABLE_USED_NUM},
                    {enum_chain_zone_relay_index, MAIN_CHAIN_RELAY_TABLE_USED_NUM},
                };
                return _pairs;
            }
        };

        using xtable_shortid_t = uint16_t;//note: short table_id = [zone_index][book_index][table_index]
        using xtable_longid_t = uint32_t;//note: long table_id = [chain_id][zone_index][book_index][table_index]
        
        #define make_table_shortid(zone_index, subaddr) ((uint16_t)((zone_index << 10) | subaddr))

        class xtable_index_t
        {
        public:       
            xtable_index_t(xvid_t xid) {
                m_zone_index = (enum_xchain_zone_index)get_vledger_zone_index(xid);
                m_subaddr = (uint8_t)get_vledger_subaddr(xid);
            }
            xtable_index_t(const xtable_index_t & rhs) {
                m_zone_index = rhs.m_zone_index;
                m_subaddr = rhs.m_subaddr;
            }
            xtable_index_t(xtable_shortid_t tableid) {
                m_zone_index = (enum_xchain_zone_index)(tableid >> 10);
                m_subaddr = (uint8_t)tableid & 0xff;
            }
            xtable_index_t(enum_xchain_zone_index zone_index, uint8_t subaddr) {
                m_zone_index = zone_index;
                m_subaddr = subaddr;
                xassert(m_zone_index <= enum_chain_zone_zec_index || (enum_chain_zone_evm_index <= m_zone_index && m_zone_index<= enum_chain_zone_relay_index));
                xassert(m_subaddr < enum_vbucket_has_tables_count);
            }
        public:
            xtable_shortid_t        to_table_shortid() const {return make_table_shortid(m_zone_index, m_subaddr);}
            enum_xchain_zone_index  get_zone_index() const {return m_zone_index;}
            uint8_t                 get_subaddr() const {
                if (get_zone_index() == enum_chain_zone_evm_index)
                    return 0;
                else
                    return m_subaddr;
            }
            uint8_t                 to_total_table_index() const {
                if (m_zone_index == enum_chain_zone_consensus_index) {
                    return m_subaddr;
                } else if (m_zone_index == enum_chain_zone_beacon_index) {
                    return enum_vbucket_has_tables_count + m_subaddr - 1;
                } else if (m_zone_index == enum_chain_zone_zec_index) {
                    return enum_vbucket_has_tables_count + MAIN_CHAIN_REC_TABLE_USED_NUM + m_subaddr - 1;
                } else if (m_zone_index == enum_chain_zone_evm_index) {
                    return 0;
                } else if (m_zone_index == enum_chain_zone_relay_index) {
                    return 0;
                }
                xassert(false);
                return 255;
            }
        private:
            enum_xchain_zone_index m_zone_index;
            uint8_t                m_subaddr;
        };

        //define some special address prefix
        XINLINE_CONSTEXPR char const * ADDRESS_PREFIX_ETH_TYPE_IN_MAIN_CHAIN            = "T80000";
        XINLINE_CONSTEXPR char const * ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN            = "T60004";

        class xvaccount_t : virtual public xrefcount_t
        {
        public:
            enum enum_vaccount_address_size
            {
                enum_vaccount_address_prefix_size = 6,  //Txyyyy, eg.T80000
                enum_vaccount_address_min_size  = 18, //(>20,<256)
                enum_vaccount_address_max_size  = 50,//(>20,<256)
            };

            enum enum_vaccount_compact_type
            {
                enum_vaccount_compact_type_no_compact       = 'T',  //complete TOP address
                enum_vaccount_compact_type_eth_main_chain   = 'U',  //TOP eth address in main chain zone consensus
                enum_vaccount_compact_type_evm_main_chain   = 'E',  //TOP eth address in main chain zone consensus
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
                if (!public_key_address.empty()) {
                    xassert(account_address_size > enum_vaccount_address_min_size);
                } else {
                    xassert(account_address_size > enum_vaccount_address_prefix_size);
                }                
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
            static const std::string  make_table_account_address(base::xtable_shortid_t tableid)
            {
                xtable_index_t _index(tableid);
                return make_table_account_address(_index.get_zone_index(), _index.get_subaddr());
            }            
            static const std::string  make_table_account_address(enum_xchain_zone_index zone_index, uint16_t subaddr)
            {
                xassert(subaddr < enum_vbucket_has_tables_count);
                const uint16_t ledger_id = make_ledger_id(enum_main_chain_id, zone_index);
                std::string public_key_address;  // table account address has empty public key address
                return make_account_address(enum_vaccount_addr_type_block_contract, ledger_id, public_key_address, subaddr);
            }
            static const std::string  make_table_account_address(const xvaccount_t & unit_account)
            {
                return make_table_account_address((enum_xchain_zone_index)unit_account.get_zone_index(), (uint16_t)unit_account.get_ledger_subaddr());
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
                if(account_addr.size() >= enum_vaccount_address_prefix_size) //at least 24 cahrs
                    _addr_type = account_addr.at(1);

                xassert(_addr_type != 0);
                return (enum_vaccount_addr_type)_addr_type;
            }
            //account format as = T[Type:1char][ledger-id:4chars][public address][sub_address(optional)]
            static const uint16_t get_ledgerid_from_account(const std::string & account_addr)
            {
                uint16_t  ledger_id = 0;//0 is valid and default value
                const int account_address_size = (int)account_addr.size();
                if( (account_address_size >= enum_vaccount_address_prefix_size) && (account_address_size <= enum_vaccount_address_max_size) )
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
                if( (account_address_size >= enum_vaccount_address_prefix_size) && (account_address_size < enum_vaccount_address_max_size) )
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
            static std::string compact_address_to(const std::string & account_addr);
            static std::string compact_address_from(const std::string & data);
            static bool check_address(const std::string & account_addr, bool isTransaction = false);
            static bool valid_zone_and_subaddr(enum_xchain_zone_index zone_index, uint16_t subaddr);
            static bool is_user_address_type(enum_vaccount_addr_type addr_type);
            static bool is_eth_address_type(enum_vaccount_addr_type addr_type);
            static bool is_table_address_type(enum_vaccount_addr_type addr_type);
            static bool is_contract_address_type(enum_vaccount_addr_type addr_type);
            static bool is_drand_address_type(enum_vaccount_addr_type addr_type);
            static bool is_timer_address_type(enum_vaccount_addr_type addr_type);
            static bool is_unit_address_type(enum_vaccount_addr_type addr_type);

        protected:
            static bool get_ledger_fulladdr_from_account(const std::string & account_addr,uint32_t & ledger_id,uint16_t & ledger_sub_addr,uint32_t & account_index)
            {
                ledger_id       = 0;//0 is valid and default value
                ledger_sub_addr = 0;//0 is valid and default value
                account_index   = get_index_from_account(account_addr);  //hash whole account address
                const int account_address_size = (int)account_addr.size();
                if( (account_address_size <= enum_vaccount_address_max_size) && (account_address_size > enum_vaccount_address_prefix_size) )
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
            static const uint32_t get_index_from_account(const std::string & account_addr);
            static const xvid_t  get_xid_from_account(const std::string & account_addr);
            
            //convert to binary/bytes address with compact mode as for DB 'key
            static const std::string  get_storage_key(const xvaccount_t & src_account);
            static std::string to_evm_address(const std::string& account);

        public:
            xvaccount_t(const std::string & account_address);
            virtual ~xvaccount_t();
            xvaccount_t();
            xvaccount_t(const xvaccount_t & obj);
            xvaccount_t & operator = (const xvaccount_t & obj);
            xvaccount_t & operator = (const std::string & new_account_addr);
        public:
            inline const int            get_ledger_id()   const {return get_vledger_ledger_id(m_account_xid);}
            inline const int            get_chainid()     const {return get_vledger_chain_id(m_account_xid);}
            inline const int            get_zone_index()  const {return get_vledger_zone_index(m_account_xid);}
            inline const int            get_bucket_index()const {return get_zone_index();}
            inline const int            get_net_id()      const {return get_chainid();}
            //full-ledger = /chainid/get_short_table_id
            inline const std::string&   get_storage_key() const {return m_account_store_key;}
            
            inline const int            get_ledger_subaddr() const {
                if (get_zone_index() == enum_chain_zone_evm_index)
                    return 0;
                else 
                    return get_vledger_subaddr(m_account_xid);
            }
            inline const int            get_book_index()     const {return get_vledger_book_index(m_account_xid);}
            inline const int            get_table_index()    const {return get_vledger_table_index(m_account_xid);}
            
            inline const xtable_shortid_t       get_short_table_id() const//note: short table_id = [zone_index][book_index][table_index]
            {
                  return (xtable_shortid_t)((get_zone_index() << 10) | get_ledger_subaddr());
            }
            inline const xtable_longid_t       get_long_table_id() const//note: long table_id = [chain_id][zone_index][book_index][table_index]
            {
                return xtable_longid_t((get_ledger_id() << 10) | get_ledger_subaddr());
            }
            xtable_index_t              get_tableid() const {return xtable_index_t(m_account_xid);}
            inline const xvid_t         get_xvid()    const {return m_account_xid;}
            inline const xvid_t         get_account_id()    const {return m_account_xid;}
            std::string                 get_xvid_str()const;
            inline const std::string&   get_address() const {return m_account_addr;}
            inline const std::string&   get_account() const {return m_account_addr;}
            inline const uint32_t       get_account_index() const {return get_xid_index(m_account_xid);}
            bool                        is_unit_address() const;
            bool                        is_user_address() const;
            bool                        is_eth_address() const;
            bool                        is_table_address() const;
            bool                        is_contract_address() const;
            bool                        is_drand_address() const;
            bool                        is_timer_address() const;
            bool                        is_relay_address() const;
            enum_vaccount_addr_type     get_addr_type()const{return get_addrtype_from_account(m_account_addr);}
            bool                        has_valid_table_addr() const;
        private:
            xvid_t                      m_account_xid;
            std::string                 m_account_addr;
            std::string                 m_account_store_key;//address as key of DB
        };
    
        //meta data of account
        class xblockmeta_t
        {
        public:
            xblockmeta_t();
            xblockmeta_t(const xblockmeta_t & obj);
            ~xblockmeta_t();

            xblockmeta_t & operator = (const xblockmeta_t & obj);
        private:
            xblockmeta_t(xblockmeta_t && move_obj);
            
        public:
            bool    operator == (const xblockmeta_t & obj) const;
            const std::string  ddump() const;//debug purpose
            
        public:
            uint64_t    _lowest_vkey2_block_height;    //since this height,introduce db key of new version(2)
            uint64_t    _highest_deleted_block_height; //latest block height that has been pruned and deleted
            
            uint64_t    _highest_cert_block_height;    //latest certificated block but not changed to lock/commit status
            uint64_t    _highest_lock_block_height;    //latest locked block that not allow fork
            uint64_t    _highest_commit_block_height;  //latest commited block to allow change state of account,like balance.

            uint64_t    _highest_full_block_height;    //latest full-block height for this account
            uint64_t    _highest_connect_block_height; //indicated the last block who is connected all the way to last full-block
            std::string _highest_connect_block_hash;
            uint64_t    _highest_cp_connect_block_height;
            std::string _highest_cp_connect_block_hash;
            uint8_t     _block_level;       //set per block 'enum_xvblock_level,each account has unique level
        };
    
        class xsyncmeta_t
        {
        public:
            xsyncmeta_t();
            xsyncmeta_t(const xsyncmeta_t & obj);
            ~xsyncmeta_t();
        protected:
            xsyncmeta_t & operator = (const xsyncmeta_t & obj);
        private:
            xsyncmeta_t(xsyncmeta_t && move_obj);
            
        public:
            bool    operator == (const xsyncmeta_t & obj) const;
            const std::string  ddump() const;//debug purpose
            
        public: //[_lowest_genesis_connect_height,_highest_genesis_connect_height]
            uint64_t    _highest_genesis_connect_height;//indicated the last block who is connected to genesis block
            // std::string _highest_genesis_connect_hash;
            // uint64_t    _highest_sync_height;           // higest continous block started from highest full table block
        };
    
        class xstatemeta_t
        {
        public:
            xstatemeta_t();
            xstatemeta_t(const xstatemeta_t & obj);
            ~xstatemeta_t();
        protected:
            xstatemeta_t & operator = (const xstatemeta_t & obj);
        private:
            xstatemeta_t(xstatemeta_t && move_obj);
            
        public:
            bool    operator == (const xstatemeta_t & obj) const;
            const std::string  ddump() const;//debug purpose

        public:
            uint64_t     _lowest_execute_block_height; //store delete/pruned height for state
            uint64_t     _highest_execute_block_height; //latest executed block that has executed and change state of account
            // std::string  _highest_execute_block_hash; // XTODO remove this field
        };
    
        class xvactmeta_t : public xdataobj_t,protected xblockmeta_t,protected xstatemeta_t,protected xsyncmeta_t
        {
            friend class xvaccountobj_t;
            enum {enum_obj_type = xdataunit_t::enum_xdata_type_vaccountmeta};
        public:
            xvactmeta_t(const xvaccount_t & _account);
            xvactmeta_t(const xvactmeta_t & obj);
            virtual ~xvactmeta_t();
            
        protected:
            xvactmeta_t(xvactmeta_t && move);
            xvactmeta_t & operator = (const xvactmeta_t & obj);
        public:
            virtual int32_t     serialize_from(xstream_t & stream) override;
            
        public:
            static xvactmeta_t* load(xvaccount_t & _account,const std::string & meta_serialized_data);

            const xblockmeta_t   clone_block_meta() const;
            const xstatemeta_t   clone_state_meta() const;
            const xsyncmeta_t    clone_sync_meta()  const;
            
            const uint16_t  get_meta_process_id() const {return _meta_process_id;}
            const uint8_t   get_meta_spec_version()const{return _meta_process_id;}
            const uint64_t  get_highest_saved_block_height() const {return _highest_saved_block_height;}
            bool    set_latest_deleted_block(const uint64_t height);
        protected: //APIs only open for  xvaccountobj_t object
            bool    set_block_meta(const xblockmeta_t & new_meta);
            bool    set_state_meta(const xstatemeta_t & new_meta);
            bool    set_sync_meta(const xsyncmeta_t & new_meta);
            bool    set_latest_executed_block(const uint64_t height);
            bool    set_lowest_executed_block(const uint64_t height);
                  
            xblockmeta_t &  get_block_meta();
            xstatemeta_t &  get_state_meta();
            xsyncmeta_t  &  get_sync_meta();
            
            void    update_highest_saved_block_height(const uint64_t new_height);
            void    update_meta_process_id(const uint16_t _process_id);
            void    init_version_control();

        protected:
            //not safe for multiple threads
            virtual int32_t   do_write(xstream_t & stream) override; //serialize whole object to binary
            void update_cp_connect(const uint64_t cp_connect_height, const std::string & cp_connect_hash);
            virtual int32_t   do_read(xstream_t & stream) override; //serialize from binary and regeneate content
            
            //caller respond to cast (void*) to related  interface ptr
            virtual void*     query_interface(const int32_t _enum_xobject_type_) override;
            virtual std::string  dump() const override;
        private: //from block meta
            using xblockmeta_t::_lowest_vkey2_block_height;
            using xblockmeta_t::_highest_deleted_block_height;
            using xblockmeta_t::_highest_cert_block_height;    //latest certificated block but not changed to lock/commit status
            using xblockmeta_t::_highest_lock_block_height;    //latest locked block that not allow fork
            using xblockmeta_t::_highest_commit_block_height;  //latest commited block to allow change state of account,like balance.
            using xblockmeta_t::_highest_full_block_height;    //latest full-block height for this account
            using xblockmeta_t::_highest_connect_block_height; //indicated the last block who is connected all the way to last full-block
            using xblockmeta_t::_highest_connect_block_hash;
            using xblockmeta_t::_highest_cp_connect_block_height;
            using xblockmeta_t::_highest_cp_connect_block_hash;
            
        private: //from sync meta
            using xsyncmeta_t::_highest_genesis_connect_height;//indicated the last block who is connected to genesis block
            // using xsyncmeta_t::_highest_genesis_connect_hash;
            // using xsyncmeta_t::_highest_sync_height;           // higest continous block started from highest full table block
 
        private: //from statemeta
            using xstatemeta_t::_lowest_execute_block_height;
            using xstatemeta_t::_highest_execute_block_height; //latest executed block that has executed and change state of account
            // using xstatemeta_t::_highest_execute_block_hash;
             
        private:
            uint64_t  _highest_saved_block_height; //just tracking purpose
            uint16_t  _meta_process_id;   //which process produce and save this meta
            uint8_t   _meta_spec_version; //add version control for compatible case
        };
    
    }//end of namespace of base
}//end of namespace top
