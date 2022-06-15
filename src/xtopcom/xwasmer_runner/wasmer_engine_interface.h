#pragma once

struct contract_instance;
extern "C" contract_instance * deploy_contract(uint8_t * code_ptr, uint32_t size);
extern "C" int32_t call_contract_1(contract_instance * ins, int32_t a, int32_t b);
extern "C" int32_t call_contract_2(contract_instance * ins);
extern "C" int32_t call_contract_3(contract_instance * ins);

extern "C" void set_gas_left(contract_instance * ins, uint64_t gas_limit);