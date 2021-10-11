// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xerror/xerror.h"
#include "xbasic/xbyte_buffer.h"

#include <system_error>
#include <typeinfo>

NS_BEG1(top)

template <typename T>
class xtop_enable_to_bytes {
public:
    xtop_enable_to_bytes() = default;
    xtop_enable_to_bytes(xtop_enable_to_bytes const &) = default;
    xtop_enable_to_bytes & operator=(xtop_enable_to_bytes const &) = default;
    xtop_enable_to_bytes(xtop_enable_to_bytes &&) = default;
    xtop_enable_to_bytes & operator=(xtop_enable_to_bytes &&) = default;
    virtual ~xtop_enable_to_bytes() = default;

    virtual xbytes_t to_bytes() const {
        std::error_code ec;
        auto r = to_bytes(ec);
        top::error::throw_error(ec);
        return r;
    }

    virtual xbytes_t to_bytes(std::error_code & ec) const = 0;

    virtual void from_bytes(xbytes_t const & bytes) {
        std::error_code ec;
        T tmp;
        tmp.from_bytes(ec);
        top::error::throw_error(ec);
        *this = std::move(tmp);
    }

    virtual void from_bytes(xbytes_t const & bytes, std::error_code & ec) = 0;
};
template <typename T>
using xenable_to_bytes_t = xtop_enable_to_bytes<T>;

template <typename T, typename std::enable_if<std::is_base_of<xenable_to_bytes_t<T>, T>::value>::type * = nullptr>
xbytes_t to_bytes(T const & v) {
    return v.to_bytes();
}

template <typename T, typename std::enable_if<std::is_same<xbytes_t, T>::value>::type * = nullptr>
auto to_bytes(T && bytes) -> decltype(std::forward<T>(bytes)){
    return std::forward<T>(bytes);
}

template <typename T, typename std::enable_if<std::is_base_of<xenable_to_bytes_t<T>, T>::value>::type * = nullptr>
xbytes_t to_bytes(T const & v, std::error_code & ec) {
    return v.to_bytes(ec);
}

template <typename T, typename std::enable_if<std::is_same<xbytes_t, T>::value>::type * = nullptr>
auto to_bytes(T && bytes, std::error_code &) -> decltype(std::forward<T>(bytes)) {
    return std::forward<T>(bytes);
}

template <typename T, typename std::enable_if<std::is_base_of<xenable_to_bytes_t<T>, T>::value>::type * = nullptr>
T from_bytes(xbytes_t const & bytes) {
    T ret;
    ret.from_bytes(bytes);
    return ret;
}

template <typename T, typename std::enable_if<std::is_same<xbytes_t, T>::value>::type * = nullptr>
auto from_bytes(T && bytes) -> decltype(std::forward<T>(bytes)) {
    return std::forward<T>(bytes);
}

template <typename T, typename std::enable_if<std::is_base_of<xenable_to_bytes_t<T>, T>::value>::type * = nullptr>
T from_bytes(xbytes_t const & bytes, std::error_code & ec) {
    T ret;
    ret.from_bytes(bytes, ec);
    return ret;
}

template <typename T, typename std::enable_if<std::is_same<xbytes_t, T>::value>::type * = nullptr>
auto from_bytes(T && bytes, std::error_code &) -> decltype(std::forward<T>(bytes)) {
    return std::forward<T>(bytes);
}

NS_END1
