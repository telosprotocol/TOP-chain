#include "xsystem_contracts/xtransfer_contract.h"

#include "xdata/xgenesis_data.h"

NS_BEG2(top, system_contracts)

void xtop_transfer_contract::setup() {
}

void xtop_transfer_contract::transfer(uint64_t const amount) {
    m_balance.withdraw(amount);
}

NS_END2
