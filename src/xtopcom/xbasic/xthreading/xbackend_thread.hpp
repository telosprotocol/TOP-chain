// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"

#include <thread>

NS_BEG2(top, threading)

class xbackend_thread final
{
public:
    xbackend_thread(xbackend_thread const &)             = delete;
    xbackend_thread & operator=(xbackend_thread const &) = delete;
    xbackend_thread(xbackend_thread &&)                  = delete;
    xbackend_thread & operator=(xbackend_thread &&)      = delete;
    ~xbackend_thread()                                   = delete;

    template <typename Callable, typename ... Args>
    static
    void
    spawn(Callable && f, Args && ... args) {
        std::thread{ std::forward<Callable>(f), std::forward<Args>(args)... }.detach();
    }
};

NS_END2
