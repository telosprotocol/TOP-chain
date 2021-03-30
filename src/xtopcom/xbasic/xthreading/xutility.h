// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"

#include <mutex>
#include <type_traits>

NS_BEG2(top, threading)

template <typename MutexT>
struct xtop_lock_guard_helper
{
    std::unique_lock<MutexT> lock;
    bool b;
};

extern
template
struct xtop_lock_guard_helper<std::mutex>;

NS_END2

#if defined XLOCK_GUARD
# error XLOCK_GUARD already defined
#endif

#define XLOCK_GUARD(MUTEX)                                                                                                                                                                                                                                                                  \
    for (::top::threading::xtop_lock_guard_helper<typename ::std::remove_cv<typename ::std::remove_reference<decltype(MUTEX)>::type>::type> MUTEX ## _lock{ ::std::unique_lock<typename ::std::remove_cv<typename ::std::remove_reference<decltype(MUTEX)>::type>::type>{MUTEX}, true };    \
         MUTEX ## _lock.b;                                                                                                                                                                                                                                                                  \
         MUTEX ## _lock.b = false)

#if defined XLOCK
# error XLOCK already defined
#endif

#define XLOCK(MUTEX)    \
    std::lock_guard<typename std::remove_cv<typename std::remove_reference<decltype(MUTEX)>::type>::type> MUTEX ## _lock{ MUTEX }

#define XSHARED_LOCK(MUTEX) \
    std::shared_lock<typename std::remove_cv<typename std::remove_reference<decltype(MUTEX)>::type>::type> MUTEX ## _lock{ MUTEX }
