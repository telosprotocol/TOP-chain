// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xlog.h"
#include "xbasic/xbyte_buffer.h"

#include <string>
#include <utility>
#include <stdexcept>

NS_BEG1(top)


xbyte_buffer_t
from_hex_string(std::string const & str);


template<class...> struct conjunction : std::true_type { };
template<class B1> struct conjunction<B1> : B1 { };
template<class B1, class... Bn>
struct conjunction<B1, Bn...>
    : std::conditional<bool(B1::value), conjunction<Bn...>, B1>::type {};

template <typename T, typename... Ts>
typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value && conjunction<std::is_same<Ts, T>...>::value ,
                        T>::type
add(T i, T j, Ts... all) {
  T values[] = { j, all... };
  T r = i;
  for (auto v : values) {
    r += v;
    if (r < v) {
        throw std::out_of_range{"add overflow"};
    }
  }
  return r;
}



template <class T>
typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value,
                        T>::type
sub(T a, T b) {
    T x = a - b;
    if (a < b) {
        throw std::out_of_range{"sub overflow"};
    }
    return x;
}

template <class T>
typename std::enable_if<std::is_integral<T>::value && std::is_unsigned<T>::value,
                        T>::type
mul(T a, T b) {
    T x = a * b;
    if (a != 0 && x / a != b) {
        throw std::out_of_range{"mul overflow"};
    }
    return x;
}

#if !defined XCXX14_OR_ABOVE

template<typename T, typename U>
constexpr
T const &
get(std::pair<T, U> const & pair) noexcept {
    return (std::get<0>(pair));
}

template<typename U, typename T>
constexpr
U const &
get(std::pair<T, U> const & pair) noexcept {
    return (std::get<1>(pair));
}

template<typename T, typename U>
constexpr
T &
get(std::pair<T, U> & pair) noexcept {
    return (std::get<0>(pair));
}

template<typename U, typename T>
constexpr
U &
get(std::pair<T, U> & pair) noexcept {
    return (std::get<1>(pair));
}

template<typename T, typename U>
constexpr
T &&
get(std::pair<T, U> && pair) noexcept {
    return (std::get<0>(std::move(pair)));
}

template<typename U, typename T>
constexpr
U &&
get(std::pair<T, U> && pair) noexcept {
    return (std::get<1>(std::move(pair)));
}


//template <typename T, std::size_t I, typename Head, typename ... Remains>
//constexpr
//T &
//get_helper(std::tuple<Head, Remains...> & tuple, ) noexcept {
//}
//
//template<typename T, typename ... Types>
//constexpr T &
//get(std::tuple<Types...> & t) noexcept {
//    return std::get<T>(t);
//}
//
//template< typename T, typename... Types >
//constexpr T &&
//get(tuple<Types...>&& t) noexcept {
//}
//
//template< typename T, typename... Types >
//constexpr T const & get(std::tuple<Types...> const & t) noexcept {
//}
//
//template< typename T, typename... Types >
//constexpr T const &&
//get(tuple<Types...> const && t) noexcept {
//}

#else

template <typename T, typename U>
constexpr
T const &
get(std::pair<T, U> const & pair) noexcept {
     return std::get<T>(pair);
}

template <typename U, typename T>
constexpr
U const &
get(std::pair<T, U> const & pair) noexcept {
    return std::get<U>(pair);
}

template<typename T, typename U>
constexpr
T &
get(std::pair<T, U> & pair) noexcept {
    return std::get<T>(pair);
}

template<typename U, typename T>
constexpr
U &
get(std::pair<T, U> & pair) noexcept {
    return std::get<U>(pair);
}

template<typename T, typename U>
constexpr
T &&
get(std::pair<T, U> && pair) noexcept {
    return std::get<T>(std::move(pair));
}

template<typename U, typename T>
constexpr
U &&
get(std::pair<T, U> && pair) noexcept {
    return std::get<U>(std::move(pair));
}

#endif

NS_END1

#if !defined NDEBUG
#   if defined XPRINT
#       error "XPRINT redefined"
#   endif

#   define XSTD_PRINT(...)          \
        std::printf(__VA_ARGS__);   \
        std::fflush(stdout)
#else
#   define XSTD_PRINT(...)
#endif

#if defined XTHROW
#   error "XTHROW redefined"
#endif
#if (defined XTHROW_ERROR && defined DEBUG)
#   define XTHROW(EXCEPTION, ERRC, EXTRA_MSG)                               \
        auto exception = EXCEPTION{ EXTRA_MSG, ERRC, __LINE__, __FILE__ };  \
        XSTD_PRINT(u8"throws exception %s\n", exception.what());            \
        xerror(u8"throws exception %s", exception.what());                  \
        throw exception
#else
#   define XTHROW(EXCEPTION, ERRC, EXTRA_MSG)                               \
        auto exception = EXCEPTION{ EXTRA_MSG, ERRC, __LINE__, __FILE__ };  \
        XSTD_PRINT(u8"throws exception %s\n", exception.what());            \
        xwarn(u8"throws exception %s", exception.what());                   \
        throw exception
#endif

#if !defined NDEBUG
#   if defined XENABLE_VERBOSE_DBG
#       define XVERBOSE_ASSERT(CONDITION)   assert(CONDITION)
#   else
#       define XVERBOSE_ASSERT(CONDITION)
#   endif
#else
#   define XVERBOSE_ASSERT(CONDITION)
#endif
