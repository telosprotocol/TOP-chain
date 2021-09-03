#include "xsystem_contracts/xelection/rec/xrec_standby_pool_contract.h"

NS_BEG2(top, system_contracts)

void xtop_rec_standby_pool_contract_new::setup() {
}

void xtop_rec_standby_pool_contract_new::nodeJoinNetwork2(common::xaccount_address_t const & node_id,
                                                          common::xnetwork_id_t const & joined_network_id,
#if defined(XENABLE_MOCK_ZEC_STAKE)
                                                          common::xrole_type_t role_type,
                                                          std::string const & consensus_public_key,
                                                          uint64_t const stake,
#endif
                                                          std::string const & program_version) {
}

void xtop_rec_standby_pool_contract_new::on_timer(common::xlogic_time_t const current_time) {
}

NS_END2