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

struct Module;

extern "C" Module *get_instance(uint8_t *s, uint32_t size);
extern "C" bool use_instance(Module *module_ptr);

void tttest(const char *file_path)
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

    Module *m_ptr = get_instance(bytes, bytes_size);
    bool res = use_instance(m_ptr);
    printf("res: %s\n", res?"true":"false");
}

int main()
{
    tttest("./example/add.wasm");
    return 1;
    assert(validate_wasm_bytes("./example/add.wasm"));
    assert(validate_wasm_with_path("./example/add.wasm"));
    assert(!validate_wasm_with_path("wrong_file_path"));
    assert(!validate_wasm_with_path("./example/fib.c"));

    // validate_wasm_with_path("./example/add1.wasm");
    // validate_wasm_with_path("./example/fib.wasm");
    // validate_wasm_with_path("./example/add.wasm");
    return 0;
}