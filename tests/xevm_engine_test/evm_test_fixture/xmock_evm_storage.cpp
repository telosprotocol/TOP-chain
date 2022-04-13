#include "tests/xevm_engine_test/evm_test_fixture/xmock_evm_storage.h"

NS_BEG4(top, contract_runtime, evm, tests)

xbytes_t xmock_evm_storage::storage_get(xbytes_t const & key) {
    auto res = ext_kv_datas[key];
    return res;
}
void xmock_evm_storage::storage_set(xbytes_t const & key, xbytes_t const & value) {
    ext_kv_datas[key] = value;
}

void xmock_evm_storage::storage_remove(xbytes_t const & key) {
    ext_kv_datas.erase(key);
}

NS_END4