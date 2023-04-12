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
#include "xdata/xrelay_block.h"
#include "xrpc/xrpc_eth_parser.h"
#include "xpbase/base/top_utils.h"

namespace top {

namespace xrpc {

xtxindex_detail_t::xtxindex_detail_t(const base::xvtxindex_ptr & txindex, std::string const & blockhash, const base::xvaction_t & txaction, uint64_t transaction_index)
: m_txindex(txindex), m_block_hash(blockhash), m_txaction(txaction), m_transaction_index(transaction_index) {
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
    if (false == base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaddress, _block.get())) {
        xwarn("xrpc_loader_t::load_tx_indx_detail,fail to load block input for hash:%s,type:%d", base::xstring_utl::to_hex(raw_tx_hash).c_str(), type);
        return nullptr;
    }    
    data::xlightunit_action_ptr_t txaction = data::xblockextract_t::unpack_one_txaction(_block.get(), raw_tx_hash);
    if (txaction == nullptr) {
        xerror("xrpc_loader_t::load_tx_indx_detail,fail to load action for hash:%s,type:%d", base::xstring_utl::to_hex(raw_tx_hash).c_str(), type);
        return nullptr;
    }
    uint64_t transaction_index = 0; // TODO(jimmy)
    xtxindex_detail_ptr_t index_detail = std::make_shared<xtxindex_detail_t>(txindex, _block->get_block_hash(), *txaction, transaction_index);
    if (type == base::enum_transaction_subtype_self || type == base::enum_transaction_subtype_send) {
        // if (false == base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaddress, _block.get())) {
        //     xwarn("xrpc_loader_t::load_tx_indx_detail,fail to load block input for hash:%s,type:%d", base::xstring_utl::to_hex(raw_tx_hash).c_str(), type);
        //     return nullptr;
        // }
        std::string orgtx_bin = _block->query_input_resource(raw_tx_hash);
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

void xrpc_loader_t::parse_logs(const xtxindex_detail_ptr_t & index, Json::Value & jv) {
    if (index == nullptr) {
        return;
    }
    data::xtop_store_receipt_t tx_receipt;
    if (false == index->get_txaction().get_tvm_transaction_receipt(tx_receipt)) {
        jv["logs"] = Json::arrayValue;
        jv["logsBloom"] = evm_common::xbloom9_t{}.to_hex_string();
        return;
    }
    Json::Value i;
    uint64_t idx = 0;
    for (auto & log: tx_receipt.get_logs()) {
        Json::Value j;
        j["logIndex"] = xrpc_eth_parser_t::uint64_to_hex_prefixed(idx);
        idx++;
        j["address"] = log.address.to_string();
        for (auto & topic : log.topics){
            j["topics"].append(top::to_hex_prefixed(topic.asBytes()));
        }
        j["data"] = top::to_hex_prefixed(log.data);
        i.append(j);
    }
    // js_v["blockNumber"] = loglocation.m_block_number;
    // js_v["blockHash"] = loglocation.m_block_hash;
    // js_v["transactionHash"] = loglocation.m_tx_hash;
    // js_v["transactionIndex"] = loglocation.m_transaction_index;
    // js_v["removed"] = false;
    jv["logs"] = i;
    jv["logsBloom"] = tx_receipt.get_logsBloom().to_hex_string();
}

void xrpc_loader_t::parse_common_info(const xtxindex_detail_ptr_t & txindex, Json::Value & jv) {
    jv["account"] = txindex->get_txindex()->get_block_addr();
    jv["height"] = static_cast<Json::UInt64>(txindex->get_txindex()->get_block_height());
    jv["used_gas"] = static_cast<Json::UInt64>(txindex->get_txaction().get_used_tgas());
    // jv["used_deposit"] = txindex->get_txaction().get_used_deposit();
    // jv["exec_status"] = data::xtransaction_t::tx_exec_status_to_str(txindex->get_txaction().get_tx_exec_status());
}

Json::Value xrpc_loader_t::parse_send_tx(const xtxindex_detail_ptr_t & sendindex) {
    Json::Value jv;
    parse_common_info(sendindex, jv);
    jv["used_deposit"] = static_cast<Json::UInt64>(sendindex->get_txaction().get_used_deposit());
    auto beacon_tx_fee = txexecutor::xtransaction_fee_t::cal_service_fee(sendindex->get_raw_tx()->source_address().to_string(), sendindex->get_raw_tx()->target_address().to_string());
    jv["tx_fee"] = static_cast<Json::UInt64>(beacon_tx_fee);
    if (sendindex->get_txaction().is_self_tx()) {  // XTODO sendtx not need set exec_status, only confirmtx and selftx need set exec_status
        jv["exec_status"] = data::xtransaction_t::tx_exec_status_to_str(sendindex->get_txaction().get_tx_exec_status());
    }

    return jv;
}
Json::Value xrpc_loader_t::parse_recv_tx(const xtxindex_detail_ptr_t & sendindex, const xtxindex_detail_ptr_t & recvindex) {
    Json::Value jv;
    if (nullptr != recvindex) {
        parse_common_info(recvindex, jv);
    } else {
        jv["account"] = sendindex->get_txindex()->get_block_addr();
        jv["height"] = static_cast<Json::UInt64>(sendindex->get_txindex()->get_block_height());
        jv["used_gas"] = 0;
        // jv["used_deposit"] = 0;  // XTODO recvtx not has used_deposit now
        // jv["exec_status"] = data::xtransaction_t::tx_exec_status_to_str(sendindex->get_txaction().get_tx_exec_status());  // XTODO recvtx not has exec_status now
    }
    return jv;
}
Json::Value xrpc_loader_t::parse_confirm_tx(const xtxindex_detail_ptr_t & sendindex, data::enum_xunit_tx_exec_status recvtx_status, const xtxindex_detail_ptr_t & confirmindex) {
    Json::Value jv;
    if (nullptr != confirmindex) {  // set real confirmtx info
        parse_common_info(confirmindex, jv);
        jv["used_deposit"] = static_cast<Json::UInt64>(confirmindex->get_txaction().get_used_deposit());
    } else {
        jv["account"] = sendindex->get_txindex()->get_block_addr();
        jv["height"] = static_cast<Json::UInt64>(sendindex->get_txindex()->get_block_height());
        jv["used_gas"] = 0;
        jv["used_deposit"] = 0;
    }
    jv["exec_status"] = data::xtransaction_t::tx_exec_status_to_str(recvtx_status);
    jv["recv_tx_exec_status"] = data::xtransaction_t::tx_exec_status_to_str(recvtx_status);
    return jv;
}

Json::Value xrpc_loader_t::load_and_parse_recv_tx(const std::string & raw_tx_hash, const xtxindex_detail_ptr_t & sendindex, xtxindex_detail_ptr_t & recvindex, data::enum_xunit_tx_exec_status & recvtx_status) {
    Json::Value jv;
    if (sendindex->get_txaction().get_inner_table_flag()) {  // not need recvindex, create a mock recv json
        jv = xrpc_loader_t::parse_recv_tx(sendindex, nullptr);
        recvtx_status = sendindex->get_txaction().get_tx_exec_status();
    } else {
        recvindex = xrpc_loader_t::load_tx_indx_detail(raw_tx_hash, base::enum_transaction_subtype_recv);
        if (recvindex != nullptr) {
            jv = xrpc_loader_t::parse_recv_tx(sendindex, recvindex);
            recvtx_status = recvindex->get_txaction().get_tx_exec_status();
        }
    }
    return jv;
}

Json::Value xrpc_loader_t::load_and_parse_confirm_tx(const std::string & raw_tx_hash, const xtxindex_detail_ptr_t & sendindex, data::enum_xunit_tx_exec_status recvtx_status) {
    Json::Value jv;
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

//----------------------xrpc_loader_t----------------
xtxindex_detail_ptr_t xrpc_loader_t::load_ethtx_indx_detail(const std::string & raw_tx_hash) {
    base::enum_transaction_subtype type = base::enum_transaction_subtype_send;
    base::xauto_ptr<base::xvtxindex_t> txindex = base::xvchain_t::instance().get_xtxstore()->load_tx_idx(raw_tx_hash, type);
    if (nullptr == txindex) {
        xwarn("xrpc_loader_t::load_ethtx_indx_detail,fail to index for hash:%s,type:%d", base::xstring_utl::to_hex(raw_tx_hash).c_str(), type);
        return nullptr;
    }  

    base::xvaccount_t _vaddress(txindex->get_block_addr());
    auto _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(_vaddress, txindex->get_block_height(), base::enum_xvblock_flag_committed, false);
    if (nullptr == _block) {
        xwarn("xrpc_loader_t::load_ethtx_indx_detail,fail to load block for hash:%s,type:%d", base::xstring_utl::to_hex(raw_tx_hash).c_str(), type);
        return nullptr;
    }
    if (false == base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaddress, _block.get())) {
        xwarn("xrpc_loader_t::load_ethtx_indx_detail,fail to load block input for hash:%s,type:%d", base::xstring_utl::to_hex(raw_tx_hash).c_str(), type);
        return nullptr;
    }
    // TODO(jimmy) performance optimize transaction_index put to xvtxindex ?
    data::xlightunit_action_ptr_t txaction_ptr = nullptr;
    std::vector<data::xlightunit_action_t> eth_txactions = data::xblockextract_t::unpack_eth_txactions(_block.get());
    uint64_t transaction_index = (uint32_t)eth_txactions.size();
    for (uint64_t i = 0; i < (uint32_t)eth_txactions.size(); i++) {
        if (eth_txactions[i].get_org_tx_hash() == raw_tx_hash) {
            transaction_index = i;
            txaction_ptr = std::make_shared<data::xlightunit_action_t>(eth_txactions[i]);
            break;
        }
    }
    if (transaction_index >= (uint32_t)eth_txactions.size()) {
        xwarn("xrpc_loader_t::load_ethtx_indx_detail,fail to find txaction hash:%s,block:%s", base::xstring_utl::to_hex(raw_tx_hash).c_str(), _block->dump().c_str());
        return nullptr;
    }
    xtxindex_detail_ptr_t index_detail = std::make_shared<xtxindex_detail_t>(txindex, _block->get_block_hash(), *txaction_ptr, transaction_index);
    if (type == base::enum_transaction_subtype_self || type == base::enum_transaction_subtype_send) {
        // if (false == base::xvchain_t::instance().get_xblockstore()->load_block_input(_vaddress, _block.get())) {
        //     xerror("xrpc_loader_t::load_ethtx_indx_detail,fail to load input hash:%s,block:%s", base::xstring_utl::to_hex(raw_tx_hash).c_str(), _block->dump().c_str());
        //     return nullptr;
        // }
        std::string orgtx_bin = _block->query_input_resource(raw_tx_hash);
        if (orgtx_bin.empty()) {
            xerror("xrpc_loader_t::load_ethtx_indx_detail,fail to find raw tx hash:%s,block:%s", base::xstring_utl::to_hex(raw_tx_hash).c_str(), _block->dump().c_str());
            return nullptr;
        }
        base::xauto_ptr<base::xdataunit_t> raw_tx = base::xdataunit_t::read_from(orgtx_bin);
        if(nullptr == raw_tx) {
            xerror("xrpc_loader_t::load_ethtx_indx_detail,fail to read from hash:%s,block:%s", base::xstring_utl::to_hex(raw_tx_hash).c_str(), _block->dump().c_str());
            return nullptr;
        }
        index_detail->set_raw_tx(raw_tx.get());
    }
    return index_detail;
}

bool xrpc_loader_t::load_relay_tx_indx_detail(const std::string & raw_tx_hash, xtx_location_t & txlocation, data::xeth_transaction_t &eth_transaction,  
                                              data::xeth_store_receipt_t &evm_tx_receipt) {
    base::enum_transaction_subtype type = base::enum_transaction_subtype_send;
    base::xauto_ptr<base::xvtxindex_t> txindex = base::xvchain_t::instance().get_xtxstore()->load_relay_tx_idx(raw_tx_hash, type);
    if (nullptr == txindex) {
        xwarn("xrpc_loader_t::load_ethtx_indx_detail,fail to index for hash:%s,type:%d", base::xstring_utl::to_hex(raw_tx_hash).c_str(), type);
        return false;
    }  
    xdbg("xrpc_loader_t::load_relay_tx_indx_detail, %llu, %s", txindex->get_block_height(), HexEncode(txindex->get_block_hash()).c_str());

    base::xvaccount_t _vaddress(txindex->get_block_addr());
    auto _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(_vaddress, txindex->get_block_height(), txindex->get_block_hash(), false);
    if (nullptr == _block) {
        xwarn("xrpc_loader_t::load_relay_tx_indx_detail,fail to load block for hash:%s,type:%d", base::xstring_utl::to_hex(raw_tx_hash).c_str(), type);
        return false;
    }

    xdbg("xrpc_loader_t:load_relay_tx_indx_detail  decode relay bock with tx_hash: %s ", base::xstring_utl::to_hex(raw_tx_hash).c_str());
    std::error_code ec;
    top::data::xrelay_block  extra_relay_block;
    data::xblockextract_t::unpack_relayblock_from_wrapblock(_block.get(), extra_relay_block, ec);    
    if (ec) {
        xerror("xrpc_loader_t:load_relay_tx_indx_detail decodeBytes decodeBytes error %s; err msg %s", ec.category().name(), ec.message().c_str());
        return false;
    }

    //get tx hash from txs
    uint64_t tx_index = 0;
    for ( auto &tx: extra_relay_block.get_all_transactions()) {
        if (tx.get_tx_hash().empty()) {
            xerror("xrpc_loader_t:load_relay_tx_indx_detail block:%s tx hash is empty", _block->dump().c_str());
            return false;
        }
        std::string tx_hash = std::string(reinterpret_cast<char*>(tx.get_tx_hash().data()), tx.get_tx_hash().size());
        if (tx_hash == raw_tx_hash) {
            xinfo("xblockacct_t:load_relay_tx_indx_detail find hash %s ", base::xstring_utl::to_hex(raw_tx_hash).c_str());
            eth_transaction = tx;
            break;
        }
        tx_index++;
    }

    //find tx 
    if (tx_index < extra_relay_block.get_all_transactions().size()) {
        if (tx_index >= extra_relay_block.get_all_receipts().size()) {
            xerror("xrpc_loader_t:load_relay_tx_indx_detail block:%s tx_index %d and receipt_size  %d not match",
                  _block->dump().c_str(), tx_index, extra_relay_block.get_all_transactions().size());
            return false;
        }
        
        txlocation.m_tx_hash = top::to_hex_prefixed(raw_tx_hash);
        txlocation.m_transaction_index = xrpc_eth_parser_t::uint64_to_hex_prefixed(tx_index);
        txlocation.m_block_hash = to_hex_prefixed(extra_relay_block.get_block_hash().to_bytes());
        txlocation.m_block_number = xrpc_eth_parser_t::uint64_to_hex_prefixed(extra_relay_block.get_block_height());

        data::xeth_receipt_t tx_receipt = extra_relay_block.get_all_receipts()[tx_index];
        evm_tx_receipt.set_tx_status(tx_receipt.get_tx_status());
        evm_tx_receipt.set_logs(tx_receipt.get_logs());
        evm_tx_receipt.set_cumulative_gas_used(tx_receipt.get_cumulative_gas_used());

        /*evm_tx_receipt.set_gas_price();
        evm_tx_receipt.set_gas_used();
        evm_tx_receipt.set_contract_address();*/
        return true;
    } else {
        xwarn("xrpc_loader_t:load_relay_tx_indx_detail block:%s tx hash %s no found.", _block->dump().c_str(), base::xstring_utl::to_hex(raw_tx_hash).c_str());
    }
    return false;

}


}  // namespace chain_info
}  // namespace top
