#pragma once
#include "xcontract_common/xcontract_api_params.h"

struct Erc20_Instance;
extern "C" Erc20_Instance * get_erc20_instance(uint8_t * s, uint32_t size);
extern "C" int32_t depoly_erc20(Erc20_Instance * ins_ptr, erc20_params * ptr);
extern "C" int32_t call_erc20(Erc20_Instance * ins_ptr, erc20_params * ptr);
extern "C" uint64_t get_gas_left(Erc20_Instance * ins_ptr);
extern "C" void release_instance(Erc20_Instance * ins_ptr);
