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
        class xvexecontxt_t;
        //xvbmaker_t responsible to build a new block based on a target_frame(block)
        //note: xvbmaker_t is not multiple thread safe,please use it at same thread
        class xvbmaker_t : public xvexecute_t
        {
        public:
            
        public:
             //block frame is not fullly filled block withtout input & outoput,but that has header & cert etc
            xvbmaker_t(const xvblock_t & target_frame);
        protected:
            virtual ~xvbmaker_t();
        private:
            xvbmaker_t();
            xvbmaker_t(xvbmaker_t &&);
            xvbmaker_t(const xvbmaker_t &);
            xvbmaker_t & operator = (xvbmaker_t &&);
            xvbmaker_t & operator = (const xvbmaker_t &);
 
        public: //public build api
            //note: caller should already verified those xvtransaction_t with valid signature,valid content ... etc
            virtual bool                   build_input(const std::vector<xvtransact_t*> & input_txs) = 0;//step#1
            virtual bool                   build_output() = 0;//step#2
            virtual xauto_ptr<xvblock_t>   build_block(); //step#3

        protected://internal use only
            inline xvblock_t*           get_block_frame()  const {return m_base_frame;}
            inline xvinput_t*           get_block_input()  const {return m_input_ptr;}
            inline xvoutput_t*          get_block_output() const {return m_output_ptr;}
            inline xvexecontxt_t*       get_exe_context()  const {return m_exe_context;}
            
            void                        reset_input(xvinput_t * new_ptr);
            void                        reset_output(xvoutput_t * new_ptr);
            
        private://the generated input & output ptr
            xvinput_t*                  m_input_ptr;     //new input
            xvoutput_t*                 m_output_ptr;    //new output
            //proposal block based on this frame
            xvblock_t*                  m_base_frame;    //block'frame as reference
        protected:
            xvexecontxt_t *             m_exe_context;
        };
    
        //xvunitmaker_t responsible to build a new block of unit
        class xvunitmaker_t : public xvbmaker_t
        {
        public:
            xvunitmaker_t(const xvblock_t & target_frame);
        protected:
            virtual ~xvunitmaker_t();
        private:
            xvunitmaker_t();
            xvunitmaker_t(xvunitmaker_t &&);
            xvunitmaker_t(const xvunitmaker_t &);
            xvunitmaker_t & operator = (xvunitmaker_t &&);
            xvunitmaker_t & operator = (const xvunitmaker_t &);
        
        public:
            virtual bool                 build_input(const std::vector<xvtransact_t*> & input_txs) override;
            virtual bool                 build_output() override;
        };
    
        class xvtablemaker_t : public xvbmaker_t
        {
            
        };
    
    }//end of namespace of base

}//end of namespace top
