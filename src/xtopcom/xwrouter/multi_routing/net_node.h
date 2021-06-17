// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xpbase/base/kad_key/kadmlia_key.h"

#include <chrono>
#include <string>
#include <vector>

namespace top {

namespace wrouter {

typedef struct WrouterTableNodes {
    common::xip2_t m_xip2;
    std::string node_id;
} WrouterTableNodes;

}  // namespace wrouter

}  // namespace top
