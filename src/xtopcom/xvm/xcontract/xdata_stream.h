// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xmem.h"
#include "xbase/xns_macro.h"
#include "xdata/xblock.h"

#include <string>
#include <tuple>

NS_BEG3(top, xvm, xcontract)
template <typename Stream>
struct Functor {
    Functor(Stream & ds) : m_ds(ds) {
    }
    template <typename T>
    void operator()(T & t) const {
        m_ds >> t;
    }

private:
    Stream & m_ds;
};
// FROM https://stackoverflow.com/questions/1198260/how-can-you-iterate-over-the-elements-of-an-stdtuple
template <std::size_t I = 0, typename FuncT, typename... Tp>
inline typename std::enable_if<I == sizeof...(Tp), void>::type for_each(std::tuple<Tp...> &, FuncT) {
}

template <std::size_t I = 0, typename FuncT, typename... Tp>
    inline typename std::enable_if < I<sizeof...(Tp), void>::type for_each(std::tuple<Tp...> & t, FuncT f) {
    f(std::get<I>(t));
    for_each<I + 1, FuncT, Tp...>(t, f);
}

template <typename... Args>
base::xstream_t & operator>>(base::xstream_t & ds, std::tuple<Args...> & t) {
    for_each(t, Functor<base::xstream_t>(ds));
    return ds;
}
NS_END3
