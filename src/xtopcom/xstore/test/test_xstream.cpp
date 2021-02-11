#include <vector>

#include "gtest/gtest.h"
#include "xstore/xstore.h"
#include "xstore/xstore_error.h"
#include "xdb/xdb.h"
#include "xbasic/xmodule_type.h"

#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"

using namespace top;
using namespace top::base;
using namespace top::store;
using namespace std;

class test_xstream : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_xstream, stream_write_and_read) {
    xstream_t stream(xcontext_t::instance());
    //stream write
    {
        const int32_t begin_pos = stream.get_back_offset();
        string str1 = "123456";
        uint32_t data2 = 0x11223344;
        uint32_t data3 = 0x55667788;
        stream << str1;
        stream << data2;
        stream << data3;
        const int32_t end_pos = stream.get_back_offset();
        string value((char*)stream.data(), stream.size());
        ASSERT_NE(begin_pos, end_pos);
        ASSERT_NE(0, stream.size());
        ASSERT_NE(0, value.size());
    }
    //stream read
    {
        string str1;
        uint32_t data2;
        uint32_t data3;
        const int32_t begin_pos = stream.get_front_offset();
        stream >> str1;
        stream >> data2;
        stream >> data3;
        const int32_t end_pos = stream.get_front_offset();
        ASSERT_NE(begin_pos, end_pos);
        ASSERT_EQ(str1, "123456");
        ASSERT_EQ(data2, 0x11223344);
        ASSERT_EQ(data3, 0x55667788);
    }
}

TEST_F(test_xstream, register_log_str) {
    // REG_XMODULE_LOG(chainbase::enum_xmodule_type::xmodule_type_xstore, store::xstore_error_to_string, store::xstore_error_base+1, store::xstore_error_max);

    // std::string str = chainbase::xmodule_error_to_str(store::xstore_check_block_hash_error);
    // std::cout << "error " << str << std::endl;
}
