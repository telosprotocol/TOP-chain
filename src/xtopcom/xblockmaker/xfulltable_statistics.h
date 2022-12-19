// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#    pragma clang diagnostic ignored "-Wsign-compare"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#    pragma GCC diagnostic ignored "-Wsign-compare"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xbase/xobject_ptr.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xdata/xblock.h"
#include "xdata/xblock_statistics_data.h"

#include <vector>

namespace top {
namespace blockmaker {

data::xstatistics_data_t tableblock_statistics(std::vector<xobject_ptr_t<data::xblock_t>> const & blks);

}  // namespace data
}  // namespace top
