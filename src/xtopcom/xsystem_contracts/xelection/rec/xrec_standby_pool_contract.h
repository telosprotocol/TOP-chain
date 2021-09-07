#pragma once

#include "xcontract_common/xcontract_execution_result.h"
#include "xcontract_common/xcontract_execution_context.h"
#include "xsystem_contracts/xbasic_system_contract.h"
#include "xcontract_common/xproperties/xproperty_string.h"
#include "xcontract_runtime/xsystem/xsystem_contract_runtime_helper.h"

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

    void setup();

    void nodeJoinNetwork2(common::xaccount_address_t const & node_id,
                          common::xnetwork_id_t const & joined_network_id,
#if defined XENABLE_MOCK_ZEC_STAKE
                          common::xrole_type_t role_type,
                          std::string const & pubkey,
                          uint64_t const stake,
#endif
                          std::string const & program_version);

    void on_timer(common::xlogic_time_t const current_time);

    BEGIN_CONTRACT_API()
        DECLARE_API(xtop_rec_standby_pool_contract_new::setup);
        DECLARE_API(xtop_rec_standby_pool_contract_new::nodeJoinNetwork2);
        DECLARE_API(xtop_rec_standby_pool_contract_new::on_timer);
    END_CONTRACT_API
private:
    contract_common::properties::xstring_property_t m_standby_prop{data::XPROPERTY_CONTRACT_STANDBYS_KEY, this};
};
using xrec_standby_pool_contract_new_t = xtop_rec_standby_pool_contract_new;

NS_END2