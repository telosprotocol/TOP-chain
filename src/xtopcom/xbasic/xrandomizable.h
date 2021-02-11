// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

template <typename T>
struct xtop_randomizable_face
{
    xtop_randomizable_face()                                           = default;
    xtop_randomizable_face(xtop_randomizable_face const &)             = default;
    xtop_randomizable_face & operator=(xtop_randomizable_face const &) = default;
    xtop_randomizable_face(xtop_randomizable_face &&)                  = default;
    xtop_randomizable_face & operator=(xtop_randomizable_face &&)      = default;
    virtual ~xtop_randomizable_face()                                  = default;

    virtual
    void
    random() = 0;
};

template <typename T>
using xrandomizable_t = xtop_randomizable_face<T>;
