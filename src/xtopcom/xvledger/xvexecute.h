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
        class xvexecute_t //multiple-thread safe
        {
        protected:
            xvexecute_t();
            virtual ~xvexecute_t();
        private:
            xvexecute_t(xvexecute_t &&);
            xvexecute_t(const xvexecute_t &);
            xvexecute_t & operator = (xvexecute_t &&);
            xvexecute_t & operator = (const xvexecute_t &);
            
        public:
            xauto_ptr<xvoutput_t>  execute_input(const xvinput_t & input,xvexecontxt_t & context);    //execute
            //bool                   execute_output(const xvoutput_t & output,xvexestate_t & target_state); //execute
            xvalue_t               execute_action(xvaction_t & action,xvexecontxt_t & context);
        protected:
            //[input entity]->[contract]->[output_entity]
            xvoutentity_t*         execute_entity(const xvinentity_t * input_entity,xvexecontxt_t & context);
            
        public:
           
        protected:
            xauto_ptr<xvcontract_t>  load_contract(const xvaction_t & action);
        };

    }//end of namespace of base

}//end of namespace top
