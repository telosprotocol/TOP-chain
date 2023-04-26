#pragma once

#include <cstdint>

extern "C" bool deploy_code();
extern "C" bool call_contract();
extern "C" bool unsafe_mint(void * engine, void * executor, uint8_t const * addr_ptr, uint64_t addr_size, char const * value_dec_str);
extern "C" bool unsafe_burn(void * engine, void * executor, uint8_t const * addr_ptr, uint64_t addr_size, char const * value_dec_str);
extern "C" bool unsafe_merkle_proof(uint8_t const * leaf_ptr, uint8_t const * branch_ptr, uint64_t branch_data_size, uint64_t depth, uint64_t index, uint8_t * root_ptr);
extern "C" bool unsafe_verify_bls_signatures(uint8_t const * signature_ptr, uint8_t const * pubkeys_ptr, uint64_t pubkeys_size, uint8_t const * signing_root_ptr);
extern "C" bool unsafe_beacon_header_root(uint8_t const * data_ptr, uint64_t data_size, uint8_t * hash_ptr);
extern "C" bool unsafe_sync_committee_root(uint8_t const * data_ptr, uint64_t data_size, uint8_t * hash_ptr);
extern "C" bool unsafe_compute_committee_signing_root(uint8_t const * object_root_ptr, uint64_t version, uint64_t signature_slot,uint8_t * signing_root_ptr);

extern "C" void * evm_engine();
extern "C" void * evm_executor();


#ifdef XENABLE_TESTS
extern "C" void do_mock_add_balance(const char * address, uint64_t address_len, uint64_t value1, uint64_t value2, uint64_t value3, uint64_t value4);
#endif
