// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie.h"

#include "xevm_common/trie/xtrie_committer.h"
#include "xevm_common/trie/xtrie_encoding.h"
#include "xevm_common/trie/xtrie_hasher.h"
#include "xevm_common/xerror/xerror.h"

NS_BEG3(top, evm_common, trie)

std::shared_ptr<xtop_trie> xtop_trie::New(xhash256_t hash, xtrie_db_ptr_t db, std::error_code & ec) {
    xassert(!ec);
    if (db == nullptr) {
        xerror("build trie from null db");
    }
    auto trie = xtop_trie{db};
    if ((hash != emptyRoot) && (hash != xhash256_t{})) {
        // resolve Hash
        auto root_hash = std::make_shared<xtrie_hash_node_t>(hash);
        auto root = trie.resolveHash(root_hash, ec);
        if (ec) {
            return nullptr;
        }
        trie.m_root = root;
    }
    return std::make_shared<xtop_trie>(trie);
}

// Reset drops the referenced root node and cleans all internal state.
void xtop_trie::Reset() {
    m_root = nullptr;
    unhashed = 0;
}

// Hash returns the root hash of the trie. It does not write to the
// database and can be used even if the trie doesn't have one.
xhash256_t xtop_trie::Hash() {
    auto result = hashRoot();
    m_root = result.second;
    if (result.first->type() == xtrie_node_type_t::hashnode) {
        assert(dynamic_cast<xtrie_hash_node_t *>(result.first.get()) != nullptr);
        return xhash256_t{std::dynamic_pointer_cast<xtrie_hash_node_t>(result.first)->data()};
    } else {
        // geth: trie.go:522 hash.(hashNode)  what if hash.type() was not hashNode...
        // ??? normal won't happen. but it do leave the possiblity in code...
        assert(false);
        return {};
    }
}

// Get returns the value for key stored in the trie.
// The value bytes must not be modified by the caller.
xbytes_t xtop_trie::Get(xbytes_t const & key) const {
    std::error_code ec;
    auto result = TryGet(key, ec);
    if (ec) {
        xerror("trie error: %s %s", ec.category().name(), ec.message().c_str());
    }
    return result;
}

// TryGet returns the value for key stored in the trie.
// The value bytes must not be modified by the caller.
// If a node was not found in the database, a MissingNodeError(trie_db_missing_node_error) is returned.
xbytes_t xtop_trie::TryGet(xbytes_t const & key, std::error_code & ec) const {
    xassert(!ec);
    xbytes_t value;
    xtrie_node_face_ptr_t newroot;
    bool didResolve;
    std::tie(value, newroot, didResolve) = tryGet(m_root, keybytesToHex(key), 0, ec);

    return value;
}

std::pair<xbytes_t, std::size_t> xtop_trie::TryGetNode(xbytes_t const & path, std::error_code & ec) {
    xassert(!ec);
    xbytes_t item;
    xtrie_node_face_ptr_t newroot;
    std::size_t resolved;
    std::tie(item, newroot, resolved) = tryGetNode(m_root, compactToHex(path), 0, ec);
    if (ec) {
        return std::make_pair(xbytes_t{}, resolved);
    }
    if (resolved > 0) {
        m_root = newroot;
    }
    if (item.empty()) {
        return std::make_pair(xbytes_t{}, resolved);
    }
    return std::make_pair(item, resolved);
}

// Update associates key with value in the trie. Subsequent calls to
// Get will return value. If value has length zero, any existing value
// is deleted from the trie and calls to Get will return nil.
//
// The value bytes must not be modified by the caller while they are
// stored in the trie.
void xtop_trie::Update(xbytes_t const & key, xbytes_t const & value) {
    // printf("update: %s %s\n", top::to_hex(key).c_str(), top::to_hex(value).c_str());
    std::error_code ec;
    TryUpdate(key, value, ec);
    if (ec) {
        xerror("trie error: %s %s", ec.category().name(), ec.message().c_str());
    }
    return;
}

// TryUpdate associates key with value in the trie. Subsequent calls to
// Get will return value. If value has length zero, any existing value
// is deleted from the trie and calls to Get will return nil.
//
// The value bytes must not be modified by the caller while they are
// stored in the trie.
//
// If a node was not found in the database, a MissingNodeError(trie_db_missing_node_error) is returned.
void xtop_trie::TryUpdate(xbytes_t const & key, xbytes_t const & value, std::error_code & ec) {
    xassert(!ec);
    unhashed++;
    auto k = keybytesToHex(key);
    if (!value.empty()) {
        auto result = insert(m_root, {}, k, std::make_shared<xtrie_value_node_t>(value), ec);
        if (ec) {
            return;
        }
        m_root = result.second;
    } else {
        auto result = erase(m_root, {}, k, ec);
        if (ec) {
            return;
        }
        m_root = result.second;
    }
    return;
}

// Delete removes any existing value for key from the trie.
void xtop_trie::Delete(xbytes_t const & key) {
    std::error_code ec;
    TryDelete(key, ec);
    if (ec) {
        xerror("trie error: %s %s", ec.category().name(), ec.message().c_str());
    }
    return;
}

// TryDelete removes any existing value for key from the trie.
// If a node was not found in the database, a MissingNodeError(trie_db_missing_node_error) is returned.
void xtop_trie::TryDelete(xbytes_t const & key, std::error_code & ec) {
    xassert(!ec); 
    unhashed++;
    auto k = keybytesToHex(key);
    auto result = erase(m_root, {}, k, ec);
    if (ec) {
        return;
    }
    m_root = result.second;
    return;
}

// Commit writes all nodes to the trie's memory database, tracking the internal
// and external (for account tries) references.
std::pair<xhash256_t, int32_t> xtop_trie::Commit(std::error_code & ec) {
    xassert(!ec);
    // todo LeafCallback
    if (m_db == nullptr) {
        xerror("commit called on trie without database");
    }
    if (m_root == nullptr) {
        return std::make_pair(emptyRoot, 0);
    }

    auto rootHash = Hash();
    if (m_root->cache().second == false) {
        return std::make_pair(rootHash, 0);
    }

    xtrie_committer_t h;

    xtrie_hash_node_ptr_t newRoot;
    int32_t committed;
    std::tie(newRoot, committed) = h.Commit(m_root, m_db, ec);
    if (ec) {
        return std::make_pair(xhash256_t{}, 0);
    }
    m_root = newRoot;
    return std::make_pair(rootHash, committed);
}

bool xtop_trie::Prove(xbytes_t const & key, uint32_t fromLevel, xkv_db_face_ptr_t proofDB, std::error_code & ec) {
    xassert(!ec);
    // Collect all nodes on the path to key.
    auto key_path = keybytesToHex(key);
    std::vector<xtrie_node_face_ptr_t> nodes;
    auto tn = m_root;
    for (; key_path.size() > 0 && tn != nullptr;) {
        xdbg("key: %s", top::to_hex(key_path).c_str());
        switch (tn->type()) {
        case xtrie_node_type_t::shortnode: {
            auto n = std::dynamic_pointer_cast<xtrie_short_node_t>(tn);
            assert(n != nullptr);

            if (key_path.size() < n->key.size() || !std::equal(key_path.begin(), key_path.begin() + n->key.size(), n->key.begin())) {
                // The trie doesn't contain the key.
                tn = nullptr;
            } else {
                tn = n->val;
                key_path = {key_path.begin() + n->key.size(), key_path.end()};
            }
            nodes.push_back(n);
            xdbg("append shortnode %s", top::to_hex(key_path).c_str());
            break;
        }
        case xtrie_node_type_t::fullnode: {
            auto n = std::dynamic_pointer_cast<xtrie_full_node_t>(tn);
            assert(n != nullptr);
            tn = n->Children[key_path[0]];
            key_path.erase(key_path.begin());
            nodes.push_back(n);
            xdbg("append fullnode %s", top::to_hex(key_path).c_str());
            break;
        }
        case xtrie_node_type_t::hashnode: {
            auto n = std::dynamic_pointer_cast<xtrie_hash_node_t>(tn);
            assert(n != nullptr);
            tn = resolveHash(n, ec);
            if (ec) {
                xerror("unhandled trie error %s", ec.message().c_str());
                return false;
            }
            break;
        }
        default: {
            xassert(false);
        }
        }
    }

    auto hasher = xtrie_hasher_t::newHasher(false);
    xdbg("nodes.size():%zu", nodes.size());
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        if (fromLevel > 0) {
            fromLevel--;
            continue;
        }
        auto n = nodes[index];
        xtrie_node_face_ptr_t hn;
        std::tie(n, hn) = hasher.proofHash(n);
        if (hn->type() == xtrie_node_type_t::hashnode || index == 0) {
            // If the node's database encoding is a hash (or is the
            // root node), it becomes a proof element.
            auto enc = xtrie_node_rlp::EncodeToBytes(n);
            xtrie_hash_node_ptr_t hash;
            if (hn->type() != xtrie_node_type_t::hashnode) {
                hash = hasher.hashData(enc);
            } else {
                hash = std::dynamic_pointer_cast<xtrie_hash_node_t>(hn);
                assert(hash != nullptr);
            }
            xdbg("[Prove]: put into db <%s> %s", top::to_hex(hash->data()).c_str(), top::to_hex(enc).c_str());
            proofDB->Put(hash->data(), enc, ec);
        }
    }

    return true;
}

std::tuple<xbytes_t, xtrie_node_face_ptr_t, bool> xtop_trie::tryGet(xtrie_node_face_ptr_t node, xbytes_t const & key, std::size_t const pos, std::error_code & ec) const {
    xdbg("tryGet key: %s ,pos: %zu", top::to_hex(key).c_str(), pos);
    if (node == nullptr) {
        return std::make_tuple(xbytes_t{}, nullptr, false);
    }
    assert(node != nullptr);
    switch (node->type()) {
    case xtrie_node_type_t::invalid: {
        return std::make_tuple(xbytes_t{}, nullptr, false);
    }
    case xtrie_node_type_t::valuenode: {
        auto n = std::dynamic_pointer_cast<xtrie_value_node_t>(node);
        assert(n != nullptr);
        return std::make_tuple(n->data(), n, false);
    }
    case xtrie_node_type_t::shortnode: {
        auto n = std::dynamic_pointer_cast<xtrie_short_node_t>(node);
        assert(n != nullptr);
        if ((key.size() - pos < n->key.size()) || (!std::equal(n->key.begin(), n->key.end(), key.begin() + pos))) {
            // key not found in trie
            return std::make_tuple(xbytes_t{}, n, false);
        }
        xbytes_t value;
        xtrie_node_face_ptr_t newnode;
        bool didResolve;
        std::tie(value, newnode, didResolve) = tryGet(n->val, key, pos + n->key.size(), ec);
        if (!ec && didResolve) {
            n = n->clone();
            n->val = newnode;
        }
        return std::make_tuple(value, n, didResolve);
    }
    case xtrie_node_type_t::fullnode: {
        auto n = std::dynamic_pointer_cast<xtrie_full_node_t>(node);
        assert(n != nullptr);
        xbytes_t value;
        xtrie_node_face_ptr_t newnode;
        bool didResolve;
        std::tie(value, newnode, didResolve) = tryGet(n->Children[key[pos]], key, pos + 1, ec);
        if (!ec && didResolve) {
            n = n->clone();
            n->Children[key[pos]] = newnode;
        }
        return std::make_tuple(value, n, didResolve);
    }
    case xtrie_node_type_t::hashnode: {
        auto n = std::dynamic_pointer_cast<xtrie_hash_node_t>(node);
        assert(n != nullptr);
        auto child = resolveHash(n, ec);
        if (ec) {
            xwarn("resolve hash error: at key: %s pos:%zu", top::to_hex(key).c_str(), pos);
            return std::make_tuple(xbytes_t{}, n, true);
        }
        xbytes_t value;
        xtrie_node_face_ptr_t newnode;
        bool _didResolve;
        std::tie(value, newnode, _didResolve) = tryGet(child, key, pos, ec);
        return std::make_tuple(value, newnode, true);
    }
    default: {
        xassert(false);
        __builtin_unreachable();
    }
    }
}

std::tuple<xbytes_t, xtrie_node_face_ptr_t, std::size_t> xtop_trie::tryGetNode(xtrie_node_face_ptr_t orig_node,
                                                                               xbytes_t const & path,
                                                                               std::size_t const pos,
                                                                               std::error_code & ec) const {
    xdbg("tryGetNode path: %s, pos: %zu", top::to_hex(path).c_str(), pos);
    // If non-existent path requested, abort
    if (orig_node == nullptr) {
        return std::make_tuple(xbytes_t{}, nullptr, 0);
    }
    // If we reached the requested path, return the current node
    if (pos >= path.size()) {
        // Although we most probably have the original node expanded, encoding
        // that into consensus form can be nasty (needs to cascade down) and
        // time consuming. Instead, just pull the hash up from disk directly.

        std::shared_ptr<xtrie_hash_node_t> hash;
        if (orig_node->type() == xtrie_node_type_t::hashnode) {
            assert(dynamic_cast<xtrie_hash_node_t *>(orig_node.get()) != nullptr);
            hash = std::dynamic_pointer_cast<xtrie_hash_node_t>(orig_node);
        } else {
            bool _;
            std::tie(hash, _) = orig_node->cache();
        }
        if (hash == nullptr) {
            ec = error::xerrc_t::trie_node_unexpected;
            return std::make_tuple(xbytes_t{}, orig_node, 0);
        }
        auto blob = m_db->Node(xhash256_t{hash->data()}, ec);
        return std::make_tuple(blob, orig_node, 1);
    }
    // Path still needs to be traversed, descend into children
    assert(orig_node != nullptr);
    switch (orig_node->type()) {
    case xtrie_node_type_t::valuenode: {
        // Path prematurely ended, abort
        return std::make_tuple(xbytes_t{}, nullptr, 0);
    }
    case xtrie_node_type_t::shortnode: {
        auto n = std::dynamic_pointer_cast<xtrie_short_node_t>(orig_node);
        assert(n != nullptr);

        if ((path.size() - pos < n->key.size()) || (!std::equal(n->key.begin(), n->key.end(), path.begin() + pos))) {
            // Path branches off from short node
            return std::make_tuple(xbytes_t{}, n, 0);
        }
        xbytes_t item;
        xtrie_node_face_ptr_t newnode;
        std::size_t resolved;
        std::tie(item, newnode, resolved) = tryGetNode(n->val, path, pos + n->key.size(), ec);
        if (!ec && resolved > 0) {
            n = n->clone();
            n->val = newnode;
        }
        return std::make_tuple(item, n, resolved);
    }
    case xtrie_node_type_t::fullnode: {
        auto n = std::dynamic_pointer_cast<xtrie_full_node_t>(orig_node);
        assert(n != nullptr);

        xbytes_t item;
        xtrie_node_face_ptr_t newnode;
        std::size_t resolved;
        std::tie(item, newnode, resolved) = tryGetNode(n->Children[path[pos]], path, pos + 1, ec);
        if (!ec && resolved > 0) {
            n = n->clone();
            n->Children[path[pos]] = newnode;
        }
        return std::make_tuple(item, n, resolved);
    }
    case xtrie_node_type_t::hashnode: {
        auto n = std::dynamic_pointer_cast<xtrie_hash_node_t>(orig_node);
        assert(n != nullptr);

        auto child = resolveHash(n, ec);
        if (ec) {
            return std::make_tuple(xbytes_t{}, n, 1);
        }

        xbytes_t item;
        xtrie_node_face_ptr_t newnode;
        std::size_t resolved;
        std::tie(item, newnode, resolved) = tryGetNode(child, path, pos, ec);
        return std::make_tuple(item, newnode, resolved + 1);
    }
    default: {
        xassert(false);
        __builtin_unreachable();
    }
    }
}

std::pair<bool, xtrie_node_face_ptr_t> xtop_trie::insert(xtrie_node_face_ptr_t node, xbytes_t prefix, xbytes_t key, xtrie_node_face_ptr_t value, std::error_code & ec) {
    if (key.empty()) {
        if (node && node->type() == xtrie_node_type_t::valuenode) {
            assert(dynamic_cast<xtrie_value_node_t *>(node.get()) != nullptr);
            assert(dynamic_cast<xtrie_value_node_t *>(value.get()) != nullptr);

            return std::make_pair(std::dynamic_pointer_cast<xtrie_value_node_t>(node)->data() != std::dynamic_pointer_cast<xtrie_value_node_t>(value)->data(), value);
        }
        return std::make_pair(true, value);
    }

    if (node == nullptr) {
        return std::make_pair(true, std::make_shared<xtrie_short_node_t>(key, value, new_node_flag()));
    }

    assert(node != nullptr);
    switch (node->type()) {
    case xtrie_node_type_t::shortnode: {
        auto short_node = std::dynamic_pointer_cast<xtrie_short_node_t>(node);
        assert(short_node != nullptr);
        auto const matchlen = prefixLen(key, short_node->key);
        // If the whole key matches, keep this short node as is
        // and only update the value.
        if (matchlen == short_node->key.size()) {
            bool dirty;
            xtrie_node_face_ptr_t new_node;
            auto key_break_pos = std::next(key.begin(), matchlen);

            prefix.insert(prefix.end(), key.begin(), key_break_pos);
            key = {key_break_pos, key.end()};

            std::tie(dirty, new_node) = insert(short_node->val, prefix, key, value, ec);
            if (!dirty || ec) {
                return std::make_pair(false, short_node);
            }
            return std::make_pair(true, std::make_shared<xtrie_short_node_t>(short_node->key, new_node, new_node_flag()));
        }

        // Otherwise branch out at the index where they differ.
        auto branch = std::make_shared<xtrie_full_node_t>(new_node_flag());
        bool _;
        {
            auto const short_node_key_break_pos = std::next(short_node->key.begin(), matchlen + 1);

            auto sprefix = prefix;
            sprefix.insert(sprefix.end(), short_node->key.begin(), short_node_key_break_pos);
            auto skey = xbytes_t{short_node_key_break_pos, short_node->key.end()};

            std::tie(_, branch->Children[short_node->key[matchlen]]) = insert(nullptr, sprefix, skey, short_node->val, ec);
            if (ec) {
                return std::make_pair(false, nullptr);
            }
        }
        {
            auto const key_break_pos = std::next(key.begin(), matchlen + 1);

            auto sprefix = prefix;
            sprefix.insert(sprefix.end(), key.begin(), key_break_pos);
            auto const skey = xbytes_t{key_break_pos, key.end()};
            std::tie(_, branch->Children[key[matchlen]]) = insert(nullptr, sprefix, skey, value, ec);
            if (ec) {
                return std::make_pair(false, nullptr);
            }
        }
        // Replace this shortNode with the branch if it occurs at index 0.
        if (matchlen == 0) {
            return std::make_pair(true, branch);
        }
        // Otherwise, replace it with a short node leading up to the branch.
        xbytes_t short_key = {key.begin(), std::next(key.begin(), matchlen)};
        return std::make_pair(true, std::make_shared<xtrie_short_node_t>(short_key, branch, new_node_flag()));
    }
    case xtrie_node_type_t::fullnode: {
        auto full_node = std::dynamic_pointer_cast<xtrie_full_node_t>(node);
        assert(full_node != nullptr);

        bool dirty;
        xtrie_node_face_ptr_t child;
        xbyte_t tkey = key[0];
        prefix.insert(prefix.end(), tkey);
        key.erase(key.begin());
        std::tie(dirty, child) = insert(full_node->Children[tkey], prefix, key, value, ec);
        if (!dirty || ec) {
            return std::make_pair(false, full_node);
        }
        full_node = full_node->clone();
        full_node->flags = new_node_flag();
        full_node->Children[tkey] = child;
        return std::make_pair(true, full_node);
    }
    case xtrie_node_type_t::invalid: {
        return std::make_pair(true, std::make_shared<xtrie_short_node_t>(key, value, new_node_flag()));
    }
    case xtrie_node_type_t::hashnode: {
        // We've hit a part of the trie that isn't loaded yet. Load
        // the node and insert into it. This leaves all child nodes on
        // the path to the value in the trie.
        auto const hash_node = std::dynamic_pointer_cast<xtrie_hash_node_t>(node);
        assert(hash_node != nullptr);
        auto real_node = resolveHash(hash_node, ec);
        if (ec) {
            return std::make_pair(false, real_node);
        }
        bool dirty;
        xtrie_node_face_ptr_t new_node;
        std::tie(dirty, new_node) = insert(real_node, prefix, key, value, ec);
        if (!dirty || ec) {
            return std::make_pair(false, real_node);
        }
        return std::make_pair(true, new_node);
    }
    default: {
        xassert(false);
        __builtin_unreachable();
    }
    }
}

std::pair<bool, xtrie_node_face_ptr_t> xtop_trie::erase(xtrie_node_face_ptr_t node, xbytes_t prefix, xbytes_t key, std::error_code & ec) {
    // printf("erase: prefix: %s key: %s\n", top::to_hex(prefix).c_str(), top::to_hex(key).c_str());
    if (node == nullptr) {
        return std::make_pair(false, nullptr);
    }
    assert(node != nullptr);

    switch (node->type()) {
    case xtrie_node_type_t::shortnode: {
        // printf("erase node_type short\n");
        auto short_node = std::dynamic_pointer_cast<xtrie_short_node_t>(node);
        assert(short_node != nullptr);
        auto const matchlen = prefixLen(key, short_node->key);
        if (matchlen < short_node->key.size()) {
            return std::make_pair(false, short_node);  // don't replace n on mismatch
        }
        if (matchlen == key.size()) {
            return std::make_pair(true, nullptr);  // remove n entirely for whole matches
        }
        // The key is longer than n.Key. Remove the remaining suffix
        // from the subtrie. Child can never be nil here since the
        // subtrie must contain at least two other values with keys
        // longer than n.Key.
        bool dirty;
        xtrie_node_face_ptr_t child;
        xbytes_t child_prefix = prefix;
        auto const key_break_pos = std::next(key.begin(), short_node->key.size());
        child_prefix.insert(child_prefix.end(), key.begin(), key_break_pos);
        xbytes_t const child_key = {key_break_pos, key.end()};
        std::tie(dirty, child) = erase(short_node->val, child_prefix, child_key, ec);
        if (!dirty || ec) {
            return std::make_pair(false, short_node);
        }

        switch (child->type()) {
        case xtrie_node_type_t::shortnode: {
            // Deleting from the subtrie reduced it to another
            // short node. Merge the nodes to avoid creating a
            // shortNode{..., shortNode{...}}. Use concat (which
            // always creates a new slice) instead of append to
            // avoid modifying n.Key since it might be shared with
            // other nodes.
            auto const child_node = std::dynamic_pointer_cast<xtrie_short_node_t>(child);
            assert(child_node != nullptr);

            xbytes_t cchild_key = short_node->key;
            cchild_key.insert(cchild_key.end(), child_node->key.begin(), child_node->key.end());
            return std::make_pair(true, std::make_shared<xtrie_short_node_t>(cchild_key, child_node->val, new_node_flag()));
        }
        default: {
            return std::make_pair(true, std::make_shared<xtrie_short_node_t>(short_node->key, child, new_node_flag()));
        }
        }
    }

    case xtrie_node_type_t::fullnode: {
        // printf("erase node_type full\n");
        auto full_node = std::dynamic_pointer_cast<xtrie_full_node_t>(node);
        assert(full_node != nullptr);

        bool dirty;
        xtrie_node_face_ptr_t new_node;
        xbyte_t tkey = key[0];
        xbytes_t nprefix = prefix;
        nprefix.insert(nprefix.end(), tkey);
        key.erase(key.begin());
        std::tie(dirty, new_node) = erase(full_node->Children[tkey], nprefix, key, ec);
        if (!dirty || ec) {
            return std::make_pair(false, full_node);
        }
        full_node = full_node->clone();
        full_node->flags = new_node_flag();
        full_node->Children[tkey] = new_node;

        // Because n is a full node, it must've contained at least two children
        // before the delete operation. If the new child value is non-nil, n still
        // has at least two children after the deletion, and cannot be reduced to
        // a short node.
        if (new_node != nullptr) {
            return std::make_pair(true, full_node);
        }

        // Reduction:
        // Check how many non-nil entries are left after deleting and
        // reduce the full node to a short node if only one entry is
        // left. Since n must've contained at least two children
        // before deletion (otherwise it would not be a full node) n
        // can never be reduced to nil.
        //
        // When the loop is done, pos contains the index of the single
        // value that is left in n or -2 if n contains at least two
        // values.
        int32_t pos = -1;
        int32_t ind = 0;
        for (auto const & cld : full_node->Children) {
            if (cld != nullptr) {
                if (pos == -1) {
                    pos = ind;
                } else {
                    pos = -2;
                    break;
                }
            }
            ind++;
        }
        if (pos >= 0) {
            if (pos != 16) {
                auto cnode = resolve(full_node->Children[pos], /*prefix,*/ ec);
                if (ec) {
                    return std::make_pair(false, nullptr);
                }
                if (cnode->type() == xtrie_node_type_t::shortnode) {
                    auto short_child_node = std::dynamic_pointer_cast<xtrie_short_node_t>(cnode);
                    xbytes_t k = short_child_node->key;
                    k.insert(k.begin(), xbyte_t{(xbyte_t)pos});
                    return std::make_pair(true, std::make_shared<xtrie_short_node_t>(k, short_child_node->val, new_node_flag()));
                }
            }

            // Otherwise, n is replaced by a one-nibble short node
            // containing the child.
            xbytes_t k{(xbyte_t)pos};
            return std::make_pair(true, std::make_shared<xtrie_short_node_t>(k, full_node->Children[pos], new_node_flag()));
        }
        // n still contains at least two values and cannot be reduced.
        return std::make_pair(true, full_node);
    }
    case xtrie_node_type_t::valuenode: {
        // printf("erase node_type value\n");
        return std::make_pair(true, nullptr);
    }
    case xtrie_node_type_t::invalid: {
        // printf("erase node_type invalid\n");
        return std::make_pair(false, nullptr);
    }
    case xtrie_node_type_t::hashnode: {
        // printf("erase node_type hash\n");
        auto hash_node = std::dynamic_pointer_cast<xtrie_hash_node_t>(node);
        assert(hash_node != nullptr);

        auto real_node = resolveHash(hash_node, /*prefix,*/ ec);
        if (ec) {
            return std::make_pair(false, nullptr);
        }
        bool dirty;
        xtrie_node_face_ptr_t nn;
        std::tie(dirty, nn) = erase(real_node, prefix, key, ec);
        if (!dirty || ec) {
            return std::make_pair(false, real_node);
        }
        return std::make_pair(true, nn);
    }
    default:
        xassert(false);
        __builtin_unreachable();
    }
}

xtrie_node_face_ptr_t xtop_trie::resolve(xtrie_node_face_ptr_t n, /*xbytes_t prefix,*/ std::error_code & ec) {
    if (n->type() == xtrie_node_type_t::hashnode) {
        auto hash_node_ptr = std::dynamic_pointer_cast<xtrie_hash_node_t>(n);
        assert(hash_node_ptr != nullptr);

        return resolveHash(std::move(hash_node_ptr) /*, prefix*/, ec);
    }
    return n;
}

xtrie_node_face_ptr_t xtop_trie::resolveHash(xtrie_hash_node_ptr_t n, /*xbytes_t prefix,*/ std::error_code & ec) const {
    auto const hash = xhash256_t{n->data()};
    auto node = m_db->node(hash);
    if (!node) {
        ec = error::xerrc_t::trie_db_missing_node_error;
        return nullptr;
    }
    return node;
}

// hashRoot calculates the root hash of the given trie
std::pair<xtrie_node_face_ptr_t, xtrie_node_face_ptr_t> xtop_trie::hashRoot() {
    if (m_root == nullptr) {
        return std::make_pair(std::make_shared<xtrie_hash_node_t>(emptyRoot), nullptr);
    }

    auto hasher = xtrie_hasher_t::newHasher(unhashed >= 100u);

    auto hash_result = hasher.hash(m_root, true);
    unhashed = 0;
    return hash_result;
}

xnode_flag_t xtop_trie::new_node_flag() {
    xnode_flag_t f;
    f.dirty = true;
    return f;
}

NS_END3
