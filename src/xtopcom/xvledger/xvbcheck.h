// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
 
#include <string>
#include <vector>
#include "xvtransact.h"
#include "xvaction.h"
#include "xventity.h"
#include "xvblock.h"
#include "xvexecute.h"
#include "xvstate.h"
#include "xbase/xdata.h"
#include "xvstatestore.h"

namespace top
{
    namespace base
    {
        class xvchecker_t : public xvexecute_t
        {
        public:
            xvchecker_t();
        public:
            virtual bool  check_input();
            virtual bool  check_all(xvblock_t & proposal);
        };
        
        class xvunitchecker_t : public xvchecker_t
        {
        public:
            virtual bool  verify_output(xvblock_t & proposal)
            {
                if(proposal.get_block_class() == enum_xvblock_class_nil)
                    return true;
                
                if(proposal.get_block_level() != enum_xvblock_level_unit)
                    return false;
                
                xvinput_t * input_ptr = proposal.get_input();
                if(input_ptr == NULL)
                    return false;
                
                xvoutput_t * output_ptr = proposal.get_output();
                if(output_ptr == NULL)
                    return false;
                
                const std::vector<xventity_t*> & input_entitis  = input_ptr->get_entitys();
                const std::vector<xventity_t*> & output_entitis = output_ptr->get_entitys();
                if(input_entitis.empty() || output_entitis.empty())
                    return false;
                
                if(input_entitis.size() != output_entitis.size())
                    return false;
                
                for(int indx = 0; indx < (int)input_entitis.size(); ++indx)
                {
                    xventity_t * ent = input_entitis[indx];
                    if(NULL == ent)
                        return false;
                    
                    xvinentity_t *input_ent = (xvinentity_t*)ent->query_interface(enum_xobject_type_vinentity);
                    if(NULL == input_ent)
                        return false;
                    
                    //xauto_ptr<xvoutentity_t> output_ent(execute(input_ent));
                    //if(output_entitis[indx] != *output_ent)
                    //    return false;
                }
                return true;
            }
        };

    }//end of namespace of base

}//end of namespace top
