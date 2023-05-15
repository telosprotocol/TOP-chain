#pragma once

#include "ethash/ethash.hpp"
#include "xbasic/xfixed_hash.h"
#include "xevm_common/xcrosschain/xeth_header.h"

NS_BEG3(top, evm_common, eth)

class double_node_with_merkle_proof {
public:
    std::vector<h512> dag_nodes;
    std::vector<h128> proof;
};

class xethash_t {
public:
    xethash_t(xethash_t const &) = delete;
    xethash_t & operator=(xethash_t const &) = delete;
    xethash_t(xethash_t &&) = delete;
    xethash_t & operator=(xethash_t &&) = delete;
    ~xethash_t() = default;

    static xethash_t & instance();
    bigint calc_difficulty(uint64_t time, xeth_header_t const & parent);
    bool verify_seal(xeth_header_t const & header, std::vector<double_node_with_merkle_proof> const & nodes);

private:
    xethash_t();
    bigint calc_difficulty(uint64_t time, xeth_header_t const & header, bigint bomb_delay);
    std::pair<::ethash::hash256, ::ethash::hash256> hashimoto_merkle(::ethash::hash256 const & hash,
                                                                     xh64_t nonce,
                                                                     uint64_t header_number,
                                                                     std::vector<double_node_with_merkle_proof> const & nodes) const;
    std::pair<::ethash::hash256, ::ethash::hash256> hashimoto(::ethash::hash256 const & hash,
                                                              xh64_t nonce,
                                                              size_t full_size,
                                                              std::function<::ethash::hash1024(uint64_t)> lookup) const;
    h128 apply_merkle_proof(uint64_t index, double_node_with_merkle_proof const & node) const;

    std::vector<h128> m_dag_merkle_roots;
};

NS_END3
