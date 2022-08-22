#include "xevm_common/xcrosschain/xeth_config.h"

NS_BEG4(top, evm_common, eth, config)

#if defined (XBUILD_DEV) || defined(XBUILD_CI) || defined(XBUILD_BOUNTY) || defined(XBUILD_GALILEO)
constexpr uint64_t LondonBlock = 0;
#else
constexpr uint64_t LondonBlock = 12965000;
#endif
constexpr uint64_t ArrowGlacierBlock = 13773000;

bool is_london(const bigint num) {
    return (num >= LondonBlock);
}

bool is_arrow_glacier(const bigint num) {
    return (num >= ArrowGlacierBlock);
}

NS_END4
