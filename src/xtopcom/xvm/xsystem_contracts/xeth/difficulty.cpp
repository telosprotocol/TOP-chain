#include "difficulty.h"
#include "util.h"
NS_BEG4(top, xvm, system_contracts, xeth)
// diff = (parent_diff +
//         (parent_diff / 2048 * max((2 if len(parent.uncles) else 1) - ((timestamp - parent.timestamp) // 9), -99))
//        ) + 2^(periodCount - 2)
bigint difficulty::calculate(int64_t time, xeth_block_header_t *header, bigint bomb_height) {
    // base difficult
    bigint target = header->difficulty();

    // adjust difficult
    bigint current_time =  bigint(time);
    bigint parent_time = bigint(header->time());
    bigint timestampDiff = current_time - parent_time;
    bigint adjFactor;
    h256 hash = util::zeroHash();
    if (header->uncle_hash() == hash) {
        adjFactor = 1 - timestampDiff / 9;
    } else {
        adjFactor = 2 - timestampDiff / 9;
    }

    if (adjFactor < -99) {
        adjFactor = -99;
    }

    target = target + target / 2048 * adjFactor;

    if (target <  131072) {
        target =  131072;
    }

    // bomb difficult
    bigint numbers = 0;
    if ((header->number() + 1) >= bomb_height) {
        numbers = header->number() + 1 - bomb_height;
    }

    bigint periodCount = numbers / 100000;
    if (periodCount > 1) {
        target += (bigint(1) << unsigned(periodCount - 2));
    }
    return target;
}

NS_END4