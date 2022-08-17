#pragma once

#include "xdepends/include/ethash/ethash.hpp"
#include "xevm_common/fixed_hash.h"
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
    bigint calc_difficulty(const uint64_t time, const xeth_header_t & parent);
    bool verify_seal(const xeth_header_t & header, const std::vector<double_node_with_merkle_proof> & nodes);

private:
    xethash_t();
    bigint calc_difficulty(const uint64_t time, const xeth_header_t & header, bigint bomb_delay);
    std::pair<::ethash::hash256, ::ethash::hash256> hashimoto_merkle(const ::ethash::hash256 & hash,
                                                                 const uint64_t nonce,
                                                                 const uint64_t header_number,
                                                                 const std::vector<double_node_with_merkle_proof> & nodes) const;
    std::pair<::ethash::hash256, ::ethash::hash256> hashimoto(const ::ethash::hash256 & hash,
                                                          const uint64_t nonce,
                                                          const size_t full_size,
                                                          std::function<::ethash::hash1024(uint64_t)> lookup) const;
    h128 apply_merkle_proof(const uint64_t index, const double_node_with_merkle_proof & node) const;

    std::vector<h128> m_dag_merkle_roots;
};

NS_END3