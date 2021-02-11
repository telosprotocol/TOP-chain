// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcrypto_key.h"
#include "xbasic/xrunnable.h"
#include "xcommon/xnode_id.h"

NS_BEG2(top, application)

template <typename T>
class xtop_application_face : public xtrival_runnable_t<xtop_application_face<T>> {
public:
    xtop_application_face()                                          = default;
    xtop_application_face(xtop_application_face const &)             = delete;
    xtop_application_face & operator=(xtop_application_face const &) = delete;
    xtop_application_face(xtop_application_face &&)                  = default;
    xtop_application_face & operator=(xtop_application_face &&)      = default;
    virtual ~xtop_application_face()                                 = default;
};

template <typename T>
using xapplication_face_t = xtop_application_face<T>;

NS_END2
