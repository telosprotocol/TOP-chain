#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcontract/xaccount_state.h"
#include "xcontract_runtime/xvm/xtype.h"
#include "xcontract/xproperty_api.h"
#include "xcontract/xproperty_define.h"
#include "xcontract/xbasic_contract.h"

#include <string>
#include <system_error>

NS_BEG2(top, contract_runtime)

class xtop_execution_context {
    observer_ptr<contract::xaccount_state_t> account_state_{};
    vm::xtype_t vm_type_{vm::xtype_t::invalid};

public:
    xtop_execution_context(xtop_execution_context const &) = delete;
    xtop_execution_context & operator=(xtop_execution_context const &) = delete;
    xtop_execution_context(xtop_execution_context &&) = default;
    xtop_execution_context & operator=(xtop_execution_context &&) = default;
    ~xtop_execution_context() = default;

    xtop_execution_context(vm::xtype_t const vm_type, observer_ptr<contract::xaccount_state_t> account_state);

    XATTRIBUTE_NODISCARD observer_ptr<contract::xaccount_state_t> account_state() const noexcept;
    XATTRIBUTE_NODISCARD vm::xtype_t vm_type(std::error_code & ec) const noexcept;

    XATTRIBUTE_NODISCARD std::string contract_src_code(std::error_code & ec) const;
    XATTRIBUTE_NODISCARD xbyte_buffer_t contract_bin_code(std::error_code & ec) const;
    // XATTRIBUTE_NODISCARD xcontract_object_t contract_obj_code(std::error_code & ec) const;
};
using xexecution_context_t = xtop_execution_context;

NS_END2
