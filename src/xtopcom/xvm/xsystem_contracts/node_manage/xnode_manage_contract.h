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


NS_BEG4(top, xvm, system_contracts, rec)

using namespace xvm;
using namespace xvm::xcontract;

class xnode_manage_contract final : public xcontract_base {
    using xbase_t = xcontract_base;

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
     * @param  account_info_str:
     * @retval None
     */
    void node_info_reg(std::string const& account_info_str);


    BEGIN_CONTRACT_WITH_PARAM(xnode_manage_contract)
    CONTRACT_FUNCTION_PARAM(xnode_manage_contract, node_info_reg);
    END_CONTRACT_WITH_PARAM

private:

    /**
     * @brief
     * @note
     * @param  reg_account_info:
     * @retval
     */
    bool node_info_reg_exist(data::system_contract::xnode_manage_account_info_t const& reg_account_info);

        /**
     * @brief
     * @note
     * @param  reg_account_info:
     * @retval
     */
    bool node_info_reg_check(data::system_contract::xnode_manage_account_info_t const& reg_account_info);

    
    /**
     * @brief
     * @note
     * @param  reg_account_info:
     * @retval
     */
    void node_info_reg_add(data::system_contract::xnode_manage_account_info_t & reg_account_info);
};

using xnode_manage_contract_t = xnode_manage_contract;

NS_END4
