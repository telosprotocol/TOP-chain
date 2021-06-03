// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
 
#include <string>
#include "xvtransact.h"
#include "xvblock.h"
#include "xvexecute.h"

namespace top
{
    namespace base
    {
        //xvbbuild_t responsible to combine input,output and header -> a new block
        class xvbbuild_t //multiple thread safe
        {
        public:
            xvbbuild_t(const xvheader_t & target_header);
            virtual ~xvbbuild_t();
        private:
            xvbbuild_t();
            xvbbuild_t(xvbbuild_t &&);
            xvbbuild_t(const xvbbuild_t &);
            xvbbuild_t & operator = (const xvbbuild_t &);
 
        public: //public build api
            virtual bool                build_input(xvinput_t * input_ptr);
            virtual bool                build_output(xvoutput_t * output_ptr);
            virtual bool                build_block(xvqcert_t * qcert_ptr);
            const std::string           build_mpt_root(const std::vector<std::string> & elements);
            
            inline const xvblock_t*     get_block()        const {return m_block_ptr;}
            inline const xvheader_t*    get_block_header() const {return m_header_ptr;}
            inline const xvinput_t*     get_block_input()  const {return m_input_ptr;}
            inline const xvoutput_t*    get_block_output() const {return m_output_ptr;}
            inline const std::string    get_block_account()const {return m_header_ptr->get_account();}
           
        protected://internal use only
            void                        reset_block(xvblock_t* new_ptr);
            xvblock_t*                  new_block_ptr(xvqcert_t & qcert_ptr);
           
            //since onlyl xvbbuild_t allow access private function of xvqcert
            void        set_crypto_hash_type_of_cert(xvqcert_t * qcert_ptr,enum_xhash_type new_type);
            void        set_input_root_hash_of_cert(xvqcert_t * qcert_ptr,const std::string & root_hash);
            void        set_output_root_hash_of_cert(xvqcert_t * qcert_ptr,const std::string & root_hash);
            void        set_extend_data_of_cert(xvqcert_t * qcert_ptr,const std::string & ext_data);
 
        private:
            xvblock_t *                 m_block_ptr;     //new block
            xvheader_t*                 m_header_ptr;    //new header
            xvinput_t*                  m_input_ptr;     //new input
            xvoutput_t*                 m_output_ptr;    //new output
        protected:
            std::recursive_mutex        m_lock;
        };
    
        class xvexecontxt_t; //in-advance declare
        //xvbmaker_t responsible to excute action->execute entity->generate input & output -> then build a new block
        class xvbmaker_t : public xvbbuild_t,public xvexecute_t //multiple thread safe
        {
        public:
            xvbmaker_t(const xvheader_t & target_header,xvexecontxt_t * exe_contxt);
            virtual ~xvbmaker_t();
        private:
            xvbmaker_t();
            xvbmaker_t(xvbmaker_t &&);
            xvbmaker_t(const xvbmaker_t &);
            xvbmaker_t & operator = (const xvbmaker_t &);
            
        public:
            //note: caller should already verified those xvtransaction_t with valid signature,valid content ... etc
            virtual bool                build_entity(const std::vector<xvtransact_t*> & input_txs);//step#1
            virtual bool                make_input(); //step#2
            virtual bool                make_output(); //step#3
 
            inline xvexecontxt_t*       get_exe_context() const {return m_exe_context;}
            inline enum_xhash_type      get_crypto_hash_type() const {return m_hash_type;}
            
        protected://internal use only
            inline xstrmap_t*           get_input_resource()  const {return m_input_resource;}
            inline xstrmap_t*           get_output_resource() const {return m_output_resource;}
            inline xvinentity_t*        get_input_entity()    const {return m_primary_input_entity;}
            inline xvoutentity_t*       get_output_entity()   const {return m_primary_output_entity;}
            
        protected: //helper function
            bool        merge_input_resource(const xstrmap_t * src_map);
            bool        merge_output_resource(const xstrmap_t * src_map);
             
            const bool  verify_input_txs(const xvheader_t & target,const std::vector<xvtransact_t*> & input_txs);

        private:
            xstrmap_t*                  m_input_resource; //resource to hold input 'big data
            xstrmap_t*                  m_output_resource;//resource to hold output ' big data
            
            xvinentity_t*               m_primary_input_entity; //#0 is primary input entity
            xvoutentity_t*              m_primary_output_entity;//#0 is primary output entity
            
            enum_xhash_type             m_hash_type;  //what hash method used to generate block
        private:
            xvexecontxt_t*              m_exe_context;
        };
    
        //xvtablemaker_t responsible to execute and make a new block of light table
        class xvtablemaker_t : public xvbmaker_t
        {
        public:
            xvtablemaker_t(const xvheader_t & table_header,xvexecontxt_t * exe_contxt);
        protected:
            virtual ~xvtablemaker_t();
        private:
            xvtablemaker_t();
            xvtablemaker_t(xvtablemaker_t &&);
            xvtablemaker_t(const xvtablemaker_t &);
            xvtablemaker_t & operator = (const xvtablemaker_t &);
            
        public:
            virtual bool                build_entity(const xvheader_t & unit_header,const std::vector<xvtransact_t*> & input_unit_txs);
            virtual bool                make_input() override;
            virtual bool                make_output() override;
            virtual bool                build_block(xvqcert_t * qcert_ptr) override;
        private:
            std::vector<xvbmaker_t*>    m_sub_units;
        };
    
        //xvfulltablemaker_t responsible to build a new block of full table
        class xvfulltablemaker_t : public xvtablemaker_t
        {
        };
    
    }//end of namespace of base

}//end of namespace top
