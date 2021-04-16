#pragma once

#include "xbase/xns_macro.h"

NS_BEG2(top, contract_runtime)

template <typename ActionT>
class xtop_action_runtime;

template <typename ActionT>
using xaction_runtime_t = xtop_action_runtime<ActionT>;

NS_END2
