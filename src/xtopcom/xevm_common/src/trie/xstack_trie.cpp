// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xstack_trie.h"

#include "xevm_common/trie/xtrie.h"
#include "xevm_common/trie/xtrie_encoding.h"
#include "xevm_common/trie/xtrie_node_coding.h"
#include "xevm_common/xerror/xerror.h"
#include "xutility/xhash.h"

NS_BEG3(top, evm_common, trie)

std::shared_ptr<xtop_stack_trie> xtop_stack_trie::NewStackTrie(xkv_db_face_ptr_t db) {
    if (db == nullptr) {
        xerror("build stack trie from null db");
    }

    auto st_trie = xtop_stack_trie{db};

    return std::make_shared<xtop_stack_trie>(st_trie);
}

void xtop_stack_trie::Update(xbytes_t const & key, xbytes_t const & value) {
    std::error_code ec;
    TryUpdate(key, value, ec);
    if (ec) {
        xerror("stack trie error: %s", ec.message().c_str());
    }
    return;
}

void xtop_stack_trie::TryUpdate(xbytes_t const & key, xbytes_t const & value, std::error_code & ec) {
    auto k = keybytesToHex(key);
    if (value.size() == 0) {
        xerror("stack trie not support delete");
        return;
    }
    auto real_key = xbytes_t{k.begin() + k.size() - 1, k.end()};
    insert(real_key, value);
    return;
}

void xtop_stack_trie::insert(xbytes_t const & key, xbytes_t const & value) {
    switch (type()) {
    case xstack_trie_node_type_t::branchNode: {
        auto idx = static_cast<std::size_t>(key[0]);
        // Unresolve elder siblings
        for (auto i = idx - 1; i >= 0; i--) {
            if (m_children[i] != nullptr) {
                if (m_children[i]->type() != xstack_trie_node_type_t::hashedNode) {
                    m_children[i]->hash();
                }
                break;
            }
        }
        // Add new child
        auto rest_key = xbytes_t{key.begin() + 1, key.end()};
        if (m_children[idx] == nullptr) {
            m_children[idx] = newLeaf(rest_key, value);
        } else {
            m_children[idx]->insert(rest_key, value);
        }
        return;
    }
    case xstack_trie_node_type_t::extNode: {
        // Compare both key chunks and see where they differ
        auto diffidx = getDiffIndex(key);

        // Check if chunks are identical. If so, recurse into
        // the child node. Otherwise, the key has to be split
        // into 1) an optional common prefix, 2) the fullnode
        // representing the two differing path, and 3) a leaf
        // for each of the differentiated subtrees.
        if (diffidx == m_key.size()) {
            // Ext key and key segment are identical, recurse into the child node.
            auto new_key = xbytes_t{key.begin() + diffidx, key.end()};
            m_children[0]->insert(new_key, value);
            return;
        }

        // Save the original part. Depending if the break is
        // at the extension's last byte or not, create an
        // intermediate extension or use the extension's child
        // node directly.
        std::shared_ptr<xtop_stack_trie> n;
        if (diffidx < m_key.size() - 1) {
            auto rest_key = xbytes_t{m_key.begin() + diffidx + 1, m_key.end()};
            n = newExt(rest_key, m_children[0]);
        } else {
            n = m_children[0];
        }
        // Convert to hash
        n->hash();
        std::shared_ptr<xtop_stack_trie> p;
        if (diffidx == 0) {
            // the break is on the first byte, so
            // the current node is converted into
            // a branch node.
            m_children[0] = nullptr;
            p = std::make_shared<xtop_stack_trie>(*this);
            m_type = xstack_trie_node_type_t::branchNode;
        } else {
            // the common prefix is at least one byte
            // long, insert a new intermediate branch
            // node.
            m_children[0] = std::make_shared<xtop_stack_trie>(m_db);
            p = m_children[0];
            p->m_type = xstack_trie_node_type_t::branchNode;
        }
        auto rest_key = xbytes_t{m_key.begin() + diffidx + 1, m_key.end()};
        auto o = newLeaf(rest_key, value);

        auto originIdx = m_key[diffidx];
        auto newIdx = key[diffidx];
        p->m_children[originIdx] = n;
        p->m_children[newIdx] = o;
        m_key = xbytes_t{m_key.begin(), m_key.begin() + diffidx};
        return;
    }
    case xstack_trie_node_type_t::leafNode: {
        // Compare both key chunks and see where they differ
        auto diffidx = getDiffIndex(key);

        // Overwriting a key isn't supported, which means that
        // the current leaf is expected to be split into 1) an
        // optional extension for the common prefix of these 2
        // keys, 2) a fullnode selecting the path on which the
        // keys differ, and 3) one leaf for the differentiated
        // component of each key.
        if (diffidx >= m_key.size()) {
            xerror("stack trie trying to insert into existing key");
        }

        // Check if the split occurs at the first nibble of the
        // chunk. In that case, no prefix extnode is necessary.
        // Otherwise, create that
        std::shared_ptr<xtop_stack_trie> p;
        if (diffidx == 0) {
            // Convert current leaf into a branch
            m_type = xstack_trie_node_type_t::branchNode;
            p = std::make_shared<xtop_stack_trie>(*this);
            m_children[0] = nullptr;
        } else {
            // Convert current node into an ext,
            // and insert a child branch node.
            m_type = xstack_trie_node_type_t::extNode;
            m_children[0] = std::make_shared<xtop_stack_trie>(m_db);
            p = m_children[0];
            p->m_type = xstack_trie_node_type_t::branchNode;
        }

        auto origIdx = m_key[diffidx];
        auto orig_rest_key = xbytes_t{m_key.begin() + diffidx + 1, m_key.end()};
        p->m_children[origIdx] = newLeaf(orig_rest_key, m_val);
        p->m_children[origIdx]->hash();

        auto newIdx = key[diffidx];
        auto new_rest_key = xbytes_t{key.begin() + diffidx + 1, key.end()};
        p->m_children[newIdx] = newLeaf(new_rest_key, value);

        m_key = xbytes_t{m_key.begin(), m_key.begin() + diffidx};
        m_val.clear();

        return;
    }
    case xstack_trie_node_type_t::emptyNode: {
        m_type = xstack_trie_node_type_t::leafNode;
        m_key = key;
        m_val = value;
        return;
    }
    case xstack_trie_node_type_t::hashedNode: {
        xerror("stack trie try insert into hash node");
    }
    default: {
        xassert(false);
    }
    }
    __builtin_unreachable();
}

void xtop_stack_trie::hash() {
    /* Shortcut if node is already hashed */
    if (m_type == xstack_trie_node_type_t::hashedNode) {
        return;
    }

    // xtrie_hasher_t h{false};
    xbytes_t hash_buffer;

    switch (m_type) {
    case xstack_trie_node_type_t::branchNode: {
        std::array<xtrie_node_face_ptr_t, 17> nodes;
        for (std::size_t index = 0; index < m_children.size(); ++index) {
            auto child = m_children[index];
            if (child == nullptr) {
                nodes[index] = std::make_shared<xtrie_value_node_t>(nilValueNode);
                continue;
            }
            child->hash();
            if (child->m_children.size() < 32) {
                nodes[index] = std::make_shared<xtrie_raw_node_t>(child->m_val);
            } else {
                nodes[index] = std::make_shared<xtrie_hash_node_t>(child->m_val);
            }
            m_children[index] = nullptr;
        }
        nodes[16] = std::make_shared<xtrie_value_node_t>(nilValueNode);
        hash_buffer = xtrie_node_rlp::EncodeNodesToBytes(nodes);
        xdbg("xtop_stack_trie hash branchNode buffer: %s", top::to_hex(hash_buffer).c_str());
    }
    case xstack_trie_node_type_t::extNode: {
        m_children[0]->hash();
        xtrie_node_face_ptr_t valuenode;
        if (m_children[0]->m_val.size() < 32) {
            valuenode = std::make_shared<xtrie_raw_node_t>(m_children[0]->m_val);
        } else {
            valuenode = std::make_shared<xtrie_hash_node_t>(m_children[0]->m_val);
        }
        auto key = hexToCompact(m_key);
        xbytes_t encoded;
        append(encoded, key);
        append(encoded, xtrie_node_rlp::EncodeToBytes(valuenode));
        hash_buffer = RLP::encodeList(encoded);
        m_children[0] = nullptr;
        xdbg("xtop_stack_trie hash extNode buffer: %s", top::to_hex(hash_buffer).c_str());
    }
    case xstack_trie_node_type_t::leafNode: {
        m_key.insert(m_key.end(), xbyte_t(16));
        auto sz = hexToCompactInPlace(m_key);
        auto key = xbytes_t{m_key.begin(), m_key.begin() + sz};
        xbytes_t encoded;
        append(encoded, key);
        append(encoded, m_val);
        hash_buffer = RLP::encodeList(encoded);
        xdbg("xtop_stack_trie hash leafNode buffer: %s", top::to_hex(hash_buffer).c_str());
    }
    case xstack_trie_node_type_t::emptyNode: {
        m_val = xbytes_t{empty_root_bytes.begin(), empty_root_bytes.end()};
        m_key = xbytes_t{m_key.begin(), m_key.begin()};
        m_type = xstack_trie_node_type_t::hashedNode;
        return;
    }
    default: {
        xassert(false);
    }
    }
    m_key = xbytes_t{m_key.begin(), m_key.begin()};
    m_type = xstack_trie_node_type_t::hashedNode;
    if (hash_buffer.size() < 32) {
        m_val = hash_buffer;
        return;
    }
    // Write the hash to the 'val'.
    utl::xkeccak256_t hasher;
    hasher.update(hash_buffer.data(), hash_buffer.size());
    hasher.get_hash(m_val);

    if (m_db != nullptr) {
        std::error_code ec;
        m_db->Put(m_key, hash_buffer, ec);
        assert(!ec);
    }
}

xh256_t xtop_stack_trie::Hash() {
    hash();
    if (m_val.size() != 32) {
        // If the node's RLP isn't 32 bytes long, the node will not
        // be hashed, and instead contain the  rlp-encoding of the
        // node. For the top level node, we need to force the hashing.
        xbytes_t res;
        utl::xkeccak256_t hasher;
        hasher.update(m_val.data(), m_val.size());
        hasher.get_hash(res);
        return xh256_t{res};
    }
    return xh256_t{m_val};
}

xh256_t xtop_stack_trie::Commit(std::error_code & ec) {
    if (m_db = nullptr) {
        ec = error::xerrc_t::trie_db_not_provided;
        return xh256_t{};
    }
    if (m_val.size() != 32) {
        // If the node's RLP isn't 32 bytes long, the node will not
        // be hashed, and instead contain the  rlp-encoding of the
        // node. For the top level node, we need to force the hashing.
        xbytes_t res;
        utl::xkeccak256_t hasher;
        hasher.update(m_val.data(), m_val.size());
        hasher.get_hash(res);
        m_db->Put(res, m_val, ec);
        return xh256_t{res};
    }
    return xh256_t{m_val};
}

std::shared_ptr<xtop_stack_trie> xtop_stack_trie::newLeaf(xbytes_t const & key, xbytes_t const & val) {
    std::shared_ptr<xtop_stack_trie> newnode = std::make_shared<xtop_stack_trie>(m_db);
    newnode->m_type = xstack_trie_node_type_t::leafNode;
    newnode->m_key = key;
    newnode->m_val = val;
    return newnode;
}

std::shared_ptr<xtop_stack_trie> xtop_stack_trie::newExt(xbytes_t const & key, std::shared_ptr<xtop_stack_trie> child) {
    std::shared_ptr<xtop_stack_trie> newnode = std::make_shared<xtop_stack_trie>(m_db);
    newnode->m_type = xstack_trie_node_type_t::extNode;
    newnode->m_key = key;
    newnode->m_children[0] = child;
    return newnode;
}

NS_END3