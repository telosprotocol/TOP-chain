#pragma once
#include "stdint.h"

#include <string>

extern "C" bool deploy_code();
extern "C" bool call_contract();

#ifdef XENABLE_TESTS
extern "C" void do_mock_add_balance(const char * address, uint64_t address_len, uint64_t value);
#endif