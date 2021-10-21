// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvtxstore.h"

NS_BEG2(top, txstore)

// base::xvtxstore_t * get_txstore();
base::xvtxstore_t * create_txstore();

NS_END2
