// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.#pragma once

#pragma once

#include "xbase/xobject_ptr.h"
#include "xbase/xrefcount.h"

#include <type_traits>

NS_BEG1(top)

template <class T, class U>
class xtop_clonable {
public:
    xtop_clonable() = default;
    xtop_clonable(xtop_clonable const &) = delete;
    xtop_clonable & operator=(xtop_clonable const &) = delete;
    xtop_clonable(xtop_clonable &&) = default;
    xtop_clonable & operator=(xtop_clonable &&) = default;
    virtual ~xtop_clonable() = default;

    virtual U clone() const = 0;
};

template <class T, template <class> class SmartPtrT>
class xtop_clonable<T, SmartPtrT<T>> {
public:
    xtop_clonable() = default;
    xtop_clonable(xtop_clonable const &) = delete;
    xtop_clonable & operator=(xtop_clonable const &) = delete;
    xtop_clonable(xtop_clonable &&) = default;
    xtop_clonable & operator=(xtop_clonable &&) = default;
    virtual ~xtop_clonable() = default;

    virtual SmartPtrT<T> clone() const = 0;
};

//template <typename T, typename std::enable_if<!std::is_base_of<base::xrefcount_t, T>::value>::type * = nullptr>
//class xtop_clonable {
//public:
//    xtop_clonable() = default;
//    xtop_clonable(xtop_clonable const &) = delete;
//    xtop_clonable & operator=(xtop_clonable const &) = delete;
//    xtop_clonable(xtop_clonable &&) = default;
//    xtop_clonable & operator=(xtop_clonable &&) = default;
//    virtual ~xtop_clonable() = default;
//
//    virtual T clone() const = 0;
//};

template <class T, template <class> class SmartPtrT = xobject_ptr_t>
using xclonable_t = xtop_clonable<T, SmartPtrT<T>>;

NS_END1
