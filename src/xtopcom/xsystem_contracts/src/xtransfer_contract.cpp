#include "xsystem_contracts/xtransfer_contract.h"

#include "xdata/xgenesis_data.h"

NS_BEG2(top, system_contracts)

void xtop_transfer_contract::setup() {
    state_accessor::xtoken_t token{100};
    m_balance.deposit(std::move(token));
}

void xtop_transfer_contract::transfer(uint64_t const amount) {
    m_balance.withdraw(amount);
}

NS_END2
