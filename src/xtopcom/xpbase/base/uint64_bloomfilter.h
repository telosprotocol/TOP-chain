//
//  bloomfilter.h
//
//  Created by Charlie Xie on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "common/xxhash.h"
#include "check_cast.h"

namespace top {

namespace base {

class Uint64BloomFilter {
public:
    Uint64BloomFilter(uint32_t bit_num,  uint32_t hash_num);
    Uint64BloomFilter(const std::vector<uint64_t>& data_vec, uint32_t hash_num);
    ~Uint64BloomFilter();

    void Add(const std::string& data);
    void Add(uint32_t bit_index);
    void Add(uint64_t hash);
    void Del(uint64_t hash);
    bool Contain(const std::string& data);
    bool Contain(uint32_t bit_index);
    bool Contain(uint64_t hash);
    void MergeMore(const std::vector<uint64_t>& other_vec);
    const std::vector<uint64_t>& Uint64Vector();
    std::string string() {
        std::string tmp_str;
        for (uint32_t i = 0; i < uint64_data_.size(); ++i) {
            tmp_str += check_cast<std::string>(uint64_data_[i]);
        }
        return tmp_str;
    }

private:
    void GetHash(const std::string& data, uint32_t& high, uint32_t& low);

    static const uint64_t kHashSeed = 1234567890;

    std::vector<uint64_t> uint64_data_;
    uint32_t hash_num_;
};

}  // namespace base

}  // namespace top
