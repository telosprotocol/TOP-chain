#pragma once

#include "xcommon/common.h"

NS_BEG4(top, evm_common, eth, config)

bool is_london(uint64_t num) noexcept;
bool is_arrow_glacier(uint64_t num) noexcept;

NS_END4
