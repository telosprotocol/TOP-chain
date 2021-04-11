    // Copyright (c) 2018-2020 Telos Foundation & contributors
    // Distributed under the MIT software license, see the accompanying
    // file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject.h"
#include "xbase/xdata.h"
#include "xbase/xvblock.h"

#include <deque>

namespace top
{
    namespace core
    {
        //3bits
        class enum_xvmobj_class :uint8_t
        {
            enum_xvmobj_class_nil        = 0,
            
            enum_xvmobj_class_number     = 1,//int64_t,uint64_t
            enum_xvmobj_class_string     = 2,//binary std::string
            
            enum_xvmobj_class_list       = 3, //
            enum_xvmobj_class_hash       = 4, //hash_map = std::unordered_map<std::string,xxx>
            enum_xvmobj_class_map        = 5, //standard map = std::map<std::string,xxx>
            enum_xvmobj_class_set        = 6, //standard set
        };
        //3bits
        class enum_xvmobj_encode : uint8_t
        {
            enum_xvmobj_encode_nil      = 0,
            enum_xvmobj_encode_int64    = 1,
            enum_xvmobj_encode_uint64   = 2,
            enum_xvmobj_encode_string   = 3,
        };
        //2bits
        class enum_xvmobj_length: uint8_t
        {
            enum_xvmobj_length_is_8bit     = 0, //length is max as 255 bytes
            enum_xvmobj_length_is_16bit    = 1, //length is max as 65535 bytes
            enum_xvmobj_length_is_24bit    = 2, //length is max as 16M bytes
            enum_xvmobj_length_is_full     = 3, //length is max as 512M bytes
        };
        
        //mini-object with minimal overhead
        class xvmobj_t  : public base::xobject_t
        {
        public:
            enum_xvmobj_class   get_obj_class() const {return (enum_xvmobj_class)(m_xvmobj_type >> 3);}
            enum_xvmobj_encode  get_obj_encode()const {return (enum_xvmobj_encode)(m_xvmobj_type & 0x07);}
        private:
            uint8_t      m_xvmobj_type;    //[2bit:enum_xvmobj_length][3bit:enum_xvmobj_class][3bit:enum_xvmobj_encode]
            uint32_t     m_xvmobj_length;  //max 16MB as security limit
        };
        
        //4bit
        class enum_xvop_code : uint8_t
        {
            enum_xvop_code_nil              = 0, //nil op code,do nothing
            
            enum_xvop_code_store            = 1,
            enum_xvop_code_load             = 2,
            enum_xvop_code_insert           = 3,
            enum_xvop_code_reset            = 4,
            
            //only for list
            enum_xvop_code_lpush            = 0x10,
            enum_xvop_code_lpop             = 0x11,
            enum_xvop_code_rpush            = 0x12,
            enum_xvop_code_rpop             = 0x13,
            
            enum_xvop_code_map_set          = 0x14,
            
            
            enum_xvop_code_delete           = 5,
        };
    
        //each core instruction is Object-oriented but following concept of Functional-Programing(FP),once object is constructed the result is certained and determined without any side-effect
        class xvoperate_t : public xvmobj_t
        {
        public:
            static xvoperate_t* store(const int64_t new_value)
            {
                return xvoperate_t<int64_t>::set(new_value);
            }
            static xvoperate_t*  list_push_back(const int64_t item)
            {
                return new xvlistop_t<int64_t>(enum_xvop_code_rpush,item);
            }
            static xvoperate_t*  list_push_back(const std::string & item)
            {
                return new xvlistop_t<std::string>(enum_xvop_code_rpush,item);
            }
        public:
            virtual bool     execute(const int64_t & input, int64_t & output) {return false;}
            virtual bool     execute(const std::string & input, std::string & output) {return false;}
            
            virtual bool     execute(const std::deque<int64_t> & input, std::deque<int64_t> & output) {return false;}
            virtual bool     execute(const std::deque<std::string> & input, std::deque<std::string> & output) {return false;}
            
            virtual bool     execute(const std::map<std::string,int64_t> & input, std::map<std::string,int64_t> & output) {return false;}
            virtual bool     execute(const std::map<std::string,std::string> & input, std::map<std::string,std::string> & output) {return false;}
            
            const  uint64_t  get_op_id()   const {return m_op_id;}
            enum_xvop_code   get_op_code() const {return m_op_code;}
        protected:
            uint64_t            m_op_id;     //each operate assign unique id under one account
            enum_xvop_code      m_op_code;   //each instruction has unique type
        };
        
        template<typename _Tp>
        class xvlistop_t : public xvoperate_t
        {
        public:
            typedef _Tp value_type;
        public:
            xvoperator(enum_xvop_code code, value_type & param);
            virtual ~xvoperator(){};
        private:
            xvoperator();
        public:
            virtual bool     execute(const std::deque<value_type> & input,std::deque<value_type> & output) override
            {
                if(get_obj_class() != enum_xvmobj_class_list)
                    return false;
                
                if(&input != &output)
                    output = input;
                
                switch(get_op_code())
                {
                    case enum_xvop_code_lpush:
                        output.push_front(m_param);
                        break;
                    case enum_xvop_code_lpop:
                        output.pop_front();
                        break;
                    case enum_xvop_code_rpush:
                       output.push_back(m_param);
                       break;
                    case enum_xvop_code_rpop:
                        output.pop_back();
                        break;

                    default:
                        return false;
                }
                return true;
            }
        private:
            value_type   m_param;
        }

        template<typename _Key,typename _Tp>
        class xvhashop_t: public xvoperate_t
        {
            typedef _Key    key_type;
            typedef _Tp     value_type;
        public:
            xvhashop_t(const key_type & op_key,const value_type & op_value);
        public:
            bool execute(const std::map<key_type,value_type> & input,std::map<key_type,value_type> & output) override
            {
                if(get_obj_class() != enum_xvmobj_class_hash)
                    return false;
                
                if(&input != &output)
                    output = input;
                
                switch(get_op_code())
                {
                case enum_xvop_code_store:
                    output[m_op_key] = m_op_value;
                    break;
                    
                default:
                    return false;
                }
                return true;
            }
        private:
            key_type   m_op_key;
            value_type m_op_value;
        };
    
    
        //total 4 bit to  max 16 types
        //each resource ask related permission to access
        enum enum_xvresource_type:uint8_t
        {
            enum_xvresource_type_nil        = 0,    //nil property,and execute nothing
            enum_xvresource_type_keychain   = 1,    //manage authorization & authentication
            enum_xvresource_type_asset      = 2,    //manage token asset
            enum_xvresource_type_code       = 3,    //deployed code
            enum_xvresource_type_fee        = 4,    //fee related
            enum_xvresource_type_meta       = 5,    //meta information
            enum_xvresource_type_attribute  = 6,    //data & attribute
        };
        
        //total 3 bit to  max 8 types,define what level of authorization of mandate for resource
        //macl:  mandatory accessl control for read & write
        enum enum_xvresource_macl_level :uint8_t
        {
            enum_xvresource_macl_level_any      = 0, //anyone allow read & write
            enum_xvresource_macl_level_parent   = 1, //allow parent account to access
            enum_xvresource_macl_level_standard = 2, //allow standard access by owner' any key
            enum_xvresource_macl_level_admin    = 3, //allow user'primary key for certain resource e.g. keychain
            enum_xvresource_macl_level_kernel   = 4, //asset can only be write by kernel vm
        };
        

    
        //KEY -> roles -> permissions  -> instructions
        enum enum_xvrole_type :uint8_t
        {
            enum_xvrole_type_guest      = 0x01,  //query only
            enum_xvrole_type_parent     = 0x02,  //vote related actions
            enum_xvrole_type_voter      = 0x03,
            enum_xvrole_type_standard   = 0x04,  //asset/tokens exchange/trade
            enum_xvrole_type_admin      = 0x06,  //manage all of this account
            enum_xvrole_type_system     = 0x07,  //consensus system
            enum_xvrole_type_kernel     = 0x0F,  //
        };
 
         //permission definition
        enum enum_xvresource_permission :uint8_t
        {
            enum_xvresource_permission_query      = 0x001,   //read-only
            enum_xvresource_permission_write      = 0x002,   //store data
            enum_xvresource_permission_create     = 0x004,
            enum_xvresource_permission_delete     = 0x008,

            enum_xvresource_permission_vote       = 0x010,   //vote related actions
            enum_xvresource_permission_exchange   = 0x020,   //asset/tokens exchange/trade
            enum_xvresource_permission_execute    = 0x040,   //deploy & execute code
            enum_xvresource_permission_authorize  = 0x080,   //authorize
            
            enum_xvresource_permission_commit     = 0x100,   //persistent save to db officialy
        };
        
        class xvresource_t : public xvmobj_t
        {
        protected:
            xvresource_t(enum_xvresource_type type, enum_xvresource_mandate_level level);
        public:
            enum_xvresource_type         get_type()      const {return m_resource_type;}
            enum_xvresource_macl_level   get_mcl_level() const {return m_resource_auth_level;}
            int                          get_permission()const {return m_resource_permission;}
            const uint64_t               get_prev_full_version() const {return m_prev_full_version;}
            const uint64_t               get_latest_version()
            {
                if(m_op_codes.empty())
                    return get_prev_full_version();
                    
                return m_op_codes.rbegin()->get_version();
            }
        public:
            virtual bool execute(xvoperate_t & instruction,xvstage_t & canvas)
            {
                return false;
            }
        protected:
            virtual bool undo();
            virtual bool redo();
        private:
            enum_xvresource_type            m_resource_type;
            enum_xvresource_macl_level      m_resource_macl_level;
            uint8_t                         m_resource_permission;//combine enum_xvresource_permission
        protected:
            std::dueue<xvoperate_t*>  m_op_codes;
            uint64_t                  m_prev_full_version;
        };
    
        //int64_t type
        class xvinteger : public xvresource_t
        {
        private:
            virtual int64_t  load_last_checkpoint() = 0;

            virtual bool     execute(xvoperate_t* op)
            {
                if(op->get_version() <= get_latest_version())
                    return false;
                    
                m_op_codes.push_back(op);
                return op->execute(m_value,m_value);
            }
            virtual bool     revert(const uint64_t target_version)
            {
                for(auto it = m_op_codes.rbegin(); it != m_op_codes.end(); ++it)
                {
                    if( (*it)->get_version() >= target_version)
                    {
                        m_op_codes.pop();
                    }
                }
            }
        private:
            int64_t     m_value;
        };
        //std::string type
        class xvstring_t : public xvresource_t
        {
        };
        //std::queue type
        template<typename value_type>
        class xvlist_t : public xvresource_t
        {
        };
        //std::map type
        template<typename value_type>
        class xvmap_t  : public xvresource_t
        {
        };
        
        //for key & authorization
        class xvkeychain_t : public xvmap_t<std::string>
        {
        };
    
        //system token & native tokens
        class xvasset_t : public xvinteger
        {
        };
    
        //gas & transaction fee etc
        class xvfee_t   : public xvinteger
        {
        };
        
        //meta information for account
        class xvmeta_t :  public xvmap_t<std::string>
        {
        };
        
        //data & attribute

        
        //event -> action(script) as input ->execute(vm) ->frame as output ->state
        
        //op#1: 只是针对resource的操作
        //op#2: 逻辑code
        //op#3: 认证/签名之类的api 操作
        //each transaction ->event -> script : include mutiple instructions -> execute -> state
        //multiple states construct one merkle tree ,and root is be xvblock'output_root_hash.
        //each state has the final hash of resource
        //each script can only handle one resource ? no. even transferout may also need modify tgas and balance as well.
        //from abstract point, operate need specified resouce name/index
        //mutiple script may change same state object, output-root-hash just include final hash of state
        
        //the closured mini-vm with data to execute script
        
        //force serializeable mode between transactions
        class xvstage_t : public base::xobject_t
        {
        public:
            virtual bool execute(xvoperate_t & instruction)
            {
                const uint64_t pre_alloc_snapshot_version = get_latest_snapshot_version() + 1;
                try
                {
                    instruction.attach_snapshot(pre_alloc_snapshot_version);
                    if(instruction.get_target().empty() == false)//resource level'instruction
                    {
                        xvresource_t * target_object = find_resource(instruction.get_target());
                        if(target_object != nullptr)
                        {
                            if(target_object->execute(instruction))
                                commit_snapshot(pre_alloc_snapshot_version);
                        }
                    }
                    else //stage level'instruction
                    {
                        if(execute(instruction))
                            commit_snapshot(pre_alloc_snapshot_version);
                    }
                }catch(...)
                {
                    
                }
                revert(pre_alloc_snapshot_version);
                return false;
            }
            virtual bool revert(const uint64_t rollback_to_snapshot_version)//failue hande for transaction
            {
                for(auto it = m_resources.begin(); it != m_resources.end(); ++it)
                {
                    it->second->revert(rollback_to_snapshot_version);
                }
                return true;
            }
            const uint64_t  alloc_snapshot_version()
            {
                return ++m_op_sequence_id;
            }
        protected:
            xvresource_t* find_resource(const std::string & name)
            {
                auto it = m_resources.find(name);
                if(it != m_resources.end())
                    return it->second;
                    
                return nullptr;
            }
            virtual bool execute(xvoperate_t & instruction)
            {
                return true;
            }
        private:
            uint64_t    m_height;  //height of stage is same as block height
            uint64_t    m_op_sequence_id;
            std::map<std::string,xvresource_t*> m_resources;
        };
        
        
    };
};
