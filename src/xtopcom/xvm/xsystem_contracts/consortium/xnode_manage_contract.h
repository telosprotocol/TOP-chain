// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xblock_statistics_data.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xdata/xtableblock.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"
#include "xvm/xcontract_helper.h"


NS_BEG3(top, xvm, consortium)

using namespace xvm;
using namespace xvm::xcontract;

class xnode_manage_contract : public xcontract::xcontract_base {
    using xbase_t = xcontract::xcontract_base;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xnode_manage_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xnode_manage_contract);

    explicit xnode_manage_contract(common::xnetwork_id_t const& network_id);

    xcontract_base*
    clone() override { return new xnode_manage_contract(network_id()); }

    /**
     * @brief setup the contract
     *
     */
    void setup();

    /**
     * @brief  
     * @note   
     * @param  account_str: 
     * @param  expiry_time: 
     * @param  account_cert: 
     * @retval None
     */
    void nodeInfoReg(std::string const& account_str, uint64_t const expiry_time, std::string const& account_cert);

    /**
     * @brief  
     * @note   
     * @param  unreg_account_str: 
     * @retval None
     */
    void nodeInfoUnreg(std::string const& unreg_account_str);

    /**
     * @brief  
     * @note   
     * @param  root_ca: 
     * @retval None
     */
    void nodeInfoRootCaReplace(std::string const& root_account, std::string const& root_ca);


    /**
     * @brief  
     * @note   
     * @param  check_type: 
     * @param  check_flag: 
     * @retval None
     */
    void nodeInfoAuthConfig(std::string const& check_type, std::string const& check_flag);

    BEGIN_CONTRACT_WITH_PARAM(xnode_manage_contract)
    CONTRACT_FUNCTION_PARAM(xnode_manage_contract, nodeInfoReg);
    CONTRACT_FUNCTION_PARAM(xnode_manage_contract, nodeInfoUnreg);
    CONTRACT_FUNCTION_PARAM(xnode_manage_contract, nodeInfoRootCaReplace);
    CONTRACT_FUNCTION_PARAM(xnode_manage_contract, nodeInfoAuthConfig);
    END_CONTRACT_WITH_PARAM

    
private:

    /**
     * @brief  
     * @note   
     * @param  account_str: 
     * @param  out_str: 
     * @retval 
     */
    bool nodeInfoExist(std::string const& account_str, std::string& out_str);

    /**
     * @brief  
     * @note   
     * @param  reg_account_info: 
     * @retval 
     */
    bool nodeInfoValidateCheck(data::system_contract::xnode_manage_account_info_t& reg_account_info);

    /**
     * @brief  
     * @note   
     * @param  reg_account_info: 
     * @retval 
     */
    bool nodeInfoValidateCaCheck(data::system_contract::xnode_manage_account_info_t& reg_account_info);

    /**
     * @brief  
     * @note   
     * @retval None
     */
    void nodeInfoCaRebase();

    /**
     * @brief  
     * @note   
     * @param  reg_account_info: 
     * @retval 
     */
    bool nodeInfoExpiryTimeCheck(data::system_contract::xnode_manage_account_info_t const& reg_account_info);

    /**
     * @brief  
     * @note   
     * @param  reg_account_info: 
     * @retval None
     */
    void nodeInfoInsert(data::system_contract::xnode_manage_account_info_t& reg_account_info);

    /**
     * @brief  
     * @note   
     * @param  reg_account_info: 
     * @retval None
     */
    void nodeInfoRemove(data::system_contract::xnode_manage_account_info_t& reg_account_info);

};

using xnode_manage_contract_t = xnode_manage_contract;

NS_END3
