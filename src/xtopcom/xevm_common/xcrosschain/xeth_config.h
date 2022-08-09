#pragma once

#include "xevm_common/common.h"

NS_BEG4(top, evm_common, eth, config)

bool is_london(const bigint num);
bool is_arrow_glacier(const bigint num);

NS_END4
