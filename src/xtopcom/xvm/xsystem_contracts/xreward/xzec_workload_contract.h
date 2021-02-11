// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvm/xcontract_helper.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"
#include "xdata/xtableblock.h"
#include "xstake/xstake_algorithm.h"

NS_BEG2(top, xstake)

using namespace xvm;
using namespace xvm::xcontract;

class xzec_workload_contract : public xcontract_base {
    using xbase_t = xcontract_base;
public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xzec_workload_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xzec_workload_contract);

    explicit
    xzec_workload_contract(common::xnetwork_id_t const & network_id);

    xcontract_base*  clone() override {return new xzec_workload_contract(network_id());}

    /**
     * @brief setup the contract
     *
     */
    void        setup();

    /**
     * @brief process on receiving workload
     *
     * @param workload_str workload
     */
    void on_receive_workload2(std::string const& workload_str);

    /**
     * @brief call zec reward contract to calculate reward
     *
     * @param timestamp the time to call
     */
    void on_timer(common::xlogic_time_t const timestamp);



    BEGIN_CONTRACT_WITH_PARAM(xzec_workload_contract)
        CONTRACT_FUNCTION_PARAM(xzec_workload_contract, on_receive_workload2);
        CONTRACT_FUNCTION_PARAM(xzec_workload_contract, on_timer);
    END_CONTRACT_WITH_PARAM

private:
    /**
     * @brief add cluster workload
     *
     * @param auditor true - auditor, false - validator
     * @param cluster_id cluster id
     * @param leader_count nodes workload
     */
    void        add_cluster_workload(bool auditor, std::string const& cluster_id, std::map<std::string, uint32_t> const& leader_count);

    /**
     * @brief Get the node info
     *
     * @param account node account
     * @param reg_node_info node registration object
     * @return int32_t 0 - success, other - failure
     */
    int32_t     get_node_info(const std::string& account, xreg_node_info& reg_node_info);

    /**
     * @brief check if mainnet is activated
     *
     * @return int 0 - not activated, other - activated
     */
    int         is_mainnet_activated();

    /**
     * @brief update tgas
     *
     * @param table_pledge_balance_change_tgas table pledge balance change tgas
     */
    void        update_tgas(int64_t table_pledge_balance_change_tgas);

    /**
     * @brief check if we can dispatch tgas now
     *
     * @param onchain_timer_round chain timer height
     * @return true we can now
     * @return false we can not now
     */
    bool        tgas_is_expire(const uint64_t onchain_timer_round);

    /**
     * @brief clear the workload
     */
    void        clear_workload();
};

NS_END2
