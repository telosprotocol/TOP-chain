#include <string>
#include <vector>
#include <cinttypes>
#include <fstream>
#include <assert.h>

extern "C" bool validate_wasm_with_path(char *s);
extern "C" bool validate_wasm_with_content(uint8_t *s, uint32_t size);

struct Module;

extern "C" Module *get_instance(uint8_t *s, uint32_t size);
extern "C" bool use_instance(Module *module_ptr);
extern "C" int32_t use_instance_2_1(Module *module_ptr, char *api_name, int32_t a, int32_t b);
extern "C" int32_t use_instance_1_1(Module *module_ptr, char *api_name, int32_t a);

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

Module *get_rustvm_instance(const char *file_path)
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

    return get_instance(bytes, bytes_size);
}

void test_validate_wasm_file()
{
    assert(validate_wasm_bytes("./example/add.wasm"));     // wasm file
    assert(validate_wasm_with_path("./example/add.wasm")); // get file content in C and pass bytes to rustvm.
    assert(!validate_wasm_with_path("wrong_file_path"));   // wrong file path
    assert(!validate_wasm_with_path("./example/fib.c"));   // file can't compile into wasm

    // validate_wasm_with_path("./example/add1.wasm");
    // validate_wasm_with_path("./example/fib.wasm");
    // validate_wasm_with_path("./example/add.wasm");
}

void test_call_wasm_api()
{
    Module *m_ptr1 = get_rustvm_instance("./example/add.wasm");
    assert(use_instance_2_1(m_ptr1, "add", 1, 2) == 3);

    Module *m_ptr2 = get_rustvm_instance("./example/fib.wasm");
    assert(use_instance_1_1(m_ptr2, "fib", 10) == 55);
}

int main()
{
    test_validate_wasm_file();
    test_call_wasm_api();

    return 0;
}