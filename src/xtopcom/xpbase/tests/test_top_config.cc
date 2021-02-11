//
//  test_bloomfilter.cc
//  test
//
//  Created by Charlie Xie 12/18/2018.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include <gtest/gtest.h>

#include <iostream>
#include <fstream>

#define private public
#include "xpbase/base/top_config.h"

namespace top {

namespace test {

class TestTopConfig : public testing::Test {
public:
    static void SetUpTestCase() {
    }

    static void TearDownTestCase() {
    }

    virtual void SetUp() {
    }

    virtual void TearDown() {
    }
};

TEST_F(TestTopConfig, NotExistsConf) {
    top::base::Config config;
    ASSERT_FALSE(config.Init("/tmp/notexist.conf"));
}

TEST_F(TestTopConfig, UnvalidConf) {
    {
        const std::string str_file = 
R"(
abcd
)";
        std::fstream fout("/tmp/unvalid.conf", std::ios::out);
        fout << str_file;
        fout.close();
    }

    top::base::Config config;
    ASSERT_FALSE(config.Init("/tmp/unvalid.conf"));
}

TEST_F(TestTopConfig, Init) {
    {
        const std::string str_file = 
R"(
#dfasdfasd
[backbone] # sdfasdf
#sdfadsfas
string=string  # sadfasdfasdf
string_e=./test/string  #asdgfasdfa
string_s= test s  t  ring  #asdgfa
bool_true=true
bool_true_e=trdue
bool_false=false
bool_false_e=fdalse
int16=1
int16_e=1d
uint16=1
uint16_e=1d
int32=1
int32_e=1d
uint32=1
uint32_e=1d
int64=1
int64_e=1d
uint64=1
uint64_e=1d
float=1
float_e=1d
double=1
double_e=1d
#dfgadsf
)";
        std::fstream fout("/tmp/bootstrap.conf", std::ios::out);
        fout << str_file;
        fout.close();
    }

    top::base::Config config;
    ASSERT_TRUE(config.Init("/tmp/bootstrap.conf"));
    std::string val;
    ASSERT_TRUE(config.Get("backbone", "string", val));
    ASSERT_EQ(val, "string");
    ASSERT_TRUE(config.Get("backbone", "string_e", val));
    ASSERT_EQ(val, "./test/string");
    ASSERT_TRUE(config.Get("backbone", "string_s", val));
    ASSERT_EQ(val, "test s  t  ring");

    bool bool_val;
    ASSERT_TRUE(config.Get("backbone", "bool_true", bool_val));
    ASSERT_TRUE(bool_val);
    ASSERT_TRUE(config.Get("backbone", "bool_false", bool_val));
    ASSERT_FALSE(bool_val);

    int16_t int16_val;
    ASSERT_TRUE(config.Get("backbone", "int16", int16_val));
    ASSERT_EQ(int16_val, 1);
    uint16_t uint16_val;
    ASSERT_TRUE(config.Get("backbone", "uint16", uint16_val));
    ASSERT_EQ(uint16_val, 1);

    int32_t int32_val;
    ASSERT_TRUE(config.Get("backbone", "int32", int32_val));
    ASSERT_EQ(int32_val, 1);
    uint32_t uint32_val;
    ASSERT_TRUE(config.Get("backbone", "uint32", uint32_val));
    ASSERT_EQ(uint32_val, 1);
    int64_t int64_val;
    ASSERT_TRUE(config.Get("backbone", "int64", int64_val));
    ASSERT_EQ(int64_val, 1);
    uint64_t uint64_val;
    ASSERT_TRUE(config.Get("backbone", "uint64", uint64_val));
    ASSERT_EQ(uint64_val, 1);
    float float_val;
    ASSERT_TRUE(config.Get("backbone", "float", float_val));
    ASSERT_EQ(float_val, 1.0f);  // problem 
    double double_val;
    ASSERT_TRUE(config.Get("backbone", "double", double_val));
    ASSERT_EQ(double_val, 1.0);  // problem 

    ASSERT_TRUE(config.Set("backbone", "bool_true", true));
    ASSERT_TRUE(config.Set("backbone", "bool_false", false));

    int16_val = 1;
    ASSERT_TRUE(config.Set("backbone", "int16", int16_val));
    uint16_val = 1;
    ASSERT_TRUE(config.Set("backbone", "uint16", uint16_val));

    int32_val = 1;
    ASSERT_TRUE(config.Set("backbone", "int32", int32_val));
    uint32_val = 1;
    ASSERT_TRUE(config.Set("backbone", "uint32", uint32_val));
    int64_val = 1;
    ASSERT_TRUE(config.Set("backbone", "int64", int64_val));
    uint64_val = 1;
    ASSERT_TRUE(config.Set("backbone", "uint64", uint64_val));
    float_val = 1.0f;
    ASSERT_TRUE(config.Set("backbone", "float", float_val));
    double_val = 1.0;
    ASSERT_TRUE(config.Set("backbone", "double", double_val));

    ASSERT_TRUE(config.DumpConfig("/tmp/dump.conf"));
}

TEST_F(TestTopConfig, InitError) {
    top::base::Config config;
    ASSERT_TRUE(config.Init("/tmp/bootstrap.conf"));
    std::string val;
    ASSERT_TRUE(config.Get("backbone", "string", val));
    ASSERT_EQ(val, "string");
    bool bool_val;
    ASSERT_FALSE(config.Get("backbone", "bool_true_e", bool_val));
    ASSERT_FALSE(config.Get("backbone", "bool_false_e", bool_val));
    int16_t int16_val;
    ASSERT_FALSE(config.Get("backbone", "int16_e", int16_val));
    uint16_t uint16_val;
    ASSERT_FALSE(config.Get("backbone", "uint16_e", uint16_val));
    int32_t int32_val;
    ASSERT_FALSE(config.Get("backbone", "int32_e", int32_val));
    uint32_t uint32_val;
    ASSERT_FALSE(config.Get("backbone", "uint32_e", uint32_val));
    int64_t int64_val;
    ASSERT_FALSE(config.Get("backbone", "int64_e", int64_val));
    uint64_t uint64_val;
    ASSERT_FALSE(config.Get("backbone", "uint64_e", uint64_val));
    float float_val;
    ASSERT_FALSE(config.Get("backbone", "float_e", float_val));
    double double_val;
    ASSERT_FALSE(config.Get("backbone", "double_e", double_val));
}

TEST_F(TestTopConfig, Set) {
    top::base::Config config;
    ASSERT_TRUE(config.Init("/tmp/dump.conf"));
    std::string val;
    ASSERT_TRUE(config.Get("backbone", "string", val));
    ASSERT_EQ(val, "string");
    ASSERT_FALSE(config.Get("backbone1", "string", val));
    ASSERT_FALSE(config.Get("backbone", "string1", val));

    bool bool_val;
    ASSERT_TRUE(config.Get("backbone", "bool_true", bool_val));
    ASSERT_FALSE(config.Get("backbone", "bool_true1", bool_val));
    ASSERT_TRUE(bool_val);
    ASSERT_TRUE(config.Get("backbone", "bool_false", bool_val));
    ASSERT_FALSE(bool_val);

    int16_t int16_val;
    ASSERT_TRUE(config.Get("backbone", "int16", int16_val));
    ASSERT_FALSE(config.Get("backbone", "int161", int16_val));
    ASSERT_EQ(int16_val, 1);
    uint16_t uint16_val;
    ASSERT_TRUE(config.Get("backbone", "uint16", uint16_val));
    ASSERT_FALSE(config.Get("backbone", "uint161", uint16_val));
    ASSERT_EQ(uint16_val, 1);

    int32_t int32_val;
    ASSERT_TRUE(config.Get("backbone", "int32", int32_val));
    ASSERT_FALSE(config.Get("backbone", "int321", int32_val));
    ASSERT_EQ(int32_val, 1);
    uint32_t uint32_val;
    ASSERT_TRUE(config.Get("backbone", "uint32", uint32_val));
    ASSERT_FALSE(config.Get("backbone", "uint321", uint32_val));
    ASSERT_EQ(uint32_val, 1);
    int64_t int64_val;
    ASSERT_TRUE(config.Get("backbone", "int64", int64_val));
    ASSERT_FALSE(config.Get("backbone", "int641", int64_val));
    ASSERT_EQ(int64_val, 1);
    uint64_t uint64_val;
    ASSERT_TRUE(config.Get("backbone", "uint64", uint64_val));
    ASSERT_FALSE(config.Get("backbone", "uint641", uint64_val));
    ASSERT_EQ(uint64_val, 1);
    float float_val;
    ASSERT_TRUE(config.Get("backbone", "float", float_val));
    ASSERT_FALSE(config.Get("backbone", "float1", float_val));
    ASSERT_EQ(float_val, 1.0f);  // problem 
    double double_val;
    ASSERT_TRUE(config.Get("backbone", "double", double_val));
    ASSERT_FALSE(config.Get("backbone", "double1", double_val));
    ASSERT_EQ(double_val, 1.0);  // problem 
    ASSERT_TRUE(config.DumpConfig("/tmp/dum.conf"));
}


TEST_F(TestTopConfig, Set_) {
    top::base::Config config;
    ASSERT_TRUE(config.Set("node", "public_endpoints", "1234"));
    std::string value;
    ASSERT_TRUE(config.Get("node", "public_endpoints", value));
    ASSERT_EQ("1234", value);
}

}  // namespace test

}  // namespace top
