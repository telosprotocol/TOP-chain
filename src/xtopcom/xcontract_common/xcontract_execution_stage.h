#pragma once

#include "xbasic/xns_macro.h"

#include <cstdint>

NS_BEG2(top, contract_common)

enum class xtop_enum_contract_execution_stage : uint8_t { invalid, source_action, target_action, confirm_action, self_action };
using xcontract_execution_stage_t = xtop_enum_contract_execution_stage;

NS_END2
