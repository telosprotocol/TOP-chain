// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xevm_common/trie/xtrie_db.h"
#include "xevm_common/xfixed_hash.h"

#include <system_error>

NS_BEG3(top, evm_common, trie)

// VerifyProof checks merkle proofs. The given proof must contain the value for
// key in a trie with the given root hash. VerifyProof returns an error if the
// proof contains invalid trie nodes or the wrong value.
xbytes_t VerifyProof(xh256_t rootHash, xbytes_t const & _key, xkv_db_face_ptr_t proofDB, std::error_code & ec);

// get returns the child of the given node. Return nil if the
// node with specified key doesn't exist at all.
//
// There is an additional flag `skipResolved`. If it's set then
// all resolved nodes won't be returned.
std::pair<xbytes_t, xtrie_node_face_ptr_t> get(xtrie_node_face_ptr_t tn, xbytes_t key, bool skipResolved, std::error_code & ec);

NS_END3