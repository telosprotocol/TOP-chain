// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/trie/xtrie_db_fwd.h"
#include "xevm_common/trie/xtrie_node_fwd.h"
#include "xevm_common/trie/xtrie_pruner_fwd.h"
#include "xevm_common/xfixed_hash.h"
#include "xtrie_kv_db_face.h"

#include <tuple>

NS_BEG3(top, evm_common, trie)

// emptyRoot is the known root hash of an empty trie.
XINLINE_CONSTEXPR auto empty_root_bytes = ConstHexBytes<32>("56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
extern xh256_t const empty_root;

// callback(paths, hexpath, leaf, parent_hash, ec);
using leaf_callback = std::function<void(std::vector<xbytes_t> const &, xbytes_t const &, xbytes_t const &, xh256_t const &, std::error_code &)>;

class xtop_trie {
private:
    std::shared_ptr<xtrie_db_t> trie_db_;
    xtrie_node_face_ptr_t trie_root_;
    std::unique_ptr<xtrie_pruner_t> pruner_;

    std::size_t unhashed_{0};

public:
    xtop_trie(xtop_trie const &) = delete;
    xtop_trie & operator=(xtop_trie const &) = delete;
    xtop_trie(xtop_trie &&) = default;
    xtop_trie & operator=(xtop_trie &&) = default;
    ~xtop_trie();

protected:
    explicit xtop_trie(std::shared_ptr<xtrie_db_t> db);

public:
    std::shared_ptr<xtrie_db_t> const & trie_db() const noexcept;

    static std::shared_ptr<xtop_trie> build_from(xh256_t const & hash, std::shared_ptr<xtrie_db_t> db, std::error_code & ec);

    // Reset drops the referenced root node and cleans all internal state.
    void reset();

    // Hash returns the root hash of the trie. It does not write to the
    // database and can be used even if the trie doesn't have one.
    xh256_t hash();

    // Get returns the value for key stored in the trie.
    // The value bytes must not be modified by the caller.
    xbytes_t get(xbytes_t const & key) const;

    // TryGet returns the value for key stored in the trie.
    // The value bytes must not be modified by the caller.
    // If a node was not found in the database, a MissingNodeError(trie_db_missing_node_error) is returned.
    xbytes_t try_get(xbytes_t const & key, std::error_code & ec) const;

    // TryGetNode attempts to retrieve a trie node by compact-encoded path. It is not
    // possible to use keybyte-encoding as the path might contain odd nibbles.
    std::pair<xbytes_t, std::size_t> try_get_node(xbytes_t const & path, std::error_code & ec);

    // Update associates key with value in the trie. Subsequent calls to
    // Get will return value. If value has length zero, any existing value
    // is deleted from the trie and calls to Get will return nil.
    //
    // The value bytes must not be modified by the caller while they are
    // stored in the trie.
    void update(xbytes_t const & key, xbytes_t const & value);

    // TryUpdate associates key with value in the trie. Subsequent calls to
    // Get will return value. If value has length zero, any existing value
    // is deleted from the trie and calls to Get will return nil.
    //
    // The value bytes must not be modified by the caller while they are
    // stored in the trie.
    //
    // If a node was not found in the database, a MissingNodeError(trie_db_missing_node_error) is returned.
    void try_update(xbytes_t const & key, xbytes_t const & value, std::error_code & ec);

    // Delete removes any existing value for key from the trie.
    void Delete(xbytes_t const & key);

    // TryDelete removes any existing value for key from the trie.
    // If a node was not found in the database, a MissingNodeError(trie_db_missing_node_error) is returned.
    void try_delete(xbytes_t const & key, std::error_code & ec);

    // Commit writes all nodes to the trie's memory database, tracking the internal
    // and external (for account tries) references.
    std::pair<xh256_t, int32_t> commit(std::error_code & ec);

    // Prove constructs a merkle proof for key. The result contains all encoded nodes
    // on the path to the value at key. The value itself is also included in the last
    // node and can be retrieved by verifying the proof.
    //
    // If the trie does not contain a value for key, the returned proof contains all
    // nodes of the longest existing prefix of the key (at least the root node), ending
    // with the node that proves the absence of the key.
    bool prove(xbytes_t const & key, uint32_t from_level, xkv_db_face_ptr_t const & proof_db, std::error_code & ec) const;

    void prune(xh256_t const & old_trie_root_hash, std::error_code & ec);
    void commit_pruned(std::error_code & ec);

    std::string to_string() const;

    std::shared_ptr<xtrie_node_face_t> root() const noexcept;
    xtrie_node_face_ptr_t resolve_hash(xh256_t const & hash, std::error_code & ec) const;

private:
    std::tuple<xbytes_t, xtrie_node_face_ptr_t, bool> try_get(xtrie_node_face_ptr_t const & node, xbytes_t const & key, std::size_t pos, std::error_code & ec) const;

    std::tuple<xbytes_t, xtrie_node_face_ptr_t, std::size_t> try_get_node(xtrie_node_face_ptr_t const & orig_node, xbytes_t const & path, std::size_t pos, std::error_code & ec) const;

    struct update_result {  // NOLINT(clang-diagnostic-padded)
        std::shared_ptr<xtrie_node_face_t> new_node{nullptr};
        bool dirty{false};

        update_result() = default;
        update_result(update_result const &) = default;
        update_result & operator=(update_result const &) = default;
        update_result(update_result &&) = default;
        update_result & operator=(update_result &&) = default;
        ~update_result() = default;

        update_result(bool updated, std::shared_ptr<xtrie_node_face_t> n) noexcept;
    };
    /**
     * @brief insert key/value under node.
     * @param node the root node for this inserted k/v
     * @param prefix consumed key path
     * @param key key to be inserted.
     * @param value value to be inserted.
     * @param ec record the error in the insert.
     * @return pair<bool, ptr>, true if insert successful, false otherwise; ptr is the new root after key/value inserted.
     */
    update_result insert(xtrie_node_face_ptr_t const & node, xbytes_t prefix, xbytes_t key, xtrie_node_face_ptr_t const & value, std::error_code & ec);

    update_result erase(xtrie_node_face_ptr_t const & node, xbytes_t const & prefix, xbytes_t key, std::error_code & ec);

    xtrie_node_face_ptr_t resolve(xtrie_node_face_ptr_t const & n, /*xbytes_t prefix,*/ std::error_code & ec) const;

    xtrie_node_face_ptr_t resolve_hash(xtrie_hash_node_ptr_t const & n, /*xbytes_t prefix,*/ std::error_code & ec) const;

    xbytes_t resolve_blob(std::shared_ptr<xtrie_hash_node_t> const & n, std::error_code & ec) const;

    // hashRoot calculates the root hash of the given trie
    std::pair<xtrie_node_face_ptr_t, xtrie_node_face_ptr_t> hash_root();

    static xnode_flag_t node_dirty();
};
using xtrie_t = xtop_trie;

NS_END3
