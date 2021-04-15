#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>
#include <functional>

NS_BEG3(top, contract_runtime, vm)

enum class xenum_type : uint8_t {
    invalid = 0,
    kernel,
    system,
    user,
    wasm,
    lua,
};
using xtype_t = xenum_type;

NS_END3

NS_BEG1(std)

template <>
struct hash<top::contract_runtime::vm::xtype_t> final {
    std::size_t operator()(top::contract_runtime::vm::xtype_t const type) const noexcept;
};

NS_END1
