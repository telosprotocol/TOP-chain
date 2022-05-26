// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <memory>

NS_BEG3(top, evm_common, rlp)

class xtop_writer_face {
public:
    xtop_writer_face() = default;
    xtop_writer_face(xtop_writer_face const &) = default;
    xtop_writer_face & operator=(xtop_writer_face const &) = default;
    xtop_writer_face(xtop_writer_face &&) = default;
    xtop_writer_face & operator=(xtop_writer_face &&) = default;
    virtual ~xtop_writer_face() = default;
};

using xwriter_face_t = xtop_writer_face;
using xwriter_face_ptr_t = std::shared_ptr<xwriter_face_t>;

NS_END3