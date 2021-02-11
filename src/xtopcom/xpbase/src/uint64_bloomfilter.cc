//
//  uint64_bloomfilter.cc
//
//  Created by Charlie Xie on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include "xpbase/base/uint64_bloomfilter.h"

#include <cassert>

namespace top {

namespace base {

    //(214013*rand_seed+2531011) is fast random algorithm
    uint64_t  bloom_math_hash(const void * src_data_ptr,const int32_t src_data_len,const int32_t hash_seed)
    {
        uint64_t  hash_result = hash_seed;
        uint8_t * raw_data_ptr = (uint8_t *)src_data_ptr;
        int32_t   left_bytes   = src_data_len;
        
        uint64_t* raw_data_8bytes = (uint64_t*)raw_data_ptr;
        const int n8bytes_count   = left_bytes >> 3;
        for(int i = 0; i < n8bytes_count; ++i)
        {
            hash_result += (214013 * hash_seed * raw_data_8bytes[i] + 2531011);
        }
        raw_data_ptr += (n8bytes_count << 3);
        left_bytes   -= (n8bytes_count << 3);
        if(left_bytes > 4)//must be 0,1,2,3,4,5,6,7
        {
            uint32_t* raw_data_4bytes = (uint32_t*)raw_data_ptr;
            hash_result += (214013 * hash_seed * raw_data_4bytes[0] + 2531011);
            
            raw_data_ptr += 4;
            left_bytes   -= 4;
        }
        for(int i = 0; i < left_bytes; ++i)
        {
            hash_result += (214013 * hash_seed * raw_data_ptr[i] + 2531011);
        }
        return hash_result;
    }
    
    uint64_t  bloom_simple_hash(const void * src_data_ptr,const int32_t src_data_len,const int32_t hash_seed)
    {
        uint64_t  hash_result = hash_seed;
        uint8_t * raw_data_ptr = (uint8_t *)src_data_ptr;
        int32_t   left_bytes   = src_data_len;
        
        uint64_t* raw_data_8bytes = (uint64_t*)raw_data_ptr;
        const int n8bytes_count   = left_bytes >> 3;
        for(int i = 0; i < n8bytes_count; ++i)
        {
            hash_result = hash_seed * hash_result + raw_data_8bytes[i];
        }
        raw_data_ptr += (n8bytes_count << 3);
        left_bytes   -= (n8bytes_count << 3);
        if(left_bytes > 4)//must be 0,1,2,3,4,5,6,7
        {
            uint32_t* raw_data_4bytes = (uint32_t*)raw_data_ptr;
            hash_result = hash_seed * hash_result + raw_data_4bytes[0];
            
            raw_data_ptr += 4;
            left_bytes   -= 4;
        }
        for(int i = 0; i < left_bytes; ++i)
        {
            hash_result = hash_seed * hash_result + raw_data_ptr[0];
        }
        return hash_result;
    }
    
    uint64_t  bloom_revert_simple_hash(const void * src_data_ptr,const int32_t src_data_len,const int32_t hash_seed)
    {
        uint64_t  hash_result = hash_seed;
        uint8_t * raw_data_ptr = (uint8_t *)src_data_ptr;
        int32_t   left_bytes   = src_data_len;
        
        uint64_t* raw_data_8bytes = (uint64_t*)raw_data_ptr;
        const int n8bytes_count   = left_bytes >> 3;
        for(int i = 0; i < n8bytes_count; ++i)
        {
            hash_result = hash_seed * hash_result + (~raw_data_8bytes[i]);
        }
        raw_data_ptr += (n8bytes_count << 3);
        left_bytes   -= (n8bytes_count << 3);
        if(left_bytes > 4)//must be 0,1,2,3,4,5,6,7
        {
            uint32_t* raw_data_4bytes = (uint32_t*)raw_data_ptr;
            hash_result = hash_seed * hash_result + (~raw_data_4bytes[0]);
            
            raw_data_ptr += 4;
            left_bytes   -= 4;
        }
        for(int i = 0; i < left_bytes; ++i)
        {
            hash_result = hash_seed * hash_result + (~raw_data_ptr[0]);
        }
        return hash_result;
    }
    
    uint64_t  bloom_xor_simple_hash(const void * src_data_ptr,const int32_t src_data_len,const int32_t hash_seed)
    {
        uint64_t  hash_result = hash_seed;
        uint8_t * raw_data_ptr = (uint8_t *)src_data_ptr;
        int32_t   left_bytes   = src_data_len;
        
        const uint64_t x_or_key = (src_data_len * hash_seed) * (src_data_len + hash_seed);
        
        uint64_t* raw_data_8bytes = (uint64_t*)raw_data_ptr;
        const int n8bytes_count   = left_bytes >> 3;
        for(int i = 0; i < n8bytes_count; ++i)
        {
            hash_result = hash_seed * hash_result + (raw_data_8bytes[i] ^ x_or_key);
        }
        raw_data_ptr += (n8bytes_count << 3);
        left_bytes   -= (n8bytes_count << 3);
        if(left_bytes > 4)//must be 0,1,2,3,4,5,6,7
        {
            uint32_t* raw_data_4bytes = (uint32_t*)raw_data_ptr;
            hash_result = hash_seed * hash_result + (raw_data_4bytes[0] ^ (uint32_t)x_or_key);
            
            raw_data_ptr += 4;
            left_bytes   -= 4;
        }
        for(int i = 0; i < left_bytes; ++i)
        {
            hash_result = hash_seed * hash_result + (raw_data_ptr[0] ^ (uint8_t)x_or_key);
        }
        return hash_result;
        
    }
 
    uint64_t  bloom_xx64_hash(const void * src_data_ptr,const int32_t src_data_len,const int32_t hash_seed)
    {
        return  XXH64(src_data_ptr, src_data_len, hash_seed);
    }
    
    typedef uint64_t (*bloom_xx64_hash_function_ptr)(const void * src_data_ptr, const int32_t src_data_len, const int32_t hash_seed);
    
    bloom_xx64_hash_function_ptr global_hash_functions_table[] = {
        bloom_math_hash,
        bloom_simple_hash,
        bloom_revert_simple_hash,
        bloom_xor_simple_hash,
        bloom_xx64_hash
    };
    const int global_hash_functions_table_size = sizeof(global_hash_functions_table) / sizeof(bloom_xx64_hash_function_ptr);
    
    uint8_t  global_hash_seeds_table[] = {
        3,5,7, 11, 13, 31, 37, 61
    };
    
Uint64BloomFilter::Uint64BloomFilter(
        uint32_t bit_num,
        uint32_t hash_num) : uint64_data_(), hash_num_(hash_num) {
    assert((bit_num % 64) == 0);
    for (uint32_t i = 0; i < (bit_num / 64); ++i) {
        uint64_data_.push_back(0ull);
    }
    assert(!uint64_data_.empty());
    if (hash_num_ > 4)
		hash_num_ = 4;
    
}

Uint64BloomFilter::Uint64BloomFilter(const std::vector<uint64_t>& data_vec, uint32_t hash_num)
        : uint64_data_(data_vec), hash_num_(hash_num) {
    assert(!uint64_data_.empty());
	if (hash_num_ > 4)
		hash_num_ = 4;
}

Uint64BloomFilter::~Uint64BloomFilter() {}

void Uint64BloomFilter::Add(uint64_t hash) {
//    uint32_t hash_high = static_cast<uint32_t>(hash >> 32 & 0x00000000FFFFFFFF);
//    uint32_t hash_low = static_cast<uint32_t>(hash & 0x00000000FFFFFFFF);
    for (uint32_t i = 0; i < hash_num_; ++i) {
//        uint32_t index = (hash_high + i * hash_low);
		char* pos = (char*)&hash;
        uint16_t index = *(uint16_t*)(pos + sizeof(uint16_t)*i);
        uint32_t vec_index = (index % (64 * uint64_data_.size())) / 64;
        uint32_t bit_index = (index % (64 * uint64_data_.size())) % 64;
        uint64_data_[vec_index] |= (uint64_t)((uint64_t)(1) << bit_index);
    }
}

void Uint64BloomFilter::Del(uint64_t hash) {
    for (uint32_t i = 0; i < hash_num_; ++i) {
		char* pos = (char*)&hash;
        uint16_t index = *(uint16_t*)(pos + sizeof(uint16_t)*i);
        uint32_t vec_index = (index % (64 * uint64_data_.size())) / 64;
        uint32_t bit_index = (index % (64 * uint64_data_.size())) % 64;
        uint64_data_[vec_index] &= ~(uint64_t)((uint64_t)(1) << bit_index);
    }
}

bool Uint64BloomFilter::Contain(uint64_t hash) {
//    uint32_t hash_high = static_cast<uint32_t>(hash >> 32 & 0x00000000FFFFFFFF);
//    uint32_t hash_low = static_cast<uint32_t>(hash & 0x00000000FFFFFFFF);
    for (uint32_t i = 0; i < hash_num_; ++i) {
//        uint32_t index = (hash_high + i * hash_low);
		char* pos = (char*)&hash;
        uint16_t index = *(uint16_t*)(pos + sizeof(uint16_t)*i);
        uint32_t vec_index = (index % (64 * uint64_data_.size())) / 64;
        uint32_t bit_index = (index % (64 * uint64_data_.size())) % 64;
        if ((uint64_data_[vec_index] & ((uint64_t)((uint64_t)(1) << bit_index))) == 0ull) {
            return false;
        }
    }
    return true;
}

//
void Uint64BloomFilter::Add(const std::string& data) {
    /*
    uint32_t hash_high = 0;
    uint32_t hash_low = 0;
    GetHash(data, hash_high, hash_low);
    for (uint32_t i = 0; i < hash_num_; ++i) {
        uint32_t index = (hash_high + i * hash_low);
        uint32_t vec_index = (index % (64 * uint64_data_.size())) / 64;
        uint32_t bit_index = (index % (64 * uint64_data_.size())) % 64;
        uint64_data_[vec_index] |= (uint64_t)((uint64_t)(1) << bit_index);
    }
    */
    #ifdef __USING_HASH_AS_SROUCE_FOR_BLOOMFILTER__
    const uint64_t input_data_hash =  XXH64(data.data(),data.size(), kHashSeed);
    std::string  input_source((const char*)&input_data_hash,sizeof(input_data_hash));
    #else
    const  std::string& input_source = data;
    #endif
    const int total_bits_count = 64 * uint64_data_.size();//TODO,should be a member of class
    if(2 == hash_num_) //let cpu parallel process
    {
        bloom_xx64_hash_function_ptr _hash_function_0 = global_hash_functions_table[0];
        const uint64_t hash_value_0 = (*_hash_function_0)(input_source.data(),input_source.size(),global_hash_seeds_table[0]);
        const uint32_t vec_index_0 = (hash_value_0 % total_bits_count) >> 6;
        const uint32_t bit_index_0 = (hash_value_0 % total_bits_count) % 64;
        uint64_data_[vec_index_0] |= (uint64_t)((uint64_t)(1) << bit_index_0);
        
        bloom_xx64_hash_function_ptr _hash_function_1 = global_hash_functions_table[1];
        const uint64_t hash_value_1 = (*_hash_function_1)(input_source.data(),input_source.size(),global_hash_seeds_table[1]);
        const uint32_t vec_index_1 = (hash_value_1 % total_bits_count) >> 6;
        const uint32_t bit_index_1 = (hash_value_1 % total_bits_count) % 64;
        uint64_data_[vec_index_1] |= (uint64_t)((uint64_t)(1) << bit_index_1);
    }
    else if(3 == hash_num_) //let cpu parallel process
    {
        bloom_xx64_hash_function_ptr _hash_function_0 = global_hash_functions_table[0];
        const uint64_t hash_value_0 = (*_hash_function_0)(input_source.data(),input_source.size(),global_hash_seeds_table[0]);
        const uint32_t vec_index_0 = (hash_value_0 % total_bits_count) >> 6;
        const uint32_t bit_index_0 = (hash_value_0 % total_bits_count) % 64;
        uint64_data_[vec_index_0] |= (uint64_t)((uint64_t)(1) << bit_index_0);
        
        bloom_xx64_hash_function_ptr _hash_function_1 = global_hash_functions_table[1];
        const uint64_t hash_value_1 = (*_hash_function_1)(input_source.data(),input_source.size(),global_hash_seeds_table[1]);
        const uint32_t vec_index_1 = (hash_value_1 % total_bits_count) >> 6;
        const uint32_t bit_index_1 = (hash_value_1 % total_bits_count) % 64;
        uint64_data_[vec_index_1] |= (uint64_t)((uint64_t)(1) << bit_index_1);
        
        bloom_xx64_hash_function_ptr _hash_function_2 = global_hash_functions_table[2];
        const uint64_t hash_value_2 = (*_hash_function_2)(input_source.data(),input_source.size(),global_hash_seeds_table[2]);
        const uint32_t vec_index_2 = (hash_value_2 % total_bits_count) >> 6;
        const uint32_t bit_index_2 = (hash_value_2 % total_bits_count) % 64;
        uint64_data_[vec_index_2] |= (uint64_t)((uint64_t)(1) << bit_index_2);
    }
    else
    {
        for(uint32_t i = 0; i < hash_num_; ++i)
        {
            bloom_xx64_hash_function_ptr _hash_function = global_hash_functions_table[i % global_hash_functions_table_size];
            const uint64_t hash_value = (*_hash_function)(input_source.data(),input_source.size(),i % sizeof(global_hash_seeds_table));
            
            const uint32_t vec_index = (hash_value % total_bits_count) >> 6;
            const uint32_t bit_index = (hash_value % total_bits_count) % 64;
            uint64_data_[vec_index] |= (uint64_t)((uint64_t)(1) << bit_index);
        }
    }
   
    
    /*
    uint32_t hash_high;
    uint32_t hash_low;
    std::string hdata1;
    std::string hash_data = data;
    for (uint32_t i = 0; i < hash_num_; ++i) {
        hash_high = 0;
        hash_low = 0;
        hdata1 = hash_data.substr(0, hash_data.size() / hash_num_);
        hash_data = hash_data.substr(hash_data.size() / hash_num_) + hdata1;
        GetHash(hash_data, hash_high, hash_low);
        uint32_t index = (hash_high + i * hash_low);
        uint32_t vec_index = (index % (64 * uint64_data_.size())) / 64;
        uint32_t bit_index = (index % (64 * uint64_data_.size())) % 64;
        uint64_data_[vec_index] |= (uint64_t)((uint64_t)(1) << bit_index);
    }
     */

}

void Uint64BloomFilter::Add(uint32_t index) {
    assert(index < (uint64_data_.size() * sizeof(uint64_t)));
    uint32_t vec_index = (index % (64 * uint64_data_.size())) / 64;
    uint32_t bit_index = (index % (64 * uint64_data_.size())) % 64;
    uint64_data_[vec_index] |= (uint64_t)((uint64_t)(1) << bit_index);
}

bool Uint64BloomFilter::Contain(uint32_t index) {
    assert(index < (uint64_data_.size() * sizeof(uint64_t)));
    uint32_t vec_index = (index % (64 * uint64_data_.size())) / 64;
    uint32_t bit_index = (index % (64 * uint64_data_.size())) % 64;
    if ((uint64_data_[vec_index] & ((uint64_t)((uint64_t)(1) << bit_index))) == 0ull) {
        return false;
    }
    return true;
}

bool Uint64BloomFilter::Contain(const std::string& data) {
    
    
#ifdef __USING_HASH_AS_SROUCE_FOR_BLOOMFILTER__
    const uint64_t input_data_hash =  XXH64(data.data(),data.size(), kHashSeed);
    std::string  input_source((const char*)&input_data_hash,sizeof(input_data_hash));
#else
    const std::string& input_source = data;
#endif
    const int total_bits_count = 64 * uint64_data_.size();//TODO,should be a member of class
    if(2 == hash_num_) //let cpu parallel process
    {
        bloom_xx64_hash_function_ptr _hash_function_0 = global_hash_functions_table[0];
        const uint64_t hash_value_0 = (*_hash_function_0)(input_source.data(),input_source.size(),global_hash_seeds_table[0]);
        const uint32_t vec_index_0 = (hash_value_0 % total_bits_count) >> 6;
        const uint32_t bit_index_0 = (hash_value_0 % total_bits_count) % 64;
        if ((uint64_data_[vec_index_0] & ((uint64_t)((uint64_t)(1) << bit_index_0))) == 0ull) 
            return false;
        
        bloom_xx64_hash_function_ptr _hash_function_1 = global_hash_functions_table[1];
        const uint64_t hash_value_1 = (*_hash_function_1)(input_source.data(),input_source.size(),global_hash_seeds_table[1]);
        const uint32_t vec_index_1 = (hash_value_1 % total_bits_count) >> 6;
        const uint32_t bit_index_1 = (hash_value_1 % total_bits_count) % 64;
        if ((uint64_data_[vec_index_1] & ((uint64_t)((uint64_t)(1) << bit_index_1))) == 0ull) 
            return false;
    }
    else if(3 == hash_num_) //let cpu parallel process
    {
        bloom_xx64_hash_function_ptr _hash_function_0 = global_hash_functions_table[0];
        const uint64_t hash_value_0 = (*_hash_function_0)(input_source.data(),input_source.size(),global_hash_seeds_table[0]);
        const uint32_t vec_index_0 = (hash_value_0 % total_bits_count) >> 6;
        const uint32_t bit_index_0 = (hash_value_0 % total_bits_count) % 64;
        if ((uint64_data_[vec_index_0] & ((uint64_t)((uint64_t)(1) << bit_index_0))) == 0ull) 
            return false;
        
        bloom_xx64_hash_function_ptr _hash_function_1 = global_hash_functions_table[1];
        const uint64_t hash_value_1 = (*_hash_function_1)(input_source.data(),input_source.size(),global_hash_seeds_table[1]);
        const uint32_t vec_index_1 = (hash_value_1 % total_bits_count) >> 6;
        const uint32_t bit_index_1 = (hash_value_1 % total_bits_count) % 64;
        if ((uint64_data_[vec_index_1] & ((uint64_t)((uint64_t)(1) << bit_index_1))) == 0ull) 
            return false;
        
        bloom_xx64_hash_function_ptr _hash_function_2 = global_hash_functions_table[2];
        const uint64_t hash_value_2 = (*_hash_function_2)(input_source.data(),input_source.size(),global_hash_seeds_table[2]);
        const uint32_t vec_index_2 = (hash_value_2 % total_bits_count) >> 6;
        const uint32_t bit_index_2 = (hash_value_2 % total_bits_count) % 64;
        if ((uint64_data_[vec_index_2] & ((uint64_t)((uint64_t)(1) << bit_index_2))) == 0ull) 
            return false;
    }
    else
    {
        for(uint32_t i = 0; i < hash_num_; ++i)
        {
            bloom_xx64_hash_function_ptr _hash_function = global_hash_functions_table[i % global_hash_functions_table_size];
            const uint64_t hash_value = (*_hash_function)(input_source.data(),input_source.size(),i % sizeof(global_hash_seeds_table));
            
            const uint32_t vec_index = (hash_value % total_bits_count) >> 6;
            const uint32_t bit_index = (hash_value % total_bits_count) % 64;
            if ((uint64_data_[vec_index] & ((uint64_t)((uint64_t)(1) << bit_index))) == 0ull) 
                return false;
        }
    }


    /*
    uint32_t hash_high;
    uint32_t hash_low;
    std::string hdata1;
    std::string hash_data = data;
    for (uint32_t i = 0; i < hash_num_; ++i) {
        hash_high = 0;
        hash_low = 0;
        hdata1 = hash_data.substr(0, hash_data.size() / hash_num_);
        hash_data = hash_data.substr(hash_data.size() / hash_num_) + hdata1;
        GetHash(hash_data, hash_high, hash_low);
        uint32_t index = (hash_high + i * hash_low);
        uint32_t vec_index = (index % (64 * uint64_data_.size())) / 64;
        uint32_t bit_index = (index % (64 * uint64_data_.size())) % 64;
        if ((uint64_data_[vec_index] & ((uint64_t)((uint64_t)(1) << bit_index))) == 0ull) {
            return false;
        }
    }
    */
    return true;

}

void Uint64BloomFilter::MergeMore(const std::vector<uint64_t>& other_vec) {
    for (uint32_t i = 0; i < uint64_data_.size(); ++i) {
        uint64_data_[i] |= other_vec[i];
    }
}

void Uint64BloomFilter::GetHash(const std::string& data, uint32_t& high, uint32_t& low) {
    uint64_t hash_value = XXH64(data.c_str(), data.size(), kHashSeed);
    high = static_cast<uint32_t>(hash_value >> 32 & 0x00000000FFFFFFFF);
    low = static_cast<uint32_t>(hash_value & 0x00000000FFFFFFFF);
}

const std::vector<uint64_t>& Uint64BloomFilter::Uint64Vector() {
    return uint64_data_;
}

}  // namespace base

}  // namespace top
