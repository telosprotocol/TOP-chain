#pragma once
#include "xevm_common/xeth/xeth_header.h"
NS_BEG3(top, evm_common, eth)
using namespace top::evm_common;
// diff = (parent_diff +
//         (parent_diff / 2048 * max((2 if len(parent.uncles) else 1) - ((timestamp - parent.timestamp) // 9), -99))
//        ) + 2^(periodCount - 2)
class difficulty {
public:
    static bigint calculate(int64_t time, xeth_block_header_t *header, bigint bomb_height = 9000000);
};
NS_END3