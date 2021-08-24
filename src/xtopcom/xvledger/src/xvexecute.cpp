// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//#include <cinttypes>
#include "../xvcontract.h"
#include "../xventity.h"
#include "../xvexecontxt.h"
#include "../xvexecute.h"
#include "../xvledger.h"

namespace top
{
    namespace base
    {
        xvexecute_t::xvexecute_t()
        {
        }
    
        xvexecute_t::~xvexecute_t()
        {
        }
    
        xauto_ptr<xvcontract_t>  xvexecute_t::load_contract(const xvaction_t & action)
        {
            return xvchain_t::instance().get_xcontractstore()->get_contract(action.get_contract_uri());
        }
    
        const xvalue_t  xvexecute_t::execute_action(xvaction_t & action,xvexecontxt_t & context)
        {
            xauto_ptr<xvcontract_t> target_contract(load_contract(action));
            if(target_contract)
            {
                action.copy_result(target_contract->execute(action,context));
                if( (action.get_method_result()->get_type() != xvalue_t::enum_xvalue_type_error) || (action.get_method_result()->get_error() == enum_xcode_successful) )
                {
                    xinfo("xvexecute_t::execute_action,successful execute contract(%s)->function(%s)", action.get_contract_uri().c_str(),action.get_method_name().c_str());
                    return enum_xcode_successful;
                }
                else
                {
                    xerror("xvexecute_t::execute_action,get error(%d) to execute contract(%s)->function(%s)",action.get_method_result()->get_error(), action.get_contract_uri().c_str(),action.get_method_name().c_str());
                    
                    return action.get_method_result()->get_error();
                }
            }
            else
            {
                xerror("xvexecute_t::execute_action,fail to load contract(%s)",action.get_contract_uri().c_str());
            }
            return enum_xerror_code_not_implement;
        }
     
        //[input entity]->[contract]->[output_entity]
        xvoutentity_t*  xvexecute_t::execute_entity(xvinentity_t * input_entity,xvexecontxt_t & context)
        {
            xauto_ptr<xvcanvas_t> output_canvas(new xvcanvas_t());
            
            context.reset_output_canvas(output_canvas());
            xassert(input_entity != NULL);
            if(input_entity != NULL)
            {
                for(auto & action : input_entity->get_actions())
                {
                    xauto_ptr<xvcontract_t> target_contract(load_contract(action));
                    if(target_contract)
                    {
                        //execute need put result into action(modify it)
                        ((xvaction_t&)action).copy_result(target_contract->execute(action,context));
                        if( (action.get_method_result()->get_type() != xvalue_t::enum_xvalue_type_error) || (action.get_method_result()->get_error() == enum_xcode_successful) )
                        {
                            xinfo("xvexecute_t::execute_entity,successful execute contract(%s)->function(%s)", action.get_contract_uri().c_str(),action.get_method_name().c_str());
                        }
                        else
                        {
                            xerror("xvexecute_t::execute_entity,get error(%d) to execute contract(%s)->function(%s)",action.get_method_result()->get_error(), action.get_contract_uri().c_str(),action.get_method_name().c_str());
                            //let continue execute rest ones
                        }
                    }
                    else
                    {
                        xerror("xvexecute_t::execute_entity,fail to load contract(%s)",action.get_contract_uri().c_str());
                        //let continue execute rest ones
                    }
                }
            }
            //reset & clean existing output canvas then
            context.reset_output_canvas(NULL);
            
            std::string output_bin_log;
            output_canvas->encode(output_bin_log);
            return new xvoutentity_t(output_bin_log);
        }
        
        //execute may put result into action
        xauto_ptr<xvoutput_t> xvexecute_t::execute_input(xvinput_t & input,xvexecontxt_t & context) //execute
        {
            const std::vector<xventity_t*> & input_entities  = input.get_entitys();
            if(input_entities.empty())
                return nullptr;
            
            //pre-check first
            for(auto & ent : input_entities)
            {
                if(NULL == ent)
                {
                    xerror("xvexecute_t::execute_input,found nil entity of input (%s)",input.dump().c_str());
                    return nullptr;
                }
                if(NULL == ent->query_interface(enum_xobject_type_vinentity))
                {
                    xerror("xvexecute_t::execute_input,found invalid entity of input (%s)",input.dump().c_str());
                    return nullptr;
                }
            }
            
            std::vector<xventity_t*> output_entities;
            for(auto & ent : input_entities)
            {
                xvinentity_t * input_ent  = (xvinentity_t*)ent->query_interface(enum_xobject_type_vinentity);
                xvoutentity_t* output_ent = execute_entity(input_ent,context);
                output_entities.emplace_back(output_ent);//execute_entity always generate output entity
                //let continue execute rest ones even has error
            }
            xvoutput_t *  output_ptr = new xvoutput_t(std::move(output_entities));
            return output_ptr;
        }

    };//end of namespace of base
};//end of namespace of top
