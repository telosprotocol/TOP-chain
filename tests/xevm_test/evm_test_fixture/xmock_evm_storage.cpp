#include "tests/xevm_test/evm_test_fixture/xmock_evm_storage.h"

namespace top {
namespace evm {
namespace tests {
std::vector<uint8_t> xmock_evm_storage::storage_get(std::vector<uint8_t> const & key) {
    auto res = ext_kv_datas[key];
    return res;
}
void xmock_evm_storage::storage_set(std::vector<uint8_t> const & key, std::vector<uint8_t> const & value) {
    ext_kv_datas[key] = value;
}

void xmock_evm_storage::storage_remove(std::vector<uint8_t> const & key) {
    ext_kv_datas.erase(key);
}
}  // namespace tests
}  // namespace evm
}  // namespace top