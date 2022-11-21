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

void xeth_build_t::build_ethheader(xethheader_para_t const& para, const xeth_transactions_t & ethtxs, const xeth_store_receipts_t & storage_receipts, evm_common::xh256_t const & state_root, xeth_header_t & ethheader) {
    xeth_receipts_t receipts;
    uint32_t count = (uint32_t)ethtxs.size();
    xassert(ethtxs.size() == storage_receipts.size());

    uint64_t block_gas_used = 0;
    for (uint32_t i = 0; i < count; i++) {
        auto & ethtx = ethtxs[i];
        auto & s_receipt = storage_receipts[i];

        block_gas_used += s_receipt.get_gas_used();
        xassert(block_gas_used == s_receipt.get_cumulative_gas_used());  // TODO(jimmy) delete get_cumulative_gas_used
        xeth_receipt_t receipt(ethtx.get_tx_version(), s_receipt.get_tx_status(), block_gas_used, s_receipt.get_logs());
        receipt.create_bloom();
        receipts.push_back(receipt);
    }

    build_ethheader(para, ethtxs, receipts, state_root, ethheader);
}

void xeth_build_t::build_ethheader(xethheader_para_t const& para, const xeth_transactions_t & ethtxs, const xeth_receipts_t & receipts, evm_common::xh256_t const & state_root, xeth_header_t & ethheader) {
    uint64_t block_gas_used = 0;
    evm_common::xbloom9_t block_logs_bloom;
    for (auto & v : receipts) {
        block_gas_used = v.get_cumulative_gas_used();
        block_logs_bloom |= v.get_logsBloom();
    }

    ethheader.set_gaslimit(para.m_gaslimit);
    ethheader.set_baseprice(para.m_baseprice);
    ethheader.set_coinbase(para.m_coinbase);
    ethheader.set_gasused(block_gas_used);
    ethheader.set_state_root(state_root);
    xassert(block_gas_used <= para.m_gaslimit);

    if (!receipts.empty()) {
        ethheader.set_logBloom(block_logs_bloom);
        auto receipts_root = build_receipts_root(receipts);
        ethheader.set_receipts_root(receipts_root);
        auto txs_root = build_transactions_root(ethtxs);
        ethheader.set_transactions_root(txs_root);
    }
}

NS_END2
