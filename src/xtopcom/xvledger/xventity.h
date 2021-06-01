// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
 
#include <string>
#include "xbase/xvmethod.h"
#include "xbase/xutl.h"
#include "xvaction.h"

namespace top
{
    namespace base
    {
        class xvexemodule_t;
        //Entity is a lambda execution(aka:"执行体") that have execution instructions and related data, like PE(Potable Execution) it may also link to the extend resource at Data-Section.
        //xventity_t of output present "bin-log" of "state" as result
        //xventity_t of input  present "action-log" of "event and call" as source
        class xventity_t : public xdataunit_t
        {
            friend class xvblock_t;
            friend class xvexemodule_t;
        public:
            static  const std::string   name(){ return std::string("xventity");}
            virtual std::string         get_obj_name() const override {return name();}
            enum{enum_obj_type = enum_xobject_type_ventity};//allow xbase create xventity_t object from xdataobj_t::read_from()
            
        protected:
            xventity_t(enum_xdata_type type = (enum_xdata_type)enum_xobject_type_ventity);
            virtual ~xventity_t();
        private:
            xventity_t(const xventity_t & other);
            xventity_t & operator = (const xventity_t & other);
            
        public:
            virtual bool    close(bool force_async = false) override;
            
            virtual void*   query_interface(const int32_t _enum_xobject_type_) override;//caller need to cast (void*) to related ptr
            //general key-value query, e.g. query leaf of merkle tree by query_data("merkle-tree-leaf")
            virtual const std::string query_value(const std::string & key) {return std::string();}//virtual key-value for entity
            
            const int          get_entity_index() const {return m_entity_index;}
            
        protected://not open for public
            void               set_exe_module(xvexemodule_t * exemodule_ptr);
            xvexemodule_t *    get_exe_module() const {return m_exe_module;}
            
            //subclass extend behavior and load more information instead of a raw one
            //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
            virtual int32_t    do_write(xstream_t & stream) override; //allow subclass extend behavior
            virtual int32_t    do_read(xstream_t & stream)  override; //allow subclass extend behavior
            
        private:
            void               set_entity_index(const uint16_t index){m_entity_index = index;}
        private:
            xvexemodule_t *    m_exe_module;
            uint16_t           m_entity_index;  //index at xvexemodule_t
        };
        
        //transaction->action->input entity->[contract]->output entity->block
        //contract just do computing,who decide output ?
        //for unit: output entity = xvbstate:bin-log as general rule
        //for table: each unit -> go follow rule ->output 'bin-log ->construct unit'block
    
        //dedicated entity for input module
        class xvinentity_t : public xventity_t
        {
            friend class xvblock_t;
            friend class xvblockstore_t;
        public:
            static  const std::string   name(){ return std::string("xvinentity");}
            virtual std::string         get_obj_name() const override {return name();}
            enum{enum_obj_type = enum_xobject_type_vinentity};
        public:
            xvinentity_t(const std::vector<xvaction_t*> & actions);
            xvinentity_t(const std::vector<xvaction_t> & actions);
            xvinentity_t(std::vector<xvaction_t> && actions);
        protected:
            xvinentity_t();
            virtual ~xvinentity_t();
        private:
            xvinentity_t(xvinentity_t &&);
            xvinentity_t(const xvinentity_t &);
            xvinentity_t & operator = (xvinentity_t &&);
            xvinentity_t & operator = (const xvinentity_t &);
            
        public:
            //caller need to cast (void*) to related ptr
            virtual void*             query_interface(const int32_t _enum_xobject_type_) override;
            //general key-value query, e.g. query leaf of merkle tree by query_data("merkle-tree-leaf")
            virtual const std::string query_value(const std::string & key) override;//virtual key-value for entity
            
            const std::vector<xvaction_t> & get_actions() const {return m_actions;}
            
        protected:
            //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
            virtual int32_t           do_write(xstream_t & stream) override; //allow subclass extend behavior
            virtual int32_t           do_read(xstream_t & stream)  override; //allow subclass extend behavior
        private:
            std::vector<xvaction_t>   m_actions;
        };
    
        //dedicated entity for output module
        class xvoutentity_t : public xventity_t
        {
            friend class xvblock_t;
            friend class xvblockstore_t;
        public:
            static  const std::string   name(){ return std::string("xvoutentity");}
            virtual std::string         get_obj_name() const override {return name();}
            enum{enum_obj_type = enum_xobject_type_voutentity};
        public:
            xvoutentity_t(const std::string & state_bin_log);
        protected:
            xvoutentity_t();
            virtual ~xvoutentity_t();
        private:
            xvoutentity_t(xvoutentity_t &&);
            xvoutentity_t(const xvoutentity_t &);
            xvoutentity_t & operator = (xvoutentity_t &&);
            xvoutentity_t & operator = (const xvoutentity_t &);
            
        public:
            //caller need to cast (void*) to related ptr
            virtual void*             query_interface(const int32_t _enum_xobject_type_) override;
            //general key-value query, e.g. query leaf of merkle tree by query_data("merkle-tree-leaf")
            virtual const std::string query_value(const std::string & key) override;//virtual key-value for entity
            
            const std::string &       get_state_binlog() const {return m_state_binlog;}
            
        protected:
            //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
            virtual int32_t           do_write(xstream_t & stream) override; //allow subclass extend behavior
            virtual int32_t           do_read(xstream_t & stream)  override; //allow subclass extend behavior
        private:
            std::string     m_state_binlog;
        };
    
        //xvbinentity_t present binary or unknow entity
        class xvbinentity_t : public xventity_t
        {
            friend class xvblock_t;
            friend class xvblockstore_t;
        public:
            static  const std::string   name(){ return std::string("xvbinentity");}
            virtual std::string         get_obj_name() const override {return name();}
            enum{enum_obj_type = enum_xobject_type_binventity};//allow xbase create xventity_t object from xdataobj_t::read_from()
            
        public:
            xvbinentity_t(const std::string & raw_bin_data);
        protected:
            xvbinentity_t();
            virtual ~xvbinentity_t();
        private:
            xvbinentity_t(const xvbinentity_t & other);
            xvbinentity_t & operator = (const xvbinentity_t & other);
            
        public:
            virtual void*   query_interface(const int32_t _enum_xobject_type_) override;//caller need to cast (void*) to related ptr
            //general key-value query, e.g. query leaf of merkle tree by query_data("merkle-tree-leaf")
            virtual const std::string query_value(const std::string & key) override {return std::string();}//virtual key-value for entity
            
        protected: //subclass extend behavior and load more information instead of a raw one
            //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
            virtual int32_t     do_write(xstream_t & stream) override; //allow subclass extend behavior
            virtual int32_t     do_read(xstream_t & stream)  override; //allow subclass extend behavior
            
        private: //note each entity not allow over 64KB
            std::string    m_raw_data;
        };
        
        //xvexemodule_t : an executable module that manage the entities and resources
        //rule#1: each block of the account always use xventity_t of entity_index(0) for own state
        //rule#2: table use xventity_t(index than #1) to present the included units
        //rule#3: each unit in table has one(only one) linked xventity
        //rule#4: each block of account get final state by combining the xvbstate of prev-block and current entity of output
        class xvexemodule_t : public xdataunit_t
        {
            friend class xvblock_t;
        protected:
            xvexemodule_t(enum_xdata_type type);
            xvexemodule_t(std::vector<xventity_t*> && entitys,const std::string & raw_resource_data,enum_xdata_type type);
            xvexemodule_t(const std::vector<xventity_t*> & entitys,const std::string & raw_resource_data,enum_xdata_type type);
            
            xvexemodule_t(std::vector<xventity_t*> && entitys,xstrmap_t & resource, enum_xdata_type type);
            xvexemodule_t(const std::vector<xventity_t*> & entitys,xstrmap_t & resource, enum_xdata_type type);
            virtual ~xvexemodule_t();
        private:
            xvexemodule_t();
            xvexemodule_t(const xvexemodule_t & other);
            xvexemodule_t & operator = (const xvexemodule_t & other);
            
        public:
            //note: close first before do the last release,otherwise may leak memory
            virtual bool      close(bool force_async = false) override;
            
            const std::vector<xventity_t*> &  get_entitys()  const {return m_entitys;}
            
        public: //resource might be treat as key-value database,or data reference by instructions
            virtual const std::string query_resource(const std::string & key);//virtual key-value for query resource
            
            virtual const std::string get_resources_data(); //combine whole extend resource into one single string
            const   std::string       get_resources_hash() const {return m_resources_hash;}//m_resource_hash for raw_resources
            bool                      has_resource_data()  const {return (m_resources_obj != NULL);}
            
        protected: //for subclass or friend class
            const xstrmap_t     *     get_resources() const {return m_resources_obj;}
            
            virtual int32_t     do_write(xstream_t & stream) override; //not allow subclass change behavior
            virtual int32_t     do_read(xstream_t & stream)  override; //not allow subclass change behavior
            
        private:  //not allow override any more
            //set_resources_data only open to xvblock where may verify hash first
            bool                set_resources_data(const std::string & raw_resource_data);
            bool                set_resources_hash(const std::string & raw_resources_hash);
            
        private://entity(instructions) <==> resource(data)
            std::vector<xventity_t*>    m_entitys;       //manage entity that are unit of execution
            xstrmap_t*                  m_resources_obj; //both key-value are string that might be from xdataobj or raw content
            std::string                 m_resources_hash; //point to a object of  xmap_t<std::string>,which may store/load seperately
        };

    }//end of namespace of base

}//end of namespace top
