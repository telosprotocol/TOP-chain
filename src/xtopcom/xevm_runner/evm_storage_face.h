#pragma once

#include <cstdint>
#include <vector>

namespace top {
namespace evm {

class xtop_evm_storage_face {
public:
    using bytes = std::vector<uint8_t>;

    xtop_evm_storage_face() = default;
    xtop_evm_storage_face(xtop_evm_storage_face const &) = delete;
    xtop_evm_storage_face & operator=(xtop_evm_storage_face const &) = delete;
    xtop_evm_storage_face(xtop_evm_storage_face &&) = default;
    xtop_evm_storage_face & operator=(xtop_evm_storage_face &&) = default;
    virtual ~xtop_evm_storage_face() = default;

    virtual bytes storage_get(bytes const & key) = 0;
    virtual void storage_set(bytes const & key, bytes const & value) = 0;
    virtual void storage_remove(bytes const & key) = 0;
};

using xevm_storage_face_t = xtop_evm_storage_face;
}  // namespace evm
}  // namespace top
