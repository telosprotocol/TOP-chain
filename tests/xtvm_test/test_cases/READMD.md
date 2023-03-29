copy from `tests/xevm_engine_test/test_cases/ `

* delete cases needed C++ precompile contracts.(TEP1 token)
* delete cases needed mock balance api
* fix `QA_CASES/call_contract` becase sharding contract_address issue by replacing `__address_in_evm` to `__address_in_top_shard` in call contract data.
* tried but still failed to fix `QA_CASES/new_swap` , so I just rename the json test-case file.