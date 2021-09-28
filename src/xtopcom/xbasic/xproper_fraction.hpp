// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "assert.h"
#include "xbase/xns_macro.h"

#include <type_traits>
NS_BEG1(top)

template <typename IntergerT,
          typename ValueT,
          typename std::enable_if<std::is_integral<IntergerT>::value>::type * = nullptr,
          typename std::enable_if<std::is_floating_point<ValueT>::value>::type * = nullptr>
class xtop_proper_fraction final {
private:
    IntergerT m_num{0};  // numerator
    IntergerT m_den{1};  // denominator
    /*
        a proper fraction is a fraction that values exists between 0 and 1.
                    num
        value = -----------    ( num <= den )
                    den
    */
public:
    xtop_proper_fraction() {
    }
    xtop_proper_fraction(IntergerT num, IntergerT den) : m_num{num}, m_den{den} {
        assert(m_den != 0);
        assert(m_num <= m_den);  //"numerator should be less than denominator"
    }

    IntergerT num() const noexcept {
        return m_num;
    }

    IntergerT den() const noexcept {
        return m_den;
    }

    void set_num(IntergerT _num) {
        m_num = (_num > m_den) ? m_den : _num;
    }

    // todo add methods: add/minus m_num

    // should not provide this .
    // void set_den(IntergerT _den){
    //     m_den = _den;
    //     if (m_num < m_den) {
    //         m_num = m_den;
    //     }
    // }

    ValueT value() const noexcept {
        return (ValueT)m_num / (ValueT)m_den;
    }
};

template <typename T, typename V>
using xproper_fraction_t = xtop_proper_fraction<T, V>;
NS_END1