#pragma once

#include "xcontract_common/xcontract_execution_context.h"
#include "xcontract_common/xcontract_execution_result.h"
#include "xcontract_common/xproperties/xproperty_string.h"
#include "xcontract_runtime/xsystem/xsystem_contract_runtime_helper.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xstake/xstake_algorithm.h"
#include "xsystem_contracts/xbasic_system_contract.h"

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

    BEGIN_CONTRACT_API()
        DECLARE_API(xtop_rec_standby_pool_contract_new::setup);
        DECLARE_API(xtop_rec_standby_pool_contract_new::nodeJoinNetwork2);
        DECLARE_API(xtop_rec_standby_pool_contract_new::on_timer);
    END_CONTRACT_API

private:
    /**
     * @brief init contract data
     */
    void setup();

    /**
     * @brief node join network
     * 
     * @param node_id node_id
     * @param joined_network_id network id
     * @param role_type role_type
     * @param pubkey public key
     * @param stake stake
     * @param program_version program_version
     */
    void nodeJoinNetwork2(common::xaccount_address_t const & node_id,
                          common::xnetwork_id_t const & joined_network_id,
#if defined XENABLE_MOCK_ZEC_STAKE
                          common::xrole_type_t role_type,
                          std::string const & pubkey,
                          uint64_t const stake,
#endif
                          std::string const & program_version);

    /**
     * @brief node join network impl
     * 
     * @param program_version program_version
     * @param node node info
     * @param standby_result_store result to return
     * @return success/fail
     */
    bool nodeJoinNetworkImpl(std::string const & program_version, xstake::xreg_node_info const & node, data::election::xstandby_result_store_t & standby_result_store);

    /**
     * @brief on_timer
     * 
     * @param current_time current_time
     */
    void on_timer(common::xlogic_time_t const current_time);

    /**
     * @brief update_standby_result_store
     * 
     * @param registration_data registration_data
     * @param standby_result_store result to return
     * @param activation_record activation_record
     * @return success/fail
     */
    bool update_standby_result_store(std::map<common::xnode_id_t, xstake::xreg_node_info> const & registration_data,
                                     data::election::xstandby_result_store_t & standby_result_store,
                                     xstake::xactivation_record const & activation_record);

    /**
     * @brief update_standby_node
     * 
     * @param reg_node node info
     * @param standby_result_store result to return
     * @return success/fail
     */
    bool update_standby_node(top::xstake::xreg_node_info const & reg_node, top::data::election::xstandby_node_info_t & standby_node_info) const;

    /**
     * @brief update_activated_state
     * 
     * @param standby_network_storage_result result to return
     * @param activation_record activation_record
     * @return success/fail
     */
    bool update_activated_state(data::election::xstandby_network_storage_result_t & standby_network_storage_result, xstake::xactivation_record const & activation_record);

private:
    contract_common::properties::xstring_property_t m_standby_prop{data::XPROPERTY_CONTRACT_STANDBYS_KEY, this};
};
using xrec_standby_pool_contract_new_t = xtop_rec_standby_pool_contract_new;

NS_END2