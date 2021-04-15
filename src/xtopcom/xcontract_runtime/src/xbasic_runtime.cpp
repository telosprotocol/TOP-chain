#include "xcontract_runtime/xvm/xbasic_runtime.h"

#include "xbasic/xmemory.hpp"
#include "xcontract_common/xcontract_state.h"

NS_BEG3(top, contract_runtime, vm)

std::unique_ptr<xsession_t> xtop_basic_runtime::new_session(observer_ptr<contract_common::xcontract_state_t> contract_state) {
    return top::make_unique<xsession_t>(top::make_observer(this), contract_state);
}

NS_END3
