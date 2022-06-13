// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie.h"

#include "xevm_common/trie/xtrie_committer.h"
#include "xevm_common/trie/xtrie_encoding.h"
#include "xevm_common/trie/xtrie_hasher.h"
#include "xevm_common/xerror/xerror.h"

NS_BEG3(top, evm_common, trie)

static xhash256_t emptyRoot = xhash256_t{emptyRootBytes};

std::shared_ptr<xtop_trie> xtop_trie::New(xhash256_t hash, xtrie_db_ptr_t db, std::error_code & ec) {
    if (db == nullptr) {
        xerror("build trie from null db");
    }
    auto trie = xtop_trie{db};
    if (hash != emptyRoot && hash != xhash256_t{}) {
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
        return xhash256_t{static_cast<xtrie_hash_node_t *>(result.first.get())->data()};
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
    xbytes_t value;
    xtrie_node_face_ptr_t newroot;
    bool didResolve;
    std::tie(value, newroot, didResolve) = tryGet(m_root, keybytesToHex(key), 0, ec);

    return value;
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
    unhashed++;
    auto k = keybytesToHex(key);
    if (value.size() != 0) {
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
    // Collect all nodes on the path to key.
    auto key_path = keybytesToHex(key);
    std::vector<xtrie_node_face_ptr_t> nodes;
    auto tn = m_root;
    for (; key_path.size() > 0 && tn != nullptr;) {
        xdbg("key: %s", top::to_hex(key_path).c_str());
        switch (tn->type()) {
        case xtrie_node_type_t::shortnode: {
            auto n = std::make_shared<xtrie_short_node_t>(*(static_cast<xtrie_short_node_t *>(tn.get())));
            if (key_path.size() < n->Key.size() || !std::equal(key_path.begin(), key_path.begin() + n->Key.size(), n->Key.begin())) {
                // The trie doesn't contain the key.
                tn = nullptr;
            } else {
                tn = n->Val;
                key_path = {key_path.begin() + n->Key.size(), key_path.end()};
            }
            nodes.push_back(n);
            xdbg("append shortnode %s", top::to_hex(key_path).c_str());
            break;
        }
        case xtrie_node_type_t::fullnode: {
            auto n = std::make_shared<xtrie_full_node_t>(*(static_cast<xtrie_full_node_t *>(tn.get())));
            tn = n->Children[key_path[0]];
            key_path.erase(key_path.begin());
            nodes.push_back(n);
            xdbg("append fullnode %s", top::to_hex(key_path).c_str());
            break;
        }
        case xtrie_node_type_t::hashnode: {
            auto n = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(tn.get())));
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
                hash = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(hn.get())));
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
        auto n = std::make_shared<xtrie_value_node_t>(*(static_cast<xtrie_value_node_t *>(node.get())));
        return std::make_tuple(n->data(), n, false);
    }
    case xtrie_node_type_t::shortnode: {
        auto n = std::make_shared<xtrie_short_node_t>(*(static_cast<xtrie_short_node_t *>(node.get())));
        if ((key.size() - pos < n->Key.size()) || (!std::equal(n->Key.begin(), n->Key.end(), key.begin() + pos))) {
            // key not found in trie
            return std::make_tuple(xbytes_t{}, n, false);
        }
        xbytes_t value;
        xtrie_node_face_ptr_t newnode;
        bool didResolve;
        std::tie(value, newnode, didResolve) = tryGet(n->Val, key, pos + n->Key.size(), ec);
        if (!ec && didResolve) {
            n = n->copy();
            n->Val = newnode;
        }
        return std::make_tuple(value, n, didResolve);
    }
    case xtrie_node_type_t::fullnode: {
        auto n = std::make_shared<xtrie_full_node_t>(*(static_cast<xtrie_full_node_t *>(node.get())));
        xbytes_t value;
        xtrie_node_face_ptr_t newnode;
        bool didResolve;
        std::tie(value, newnode, didResolve) = tryGet(n->Children[key[pos]], key, pos + 1, ec);
        if (!ec && didResolve) {
            n = n->copy();
            n->Children[key[pos]] = newnode;
        }
        return std::make_tuple(value, n, didResolve);
    }
    case xtrie_node_type_t::hashnode: {
        auto n = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(node.get())));
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

std::pair<bool, xtrie_node_face_ptr_t> xtop_trie::insert(xtrie_node_face_ptr_t node, xbytes_t prefix, xbytes_t key, xtrie_node_face_ptr_t value, std::error_code & ec) {
    if (key.size() == 0) {
        if (node && node->type() == xtrie_node_type_t::valuenode) {
            auto v = std::make_shared<xtrie_value_node_t>(*(static_cast<xtrie_value_node_t *>(node.get())));
            auto vv = std::make_shared<xtrie_value_node_t>(*(static_cast<xtrie_value_node_t *>(value.get())));
            return std::make_pair(!EqualBytes(v->data(), vv->data()), value);
        }
        return std::make_pair(true, value);
    }

    if (node == nullptr) {
        return std::make_pair(true, std::make_shared<xtrie_short_node_t>(key, value, newFlag()));
    }
    assert(node != nullptr);
    switch (node->type()) {
    case xtrie_node_type_t::shortnode: {
        auto n = std::make_shared<xtrie_short_node_t>(*(static_cast<xtrie_short_node_t *>(node.get())));
        auto matchlen = prefixLen(key, n->Key);
        // If the whole key matches, keep this short node as is
        // and only update the value.
        if (matchlen == n->Key.size()) {
            bool dirty;
            xtrie_node_face_ptr_t nn;
            prefix.insert(prefix.end(), key.begin(), key.begin() + matchlen);
            key = {key.begin() + matchlen, key.end()};
            std::tie(dirty, nn) = insert(n->Val, prefix, key, value, ec);
            if (!dirty || ec) {
                return std::make_pair(false, n);
            }
            return std::make_pair(true, std::make_shared<xtrie_short_node_t>(n->Key, nn, newFlag()));
        }
        // Otherwise branch out at the index where they differ.
        auto branch = std::make_shared<xtrie_full_node_t>(newFlag());
        bool _;
        {
            auto sprefix = prefix;
            sprefix.insert(sprefix.end(), n->Key.begin(), n->Key.begin() + matchlen + 1);
            auto skey = n->Key;
            skey = {skey.begin() + matchlen + 1, skey.end()};
            std::tie(_, branch->Children[n->Key[matchlen]]) = insert(nullptr, sprefix, skey, n->Val, ec);
            if (ec) {
                return std::make_pair(false, nullptr);
            }
        }
        {
            auto sprefix = prefix;
            sprefix.insert(sprefix.end(), key.begin(), key.end() + matchlen + 1);
            auto skey = key;
            skey = {key.begin() + matchlen + 1, key.end()};
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
        xbytes_t short_key = {key.begin(), key.begin() + matchlen};
        return std::make_pair(true, std::make_shared<xtrie_short_node_t>(short_key, branch, newFlag()));
    }
    case xtrie_node_type_t::fullnode: {
        auto n = std::make_shared<xtrie_full_node_t>(*(static_cast<xtrie_full_node_t *>(node.get())));
        bool dirty;
        xtrie_node_face_ptr_t nn;
        xbyte_t tkey = key[0];
        prefix.insert(prefix.end(), tkey);
        key.erase(key.begin());
        std::tie(dirty, nn) = insert(n->Children[tkey], prefix, key, value, ec);
        if (!dirty || ec) {
            return std::make_pair(false, n);
        }
        n = n->copy();
        n->flags = newFlag();
        n->Children[tkey] = nn;
        return std::make_pair(true, n);
    }
    case xtrie_node_type_t::invalid: {
        return std::make_pair(true, std::make_shared<xtrie_short_node_t>(key, value, newFlag()));
    }
    case xtrie_node_type_t::hashnode: {
        // We've hit a part of the trie that isn't loaded yet. Load
        // the node and insert into it. This leaves all child nodes on
        // the path to the value in the trie.
        auto n = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(node.get())));
        auto rn = resolveHash(n, ec);
        if (ec) {
            return std::make_pair(false, rn);
        }
        bool dirty;
        xtrie_node_face_ptr_t nn;
        std::tie(dirty, nn) = insert(rn, prefix, key, value, ec);
        if (!dirty || ec) {
            return std::make_pair(false, rn);
        }
        return std::make_pair(true, nn);
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
        auto n = std::make_shared<xtrie_short_node_t>(*(static_cast<xtrie_short_node_t *>(node.get())));
        auto matchlen = prefixLen(key, n->Key);
        if (matchlen < n->Key.size()) {
            return std::make_pair(false, n);  // don't replace n on mismatch
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
        child_prefix.insert(child_prefix.end(), key.begin(), key.begin() + n->Key.size());
        xbytes_t child_key = {key.begin() + n->Key.size(), key.end()};
        std::tie(dirty, child) = erase(n->Val, child_prefix, child_key, ec);
        if (!dirty || ec) {
            return std::make_pair(false, n);
        }
        switch (child->type()) {
        case xtrie_node_type_t::shortnode: {
            // Deleting from the subtrie reduced it to another
            // short node. Merge the nodes to avoid creating a
            // shortNode{..., shortNode{...}}. Use concat (which
            // always creates a new slice) instead of append to
            // avoid modifying n.Key since it might be shared with
            // other nodes.
            auto child_node = std::make_shared<xtrie_short_node_t>(*(static_cast<xtrie_short_node_t *>(child.get())));
            xbytes_t cchild_key = n->Key;
            cchild_key.insert(cchild_key.end(), child_node->Key.begin(), child_node->Key.end());
            return std::make_pair(true, std::make_shared<xtrie_short_node_t>(cchild_key, child_node->Val, newFlag()));
        }
        default: {
            return std::make_pair(true, std::make_shared<xtrie_short_node_t>(n->Key, child, newFlag()));
        }
        }
    }
    case xtrie_node_type_t::fullnode: {
        // printf("erase node_type full\n");
        auto n = std::make_shared<xtrie_full_node_t>(*(static_cast<xtrie_full_node_t *>(node.get())));
        bool dirty;
        xtrie_node_face_ptr_t nn;
        xbyte_t tkey = key[0];
        xbytes_t nprefix = prefix;
        nprefix.insert(nprefix.end(), tkey);
        key.erase(key.begin());
        std::tie(dirty, nn) = erase(n->Children[tkey], nprefix, key, ec);
        if (!dirty || ec) {
            return std::make_pair(false, n);
        }
        n = n->copy();
        n->flags = newFlag();
        n->Children[tkey] = nn;

        // Because n is a full node, it must've contained at least two children
        // before the delete operation. If the new child value is non-nil, n still
        // has at least two children after the deletion, and cannot be reduced to
        // a short node.
        if (nn != nullptr) {
            return std::make_pair(true, n);
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
        for (auto const & cld : n->Children) {
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
                auto cnode = resolve(n->Children[pos], /*prefix,*/ ec);
                if (ec) {
                    return std::make_pair(false, nullptr);
                }
                if (cnode->type() == xtrie_node_type_t::shortnode) {
                    auto n = std::make_shared<xtrie_short_node_t>(*(static_cast<xtrie_short_node_t *>(cnode.get())));
                    xbytes_t k = n->Key;
                    k.insert(k.begin(), xbyte_t{(xbyte_t)pos});
                    return std::make_pair(true, std::make_shared<xtrie_short_node_t>(k, n->Val, newFlag()));
                }
            }

            // Otherwise, n is replaced by a one-nibble short node
            // containing the child.
            xbytes_t k{(xbyte_t)pos};
            return std::make_pair(true, std::make_shared<xtrie_short_node_t>(k, n->Children[pos], newFlag()));
        }
        // n still contains at least two values and cannot be reduced.
        return std::make_pair(true, n);
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
        auto n = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(node.get())));
        auto rn = resolveHash(n, /*prefix,*/ ec);
        if (ec) {
            return std::make_pair(false, nullptr);
        }
        bool dirty;
        xtrie_node_face_ptr_t nn;
        std::tie(dirty, nn) = erase(rn, prefix, key, ec);
        if (!dirty || ec) {
            return std::make_pair(false, rn);
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
        auto hash_node_ptr = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(n.get())));
        return resolveHash(hash_node_ptr /*, prefix*/, ec);
    }
    return n;
}

xtrie_node_face_ptr_t xtop_trie::resolveHash(xtrie_hash_node_ptr_t n, /*xbytes_t prefix,*/ std::error_code & ec) const {
    auto hash = xhash256_t{n->data()};
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

    auto hasher = xtrie_hasher_t::newHasher(unhashed >= (std::size_t)100);

    auto hash_result = hasher.hash(m_root, true);
    unhashed = 0;
    return hash_result;
}

NS_END3