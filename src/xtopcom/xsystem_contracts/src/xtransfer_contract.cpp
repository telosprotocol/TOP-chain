#include "xsystem_contracts/xtransfer_contract.h"

#include "xdata/xgenesis_data.h"

NS_BEG2(top, system_contracts)

void xtop_transfer_contract::setup() {
}

void xtop_transfer_contract::transfer(uint64_t const amount) {
    m_balance.withdraw(amount);
}

void xtop_transfer_contract::follow_up_delay() {
    for (auto i = 0; i < enum_vledger_const::enum_vbucket_has_tables_count; i++) {
        auto contract = data::make_address_by_prefix_and_subaddr(sys_contract_sharding_reward_claiming_addr, uint16_t(i));
        std::error_code ec;
        contract_common::xbasic_contract_t::transfer(contract, i * 1000, contract_common::xfollowup_transaction_schedule_type_t::delay, ec);
    }
}

NS_END2
