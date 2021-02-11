#include <vector>
#include <stdio.h>

#include "gtest/gtest.h"
#include "xbase/xdata.h"
#include "xbase/xutl.h"

using namespace std;
using namespace top;
using namespace top::base;

#if 0

class test_data_leak : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

int test_xstring_t_count = 0;
class test_xstring_t : public xstring_t {
 public:
    test_xstring_t() {
        test_xstring_t_count++;
    }
 protected:
    virtual ~test_xstring_t() {
        test_xstring_t_count--;
    }
};
using test_xstring_ptr_t = xobject_ptr_t<test_xstring_t>;

TEST_F(test_data_leak, xobject_mem_leak_1) {
    test_xstring_t_count = 0;
    size_t count = 10000000;
    std::vector<test_xstring_ptr_t> myvectors;
    std::string value(1024, 'A');
    ASSERT_EQ(value.size(), 1024);
    for (size_t i = 0; i < count; i++) {
        test_xstring_ptr_t _str = make_object_ptr<test_xstring_t>();
        _str->set(value);
        myvectors.push_back(_str);
    }
    std::cout << "push all " << test_xstring_t_count << std::endl;
    sleep(10);
    myvectors.clear();
    std::cout << "pop all " << test_xstring_t_count << std::endl;
    sleep(1000000);
}

TEST_F(test_data_leak, xobject_mem_leak_2) {
    test_xstring_t_count = 0;
    size_t count = 10000000;
    std::vector<test_xstring_t*> myvectors;
    std::string value(1024, 'A');
    ASSERT_EQ(value.size(), 1024);
    for (size_t i = 0; i < count; i++) {
        test_xstring_t* _str = new test_xstring_t();
        _str->set(value);
        myvectors.push_back(_str);
    }
    std::cout << "push all " << test_xstring_t_count << std::endl;
    sleep(10);

    for (size_t i = 0; i < count; i++) {
        test_xstring_t* _str = myvectors[i];
        _str->release_ref();
    }
    //myvectors.clear();
    std::cout << "pop all " << test_xstring_t_count << std::endl;
    sleep(1000000);
}

TEST_F(test_data_leak, xobject_mem_leak_3) {
    test_xstring_t_count = 0;
    size_t count = 10000000;
    std::vector<test_xstring_t*> myvectors;
    std::string value(1024, 'A');
    ASSERT_EQ(value.size(), 1024);
    for (size_t i = 0; i < count; i++) {
        test_xstring_t* _str = new test_xstring_t();
        _str->set(value);
        myvectors.push_back(_str);
    }
    std::cout << "push all " << test_xstring_t_count << std::endl;

    while (!myvectors.empty()) {
        test_xstring_t* _str = myvectors.back();
        _str->release_ref();
        myvectors.pop_back();
    }
    std::cout << "pop all " << test_xstring_t_count << std::endl;
    sleep(1000000);
}

TEST_F(test_data_leak, xobject_mem_leak_4) {
    test_xstring_t_count = 0;
    size_t count = 10000000;
    std::vector<xstring_t*> myvectors;
    std::string value(1024, 'A');
    ASSERT_EQ(value.size(), 1024);
    for (size_t i = 0; i < count; i++) {
        xstring_t* _str = new xstring_t();
        _str->set(value);
        myvectors.push_back(_str);
    }
    std::cout << "push all " << test_xstring_t_count << std::endl;

    while (!myvectors.empty()) {
        xstring_t* _str = myvectors.back();
        _str->release_ref();
        myvectors.pop_back();
    }
    std::cout << "pop all " << test_xstring_t_count << std::endl;
    sleep(1000000);
}

TEST_F(test_data_leak, xobject_mem_leak_5) {
{
    size_t count = 1000000;
    //size_t count = 10000000;
    std::vector<std::string *> myvectors;

    for (size_t i = 0; i < count; i++) {
        std::string * _str = new std::string(1024, 'A');
        ASSERT_EQ(_str->size(), 1024);
        myvectors.push_back(_str);
    }
    std::cout << "push all " << std::endl;

    while (!myvectors.empty()) {
        std::string* _str = myvectors.back();
        delete _str;
        myvectors.pop_back();
    }
    std::cout << "pop all " << std::endl;
}
    sleep(1000000);
}

TEST_F(test_data_leak, xobject_mem_leak_6) {
    size_t count = 1000000;
    std::vector<char *> myvectors;
    for (size_t i = 0; i < count; i++) {
        char * _str = new char[1000];
        myvectors.push_back(_str);
    }
    std::cout << "push all " << std::endl;

    while (!myvectors.empty()) {
        char * _str = myvectors.back();
        delete [] _str;
        myvectors.pop_back();
    }
    std::cout << "pop all " << std::endl;
    sleep(1000000);
}
#endif
