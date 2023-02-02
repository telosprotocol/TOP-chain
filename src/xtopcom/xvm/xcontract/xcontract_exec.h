// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include "xvm/xvm_context.h"
#include "xvm/xerror/xvm_error.h"
#include "xdata_stream.h"
#include "xbasic/xerror/xerror.h"

NS_BEG3(top, xvm, xcontract)

// FROM https://stackoverflow.com/questions/7858817/unpacking-a-tuple-to-call-a-matching-function-pointer
template<size_t ...>
struct seq { };

template<size_t N, size_t ...S>
struct gens : gens<N-1, N-1, S...> { };

template<size_t ...S>
struct gens<0, S...> {
  typedef seq<S...> type;
};

template<typename Func, typename Obj, typename Tuple, std::size_t... index>
auto apply_helper(Func&& func, Obj&& obj, Tuple&& tuple, seq<index...>) ->
    decltype(func(obj, std::get<index>(std::forward<Tuple>(tuple))...)) {
    return func(obj, std::get<index>(std::forward<Tuple>(tuple))...);
}

template<typename Func, typename Obj, typename Tuple>
auto apply(Func&& func, Obj&& obj, Tuple&& tuple) ->
    decltype(apply_helper(std::forward<Func>(func),
                          std::forward<Obj>(obj),
                          std::forward<Tuple>(tuple),
                          std::declval<typename gens<(std::tuple_size<typename std::decay<Tuple>::type>::value)>::type>())) {
    typename gens<(std::tuple_size<typename std::decay<Tuple>::type>::value)>::type temp;
    return apply_helper(std::forward<Func>(func),
                        std::forward<Obj>(obj),
                        std::forward<Tuple>(tuple),
                        temp);
}

template<typename T>
T unpack( base::xstream_t& stream ) {
    T result;
    stream >> result;
    return result;
}


template<typename T, typename U, typename Callable, typename... Args>
void do_action(T* obj, top::base::xstream_t& stream, Callable&& callable, void (U::*)(Args...))
{
	auto args = unpack<std::tuple<typename std::decay<Args>::type...>>(stream);
    apply(callable, obj, args);
}


/**
 * @brief define exec function in contract
 *
 */
#define BEGIN_CONTRACT_WITH_PARAM(class_name) void exec(top::xvm::xvm_context* vm_ctx) {\
    auto& func_name = vm_ctx->m_action_name;\
    const auto& params = vm_ctx->m_action_para;\
    xcontract_base::set_contract_helper(vm_ctx->m_contract_helper);\
    base::xstream_t stream(base::xcontext_t::instance(), (uint8_t*)params.data(), params.size());\
    CONTRACT_FUNCTION_PARAM(class_name, setup);\
    CONTRACT_FUNCTION_PARAM(class_name, on_event)

#define END_CONTRACT_WITH_PARAM std::error_code ec{top::xvm::enum_xvm_error_code::enum_vm_no_func_find};\
                                top::error::throw_error(ec, "no exec function find");\
}

#define CONTRACT_FUNCTION_PARAM(class_name, func)  CALL_FUNC_PARAM(class_name, func_name, func, params)

/**
 * @brief call the func in class with the param
 *
 */
#define CALL_FUNC_PARAM(class_name, func_name, func, params)    \
    if (#func == func_name) {                                   \
        auto fn = std::mem_fn(&class_name::func);               \
        do_action(this, stream, fn, &class_name::func);         \
        return;                                                 \
    }

NS_END3
