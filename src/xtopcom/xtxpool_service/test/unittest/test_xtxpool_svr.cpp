#include "gtest/gtest.h"
#include "xtxpool_service/xtxpool_service.h"

using namespace top::xtxpool_service;
using namespace top;
using namespace top::base;
using namespace std;

class test_xtxpool_svr : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

public:
};

// uint32_t get_select_num(const std::string & hash, uint32_t resend_time, uint32_t shard_size) {
//     std::string random = hash + std::to_string(resend_time);
//     return base::xhash32_t::digest(random) % shard_size;
// }

// TEST_F(test_xtxpool, select_num) {
//     std::string hash = "0x24ea4258db4e299cb50c10a0fe45a7ba7d2728ae39c0d900f2186555f5242381";

//     std::vector<uint8_t> ret_vec = std::move(data::xaction_t::hex_to_uint(hash));
//     xassert(ret_vec.size() == 32);
//     uint256_t hash256 = uint256_t(ret_vec.data());
//     std::string hexhash = to_hex_str(hash256);
//     std::cout << "hexhash=" << hexhash << std::endl;

//     uint32_t select_num = 0;
//     for (uint32_t i = 0; i < 100; i++) {
//         select_num = get_select_num(hexhash, i, 4);
//         std::cout << "i=" << i << ", select_num=" << select_num << std::endl;
//     }
// }


// bool xtxpool_service::is_selected_sender(uint32_t pos, uint32_t rand_pos, uint32_t select_num, uint32_t size) {
//     if (pos >= rand_pos) {
//         return pos < rand_pos + select_num;
//     } else {
//         return (rand_pos + select_num > size) && pos < (rand_pos + select_num)%size;
//     }
// }

TEST_F(test_xtxpool_svr, is_select) {
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(0, 0, 1, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(1, 0, 1, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(2, 0, 1, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(3, 0, 1, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(0, 1, 1, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(1, 1, 1, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(2, 1, 1, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(3, 1, 1, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(0, 2, 1, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(1, 2, 1, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(2, 2, 1, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(3, 2, 1, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(0, 3, 1, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(1, 3, 1, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(2, 3, 1, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(3, 3, 1, 4), true);

    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(0, 0, 2, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(1, 0, 2, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(2, 0, 2, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(3, 0, 2, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(0, 1, 2, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(1, 1, 2, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(2, 1, 2, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(3, 1, 2, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(0, 2, 2, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(1, 2, 2, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(2, 2, 2, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(3, 2, 2, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(0, 3, 2, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(1, 3, 2, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(2, 3, 2, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(3, 3, 2, 4), true);

    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(0, 0, 3, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(1, 0, 3, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(2, 0, 3, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(3, 0, 3, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(0, 1, 3, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(1, 1, 3, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(2, 1, 3, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(3, 1, 3, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(0, 2, 3, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(1, 2, 3, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(2, 2, 3, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(3, 2, 3, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(0, 3, 3, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(1, 3, 3, 4), true);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(2, 3, 3, 4), false);
    ASSERT_EQ(xtxpool_service::xtxpool_service::is_selected_sender(3, 3, 3, 4), true);
}

