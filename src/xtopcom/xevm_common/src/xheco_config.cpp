#include "xevm_common/xcrosschain/xheco_config.h"

NS_BEG4(top, evm_common, heco, config)

#if defined (XBUILD_DEV) || defined(XBUILD_CI) || defined(XBUILD_BOUNTY) || defined(XBUILD_GALILEO)
constexpr uint64_t LondonBlock = 0;
#else
constexpr uint64_t LondonBlock = 8577000;
#endif

bool is_london(const bigint num) {
    return (num >= LondonBlock);
}

NS_END4
