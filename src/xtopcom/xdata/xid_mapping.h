// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"

#include <limits>
#include <type_traits>
#include <vector>

NS_BEG2(top, data)

template <typename IdT>
std::vector<IdT>
filter_ids(std::vector<IdT> const & input, std::size_t const id_offset, std::size_t const count) {
    XSTATIC_ASSERT(std::is_integral<IdT>::value);
    assert(input.size() <= std::numeric_limits<IdT>::max());
    assert(id_offset < count);

    auto const size_per_count = input.size() / count;

    auto const begin_id = id_offset * size_per_count;
    auto end_id = begin_id + size_per_count;
    if (end_id > input.size()) {
        end_id = input.size();
    }

    std::vector<IdT> ids;
    ids.reserve(end_id - begin_id);
    for (auto i = begin_id; i < end_id; ++i) {
        ids.push_back(input[i]);
    }

    return ids;
}

inline
std::size_t
book_id_mapping_offset(std::uint8_t const book_id, std::size_t count) {
    return book_id % count;
}

NS_END2
