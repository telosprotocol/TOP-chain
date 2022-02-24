// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xsystem_contract/xdata_structures.h"
#include "xdata/xtableblock.h"
#include "xvm/xcontract/xcontract_base.h"
#include "xvm/xcontract/xcontract_exec.h"
#include "xvm/xcontract_helper.h"

NS_BEG2(top, xstake)

using namespace xvm;
using namespace xvm::xcontract;

class xzec_vote_contract : public xcontract_base {
    using xbase_t = xcontract_base;
public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xzec_vote_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xzec_vote_contract);

    explicit
    xzec_vote_contract(common::xnetwork_id_t const & network_id);

    xcontract_base*  clone() override {return new xzec_vote_contract(network_id());}

    /**
     * @brief setup the contract
     *
     */
    void        setup();

    /**
     * @brief receive contract nodes votes
     *
     * @param report_time
     * @param contract_adv_votes
     */
    void on_receive_shard_votes_v2(uint64_t report_time, std::map<std::string, std::string> const & contract_adv_votes);

    BEGIN_CONTRACT_WITH_PARAM(xzec_vote_contract)
        CONTRACT_FUNCTION_PARAM(xzec_vote_contract, on_receive_shard_votes_v2);
    END_CONTRACT_WITH_PARAM

private:
    /**
     * @brief check if mainnet is activated
     *
     * @return int 0 - not activated, other - activated
     */
    int         is_mainnet_activated();

    /**
     * @brief
     *
     * @param report_time
     * @param last_report_time
     * @param contract_adv_votes
     * @param merge_contract_adv_votes
     * @return true
     * @return false
     */
    bool        handle_receive_shard_votes(uint64_t report_time, uint64_t last_report_time, std::map<std::string, std::string> const & contract_adv_votes, std::map<std::string, std::string> & merge_contract_adv_votes);
};

NS_END2
