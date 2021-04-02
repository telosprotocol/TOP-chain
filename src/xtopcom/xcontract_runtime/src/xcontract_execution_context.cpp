
//
//#if defined(__clang__)
//
//#    pragma clang diagnostic push
//#    pragma clang diagnostic ignored "-Wpedantic"
//
//#elif defined(__GNUC__)
//
//#    pragma GCC diagnostic push
//#    pragma GCC diagnostic ignored "-Wpedantic"
//
//#elif defined(_MSC_VER)
//
//#    pragma warning(push, 0)
//
//#endif
//
//#include "xbase/xvstate.h"
//
//#if defined(__clang__)
//#    pragma clang diagnostic pop
//#elif defined(__GNUC__)
//#    pragma GCC diagnostic pop
//#elif defined(_MSC_VER)
//#    pragma warning(pop)
//#endif
//
//#include "xcontract_runtime/xcontract_execution_context.h"
//#include "xcontract_runtime/xerror/xerror.h"
//
//#include <cassert>
//
//NS_BEG2(top, contract_runtime)
//
//xtop_execution_context::xtop_execution_context(vm::xtype_t const vm_type, observer_ptr<contract::xaccount_state_t> account_state)
//  : account_state_{std::move(account_state)}, vm_type_{vm_type} {
//}
//
//observer_ptr<contract::xaccount_state_t> xtop_execution_context::account_state() const noexcept {
//    return account_state_;
//}
//
//vm::xtype_t xtop_execution_context::vm_type(std::error_code & ec) const noexcept {
//    assert(!ec);
//
//    switch (vm_type_) {
//    case vm::xtype_t::lua:
//        XATTRIBUTE_FALLTHROUGH;
//    case vm::xtype_t::wasm:
//        XATTRIBUTE_FALLTHROUGH;
//    case vm::xtype_t::system:
//        return vm_type_;
//
//    default:
//        assert(false);
//        break;
//    }
//
//    ec = error::xerrc_t::invalid_vm_type;
//    return vm_type_;
//}
//
//std::string xtop_execution_context::contract_src_code(std::error_code & ec) const {
//    assert(!ec);
//    auto type = vm_type(ec);
//    if (ec) {
//        return {};
//    }
//
//    if (type == vm::xtype_t::lua) {
//        return account_state_->get_bstate()->load_code_var("@code")->get_code();
//    }
//
//    ec = error::xerrc_t::invalid_vm_type;
//    return {};
//}
//
//xbyte_buffer_t xtop_execution_context::contract_bin_code(std::error_code & ec) const {
//    assert(!ec);
//    auto type = vm_type(ec);
//    if (ec) {
//        return {};
//    }
//
//    if (type == vm::xtype_t::wasm) {
//        return {};
//    }
//
//    ec = error::xerrc_t::invalid_vm_type;
//    return {};
//}
//
//NS_END2
