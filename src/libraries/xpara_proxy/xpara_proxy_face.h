// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcrypto_key.h"
#include "xbasic/xrunnable.h"
#include "xcommon/xnode_id.h"

NS_BEG2(top, para_proxy)

template <typename T>
class xtop_para_proxy_face : public xtrival_runnable_t<xtop_para_proxy_face<T>> {
public:
    xtop_para_proxy_face()                                          = default;
    xtop_para_proxy_face(xtop_para_proxy_face const &)             = delete;
    xtop_para_proxy_face & operator=(xtop_para_proxy_face const &) = delete;
    xtop_para_proxy_face(xtop_para_proxy_face &&)                  = default;
    xtop_para_proxy_face & operator=(xtop_para_proxy_face &&)      = default;
    virtual ~xtop_para_proxy_face()                                 = default;
};

template <typename T>
using xpara_proxy_face_t = xtop_para_proxy_face<T>;

NS_END2
