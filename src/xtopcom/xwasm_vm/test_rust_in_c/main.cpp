#include <string>
#include <vector>
#include <cinttypes>
#include <fstream>
#include <assert.h>
#include <chrono>
#include <thread>

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

struct params
{
    std::string account_from;
    std::string account_to;
    int value;
    params(std::string _f, std::string _t, int _v) : account_from{_f}, account_to{_t}, value{_v} {}
};

struct Erc20_Instance;
extern "C" Erc20_Instance *get_erc20_instance(uint8_t *s, uint32_t size);
extern "C" int32_t depoly_erc20(Erc20_Instance *ins_ptr, params *ptr);
extern "C" int32_t call_erc20(Erc20_Instance *ins_ptr, params *ptr);
extern "C" uint64_t get_gas_left(Erc20_Instance *ins_ptr);
extern "C" void release_instance(Erc20_Instance *ins_ptr);

Erc20_Instance *get_erc20_by_path(const char *file_path)
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

    return get_erc20_instance(bytes, bytes_size);
}

void test_erc20()
{
    params p1{"fromAAA", "toBBB", 1111};
    printf("[debug] p1 %p \n", &p1);

    params p2{"fromCCC", "toDDD", 2222};
    printf("[debug] p2 %p \n", &p2);

    Erc20_Instance *ins_ptr = get_erc20_by_path("./test_erc20.wasm");
    depoly_erc20(ins_ptr, &p1);
    std::printf("left gas: %llu\n", get_gas_left(ins_ptr));
    call_erc20(ins_ptr, &p2);
    std::printf("left gas: %llu\n", get_gas_left(ins_ptr));
    release_instance(ins_ptr);
}

int main()
{
    test_erc20();

    // // test memory-leak
    // for (auto index = 0; index < 100000; ++index)
    // {
    //     std::printf("-------- %zu ---------", index);
    //     test_erc20();
    //     std::this_thread::sleep_for(std::chrono::microseconds(100));
    // }

    // test_validate_wasm_file();
    // test_call_wasm_api();

    return 0;
}