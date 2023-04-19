// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xip.h"

#include <string>

namespace top {
namespace wrouter {

typedef struct {
    common::xip2_t m_xip2;
    std::string node_id;
} WrouterTableNode;

}  // namespace wrouter

}  // namespace top
