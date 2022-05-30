#include "xvledger/xvtxindex.h"
#include "xdata/xlightunit_info.h"
#include "xdata/xtransaction.h"
#include "xdata/xdatautil.h"
#include "xdata/xblocktool.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvtxstore.h"
#include "xrpc/xrpc_loader.h"
#include "xtxexecutor/xtransaction_fee.h"
#include "xbasic/xhex.h"

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

void xtxindex_detail_t::set_transaction_index(uint32_t transaction_index) {
    m_transaction_index = transaction_index;
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
    data::xlightunit_action_ptr_t txaction = data::xblockextract_t::unpack_one_txaction(_block.get(), raw_tx_hash);
    if (txaction == nullptr) {
        xerror("xrpc_loader_t::load_tx_indx_detail,fail to load action for hash:%s,type:%d", base::xstring_utl::to_hex(raw_tx_hash).c_str(), type);
        return nullptr;
    }
    xtxindex_detail_ptr_t index_detail = std::make_shared<xtxindex_detail_t>(txindex, *txaction);
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

//----------------------xrpc_eth_loader_t----------------
xtxindex_detail_ptr_t xrpc_eth_loader_t::load_tx_indx_detail(const std::string & raw_tx_hash) {
    base::enum_transaction_subtype type = base::enum_transaction_subtype_send;
    base::xauto_ptr<base::xvtxindex_t> txindex = base::xvchain_t::instance().get_xtxstore()->load_tx_idx(raw_tx_hash, type);
    if (nullptr == txindex) {
        xwarn("xrpc_eth_loader_t::load_tx_indx_detail,fail to index for hash:%s,type:%d", base::xstring_utl::to_hex(raw_tx_hash).c_str(), type);
        return nullptr;
    }  

    base::xvaccount_t _vaddress(txindex->get_block_addr());
    auto _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(_vaddress, txindex->get_block_height(), txindex->get_block_hash(), false);
    if (nullptr == _block) {
        xwarn("xrpc_eth_loader_t::load_tx_indx_detail,fail to load block for hash:%s,type:%d", base::xstring_utl::to_hex(raw_tx_hash).c_str(), type);
        return nullptr;
    }

    // TODO(jimmy) performance optimize transaction_index put to xvtxindex ?
    data::xlightunit_action_ptr_t txaction_ptr = nullptr;
    std::vector<data::xlightunit_action_t> eth_txactions = data::xblockextract_t::unpack_eth_txactions(_block.get());
    uint32_t transaction_index = (uint32_t)eth_txactions.size();
    for (uint32_t i = 0; i < (uint32_t)eth_txactions.size(); i++) {
        if (eth_txactions[i].get_org_tx_hash() == raw_tx_hash) {
            transaction_index = i;
            txaction_ptr = std::make_shared<data::xlightunit_action_t>(eth_txactions[i]);
            break;
        }
    }
    if (transaction_index >= (uint32_t)eth_txactions.size()) {
        xerror("xrpc_eth_loader_t::load_tx_indx_detail,fail to find txaction hash:%s,block:%s", base::xstring_utl::to_hex(raw_tx_hash).c_str(), _block->dump().c_str());
        return nullptr;
    }
    xtxindex_detail_ptr_t index_detail = std::make_shared<xtxindex_detail_t>(txindex, *txaction_ptr);
    if (type == base::enum_transaction_subtype_self || type == base::enum_transaction_subtype_send) {
        if (false == base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaddress, _block.get())) {
            xerror("xrpc_eth_loader_t::load_tx_indx_detail,fail to load input hash:%s,block:%s", base::xstring_utl::to_hex(raw_tx_hash).c_str(), _block->dump().c_str());
            return nullptr;
        }
        std::string orgtx_bin = _block->get_input()->query_resource(raw_tx_hash);
        if (orgtx_bin.empty()) {
            xerror("xrpc_eth_loader_t::load_tx_indx_detail,fail to find raw tx hash:%s,block:%s", base::xstring_utl::to_hex(raw_tx_hash).c_str(), _block->dump().c_str());
            return nullptr;
        }
        base::xauto_ptr<base::xdataunit_t> raw_tx = base::xdataunit_t::read_from(orgtx_bin);
        if(nullptr == raw_tx) {
            xerror("xrpc_eth_loader_t::load_tx_indx_detail,fail to read from hash:%s,block:%s", base::xstring_utl::to_hex(raw_tx_hash).c_str(), _block->dump().c_str());
            return nullptr;
        }
        index_detail->set_raw_tx(raw_tx.get());
    }
    return index_detail;
}

data::xeth_local_receipt_prt_t xrpc_eth_loader_t::load_tx_receipt(const std::string & raw_tx_hash) {
    xtxindex_detail_ptr_t txindex = load_tx_indx_detail(raw_tx_hash);
    if (nullptr == txindex) {
        xwarn("xrpc_eth_loader_t::load_tx_receipt,fail to load tx index for hash:%s", base::xstring_utl::to_hex(raw_tx_hash).c_str());
        return nullptr;
    }

    data::xeth_store_receipt_t evm_result;
    auto ret = txindex->get_txaction().get_evm_transaction_receipt(evm_result);
    if (false == ret) {
        xwarn("xrpc_eth_loader_t::load_tx_receipt,fail to get evm result for hash:%s", base::xstring_utl::to_hex(raw_tx_hash).c_str());
        return nullptr;
    }

    data::xeth_local_receipt_prt_t local_receipt = std::make_shared<data::xeth_local_receipt_t>();
    local_receipt->set_tx_hash(raw_tx_hash);
    local_receipt->set_block_hash(txindex->get_txindex()->get_block_hash());
    local_receipt->set_block_number(txindex->get_txindex()->get_block_height());
    local_receipt->set_transaction_index(txindex->get_transaction_index());

    uint16_t tx_type = txindex->get_raw_tx()->get_tx_type();
    if (tx_type == data::xtransaction_type_transfer) {
        common::xaccount_address_t _top_addr(txindex->get_raw_tx()->get_target_addr());
        common::xtop_eth_address _eth_addr = common::xtop_eth_address::build_from(_top_addr);
        local_receipt->set_to_address(_eth_addr);
    } else if (tx_type == data::xtransaction_type_run_contract) {
        common::xaccount_address_t _top_addr(txindex->get_raw_tx()->get_target_addr());
        common::xtop_eth_address _eth_addr = common::xtop_eth_address::build_from(_top_addr);
        local_receipt->set_to_address(_eth_addr);
        local_receipt->set_contract_address(_eth_addr);
    } else if (tx_type == data::xtransaction_type_deploy_evm_contract) {
        // TODO(jimmy)
        std::string _str1 = evm_result.get_contract_address().to_hex_string().substr(2);
        xbytes_t _bytes = top::to_bytes(_str1);
        common::xtop_eth_address _eth_addr = common::xtop_eth_address::build_from(_bytes);
        local_receipt->set_contract_address(_eth_addr);
    }
    uint64_t used_gas = evm_result.get_gas_used();// TODO(jimmy)
    local_receipt->set_used_gas(used_gas);
    local_receipt->set_tx_status(evm_result.get_tx_status());
    local_receipt->set_logs(evm_result.get_logs());

    return local_receipt;
}

void xrpc_eth_loader_t::parse_log_to_json(evm_common::xevm_log_t const& log, xJson::Value & js_v) {
    js_v["address"] = log.address.to_hex_string();
    for (auto & topic : log.topics) {
        js_v["topics"].append(top::to_hex_prefixed(topic.asBytes()));
    }
    js_v["data"] = top::to_hex_prefixed(log.data);
}

std::string xrpc_eth_loader_t::uint64_to_hex_prefixed(uint64_t value) {
    std::stringstream outstr;
    outstr << "0x" << std::hex << value;
    return outstr.str();
}

xJson::Value xrpc_eth_loader_t::parse_receipt(const data::xeth_local_receipt_prt_t & _receipt) {
    std::string block_hash = top::to_hex_prefixed(_receipt->get_block_hash());
    std::string block_num = uint64_to_hex_prefixed(_receipt->get_block_number());
    std::string tx_hash = top::to_hex_prefixed(_receipt->get_tx_hash());
    std::string tx_idx = uint64_to_hex_prefixed((uint64_t)_receipt->get_transaction_index());

    xJson::Value js_result;
    js_result["type"] = uint64_to_hex_prefixed((uint64_t)_receipt->get_tx_version_type());
    js_result["transactionHash"] = tx_hash;
    js_result["blockHash"] = block_hash;
    js_result["transactionIndex"] = tx_idx;
    js_result["blockNumber"] = block_num;
    js_result["from"] = _receipt->get_from_address().to_hex_string();
    js_result["to"] = _receipt->get_to_address().to_hex_string();
    js_result["status"] = _receipt->get_tx_status() == data::enum_ethreceipt_status::ethreceipt_status_successful ? "0x1": "0x0";
    if (!_receipt->get_contract_address().empty()) {
        js_result["contractAddress"] = _receipt->get_contract_address().to_hex_string();
    }
    js_result["cumulativeGasUsed"] = uint64_to_hex_prefixed(_receipt->get_cumulative_gas_used());
    js_result["gasUsed"] = uint64_to_hex_prefixed(_receipt->get_gas_used());
    js_result["effectiveGasPrice"] = "0x0";
    js_result["logsBloom"] = top::to_hex_prefixed(_receipt->get_logsBloom().get_data());

    if (_receipt->get_logs().empty()) {
        js_result["logs"].resize(0);
    } else {
        uint64_t logindex = 0;
        for (auto & log : _receipt->get_logs()) {
            xJson::Value js_log;

            js_log["logIndex"] = uint64_to_hex_prefixed(logindex);
            js_log["blockNumber"] = block_num;
            js_log["blockHash"] = block_hash;
            js_log["transactionHash"] = tx_hash;
            js_log["transactionIndex"] = tx_idx;
            parse_log_to_json(log, js_log);

            js_log["removed"] = false;
            js_result["logs"].append(js_log);
            logindex++;
        }
    }
    return js_result;
}


}  // namespace chain_info
}  // namespace top
