#pragma once
#include "stdint.h"

extern "C" void deploy_code();
extern "C" void call_contract();
extern "C" void serial_param_function_callargs(const char * address,
                                               uint64_t address_len,
                                               uint64_t value,
                                               const unsigned char * params,
                                               uint64_t params_len,
                                               uint64_t max_output_len,
                                               unsigned char * output,
                                               uint64_t * output_len);
extern "C" void mock_add_balance();