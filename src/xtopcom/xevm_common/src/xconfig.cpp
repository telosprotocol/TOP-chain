// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/xcrosschain/xbsc/xconfig.h"

NS_BEG4(top, evm, crosschain, bsc)

xchain_config_t const bsc_chain_config{
    56,  // chain id

    0,  // homestead block

    0,      // dao fork block
    false,  // dao fork support

    0,   // eip150 block
    {},  // eip150 hash

    0,  // eip155 block
    0,  // eip158 block

    0,         // byzantium block
    0,         // constantinople block
    0,         // petersburg block
    0,         // istanbul block
    0,         // muir_glacier block
    31302048,  // berlin block
    0,         // yolo_v3 block
    0,         // catalyst block
    31302048,  // london block
    0,         // arrow_glacier block
    0,         // merge_fork block

    0,  // terminal_total_difficulty

    0,         // ramanujan block
    0,         // niels block
    5184000,   // mirror_sync block
    13082000,  // bruno block
    18907621,  // euler block
    21962149,  // nano block
    22107423,  // moran block
    23846001,  // gibbs block
    27281024,  // planck block

    29020050,  // luban block
    30720096,  // plato block
    31302048,  // hertz block

    {3, 200},  // parlia config
};


NS_END4
