// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assert.h>
#include "xtxstore/xtransaction_prepare.h"

#include "xbase/xlog.h"
#include "xtxexecutor/xunit_service_error.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xverifier/xverifier_utl.h"

using namespace top::data;

NS_BEG2(top, txexecutor)


int32_t xtransaction_prepare_t::parse() {
    switch (m_trans->get_transaction()->get_tx_type()) {
#ifdef ENABLE_CREATE_USER  // debug use
        case xtransaction_type_create_user_account:
            m_trans_obj = std::make_shared<xtransaction_create_user_account>(m_account_ctx, m_trans);
            break;
#endif
        case xtransaction_type_run_contract:
            m_trans_obj = std::make_shared<xtransaction_run_contract>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_transfer:
            m_trans_obj = std::make_shared<xtransaction_transfer>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_vote:
            m_trans_obj = std::make_shared<xtransaction_vote>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_abolish_vote:
            m_trans_obj = std::make_shared<xtransaction_abolish_vote>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_pledge_token_tgas:
            m_trans_obj = std::make_shared<xtransaction_pledge_token_tgas>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_redeem_token_tgas:
            m_trans_obj = std::make_shared<xtransaction_redeem_token_tgas>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_pledge_token_vote:
            m_trans_obj = std::make_shared<xtransaction_pledge_token_vote>(m_account_ctx, m_trans);
            break;
        case xtransaction_type_redeem_token_vote:
            m_trans_obj = std::make_shared<xtransaction_redeem_token_vote>(m_account_ctx, m_trans);
            break;
        default:
            xwarn("invalid tx type:%d", m_trans->get_transaction()->get_tx_type());
            return xtransaction_parse_type_invalid;
    }
    try {
        auto ret = m_trans_obj->parse();
        if (ret != xsuccess) {
            xwarn("[global_trace][unit_service][parse][fail]%s, ret:0x%x error:%s",
                m_trans->get_digest_hex_str().c_str(), ret, chainbase::xmodule_error_to_str(ret).c_str());
            return ret;
        }
    }
    catch(const std::exception& e) {
        xwarn("[global_trace][unit_service][parse][fail]%s, error:xchain_error_action_parse_exception",
            m_trans->get_digest_hex_str().c_str());
        return xchain_error_action_parse_exception;
    }    
    return xsuccess;
}
int32_t xtransaction_prepare_t::check() {
    int32_t ret;
    if (m_trans_obj == nullptr) {
        ret = parse();
        if (ret != xsuccess) {
            return ret;
        }
    }

    ret = m_trans_obj->check();

    if (ret != xsuccess) {
        xdbg("check fail:%d", ret);
        return ret;
    }
    return xsuccess;
}

std::string xtransaction_prepare_t::get_err_msg(const int32_t type) const {
    if (type == xverifier::xverifier_error::xverifier_error_src_dst_addr_same)
        return "Source address and destination address are the same";
    else if (type == xverifier::xverifier_error::xverifier_error_tx_min_deposit_invalid)
        return "Insufficient deposit";
    else if (type == xverifier::xverifier_error::xverifier_error_transfer_tx_min_amount_invalid)
        return "Transfer amount is 0";
    else if (type == xverifier::xverifier_error::xverifier_error_transfer_tx_amount_over_max)
        return "Transfer amount is greater than 20 billion TOP";
    else if (type == txexecutor::enum_xunit_service_error_type::xtransaction_pledge_redeem_vote_err)
        return "Voting check failed";
    return "unknown error";
}

NS_END2
