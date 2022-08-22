// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xethreceipt.h"
#include "xethtransaction.h"
#include "xevm_common/rlp.h"

namespace top {
namespace data {

class xrelayblock_crosstx_info_t {
 public:
    xrelayblock_crosstx_info_t() = default;
    xrelayblock_crosstx_info_t(xeth_transaction_t const& _tx, xeth_receipt_t const& _receipt); 
    xbytes_t      encodeBytes() const;
    void          decodeBytes(xbytes_t const& _d, std::error_code & ec);
    void          streamRLP(evm_common::RLPStream& _s) const;
    void          decodeRLP(evm_common::RLP const& _r, std::error_code & ec);

 public:
    // TODO(jimmy) cross chain type
    xeth_transaction_t  tx;
    xeth_receipt_t      receipt;
};

class xrelayblock_crosstx_infos_t {
 public:
    std::string   serialize_to_string() const;
    void          serialize_from_string(const std::string & bin_data, std::error_code & ec);
    xbytes_t      encodeBytes() const;
    void          decodeBytes(xbytes_t const& _d, std::error_code & ec);
    void          streamRLP(evm_common::RLPStream& _s) const;
    void          decodeRLP(evm_common::RLP const& _r, std::error_code & ec);
 public:
    std::vector<xrelayblock_crosstx_info_t>  tx_infos;
};

} // namespace data
} // namespace top
