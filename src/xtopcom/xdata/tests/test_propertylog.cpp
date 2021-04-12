#include <vector>
#include <iostream>

#include "gtest/gtest.h"
#include "xdata/xpropertylog.h"
#include "xbase/xobject_ptr.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"

using namespace top;
using namespace top::data;

class test_propertylog : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

void print_logs(xproperty_log_ptr_t propertylog) {
    auto logs = propertylog->get_instruction();
    for (auto & log : logs) {
        std::cout << "prop_name:" << log.first << std::endl;
        for (auto & instruction : log.second.get_logs()) {
            std::cout << "instruction code;" << instruction.m_op_code << " para1:" << instruction.m_op_para1 << " para2:" << instruction.m_op_para2 << std::endl;
        }
    }
}

TEST_F(test_propertylog, instruction_add_1) {
    xproperty_log_ptr_t propertylog = make_object_ptr<xaccount_binlog_t>();

    ASSERT_EQ(0, propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_list_create));
    ASSERT_EQ(0, propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_list_push_back, "male"));
    ASSERT_EQ(0, propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_list_push_back, "male"));
    ASSERT_EQ(0, propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_list_push_back, "male"));
    ASSERT_EQ(0, propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_list_push_back, "male"));
    print_logs(propertylog);
    ASSERT_EQ(propertylog->get_property_size(), 1);
    ASSERT_EQ(propertylog->get_instruction_size(), 5);
    xhash_base_t::calc_dataunit_hash(propertylog.get());
}


TEST_F(test_propertylog, instruction_add_2) {
    xproperty_log_ptr_t propertylog = make_object_ptr<xaccount_binlog_t>();

    ASSERT_EQ(0, propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_map_create));
    ASSERT_EQ(0, propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_map_set, "1", "aaaaa"));
    ASSERT_EQ(0, propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_map_set, "2", "aaaaa"));
    ASSERT_EQ(0, propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_map_set, "3", "aaaaa"));
    ASSERT_EQ(0, propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_map_set, "4", "aaaaa"));

    print_logs(propertylog);
    ASSERT_EQ(propertylog->get_property_size(), 1);
    ASSERT_EQ(propertylog->get_instruction_size(), 5);
    xhash_base_t::calc_dataunit_hash(propertylog.get());
}

TEST_F(test_propertylog, instruction_add_3) {
    xproperty_log_ptr_t propertylog = make_object_ptr<xaccount_binlog_t>();
    propertylog->add_instruction("prop1", xproperty_op_code_t::xproperty_cmd_type_string_create);
    propertylog->add_instruction("prop1", xproperty_op_code_t::xproperty_cmd_type_string_set, "111");
    propertylog->add_instruction("prop1", xproperty_op_code_t::xproperty_cmd_type_string_set, "222");
    propertylog->add_instruction("prop1", xproperty_op_code_t::xproperty_cmd_type_string_set, "333");

    propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_list_push_back, "male");
    propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_list_pop_back, "female");
    propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_list_push_back, "male");

    propertylog->add_instruction("prop3", xproperty_op_code_t::xproperty_cmd_type_list_push_front, "male");
    propertylog->add_instruction("prop3", xproperty_op_code_t::xproperty_cmd_type_list_pop_front, "female");
    propertylog->add_instruction("prop3", xproperty_op_code_t::xproperty_cmd_type_list_push_front, "male");

    propertylog->add_instruction("prop4", xproperty_op_code_t::xproperty_cmd_type_list_push_front, "male");
    propertylog->add_instruction("prop4", xproperty_op_code_t::xproperty_cmd_type_list_pop_front, "male");

    propertylog->add_instruction("prop5", xproperty_op_code_t::xproperty_cmd_type_map_set, "1", "aaaaa");
    propertylog->add_instruction("prop5", xproperty_op_code_t::xproperty_cmd_type_map_set, "2", "bbbbb");
    propertylog->add_instruction("prop5", xproperty_op_code_t::xproperty_cmd_type_map_remove, "2");

    propertylog->add_instruction("prop6", xproperty_op_code_t::xproperty_cmd_type_map_set, "2", "bbbbb");
    propertylog->add_instruction("prop6", xproperty_op_code_t::xproperty_cmd_type_map_remove, "2");
    print_logs(propertylog);

    ASSERT_EQ(propertylog->get_property_size(), 4);
    ASSERT_EQ(propertylog->get_instruction_size(), 5);
    xhash_base_t::calc_dataunit_hash(propertylog.get());
}

TEST_F(test_propertylog, instruction_add_4) {
    xproperty_log_ptr_t propertylog = make_object_ptr<xaccount_binlog_t>();
    propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_list_push_back, "11");
    propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_list_push_back, "22");
    propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_list_push_back, "33");
    propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_list_clear);

    propertylog->add_instruction("prop4", xproperty_op_code_t::xproperty_cmd_type_map_set, "1", "aaaaa");
    propertylog->add_instruction("prop4", xproperty_op_code_t::xproperty_cmd_type_map_set, "2", "bbbbb");
    propertylog->add_instruction("prop4", xproperty_op_code_t::xproperty_cmd_type_map_set, "3", "ccccc");
    propertylog->add_instruction("prop4", xproperty_op_code_t::xproperty_cmd_type_map_clear);
    print_logs(propertylog);

    ASSERT_EQ(propertylog->get_property_size(), 2);
    ASSERT_EQ(propertylog->get_instruction_size(), 2);

    xhash_base_t::calc_dataunit_hash(propertylog.get());
}

TEST_F(test_propertylog, proertylog_serialize) {
    xproperty_log_ptr_t propertylog = make_object_ptr<xaccount_binlog_t>();
    propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_list_push_back, "11");
    propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_list_push_back, "22");
    propertylog->add_instruction("prop2", xproperty_op_code_t::xproperty_cmd_type_list_push_back, "33");

    propertylog->add_instruction("prop4", xproperty_op_code_t::xproperty_cmd_type_map_set, "1", "aaaaa");
    propertylog->add_instruction("prop4", xproperty_op_code_t::xproperty_cmd_type_map_set, "2", "bbbbb");
    propertylog->add_instruction("prop4", xproperty_op_code_t::xproperty_cmd_type_map_set, "3", "ccccc");

    std::string hash1 = xhash_base_t::calc_dataunit_hash(propertylog.get());

    base::xstream_t stream(base::xcontext_t::instance());
    propertylog->serialize_to(stream);

    xproperty_log_ptr_t propertylog2 = make_object_ptr<xaccount_binlog_t>();
    auto serialize_size = propertylog2->serialize_from(stream);
    assert(serialize_size > 0);
    ASSERT_EQ(xhash_base_t::calc_dataunit_hash(propertylog2.get()), hash1);
}

