#include "xevm_common/xcrosschain/xeth_config.h"

NS_BEG4(top, evm_common, eth, config)

#if defined (XBUILD_DEV) || defined(XBUILD_CI) || defined(XBUILD_BOUNTY) || defined(XBUILD_GALILEO)
constexpr uint64_t london_block_number = 0;
#else
constexpr uint64_t london_block_number = 12965000;
#endif
constexpr uint64_t arrow_glacier_block_number = 13773000;

bool is_london(uint64_t const num) noexcept {
    return num >= london_block_number;
}

bool is_arrow_glacier(uint64_t const num) noexcept {
    return num >= arrow_glacier_block_number;
}

NS_END4
