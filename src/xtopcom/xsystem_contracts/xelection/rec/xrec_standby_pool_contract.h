#pragma once

#include "xcontract_common/xcontract_execution_result.h"
#include "xcontract_common/xcontract_execution_context.h"
#include "xsystem_contracts/xbasic_system_contract.h"
#include "xcontract_common/xproperties/xproperty_token.h"
#include "xcontract_runtime/xsystem/xsystem_contract_runtime_helper.h"
// #include "xcommon/xip.h"
// #include "xcommon/xlogic_time.h"
// #include "xcommon/xrole_type.h"
// #include "xdata/xelection/xstandby_result_store.h"
// #include "xstake/xstake_algorithm.h"
// #include "xvm/xcontract/xcontract_base.h"
// #include "xvm/xcontract/xcontract_exec.h"

// #include <string>

NS_BEG2(top, system_contracts)

class xtop_rec_standby_pool_contract_new final : public xbasic_system_contract_t {
    using xbase_t = xbasic_system_contract_t;

public:
    xtop_rec_standby_pool_contract_new() = default;
    xtop_rec_standby_pool_contract_new(xtop_rec_standby_pool_contract_new const &) = delete;
    xtop_rec_standby_pool_contract_new & operator=(xtop_rec_standby_pool_contract_new const &) = delete;
    xtop_rec_standby_pool_contract_new(xtop_rec_standby_pool_contract_new &&) = default;
    xtop_rec_standby_pool_contract_new & operator=(xtop_rec_standby_pool_contract_new &&) = default;
    ~xtop_rec_standby_pool_contract_new() override = default;

    void setup() {}

    void nodeJoinNetwork2(common::xaccount_address_t const & node_id,
                          common::xnetwork_id_t const & joined_network_id,
#if defined XENABLE_MOCK_ZEC_STAKE
                          common::xrole_type_t role_type,
                          std::string const & pubkey,
                          uint64_t const stake,
#endif
                          std::string const & program_version) {}

    void on_timer(common::xlogic_time_t const current_time) {}

    BEGIN_CONTRACT_API()
        DECLARE_API(xtop_rec_standby_pool_contract_new::setup);
        DECLARE_API(xtop_rec_standby_pool_contract_new::nodeJoinNetwork2);
        DECLARE_API(xtop_rec_standby_pool_contract_new::on_timer);
    END_CONTRACT_API
private:

//     explicit xtop_rec_standby_pool_contract(common::xnetwork_id_t const & network_id);

//     xcontract::xcontract_base * clone() override { return new xtop_rec_standby_pool_contract(network_id()); }

// private:

//     bool nodeJoinNetworkImpl(std::string const & program_version,
//                              xstake::xreg_node_info const & node,
//                              data::election::xstandby_result_store_t & standby_result_store);

//     bool update_standby_result_store(std::map<common::xnode_id_t, xstake::xreg_node_info> const & registration_data,
//                                      data::election::xstandby_result_store_t & standby_result_store,
//                                      xstake::xactivation_record const & activation_record);

//     bool update_standby_node(top::xstake::xreg_node_info const & reg_node, top::data::election::xstandby_node_info_t & standby_node_info) const;

//     bool update_activated_state(data::election::xstandby_network_storage_result_t & standby_network_storage_result, xstake::xactivation_record const & activation_record);
};
using xrec_standby_pool_contract_new_t = xtop_rec_standby_pool_contract_new;

NS_END2