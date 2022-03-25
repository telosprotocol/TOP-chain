#pragma once
#include <map>
#include <string>
#include <vector>

namespace top {
namespace evm {
namespace memory_tools {

void read_memory(uint64_t offset, std::vector<uint8_t> & buffer) {
    char * begin_address = (char *)offset;
    for (std::size_t i = 0; i < buffer.size(); ++i) {
        buffer[i] = *begin_address;
        begin_address++;
    }
}

void write_memory(uint64_t offset, std::vector<uint8_t> const & buffer) {
    char * begin_address = (char *)offset;
    for (std::size_t i = 0; i < buffer.size(); ++i) {
        *begin_address = buffer[i];
        begin_address++;
    }
}
}  // namespace memory_tools
}  // namespace evm
}  // namespace top