#pragma once

#include <cstdint>

namespace top {
namespace wasm {

class xtop_wasm_logic_face {
public:
    xtop_wasm_logic_face() = default;
    xtop_wasm_logic_face(xtop_wasm_logic_face const &) = delete;
    xtop_wasm_logic_face & operator=(xtop_wasm_logic_face const &) = delete;
    xtop_wasm_logic_face(xtop_wasm_logic_face &&) = default;
    xtop_wasm_logic_face & operator=(xtop_wasm_logic_face &&) = default;
    virtual ~xtop_wasm_logic_face() = default;

public:
    virtual uint64_t block_index() = 0;
};
using xwasmer_logic_face_t = xtop_wasm_logic_face;

}  // namespace wasm
}  // namespace top
