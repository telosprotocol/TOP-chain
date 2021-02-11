#include "gtest/gtest.h"
#include "xcrypto/xckey.h"
#include "xcrypto/xcrypto_util.h"
#include "xtxpool/xtxpool_receipt_receiver_counter.h"

using namespace top::xtxpool;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;

class test_receipt_counter : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }

public:
};

TEST_F(test_receipt_counter, all_not_full) {
    xtxpool_receipt_receiver_counter counter;
    for (int32_t i = 0; i < 256; i++) {
        for (int32_t j = 0; j < 5; j++) {
            xecprikey_t pri_key_obj;
            xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
            std::string account = pub_key_obj.to_address('0', 0) + "@" + std::to_string(i);
            bool ret = counter.is_receiver_count_full_and_insert(account);
            ASSERT_EQ(ret, false);
        }
    }
}

TEST_F(test_receipt_counter, one_table_full) {
    xtxpool_receipt_receiver_counter counter;
    for (int32_t j = 0; j < 6; j++) {
        xecprikey_t pri_key_obj;
        xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
        std::string account = pub_key_obj.to_address('0', 0) + "@0";
        bool ret = counter.is_receiver_count_full_and_insert(account);
        if (j < 5) {
            ASSERT_EQ(ret, false);
        } else {
            ASSERT_EQ(ret, true);
        }
    }
}

TEST_F(test_receipt_counter, one_account_insert_more_than_once) {
    xtxpool_receipt_receiver_counter counter;
    xecprikey_t pri_key_obj;
    xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
    std::string account = pub_key_obj.to_address('0', 0) + "@" + std::to_string(0);
    for (int32_t j = 0; j < 100; j++) {
        bool ret = counter.is_receiver_count_full_and_insert(account);
        if (j < 1) {
            ASSERT_EQ(ret, false);
        } else {
            ASSERT_EQ(ret, true);
        }
    }
}

