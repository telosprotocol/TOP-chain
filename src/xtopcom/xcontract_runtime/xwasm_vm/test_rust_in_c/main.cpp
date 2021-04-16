#include <string>
#include <vector>
#include <cinttypes>
#include <fstream>
#include <assert.h>

extern "C" bool validate_wasm_with_path(char *s);
extern "C" bool validate_wasm_with_content(uint8_t *s, uint32_t size);

bool validate_wasm_bytes(const char *file_path)
{
    uint8_t *bytes;
    uint32_t bytes_size;

    std::ifstream file_size(file_path, std::ifstream::ate | std::ifstream::binary);
    bytes_size = file_size.tellg();

    file_size.close();

    std::ifstream in(file_path, std::ifstream::binary);
    bytes = (uint8_t *)malloc(bytes_size);
    in.read(reinterpret_cast<char *>(bytes), bytes_size);
    in.close();

    return validate_wasm_with_content(bytes, bytes_size);
}

int main()
{
    assert(validate_wasm_bytes("./example/add.wasm"));
    assert(validate_wasm_with_path("./example/add.wasm"));
    assert(!validate_wasm_with_path("wrong_file_path"));
    assert(!validate_wasm_with_path("./example/fib.c"));

    // validate_wasm_with_path("./example/add1.wasm");
    // validate_wasm_with_path("./example/fib.wasm");
    // validate_wasm_with_path("./example/add.wasm");
    return 0;
}