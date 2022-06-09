// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xethbuild.h"
#include "xevm_common/xtriehash.h"

NS_BEG2(top, data)

// evm_common::LogBloom xeth_build_t::build_block_logsbloom(const xeth_receipts_t & receipts) {
//     evm_common::LogBloom block_bloom;
//     for (auto & receipt : receipts) {
//         evm_common::LogBloom bloom = receipt.get_logsBloom();
//         block_bloom |= bloom;
//     }
//     return block_bloom;
// }
evm_common::h256 xeth_build_t::build_transactions_root(const xeth_transactions_t & ethtxs) {
    std::vector<xbytes_t> _leafs;
    for (auto & tx: ethtxs) {
        xbytes_t _rb = tx.encodeBytes();
        _leafs.push_back(_rb);
    }
    evm_common::h256 txsRoot = evm_common::orderedTrieRoot(_leafs);
    return txsRoot;
}
evm_common::h256 xeth_build_t::build_receipts_root(const xeth_receipts_t & receipts) {
    std::vector<xbytes_t> receipt_leafs;
    for (auto & receipt: receipts) {
        xbytes_t _rb = receipt.encodeBytes();
        receipt_leafs.push_back(_rb);
    }
    evm_common::h256 receiptsRoot = evm_common::orderedTrieRoot(receipt_leafs);
    return receiptsRoot;
}

uint64_t xeth_build_t::calc_receipts_gas_used(const xeth_receipts_t & receipts) {
    uint64_t total_gasused = receipts.back().get_cumulative_gas_used();
    return total_gasused;
}

NS_END2
