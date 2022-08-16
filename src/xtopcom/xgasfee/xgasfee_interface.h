#pragma once
#include "xgasfee/xgas_state_operator.h"
#include "xgasfee/xgas_tx_operator.h"
#include "xtxexecutor/xvm_face.h"

namespace top {
namespace gasfee {
class xgasfee_interface : public xgas_state_operator_t, public xgas_tx_operator_t {
public:
    xgasfee_interface(std::shared_ptr<data::xunit_bstate_t> const & state, xobject_ptr_t<data::xcons_transaction_t> const & tx, uint64_t time, uint64_t onchain_tgas_deposit)
      : xgas_state_operator_t(state), xgas_tx_operator_t(tx), m_time(time), m_onchain_tgas_deposit(onchain_tgas_deposit) {
        xassert(state != nullptr);
        xassert(tx != nullptr);
    }

public:
    virtual void preprocess(std::error_code & ec) = 0;
    virtual void postprocess(const evm_common::u256 supplement_gas, std::error_code & ec) = 0;

    txexecutor::xvm_gasfee_detail_t gasfee_detail() const { return m_detail;}
    virtual evm_common::u256 get_tx_eth_gas_limit() const = 0;
protected:    
    // output
    txexecutor::xvm_gasfee_detail_t m_detail;
    // onchain related param
    uint64_t m_time{0};
    uint64_t m_onchain_tgas_deposit{0};
};

}  // namespace gasfee
}  // namespace top    