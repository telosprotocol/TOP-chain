// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xpbase/base/top_utils.h"
#include "xbase/xutl.h"
#include "trezor-crypto/sha3.h"
#include <secp256k1/secp256k1.h>
#include <secp256k1/secp256k1_recovery.h>
#include "xevm_common/xtriehash.h"
#include "xevm_common/rlp.h"
#include "xvledger/xmerkle.hpp"
#include "xcrypto/xckey.h"
#include "xcrypto/xcrypto_util.h"
#include "xevm_common/xtriecommon.h"
#include <openssl/sha.h>
#include "xevm_common/common_data.h"
#include "xevm_common/xborsh.hpp"



namespace top {

namespace data {

    struct xMerklePathItem {
        top::evm_common::h256 m_hash;
        uint8_t m_direction; // 0 is left, 1 is right
    };

    using xMerklePath = std::vector<xMerklePathItem>;
    top::evm_common::h256 combine_hash(top::evm_common::h256 hash1, top::evm_common::h256 hash2);

    class xPartialMerkleTree {

    public:
        xPartialMerkleTree(){}
        xPartialMerkleTree( const xPartialMerkleTree &obj)
        {
            m_path = obj.m_path;
            m_len = obj.m_len;
        }

        top::evm_common::h256 get_root()
        {
            if (m_path.size() == 0) {
                return top::evm_common::h256 { 0 };
            } else {
                auto res = m_path.back();
                uint64_t len = m_path.size();
                for (int index = len - 2; index >= 0; index--) {
                    res = combine_hash(m_path[index], res);
                }
                return res;
            }
        }

        void  insert(top::evm_common::h256 elem)
        {
            uint64_t size = m_len;
            top::evm_common::h256 node = elem;

            while ((size % 2) == 1) {
                auto last = m_path.back();
                m_path.pop_back();
                node = combine_hash(last, node);
                size /= 2;
            }
            m_path.push_back(node);
            m_len++;
        }

        uint64_t size() { return m_len; }

        std::vector<top::evm_common::h256> get_path() { return m_path; }

    private:
        /// Path for the next leaf.
        std::vector<top::evm_common::h256> m_path{0};
        uint64_t      m_len{0};
    };

}

}