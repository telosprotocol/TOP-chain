#include "difficulty.h"
NS_BEG4(top, xvm, system_contracts, xeth)
// diff = (parent_diff +
//         (parent_diff / 2048 * max((2 if len(parent.uncles) else 1) - ((timestamp - parent.timestamp) // 9), -99))
//        ) + 2^(periodCount - 2)
bigint difficulty::calculate(int64_t time, xeth_block_header_t *header, bigint bomb_height) {
    // base difficult
    bigint target = bigint(header->difficulty());

    // adjust difficult
    bigint current_time =  bigint(time);
    bigint parent_time = bigint(header->time());
    bigint x = current_time - parent_time;
    x = x / 9;
    if (header->uncle_hash().asBytes().size() == 0) {
        x = 1 - x;
    } else {
        x = 2 - x;
    }

    if (x < -99) {
        x = -99;
    }

    target = target + target / 2048 * x;

    if (target <  131072) {
        target =  131072;
    }

    // bomb difficult
    bigint numbers;
    if (header->number() > bomb_height) {
        numbers = header->number() - bomb_height;
    }

    bigint periodCount = numbers / 100000;
    if (periodCount > 1) {
        target += (bigint(2) << unsigned(periodCount - 2));
    }
    return target;
}

NS_END4