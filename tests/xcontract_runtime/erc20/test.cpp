#include <assert.h>
#include <xcontract_common/xcontract_api_params.h>

#include <gtest/gtest.h>

#include <cinttypes>
#include <fstream>
#include <string>

struct Erc20_Instance;
extern "C" Erc20_Instance * get_erc20_instance(uint8_t * s, uint32_t size);
extern "C" int32_t depoly_erc20(Erc20_Instance * ins_ptr, params * ptr);
extern "C" int32_t call_erc20(Erc20_Instance * ins_ptr, params * ptr);
extern "C" uint64_t get_gas_left(Erc20_Instance * ins_ptr);
extern "C" void release_instance(Erc20_Instance * ins_ptr);

Erc20_Instance * get_erc20_by_path(const char * file_path) {
    uint8_t * bytes;
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

void test_erc20() {
    params p1{"fromAAA", "toBBB", 1111};
    printf("[debug] p1 %p \n", &p1);

    params p2{"fromCCC", "toDDD", 2222};
    printf("[debug] p2 %p \n", &p2);

    Erc20_Instance * ins_ptr = get_erc20_by_path("./test_erc20.wasm");
    depoly_erc20(ins_ptr, &p1);
    std::printf("left gas: %lu \n", get_gas_left(ins_ptr));
    call_erc20(ins_ptr, &p2);
    std::printf("left gas: %lu \n", get_gas_left(ins_ptr));
    release_instance(ins_ptr);
}

TEST(tmp, _1) {
    test_erc20();
}