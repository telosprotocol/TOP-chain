// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <string>

NS_BEG4(top, tools, decoders, details)

struct xtop_vote_data_decoder {
    static void decode(std::string const & input);
};

NS_END4

NS_BEG3(top, tools, decoders)

using xvote_data_decoder_t = details::xtop_vote_data_decoder;

NS_END3
