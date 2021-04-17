
#include "xcontract_runtime/xvm_session.h"

#include "xbasic/xmemory.hpp"
#include "xbasic/xscope_executer.h"
#include "xcontract_common/xcontract_execution_context.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_runtime/xvm/xruntime_face.h"
#include "xcontract_runtime/xuser/xuser_contract_runtime.h"
#include "xcontract_runtime/xerror/xerror.h"
#include "xdata/xconsensus_action.h"

#include <cassert>

NS_BEG2(top, contract_runtime)

xtop_session::xtop_session(observer_ptr<vm::xruntime_face_t> associated_runtime, observer_ptr<contract_common::xcontract_state_t> account_state) noexcept
  : m_associated_runtime{std::move(associated_runtime)}, m_contract_state{std::move(account_state)} {
}

xtransaction_execution_result_t xtop_session::execute_transaction(data::xcons_transaction_ptr_t const & tx) {
    xtransaction_execution_result_t result;
    auto * raw_tx = tx->get_transaction();
    assert(raw_tx != nullptr);
    raw_tx->add_ref();
    data::xtransaction_ptr_t origin_tx;
    origin_tx.attach(raw_tx);

    std::unique_ptr<contract_common::xcontract_execution_context_t> execution_context{top::make_unique<contract_common::xcontract_execution_context_t>(std::move(origin_tx), m_contract_state)};
    assert(m_associated_runtime != nullptr);
    auto observed_exectx = top::make_observer(execution_context.get());

    switch (tx->get_tx_subtype()) {
    case base::enum_transaction_subtype::enum_transaction_subtype_send : {
        xscope_executer_t reset_action{[&execution_context] {
            execution_context->execution_stage(contract_common::xcontract_execution_stage_t::invalid);
        }};
        execution_context->execution_stage(contract_common::xcontract_execution_stage_t::source_action);
        result = m_associated_runtime->execute_transaction(observed_exectx);
        if (result.status.ec) {
            return result;
        }

        return result;
    }

    case base::enum_transaction_subtype::enum_transaction_subtype_recv : {
        xscope_executer_t reset_action{[&execution_context] {
            execution_context->execution_stage(contract_common::xcontract_execution_stage_t::invalid);
            execution_context->receipt_data({});
        }};
        execution_context->execution_stage(contract_common::xcontract_execution_stage_t::target_action);
        execution_context->receipt_data(tx->get_receipt()->data());

        result = m_associated_runtime->execute_transaction(observed_exectx);
        if (result.status.ec) {
            return result;
        }

        return result;
    }

    case base::enum_transaction_subtype::enum_transaction_subtype_confirm : {
        xscope_executer_t reset_action{[&execution_context] {
            execution_context->execution_stage(contract_common::xcontract_execution_stage_t::invalid);
            execution_context->receipt_data({});
        }};
        execution_context->execution_stage(contract_common::xcontract_execution_stage_t::confirm_action);
        execution_context->receipt_data(tx->get_receipt()->data());

        result = m_associated_runtime->execute_transaction(observed_exectx);
        //if (result.status.ec) {
        //    return result;
        //}

        return result;
    }

    case base::enum_transaction_subtype::enum_transaction_subtype_self: {
        xscope_executer_t reset_action{[&execution_context] {
            execution_context->execution_stage(contract_common::xcontract_execution_stage_t::invalid);
        }};
        execution_context->execution_stage(contract_common::xcontract_execution_stage_t::self_action);
        result = m_associated_runtime->execute_transaction(observed_exectx);
        return result;
    }

    default: {
        assert(false);
        result.status.ec = error::xerrc_t::invalid_transaction_subtype;
        return result;
    }
    }
}

xtransaction_execution_result_t xtop_session::execute_action(data::xbasic_top_action_t const & action) {
    xtransaction_execution_result_t result;

    std::unique_ptr<contract_common::xcontract_execution_context_t> execution_context{ top::make_unique<contract_common::xcontract_execution_context_t>(action, m_contract_state) };
    assert(m_associated_runtime != nullptr);
    auto observed_exectx = top::make_observer(execution_context.get());

    switch (action.type()) {
    case data::xtop_action_type_t::system: {
        auto const & consensus_action = dynamic_cast<data::xconsensus_action_t<data::xtop_action_type_t::system> const &>(action);
        xscope_executer_t reset_action{ [&execution_context] {
            execution_context->execution_stage(contract_common::xcontract_execution_stage_t::invalid);
            execution_context->consensus_action_stage(data::xconsensus_action_stage_t::invalid);
        } };
        execution_context->execution_stage(contract_common::xcontract_execution_stage_t::source_action);
        execution_context->consensus_action_stage(consensus_action.stage());
        result = m_associated_runtime->execute_transaction(observed_exectx);
        if (result.status.ec) {
            return result;
        }

        return result;
    }

    //case base::enum_transaction_subtype::enum_transaction_subtype_recv:
    //{
    //    xscope_executer_t reset_action{ [&execution_context] {
    //        execution_context->execution_stage(contract_common::xcontract_execution_stage_t::invalid);
    //        execution_context->receipt_data({});
    //    } };
    //    execution_context->execution_stage(contract_common::xcontract_execution_stage_t::target_action);
    //    execution_context->receipt_data(tx->get_receipt()->data());

    //    result = m_associated_runtime->execute_transaction(observed_exectx);
    //    if (result.status.ec) {
    //        return result;
    //    }

    //    return result;
    //}

    //case base::enum_transaction_subtype::enum_transaction_subtype_confirm:
    //{
    //    xscope_executer_t reset_action{ [&execution_context] {
    //        execution_context->execution_stage(contract_common::xcontract_execution_stage_t::invalid);
    //        execution_context->receipt_data({});
    //    } };
    //    execution_context->execution_stage(contract_common::xcontract_execution_stage_t::confirm_action);
    //    execution_context->receipt_data(tx->get_receipt()->data());

    //    result = m_associated_runtime->execute_transaction(observed_exectx);
    //    //if (result.status.ec) {
    //    //    return result;
    //    //}

    //    return result;
    //}

    //case base::enum_transaction_subtype::enum_transaction_subtype_self:
    //{
    //    xscope_executer_t reset_action{ [&execution_context] {
    //        execution_context->execution_stage(contract_common::xcontract_execution_stage_t::invalid);
    //    } };
    //    execution_context->execution_stage(contract_common::xcontract_execution_stage_t::self_action);
    //    result = m_associated_runtime->execute_transaction(observed_exectx);
    //    return result;
    //}

    default:
    {
        assert(false);
        result.status.ec = error::xerrc_t::invalid_transaction_subtype;
        return result;
    }
    }
}


NS_END2
