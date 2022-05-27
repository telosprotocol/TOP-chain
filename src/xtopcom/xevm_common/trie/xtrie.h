// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "assert.h"
#include "xbase/xns_macro.h"
#include "xbasic/xhash.hpp"
#include "xbasic/xhex.h"
#include "xevm_common/trie/xtrie_db_face.h"
#include "xevm_common/trie/xtrie_hasher.h"
#include "xevm_common/trie/xtrie_node.h"
#include "xevm_common/xerror/xerror.h"  // move to .cpp

#include <algorithm>
#include <tuple>
#include <type_traits>

NS_BEG3(top, evm_common, trie)

// emptyRoot is the known root hash of an empty trie.
// emptyRoot = common.HexToHash("56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421")
static std::error_code static_ec;  // todo delete this use some compile check form_hex to bytes/hash
static const xhash256_t emptyRoot = xhash256_t{top::from_hex("56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421", static_ec)};
// assert(!static_ec);

class xtop_trie {
private:
    xtrie_db_face_ptr_t m_db;
    xtrie_node_face_ptr_t m_root;

    std::size_t unhashed{0};

public:
    xtop_trie(xtrie_db_face_ptr_t db) : m_db{db} {
    }

public:
    static std::shared_ptr<xtop_trie> New(xhash256_t hash, xtrie_db_face_ptr_t db, std::error_code & ec) {
        if (db == nullptr) {
            xerror("build trie from null db");
        }
        auto trie = xtop_trie{db};
        if (hash != emptyRoot && hash != xhash256_t{}) {
            // resolve Hash
            auto root_hash = std::make_shared<xtrie_hash_node_t>(hash);
            auto root = trie.resolveHash(root_hash, ec);
            if (!ec) {
                return nullptr;
            }
            trie.m_root = root;
        }
        return std::make_shared<xtop_trie>(trie);
    }

public:
    // Reset drops the referenced root node and cleans all internal state.
    void Reset() {
        m_root = nullptr;
        unhashed = 0;
    }

    xhash256_t Hash() {
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

    xbytes_t Get(xbytes_t const & key) {
        std::error_code ec;
        auto result = TryGet(key, ec);
        if (ec) {
            xerror("trie error: %s %s", ec.category().name(), ec.message().c_str());
        }
        return result;
    }

    xbytes_t TryGet(xbytes_t const & key, std::error_code & ec) {
        xbytes_t value;
        xtrie_node_face_ptr_t newroot;
        bool didResolve;
        std::tie(value, newroot, didResolve) = tryGet(m_root, keybytesToHex(key), 0, ec);

        return value;
    }

    void Update(xbytes_t const & key, xbytes_t const & value) {
        std::error_code ec;
        TryUpdate(key, value, ec);
        if (ec) {
            xerror("trie error: %s %s", ec.category().name(), ec.message().c_str());
        }
        return;
    }

private:
    std::tuple<xbytes_t, xtrie_node_face_ptr_t, bool> tryGet(xtrie_node_face_ptr_t node, xbytes_t const & key, std::size_t const pos, std::error_code & ec) {
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
            std::tie(value, newnode, didResolve) = tryGet(n->Childern[key[pos]], key, pos + 1, ec);
            if (!ec && didResolve) {
                n = n->copy();
                n->Childern[key[pos]] = newnode;
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
        }
        }
    }

    void TryUpdate(xbytes_t const & key, xbytes_t const & value, std::error_code & ec) {
        unhashed++;
        auto k = keybytesToHex(key);
        if (value.size() != 0) {
            // todo
            // insert
        } else {
            // todo
            // delete
        }
    }

private:
    std::pair<bool, xtrie_node_face_ptr_t> insert(xtrie_node_face_ptr_t node, xbytes_t prefix, xbytes_t key, xtrie_node_face_ptr_t value, std::error_code & ec) {
        if (key.size() == 0) {
            if (node->type() == xtrie_node_type_t::valuenode) {
                auto v = std::make_shared<xtrie_value_node_t>(*(static_cast<xtrie_value_node_t *>(node.get())));
                auto vv = std::make_shared<xtrie_value_node_t>(*(static_cast<xtrie_value_node_t *>(value.get())));
                return std::make_pair(!EqualBytes(v->data(), vv->data()), value);
            }
            return std::make_pair(true, value);
        }

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
                std::tie(_, branch->Childern[n->Key[matchlen]]) = insert(nullptr, sprefix, skey, n->Val, ec);
                if (ec) {
                    return std::make_pair(false, nullptr);
                }
            }
            {
                auto sprefix = prefix;
                sprefix.insert(sprefix.end(), key.begin(), key.end() + matchlen + 1);
                auto skey = key;
                skey = {key.begin() + matchlen + 1, key.end()};
                std::tie(_, branch->Childern[key[matchlen]]) = insert(nullptr, sprefix, skey, value, ec);
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
            std::tie(dirty, nn) = insert(n->Childern[tkey], prefix, key, value, ec);
            if (!dirty || ec) {
                return std::make_pair(false, n);
            }
            n = n->copy();
            n->flags = newFlag();
            n->Childern[tkey] = nn;
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
        }
        }
    }

private:
    xtrie_node_face_ptr_t resolve(xtrie_node_face_ptr_t n, /*xbytes_t prefix,*/ std::error_code & ec) {
        if (n->type() == xtrie_node_type_t::hashnode) {
            auto hash_node_ptr = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(n.get())));
            return resolveHash(hash_node_ptr /*, prefix*/, ec);
        }
        return n;
    }

    xtrie_node_face_ptr_t resolveHash(xtrie_hash_node_ptr_t n, /*xbytes_t prefix,*/ std::error_code & ec) {
        auto hash = xhash256_t{n->data()};
        auto node = m_db->node(hash);
        if (!node) {
            ec = error::xerrc_t::trie_db_missing_node_error;
            return nullptr;
        }
        return node;
    }

    // hashRoot calculates the root hash of the given trie
    std::pair<xtrie_node_face_ptr_t, xtrie_node_face_ptr_t> hashRoot() {
        if (m_root == nullptr) {
            return std::make_pair(std::make_shared<xtrie_hash_node_t>(emptyRoot), nullptr);
        }

        auto hasher = xtrie_hasher_t::newHasher(unhashed >= (std::size_t)100);

        auto hash_result = hasher.hash(m_root, true);
        unhashed = 0;
        return hash_result;
    }

private:
    bool EqualBytes(xbytes_t const & a, xbytes_t const & b) {
        return (a.size() == b.size()) && std::equal(a.begin(), a.end(), b.begin());
    }

    inline nodeFlag newFlag() {
        nodeFlag f;
        f.dirty = true;
        return f;
    }
};
using xtrie_t = xtop_trie;

NS_END3