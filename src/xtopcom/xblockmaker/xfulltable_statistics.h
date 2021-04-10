// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xobject_ptr.h"
#include "xcommon/xip.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xdata/xblock.h"
#include "xdata/xblock_statistics_data.h"

#include <cstdint>

namespace top {
namespace blockmaker {

data::xstatistics_data_t tableblock_statistics(std::vector<xobject_ptr_t<data::xblock_t>> const & blks);

}  // namespace data
}  // namespace top
