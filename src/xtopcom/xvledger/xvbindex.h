// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvblock.h"
#include "xstatistic/xstatistic.h"

#ifndef STORE_UNIT_BLOCK
#define STORE_UNIT_BLOCK
#endif

//index of block
namespace top
{
    namespace base
    {
        //manage index of single block
        enum enum_index_store_flag
        {
            enum_index_store_flag_mini_block      = 0x01, //block  header & cert have been stored
            enum_index_store_flag_input_entity    = 0x02, //input  entity has been stored
            enum_index_store_flag_output_entity   = 0x04, //output entity has been stored
            enum_index_store_flag_input_resource  = 0x08, //input  resource has bedn stored
            enum_index_store_flag_output_resource = 0x10, //output resource has bedn stored
            enum_index_store_flag_unit_in_index   = 0x20, //mark it when save xvbindex with unitblock together
            enum_index_store_flag_transactions    = 0x40, //mark txs of this block has been decode and stored seperately
            enum_index_store_flag_full_block      = 0x7F, //mark when every piece of block been on DB
            enum_index_store_flag_main_entry      = 0x80, //indicate that is main entry of mutiple blocks
            enum_index_store_flags_mask           = 0xFF, //Mask to keep them
            //note:all bit has been used up, not allow add more
        };
        class xvbindex_t : public xvaccount_t, public xstatistic::xstatistic_obj_face_t
        {
            friend class xvblock_t;
        public:
            //static  void  register_object(xcontext_t & context);
            enum{enum_obj_type = enum_xobject_type_vbindex};//allow xbase create xvstate_t object from xdataobj_t::read_from()

        public:
            xvbindex_t();
            xvbindex_t(xvblock_t & block_obj);
            xvbindex_t(xvbindex_t && obj);
            xvbindex_t(const xvbindex_t & obj);
            xvbindex_t & operator = (const xvbindex_t & obj);
            ~xvbindex_t();
        private:

        public:
            inline const uint64_t       get_height()  const {return m_block_height;}
            inline const uint64_t       get_viewid()  const {return m_block_viewid;}
            inline const uint64_t       get_viewtoken()  const {return m_block_viewtoken;}
            inline const std::string &  get_block_hash()      const {return m_block_hash;}
            inline const std::string &  get_last_block_hash() const {return m_last_block_hash;}
            inline const uint64_t       get_last_full_block_height()  const {return m_last_fullblock_height;}
            
            inline const int32_t        get_next_viewid_offset() const {return m_next_viewid_offset;}
            inline const uint64_t       get_next_viewid()  const {return (m_block_viewid + m_next_viewid_offset);}

            inline const uint64_t       get_parent_block_height() const {return m_parent_block_height;}
            inline const uint64_t       get_parent_block_viewid() const {return m_parent_block_viewid;}
            inline const uint32_t       get_parent_block_entity() const {return m_parent_block_entity_id;}

            inline const std::string &  get_extend_cert()      const {return m_extend_cert;}
            inline const std::string &  get_extend_data()      const {return m_extend_data;}
            inline void                 set_extend_data(const std::string& data) { m_extend_data = data;}

            inline enum_xvblock_level   get_block_level()  const {return xvheader_t::cal_block_level(m_block_types);}
            inline enum_xvblock_class   get_block_class()  const {return xvheader_t::cal_block_class(m_block_types);}
            inline enum_xvblock_type    get_block_type()   const {return xvheader_t::cal_block_type(m_block_types);}
            inline const int            get_block_characters() const{return xvheader_t::cal_block_character(m_block_types);}
        public:
            //refer enum_xvblock_flag
            bool                        check_block_flag(enum_xvblock_flag flag) const;
            int                         set_block_flag(enum_xvblock_flag flag);
            int                         remove_block_flag(enum_xvblock_flag flag);
            int                         get_block_flags() const;  //return all flags related block
            int                         reset_block_flags(const uint32_t new_flags); //replace all flags of block

            //refer enum_index_store_flag
            bool                        check_store_flag(enum_index_store_flag flag) const;
            bool                        check_store_flags(const uint16_t flags) const;
            int                         set_store_flag(enum_index_store_flag flag);//return newed one
            int                         remove_store_flag(enum_index_store_flag flag);//return updated one
            int                         get_store_flags() const;  //return all flags related index
            int                         reset_store_flags(const uint32_t new_flags); //clean all flags related index

            void                        set_block_character(base::enum_xvblock_character character);
            void                        remove_block_character(base::enum_xvblock_character character);
        public:
            inline xvbindex_t*          get_prev_block() const {return m_prev_index;}
            bool                        reset_prev_block(xvbindex_t * _new_prev_ptr);//return false if hash or height not match

            //note:only commited block/index has valid next ptr
            inline xvbindex_t*          get_next_block() const {return m_next_index;}


            inline xvblock_t*           get_this_block() const {return m_linked_block;}
            bool                        reset_this_block(xvblock_t* _block_ptr,bool delay_release_existing_one = false);

            void                        reset_next_viewid_offset(const int32_t next_viewid_offset);
        public:
            void               set_unitblock_on_index(base::xvblock_t* _unit);
            base::xauto_ptr<base::xvblock_t> create_unitblock_on_index();
            void               set_unitblock_bin(std::string const& bin) {m_reserved = bin;}
            std::string const& get_unitblock_bin() const {return m_reserved;}

            inline bool        check_modified_flag() const { return (m_modified != 0);}
            void               set_modified_flag();
            void               reset_modify_flag();
            
            //only allow reset it when index has empty address
            bool               reset_account_addr(const xvaccount_t & addr);

            bool               is_close() const { return (m_closed != 0);}
            bool               close(); //force close object and release linked objects
            const std::string  dump() const;  //just for debug purpose

            //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
            int32_t            serialize_to(xstream_t & stream);   //write whole object to binary
            int32_t            serialize_from(xstream_t & stream);  //read from binary and regeneate content

            int32_t            serialize_to(std::string & bin_data);   //write whole object to binary
            int32_t            serialize_from(const std::string & bin_data);  //read from binary and regeneate content of
            virtual int32_t    get_class_type() const override {return xstatistic::enum_statistic_bindex;}
        private:
            void               init(); //init object
            bool               reset_next_block(xvbindex_t * _new_next_ptr);//return false if hash or height not match
            size_t get_object_size_real() const override;

        private://not serialized to db
            xvbindex_t*     m_prev_index;
            xvbindex_t*     m_next_index;
            xvblock_t*      m_linked_block;

        private: //serialized from/to stream/db
            uint64_t        m_block_height;     //block 'height
            uint64_t        m_block_viewid;     //view#id associated this block
            uint64_t        m_block_viewtoken;  //view token associated this block
            std::string     m_block_hash;       //point to block'qcert hash that logicaly same as block hash
            std::string     m_last_block_hash;  //point the last block'hash
            std::string     m_last_fullblock_hash; //point to last full-block'hash
            uint64_t        m_last_fullblock_height;//height of m_last_full_block
            
            std::string     m_extend_cert;
            std::string     m_extend_data;
            
            uint64_t        m_parent_block_height;//height of container(e.gtableblock) that may carry this block
            uint64_t        m_parent_block_viewid;//viewid of container(e.gtableblock) that may carry this block
            uint32_t        m_parent_block_entity_id{0};//entity id at parent block

            //(m_block_viewid + m_next_viewid_offset)point the block at same height but different viewid
            int32_t         m_next_viewid_offset;
            std::string     m_reserved;  //for future
            
            uint16_t        m_combineflags;     //[8bit:block-flags][1bit][7bit:store-bits]
            //[1][enum_xvblock_class][enum_xvblock_level][enum_xvblock_type][enum_xvblock_reserved]
            uint16_t        m_block_types;
            uint8_t         m_closed;           //indicated whether closed or not
            uint8_t         m_modified;         //indicated whether has any change that need persist again
            uint8_t         m_version;          //version of data format
        };

        class xvbindex_vector
        {
        public:
            xvbindex_vector()
            {
            }
            xvbindex_vector(std::nullptr_t)
            {
            }
            xvbindex_vector(std::vector<xvbindex_t*> & blocks)
            {
                m_vector = blocks;//transfer owner of ptr
                blocks.clear();
            }
            xvbindex_vector(std::vector<xvbindex_t*> && blocks)
            {
                m_vector = blocks;//transfer owner of ptr
                blocks.clear();
            }
            xvbindex_vector(xvbindex_vector && moved)
            {
                m_vector = moved.m_vector;//transfer owner of ptr
                moved.m_vector.clear();
            }
            ~xvbindex_vector()
            {
                for(auto it : m_vector)
                {
                    if(it != nullptr)
                        it->release_ref();
                }
            }
        private:
            xvbindex_vector(const xvbindex_vector &);
            xvbindex_vector & operator = (const xvbindex_vector &);
            xvbindex_vector & operator = (xvbindex_vector && moved);
        public:
            const std::vector<xvbindex_t*> &  get_vector() const {return m_vector;}
        private:
            std::vector<xvbindex_t*> m_vector;
        };

        //xvbnode_t that connected each other into one chain,and the node 'height and hash has following rule:
            //parent_node'height must = it'height - 1, parent'hash must = it'last_block_hash
            //child_node 'height must = it'height + 1, child'last_block_hash = it'hash
        //note:multiple-threads unsafe for the below operations
        class xvbnode_t : public xobject_t
        {
            friend class xvbtree_t;
        public:
            xvbnode_t(xvbnode_t * parent_node,xvblock_t & raw_block);
        protected:
            virtual ~xvbnode_t();
        private:
            xvbnode_t();
            xvbnode_t(const xvbnode_t&);
            xvbnode_t & operator = (const xvbnode_t &);
        public: //wrap readonly-function of xvblock
            inline xvbnode_t*          get_parent()  const {return m_parent_node;}
            inline xvbnode_t*          get_child()   const {return m_child_node;}

            inline uint64_t            get_viewid()  const {return m_block->get_viewid();}
            inline uint64_t            get_viewtoken() const {return m_block->get_viewtoken();}
            inline uint64_t            get_height()  const {return m_block->get_height();}
            inline uint32_t            get_chainid() const {return m_block->get_chainid();}
            inline const std::string&  get_account() const {return m_block->get_account();}
            inline const std::string&  get_hash()    const {return m_block->get_block_hash();}
            inline const std::string&  get_block_hash()  const {return m_block->get_block_hash();}
            inline const std::string&  get_header_hash() const {return m_block->get_header_hash();}
            inline const std::string&  get_input_hash()  const {return m_block->get_input_hash();}
            inline const std::string&  get_output_hash() const {return m_block->get_output_hash();}
            inline const std::string&  get_input_root_hash()   const {return m_block->get_input_root_hash();}
            inline const std::string&  get_output_root_hash()  const {return m_block->get_output_root_hash();}
            inline const std::string&  get_last_block_hash()   const {return m_block->get_last_block_hash();}
            inline const std::string&  get_justify_cert_hash() const {return m_block->get_justify_cert_hash();}

            inline operator xvblock_t* ()            const {return m_block;}
            inline xvblock_t*          get_block()   const {return m_block;}//it always be valid before xvbnode_t release
            inline xvheader_t*         get_header()  const {return m_block->get_header();}  //raw ptr of xvheader_t
            inline xvqcert_t*          get_cert()    const {return m_block->get_cert();}    //raw ptr of xvqcert_t
            // inline xvinput_t *         get_input()   const {return m_block->get_input();}   //raw ptr of xvinput_t,might be nullptr
            // inline xvoutput_t*         get_output()  const {return m_block->get_output();}  //raw ptr of xvoutput_t,might be nullptr

            inline bool                is_valid(bool deeptest)  {return m_block->is_valid(deeptest);}
            inline bool                is_deliver(bool deeptest){return m_block->is_deliver(deeptest);}
            inline bool                is_input_ready(bool full_check_resources = false) const   {return m_block->is_input_ready(full_check_resources);}//nil-block return true because it dont need input
            inline bool                is_genesis_block() const {return m_block->is_genesis_block();}//test whether it is a genesis block

            inline bool                check_block_flag(enum_xvblock_flag flag) const{return m_block->check_block_flag(flag);} //test whether has been setted
        public:
            virtual bool        reset_parent(xvbnode_t * _new_parent_node);//return false if hash or height not match
            virtual bool        reset_child(xvbnode_t * _new_node_ptr);  //helper function only

            virtual xvbnode_t*  attach_child(xvblock_t* new_child_block);//note:must release the retured ptr after use
            virtual xvbnode_t*  detach_child(const uint64_t height,const uint64_t viewid);//caller should release returned ptr after use

            virtual xvbnode_t*  find_node(const uint64_t target_height);//note:must release the retured ptr after use
            virtual xvbnode_t*  find_node(const uint64_t target_height,const uint64_t target_viewid);//note:must release the retured ptr after use

            virtual bool        close(bool force_async = true) override; //close and release this node only
            virtual bool        close_all(); //close and release all child nodes  as well
            virtual std::string dump() const override;  //just for debug purpose
        protected:
            bool                reset_block(xvblock_t *  new_block_ptr); //replace it with newer viewid
        private:
            xvblock_t*          m_block;
            xvbnode_t*          m_parent_node;  //note: m_parent_node'height must = m_block'height - 1
            xvbnode_t*          m_child_node;   //note: m_child_node'height  must = m_block'height + 1
        };

        //xvbmnode_t support multiple nodes as child
        //note:multiple-threads unsafe for the below operations
        class xvbmnode_t : public xvbnode_t
        {
            friend class xvbtree_t;
        public:
            xvbmnode_t(xvbnode_t * parent_node,xvblock_t & raw_block);
        protected:
            virtual ~xvbmnode_t();
        private:
            xvbmnode_t();
            xvbmnode_t(const xvbmnode_t&);
            xvbmnode_t & operator = (const xvbmnode_t &);
        public:
            virtual xvbnode_t*  attach_child(base::xvblock_t* new_block) override;//note:must release the retured ptr after use
            virtual xvbnode_t*  detach_child(const uint64_t height,const uint64_t viewid) override;//caller should release returned ptr after use

            virtual xvbnode_t*  find_node(const uint64_t target_height) override;//note:must release the retured ptr after use
            virtual xvbnode_t*  find_node(const uint64_t target_height,const uint64_t target_viewid) override;//note:must release the retured ptr after use

            virtual bool        close(bool force_async = true) override; //close and release this node only
            virtual bool        close_all() override; //close and release all child nodes  as well
        private:
            //blocks of m_child has same block height as (m_block.get_height() - 1)
            std::map<uint64_t,xvbnode_t*,std::greater<uint64_t> > m_child_nodes;//sort from higher view# -->lower view#
        };

        //operations for xvbnode_t
        struct less_by_vbnode_viewid
        {
            bool operator()(const base::xvbnode_t * front,const base::xvbnode_t * back)
            {
                if(nullptr == front)
                    return true;

                if(nullptr == back)
                    return false;

                return (front->get_viewid() < back->get_viewid());
            }

            bool operator()(const base::xvbnode_t & front,const base::xvbnode_t & back)
            {
                return (front.get_viewid() < back.get_viewid());
            }
        };
    }//end of namespace of base

}//end of namespace top
