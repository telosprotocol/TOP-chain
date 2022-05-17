#include "tests/xevm_engine_test/evm_test_fixture/xtest_evm_fixture.h"

NS_BEG4(top, contract_runtime, evm, tests)

using namespace top::evm;
using namespace top::contract_runtime::evm;
using tests::xmock_evm_storage;

using json = nlohmann::json;

TEST_F(xtest_evm_fixture, ALL_IN_ONE) {
    execute();
}

NS_END4
