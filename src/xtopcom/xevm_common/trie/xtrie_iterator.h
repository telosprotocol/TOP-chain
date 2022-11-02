// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xhash.hpp"
#include "xbasic/xmemory.hpp"
#include "xevm_common/trie/xtrie.h"

#include <memory>
//#include <system_error>
//#include <tuple>
#include <vector>

NS_BEG3(top, evm_common, trie)

class xtop_trie_simple_iterator {
public:
    static std::vector<xbytes_t> trie_leafs(xhash256_t const & trie_root_hash, observer_ptr<xtrie_db_t> const & trie_db);

private:
    static void get_trie_leafs(std::shared_ptr<xtrie_node_face_t> const & node, observer_ptr<xtrie_db_t> const & trie_db, std::vector<xbytes_t> & leafs);
    static void get_hash_node_leafs(std::shared_ptr<xtrie_hash_node_t> const & hash_node, observer_ptr<xtrie_db_t> const & trie_db, std::vector<xbytes_t> & leafs);
    static void get_short_node_leafs(std::shared_ptr<xtrie_short_node_t> const & short_node, observer_ptr<xtrie_db_t> const & trie_db, std::vector<xbytes_t> & leafs);
    static void get_value_node_leaf(std::shared_ptr<xtrie_value_node_t> const & value_node, observer_ptr<xtrie_db_t> const & trie_db, std::vector<xbytes_t> & leafs);
    static void get_full_node_leafs(std::shared_ptr<xtrie_full_node_t> const & full_node, observer_ptr<xtrie_db_t> const & trie_db, std::vector<xbytes_t> & leafs);
};
using xtrie_simple_iterator_t = xtop_trie_simple_iterator;

//
//class xtop_trie_node_iterator_face {
//public:
//    xtop_trie_node_iterator_face() = default;
//    xtop_trie_node_iterator_face(xtop_trie_node_iterator_face const &) = default;
//    xtop_trie_node_iterator_face & operator=(xtop_trie_node_iterator_face const &) = default;
//    xtop_trie_node_iterator_face(xtop_trie_node_iterator_face &&) = default;
//    xtop_trie_node_iterator_face & operator=(xtop_trie_node_iterator_face &&) = default;
//    virtual ~xtop_trie_node_iterator_face() = default;
//
//    // Next moves the iterator to the next node. If the parameter is false, any child
//    // nodes will be skipped.
//    virtual bool next(bool descend) const = 0;
//
//    // Error returns the error status of the iterator.
//    virtual bool error() const = 0;
//
//    // Hash returns the hash of the current node.
//    virtual xhash256_t hash() const = 0;
//
//    // Parent returns the hash of the parent of the current node. The hash may be the one
//    // grandparent if the immediate parent is an internal node with no hash.
//    virtual xhash256_t parent_hash() const = 0;
//
//    // Path returns the hex-encoded path to the current node.
//    // Callers must not retain references to the return value after calling Next.
//    // For leaf nodes, the last element of the path is the 'terminator symbol' 0x10.
//    virtual xbytes_t path() const = 0;
//
//    // NodeBlob returns the rlp-encoded value of the current iterated node.
//    // If the node is an embedded node in its parent, nil is returned then.
//    virtual xbytes_t node_blob() const = 0;
//
//    // Leaf returns true iff the current node is a leaf node.
//    virtual bool leaf() const noexcept = 0;
//
//    // LeafKey returns the key of the leaf. The method panics if the iterator is not
//    // positioned at a leaf. Callers must not retain references to the value after
//    // calling Next.
//    virtual xbytes_t leaf_key() const = 0;
//
//    // LeafBlob returns the content of the leaf. The method panics if the iterator
//    // is not positioned at a leaf. Callers must not retain references to the value
//    // after calling Next.
//    virtual xbytes_t leaf_blob() const = 0;
//
//    // LeafProof returns the Merkle proof of the leaf. The method panics if the
//    // iterator is not positioned at a leaf. Callers must not retain references
//    // to the value after calling Next.
//    virtual std::vector<xbytes_t> leaf_proof() const = 0;
//
//    // AddResolver sets an intermediate database to use for looking up trie nodes
//    // before reaching into the real persistent layer.
//    //
//    // This is not required for normal operation, rather is an optimization for
//    // cases where trie nodes can be recovered from some external mechanism without
//    // reading from disk. In those cases, this resolver allows short circuiting
//    // accesses and returning them from memory.
//    //
//    // Before adding a similar mechanism to any other place in Geth, consider
//    // making trie.Database an interface and wrapping at that level. It's a huge
//    // refactor, but it could be worth it if another occurrence arises.
//    // virtual void add_resolver()
//};
//using xtrie_node_iterator_face_t = xtop_trie_node_iterator_face;
//
//class xtop_trie_iterator {
//    std::unique_ptr<xtrie_node_iterator_face_t> node_iterator_;
//    xbytes_t key_;
//    xbytes_t value_;
//    std::error_code ec_;
//
//public:
//    xtop_trie_iterator() = default;
//    xtop_trie_iterator(xtop_trie_iterator const &) = delete;
//    xtop_trie_iterator & operator=(xtop_trie_iterator const &) = delete;
//    xtop_trie_iterator(xtop_trie_iterator &&) = default;
//    xtop_trie_iterator & operator=(xtop_trie_iterator &&) = default;
//    ~xtop_trie_iterator() = default;
//
//    explicit xtop_trie_iterator(std::unique_ptr<xtrie_node_iterator_face_t> iter);
//
//    bool next();
//
//    xbytes_t key() const;
//    xbytes_t value() const;
//    std::error_code error_code() const;
//};
//using xtrie_iterator_t = xtop_trie_iterator;
//
//struct xtop_trie_node_iterator_state;
//using xtrie_node_iterator_state_t = xtop_trie_node_iterator_state;
//
//class xtop_trie_node_iterator : public xtrie_node_iterator_face_t {
//    std::shared_ptr<xtrie_t> trie_;                                     // Trie being iterated
//    std::vector<std::unique_ptr<xtrie_node_iterator_state_t>> stack_;   // Hierarchy of trie nodes persisting the iteration state
//    xbytes_t path_;                                                     // Path to the current node
//    std::error_code error_code_;                                        // Failure set in case of an internal error in the iterator
//
//public:
//    xtop_trie_node_iterator(xtop_trie_node_iterator const &) = delete;
//    xtop_trie_node_iterator & operator=(xtop_trie_node_iterator const &) = delete;
//    xtop_trie_node_iterator(xtop_trie_node_iterator &&) = default;
//    xtop_trie_node_iterator & operator=(xtop_trie_node_iterator &&) = default;
//    ~xtop_trie_node_iterator() override = default;
//
//    explicit xtop_trie_node_iterator(std::shared_ptr<xtrie_t> trie);
//
//    bool next(bool descend) const override;
//    bool error() const override;
//    xhash256_t hash() const override;
//    xhash256_t parent_hash() const override;
//    xbytes_t path() const override;
//    xbytes_t node_blob() const override;
//    bool leaf() const noexcept override;
//    xbytes_t leaf_key() const override;
//    xbytes_t leaf_blob() const override;
//    std::vector<xbytes_t> leaf_proof() const override;
//
//    std::shared_ptr<xtrie_node_face_t> resolve_hash(xhash256_t const & hash, std::error_code & ec) const;
//
//private:
//    std::unique_ptr<xtrie_node_iterator_state_t> init(std::error_code & ec);
//    void pop();
//    void seek(xbytes_t const & prefix, std::error_code & ec);
//    std::tuple<std::unique_ptr<xtrie_node_iterator_state_t>, std::unique_ptr<int>, xbytes_t> peek_seek(xbytes_t const & seek_key, std::error_code & ec);
//};
//using xtrie_node_iterator_t = xtop_trie_node_iterator;
//
//// xtrie_node_iterator_state_t represents the iteration state at one particular node of the
//// trie, which can be resumed at a later invocation.
//struct xtop_trie_node_iterator_state {
//    xhash256_t hash;                            // Hash of the node being iterated (nil if not standalone)
//    std::shared_ptr<xtrie_node_face_t> node;    // Trie node being iterated
//    xhash256_t parent_hash;                     // Hash of the first full ancestor node (nil if current is the root)
//    size_t index;                               // Child to be processed next
//    size_t path_length;                         // Length of the path to this node
//
//private:
//    friend xtrie_node_iterator_t;
//    void resolve(observer_ptr<xtrie_node_iterator_t> trie_node_iterator, std::error_code & ec);
//};

NS_END3
