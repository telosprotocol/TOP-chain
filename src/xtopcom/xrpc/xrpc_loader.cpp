#include "xvledger/xvtxindex.h"
#include "xdata/xlightunit_info.h"
#include "xdata/xtransaction.h"
#include "xdata/xdatautil.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvtxstore.h"
#include "xrpc/xrpc_loader.h"
#include "xtxexecutor/xtransaction_fee.h"

namespace top {

namespace xrpc {

xtxindex_detail_t::xtxindex_detail_t(const base::xvtxindex_ptr & txindex, const base::xvaction_t & txaction)
: m_txindex(txindex), m_txaction(txaction) {
}

void xtxindex_detail_t::set_raw_tx(base::xdataunit_t* tx) {
    if (m_raw_tx == nullptr && tx != nullptr) {
        data::xtransaction_t* _tx_ptr = dynamic_cast<data::xtransaction_t*>(tx);
        _tx_ptr->add_ref();
        m_raw_tx.attach(_tx_ptr);
    }
}

xtxindex_detail_ptr_t  xrpc_loader_t::load_tx_indx_detail(const std::string & raw_tx_hash,base::enum_transaction_subtype type) {
    base::xauto_ptr<base::xvtxindex_t> txindex = base::xvchain_t::instance().get_xtxstore()->load_tx_idx(raw_tx_hash, type);
    if (nullptr == txindex) {
        xwarn("xrpc_loader_t::load_tx_indx_detail,fail to index for hash:%s,type:%d", base::xstring_utl::to_hex(raw_tx_hash).c_str(), type);
        return nullptr;
    }  

    base::xvaccount_t _vaddress(txindex->get_block_addr());
    auto _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(_vaddress, txindex->get_block_height(), base::enum_xvblock_flag_committed, false);
    if (nullptr == _block) {
        xwarn("xrpc_loader_t::load_tx_indx_detail,fail to load block for hash:%s,type:%d", base::xstring_utl::to_hex(raw_tx_hash).c_str(), type);
        return nullptr;
    }
    std::vector<base::xvaction_t> txaction = _block->get_one_tx_action(raw_tx_hash);
    if (txaction.empty()) {
        xerror("xrpc_loader_t::load_tx_indx_detail,fail to load action for hash:%s,type:%d", base::xstring_utl::to_hex(raw_tx_hash).c_str(), type);
        return nullptr;
    }
    xtxindex_detail_ptr_t index_detail = std::make_shared<xtxindex_detail_t>(txindex, txaction[0]);
    if (type == base::enum_transaction_subtype_self || type == base::enum_transaction_subtype_send) {
        if (false == base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaddress, _block.get())) {
            xwarn("xrpc_loader_t::load_tx_indx_detail,fail to load block input for hash:%s,type:%d", base::xstring_utl::to_hex(raw_tx_hash).c_str(), type);
            return nullptr;
        }
        std::string orgtx_bin = _block->get_input()->query_resource(raw_tx_hash);
        if (orgtx_bin.empty())
        {
            xerror("xvblockstore_impl::query_tx fail-query tx from send unit.account=%s,tx=%s", txindex->get_block_addr().c_str(), base::xstring_utl::to_hex(raw_tx_hash).c_str());
            return nullptr;
        }
        base::xauto_ptr<base::xdataunit_t> raw_tx = base::xdataunit_t::read_from(orgtx_bin);
        if(nullptr == raw_tx)
        {
            xerror("xvblockstore_impl::query_tx fail-tx content read from fail.tx=%s", base::xstring_utl::to_hex(raw_tx_hash).c_str());
            return nullptr;
        }
        index_detail->set_raw_tx(raw_tx.get());
    }
    return index_detail;
}

void xrpc_loader_t::parse_common_info(const xtxindex_detail_ptr_t & txindex, xJson::Value & jv) {
    jv["account"] = txindex->get_txindex()->get_block_addr();
    jv["height"] = static_cast<xJson::UInt64>(txindex->get_txindex()->get_block_height());
    jv["used_gas"] = static_cast<xJson::UInt64>(txindex->get_txaction().get_used_tgas());
    // jv["used_deposit"] = txindex->get_txaction().get_used_deposit();
    // jv["exec_status"] = data::xtransaction_t::tx_exec_status_to_str(txindex->get_txaction().get_tx_exec_status());
}

xJson::Value xrpc_loader_t::parse_send_tx(const xtxindex_detail_ptr_t & sendindex) {
    xJson::Value jv;
    parse_common_info(sendindex, jv);
    jv["used_deposit"] = static_cast<xJson::UInt64>(sendindex->get_txaction().get_used_deposit());
    auto beacon_tx_fee = txexecutor::xtransaction_fee_t::cal_service_fee(sendindex->get_raw_tx()->get_source_addr(), sendindex->get_raw_tx()->get_target_addr());
    jv["tx_fee"] = static_cast<xJson::UInt64>(beacon_tx_fee);
    if (sendindex->get_txaction().is_self_tx()) {  // XTODO sendtx not need set exec_status, only confirmtx and selftx need set exec_status
        jv["exec_status"] = data::xtransaction_t::tx_exec_status_to_str(sendindex->get_txaction().get_tx_exec_status());
    }

    return jv;
}
xJson::Value xrpc_loader_t::parse_recv_tx(const xtxindex_detail_ptr_t & sendindex, const xtxindex_detail_ptr_t & recvindex) {
    xJson::Value jv;
    if (nullptr != recvindex) {
        parse_common_info(recvindex, jv);
    } else {
        jv["account"] = sendindex->get_txindex()->get_block_addr();
        jv["height"] = static_cast<xJson::UInt64>(sendindex->get_txindex()->get_block_height());
        jv["used_gas"] = 0;
        // jv["used_deposit"] = 0;  // XTODO recvtx not has used_deposit now
        // jv["exec_status"] = data::xtransaction_t::tx_exec_status_to_str(sendindex->get_txaction().get_tx_exec_status());  // XTODO recvtx not has exec_status now
    }
    return jv;
}
xJson::Value xrpc_loader_t::parse_confirm_tx(const xtxindex_detail_ptr_t & sendindex, data::enum_xunit_tx_exec_status recvtx_status, const xtxindex_detail_ptr_t & confirmindex) {
    xJson::Value jv;
    if (nullptr != confirmindex) {  // set real confirmtx info
        parse_common_info(confirmindex, jv);
        jv["used_deposit"] = static_cast<xJson::UInt64>(confirmindex->get_txaction().get_used_deposit());
    } else {
        jv["account"] = sendindex->get_txindex()->get_block_addr();
        jv["height"] = static_cast<xJson::UInt64>(sendindex->get_txindex()->get_block_height());
        jv["used_gas"] = 0;
        jv["used_deposit"] = 0;
    }
    jv["exec_status"] = data::xtransaction_t::tx_exec_status_to_str(recvtx_status);
    jv["recv_tx_exec_status"] = data::xtransaction_t::tx_exec_status_to_str(recvtx_status);
    return jv;
}

xJson::Value xrpc_loader_t::load_and_parse_recv_tx(const std::string & raw_tx_hash, const xtxindex_detail_ptr_t & sendindex, data::enum_xunit_tx_exec_status & recvtx_status) {
    xJson::Value jv;
    if (sendindex->get_txaction().get_inner_table_flag()) {  // not need recvindex, create a mock recv json
        jv = xrpc_loader_t::parse_recv_tx(sendindex, nullptr);
        recvtx_status = sendindex->get_txaction().get_tx_exec_status();
    } else {
        xtxindex_detail_ptr_t recvindex = xrpc_loader_t::load_tx_indx_detail(raw_tx_hash, base::enum_transaction_subtype_recv);
        if (recvindex != nullptr) {
            jv = xrpc_loader_t::parse_recv_tx(sendindex, recvindex);
            recvtx_status = recvindex->get_txaction().get_tx_exec_status();
        }
    }
    return jv;
}

xJson::Value xrpc_loader_t::load_and_parse_confirm_tx(const std::string & raw_tx_hash, const xtxindex_detail_ptr_t & sendindex, data::enum_xunit_tx_exec_status recvtx_status) {
    xJson::Value jv;
    if (sendindex->get_txaction().get_not_need_confirm()) {
        jv = xrpc_loader_t::parse_confirm_tx(sendindex, recvtx_status, nullptr);
    } else {
        xtxindex_detail_ptr_t confirmindex = xrpc_loader_t::load_tx_indx_detail(raw_tx_hash, base::enum_transaction_subtype_confirm);
        if (confirmindex != nullptr) {
            jv = xrpc_loader_t::parse_confirm_tx(sendindex, recvtx_status, confirmindex);
        }
    }
    return jv;
}

}  // namespace chain_info
}  // namespace top
