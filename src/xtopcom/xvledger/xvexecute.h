// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
 
#include <string>
#include <vector>
#include "xvblock.h"

namespace top
{
    namespace base
    {
        class xvcontract_t;
        class xvexecontxt_t;
        //link xvaction_t,xvcontract_t and xvexestate_t together by following process:
            //xvaction_t->[xvexecute_t]->xvcontract_t->xvexestate_t
        //xvinentity_t->[xvexecute_t]->xvoutentity_t
        class xvexecute_t //multiple-thread safe only when xvexecontxt_t is safe on multiple threads
        {
        protected:
            xvexecute_t();
            virtual ~xvexecute_t();
        private:
            xvexecute_t(xvexecute_t &&);
            xvexecute_t(const xvexecute_t &);
            xvexecute_t & operator = (xvexecute_t &&);
            xvexecute_t & operator = (const xvexecute_t &);
            
        public://note: excution may modify input & output
            xauto_ptr<xvoutput_t>  execute_input(xvinput_t & input,xvexecontxt_t & context);    //execute
            //bool                   execute_output(const xvoutput_t & output,xvexestate_t & target_state); //execute
            //[input entity]->[contract]->[output_entity]
            xvoutentity_t*         execute_entity(xvinentity_t * input_entity,xvexecontxt_t & context);
            const xvalue_t         execute_action(xvaction_t & action,xvexecontxt_t & context);
        protected:

            
        public:
           
        protected:
            xauto_ptr<xvcontract_t>  load_contract(const xvaction_t & action);
        };

    }//end of namespace of base

}//end of namespace top
