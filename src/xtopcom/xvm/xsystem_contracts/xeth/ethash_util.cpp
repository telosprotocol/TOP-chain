#include "ethash_util.h"
#include "util.h"
#include <boost/numeric/conversion/cast.hpp>
NS_BEG4(top, xvm, system_contracts, xeth)

bool ethash_util::verify(xeth_block_header_t *header) {
    const ethash::epoch_context_ptr context = ethash::create_epoch_context(epoch(header->number()));
    const ethash::hash256 headerHash = toHash256(header->hash());
    const ethash::hash256 mixHash = toHash256(header->mixDigest());
    const ethash::hash256 difficulty = toHash256((u256)header->difficulty());
    //
    auto ret = ethash::verify_against_difficulty(*context, headerHash, mixHash, util::bytes_to_uint64(header->nonce().asArray()), difficulty);
    if (ret != ETHASH_SUCCESS) {
        return false;
    }

    return true;
}

int32_t ethash_util::epoch(uint64_t height) {
    return (int32_t)(height / block_number_of_per_epoch);
}

ethash::hash256 ethash_util::toHash256(h256 hash) {
    ethash::hash256 ehash = {};
    std::memcpy(ehash.bytes, hash.data(), 32);
    return ehash;
}

ethash::hash256 ethash_util::toHash256(u256 hash) {
    ethash::hash256 ehash = {};
    auto v = util::fromU256(hash);
    std::memcpy(ehash.bytes, &v[0], v.size());
    return ehash;
}

// uint64_t ethash_util::datasetSize(uint64_t height) {
//     int32_t epoch = (int32_t)(height / block_number_of_per_epoch);
//     if (epoch < max_epoch) {
//         return m_dataset_sizes[epoch];
//     }
//     return calcDatasetSize(epoch);
// }

// uint64_t ethash_util::calcDatasetSize(int32_t epoch ){
//     uint64_t size = dataset_init_bytes + dataset_growth_bytes*(uint64_t)epoch - mix_bytes;
//     uint64_t tmp = size / mix_bytes;
//     // while (!tmp) {
//     //     size -= 2 * mix_bytes;
//     // }
//     return size;
// }

// uint64_t ethash_util::cacheSize(uint64_t height){
//     int32_t epoch = (int32_t)(height / block_number_of_per_epoch);
//     if (epoch < max_epoch) {
//         return m_cache_sizes[epoch];
//     }
//     return calcCacheSize(epoch);
// }

// uint64_t ethash_util::calcCacheSize(int32_t epoch){
//     uint64_t size = dataset_init_bytes + dataset_growth_bytes*(uint64_t)epoch - hash_bytes;
//     uint64_t tmp = size / hash_bytes;
//     // while (!tmp) {
//     //     size -= 2 * mix_bytes;
//     // }
//     return size;
// }
NS_END4