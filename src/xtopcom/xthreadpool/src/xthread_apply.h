#pragma once

#include <string>
#include <map>
#include <iostream>
#include "xbasic/xns_macro.h"


NS_BEG2(top, xthreadpool)

template<size_t ...>
struct seq { };


template<size_t N, size_t ...S>
struct gens : gens<N - 1, N - 1, S...> { };


template<size_t ...S>
struct gens<0, S...> {
    typedef seq<S...> type;
};

template<typename Func, typename Obj, typename Tuple, std::size_t... index>
auto tuple_apply_impl(Func && func, Obj && obj, Tuple && tuple, seq<index...>) ->
decltype(func(obj, std::get<index>(std::forward<Tuple>(tuple))...))
{
    return func(obj, std::get<index>(std::forward<Tuple>(tuple))...);
}

template<typename Func, typename Obj, typename Tuple>
auto tuple_apply(Func && func, Obj && obj, Tuple && tuple) ->
decltype(tuple_apply_impl(std::forward<Func>(func),
    std::forward<Obj>(obj),
    std::forward<Tuple>(tuple),
    std::declval<typename gens<(std::tuple_size<typename std::decay<Tuple>::type>::value)>::type>()))
{
    using type_t = typename std::decay<Tuple>::type;
    constexpr size_t size = std::tuple_size<type_t>::value;
    typename gens<size>::type a;
    return tuple_apply_impl(std::forward<Func>(func),
        std::forward<Obj>(obj),
        std::forward<Tuple>(tuple),
        a
    );
}

NS_END2
