#pragma once

#include "json/json.h"
#include "xrpc/xrpc_loader.h"
#include "xevm_common/xevm_transaction_result.h"
#include "xdata/xethtransaction.h"

namespace top {

namespace xrpc {

struct xblock_location_t {
    xblock_location_t(std::string const& blockhash, std::string const& blocknumber)
    : m_block_hash(blockhash),m_block_number(blocknumber) {}
    std::string     m_block_hash;
    std::string     m_block_number;
};
struct xtx_location_t : public xblock_location_t {
    xtx_location_t(std::string const& blockhash, std::string const& blocknumber)
    : xblock_location_t(blockhash, blocknumber) {}
    xtx_location_t(std::string const& blockhash, std::string const& blocknumber, std::string const& txhash, std::string const& txindex)
    : xblock_location_t(blockhash, blocknumber), m_tx_hash(txhash),m_transaction_index(txindex) {}
    std::string     m_tx_hash;
    std::string     m_transaction_index;
};
struct xlog_location_t : public xtx_location_t {
    xlog_location_t() = default;
    xlog_location_t(std::string const& blockhash, std::string const& blocknumber, std::string const& txhash, std::string const& txindex)
    : xtx_location_t(blockhash, blocknumber, txhash, txindex) {}
    std::string     m_log_index;
};

class xrpc_eth_parser_t {
 public:
    static  void log_to_json(xlog_location_t const& loglocation, evm_common::xevm_log_t const& log, xJson::Value & js_v);
    static  void receipt_to_json(const std::string & tx_hash, xtxindex_detail_ptr_t const& sendindex, xJson::Value & js_v, std::error_code & ec);
    static  void receipt_to_json(xtx_location_t const& txlocation,  data::xeth_transaction_t const& ethtx,
                                data::xeth_store_receipt_t const &evm_tx_receipt,xJson::Value & js_v, std::error_code & ec);
    static  void transaction_to_json(xtx_location_t const& txlocation, data::xtransaction_ptr_t const& rawtx, xJson::Value & js_v, std::error_code & ec);
    static  void transaction_to_json(xtx_location_t const& txlocation, data::xeth_transaction_t const& ethtx, xJson::Value & js_v, std::error_code & ec);
    static  void blockheader_to_json(base::xvblock_t* _block, xJson::Value & js_v, std::error_code & ec);
    static  data::xtransaction_ptr_t json_to_ethtx(xJson::Value const& request, data::eth_error& ec);

    static  std::string                 uint64_to_hex_prefixed(uint64_t value);
    static  std::string                 u256_to_hex_prefixed(evm_common::u256 const& value);
};

}  // namespace chain_info
}  // namespace top
