#pragma once
#include "stdint.h"

#include <string>

extern "C" bool deploy_code();
extern "C" bool call_contract();

#ifdef XENABLE_TESTS
extern "C" void do_mock_add_balance(const char * address, uint64_t address_len, uint64_t value1, uint64_t value2, uint64_t value3, uint64_t value4);
#endif