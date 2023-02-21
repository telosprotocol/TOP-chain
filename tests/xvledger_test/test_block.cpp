#include "gtest/gtest.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvblockbuild.h"
#include "xvledger/xvaccount.h"
#include "xvledger/xvbindex.h"

using namespace top;
using namespace top::base;

class test_block : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_block, clock_to_gmtime) {
    {
        uint64_t _clock = 0;
        uint64_t _time = base::clock_to_gmtime(_clock);
        ASSERT_EQ(_time, base::TOP_BEGIN_GMTIME + 10*_clock);
    }
    {
        uint64_t _clock = 360;
        uint64_t _time = base::clock_to_gmtime(_clock);
        ASSERT_EQ(_time, base::TOP_BEGIN_GMTIME + 10*_clock);
    }
    {
        uint64_t _clock = 10000000000;
        uint64_t _time = base::clock_to_gmtime(_clock);
        ASSERT_EQ(_time, base::TOP_BEGIN_GMTIME + 10*_clock);
    }
}

TEST_F(test_block, header_create) {
    std::string account = "Ta0000@0";
    uint64_t height = 1;
    std::string last_block_hash = "111";
    std::string last_full_block_hash = "222";
    uint64_t _clock = 1000000000;
    base::xbbuild_para_t _para(base::enum_main_chain_id, account, height, base::enum_xvblock_class_light, base::enum_xvblock_level_table, base::enum_xvblock_type_general, last_block_hash, last_full_block_hash);
    _para.set_table_cert_para(_clock, 1, 1, {2,2}, {2,2}, 1, {}, false);
    
    xvblockbuild_t _blockbuild;
    _blockbuild.init_header(_para);
    auto _header = _blockbuild.get_header();

    xvblockbuild_t _blockbuild2;
    _blockbuild2.init_header(_para);
    auto _header2 = _blockbuild2.get_header();

    std::cout << "_header:" << _header->dump() << std::endl;
    std::cout << "_header2:" << _header2->dump() << std::endl;
    ASSERT_TRUE(_header->is_equal(*_header2));
    // ASSERT_FALSE(_header->is_valid());

    std::string vheader_bin;
    _header->serialize_to_string(vheader_bin);    

    base::xauto_ptr<xvheader_t> _header3 = base::xvblock_t::create_header_object(vheader_bin);
    ASSERT_NE(_header3, nullptr);
    ASSERT_TRUE(_header->is_equal(*_header3));

    ASSERT_NE(_header3->query_interface(base::enum_xobject_type_vheader), nullptr);
    ASSERT_EQ(_header3->query_interface(base::enum_xobject_type_vqccert), nullptr);

    base::xauto_ptr<xvheader_t> _header4 = new xvheader_t(*_header3);
    ASSERT_TRUE(_header->is_equal(*_header4));

    base::xauto_ptr<xvheader_t> _header5 = _header->clone();
    ASSERT_TRUE(_header->is_equal(*_header5));
}

TEST_F(test_block, qcert_create) {
    std::string account = "Ta0000@0";
    uint64_t height = 1;
    std::string last_block_hash = "111";
    std::string last_full_block_hash = "222";
    uint64_t _clock = 1000000000;
    xvip2_t validator_xip{2,2};
    set_group_nodes_count_to_xip2(validator_xip, 10);
    xvip2_t auditor_xip{2,2};
    set_group_nodes_count_to_xip2(auditor_xip, 20);
    base::xbbuild_para_t _para(base::enum_main_chain_id, account, height, base::enum_xvblock_class_light, base::enum_xvblock_level_table, base::enum_xvblock_type_general, last_block_hash, last_full_block_hash);
    _para.set_table_cert_para(_clock, 1, 1, validator_xip, auditor_xip, 1, {}, false);
    
    xvblockbuild_t _blockbuild;
    _blockbuild.init_header(_para);
    _blockbuild.init_qcert(_para);
    auto _qcert = _blockbuild.get_qcert();

    xvblockbuild_t _blockbuild2;
    _blockbuild2.init_header(_para);
    _blockbuild2.init_qcert(_para);
    auto _qcert2 = _blockbuild2.get_qcert();

    std::cout << "_qcert:" << _qcert->dump() << std::endl;
    std::cout << "_qcert2:" << _qcert2->dump() << std::endl;
    ASSERT_TRUE(_qcert->is_equal(*_qcert2));

    std::string vqcert_bin;
    _qcert->serialize_to_string(vqcert_bin);    

    base::xauto_ptr<xvqcert_t> _qcert3 = base::xvblock_t::create_qcert_object(vqcert_bin);
    ASSERT_NE(_qcert3, nullptr); 
    ASSERT_TRUE(_qcert->is_equal(*_qcert3));

    ASSERT_EQ(_qcert3->query_interface(base::enum_xobject_type_vheader), nullptr);
    ASSERT_NE(_qcert3->query_interface(base::enum_xobject_type_vqccert), nullptr);

    base::xauto_ptr<xvqcert_t> _qcert4 = new xvqcert_t(*_qcert3);
    ASSERT_TRUE(_qcert->is_equal(*_qcert4));

    xauto_ptr<xvqcert_t> _qcert5 = _qcert4->clone();
    ASSERT_TRUE(_qcert->is_equal(*_qcert5));

    std::string _empty_str;
    ASSERT_EQ(_empty_str, _qcert->hash(_empty_str));

    ASSERT_EQ(_qcert->get_validator_threshold(), 10*2/3+1);
    ASSERT_EQ(_qcert->get_auditor_threshold(), 20*2/3+1);

    ASSERT_FALSE(_qcert->get_hash_to_sign().empty());
}
TEST_F(test_block, block_create) {
    std::string account = "Ta0000@0";
    uint64_t height = 1;
    std::string last_block_hash = "111";
    std::string last_full_block_hash = "222";
    uint64_t _clock = 1000000000;
    base::xbbuild_para_t _para(base::enum_main_chain_id, account, height, base::enum_xvblock_class_light, base::enum_xvblock_level_table, base::enum_xvblock_type_general, last_block_hash, last_full_block_hash);
    _para.set_table_cert_para(_clock, 1, 1, {2,2}, {2,2}, 1, {}, false);
    
    xvblockmaker_t _blockbuild;
    _blockbuild.init_header(_para);
    _blockbuild.init_qcert(_para);
    base::xvaction_t _action("000", "000", "000", "000");
    _blockbuild.set_input_entity(_action);
    _blockbuild.set_input_resource("1111","1111");
    _blockbuild.set_output_resource("2222", "2222");
    _blockbuild.set_output_full_state_hash("3333");
    std::string binlog = "4444";
    _blockbuild.set_output_binlog(binlog);

    base::xauto_ptr<base::xvblock_t> _block = _blockbuild.build_new_block();
    ASSERT_NE(_block, nullptr);

    ASSERT_TRUE(_block->get_header()->is_valid());
    ASSERT_TRUE(_block->get_cert()->is_valid());

    bool include_resource = _block->get_header()->is_character_cert_header_only();
    ASSERT_TRUE(include_resource);
    auto _input = _blockbuild.get_input();
    std::string input_bin;
    ASSERT_TRUE(_input->serialize_to_string(include_resource, input_bin) > 0);
    base::xauto_ptr<xvinput_t> _input3 = base::xvblock_t::create_input_object(include_resource, input_bin);
    ASSERT_NE(_input3, nullptr); 
    ASSERT_EQ(_block->get_input_hash(), _block->get_cert()->hash(input_bin));
    ASSERT_EQ(_input->get_action_count(), 1);

    auto _output = _blockbuild.get_output();
    std::string output_bin;
    ASSERT_TRUE(_output->serialize_to_string(include_resource, output_bin) > 0);
    base::xauto_ptr<xvoutput_t> _output3 = base::xvblock_t::create_output_object(include_resource, output_bin);
    ASSERT_NE(_output3, nullptr); 
    ASSERT_EQ(_block->get_output_hash(), _block->get_cert()->hash(output_bin));
    ASSERT_NE(_block->get_output_hash(), _block->get_input_hash());

    std::string object_bin;
    ASSERT_TRUE(_block->serialize_to_string(object_bin) > 0);
    base::xauto_ptr<base::xvblock_t> _block2 = base::xvblock_t::create_block_object(object_bin, true);
    ASSERT_NE(_block2, nullptr); 

    base::xauto_ptr<base::xvblock_t> cloned_block(_block->clone_block());
    ASSERT_TRUE(cloned_block->close(true));

    ASSERT_EQ(_block->get_binlog(), binlog);
    ASSERT_EQ(_block->get_binlog_hash(), _block->get_cert()->hash(binlog));
    ASSERT_EQ(_block->get_pledge_balance_change_tgas(), 0);
    std::string account_indexs_str = _block->get_account_indexs();
    ASSERT_TRUE(account_indexs_str.empty());
    ASSERT_TRUE(_block->get_block_size() > 0);
    ASSERT_TRUE(_block->is_body_and_offdata_ready());

    _block->set_verify_signature("1");
    _block->set_audit_signature("1");
    ASSERT_EQ(_block->get_second_level_gmtime(), _block->get_timestamp());

    std::error_code ec;
    ASSERT_TRUE(_block->set_input_data(_block->get_input_data(), true));
    ASSERT_TRUE(_block->set_output_data(_block->get_output_data(), true));
    ASSERT_TRUE(_block->set_output_offdata(_block->get_output_offdata(), true));
    ASSERT_TRUE(_block->set_input_output(_block->load_input(ec), _block->load_output(ec)));
    ASSERT_EQ(_block->query_input_resource("1111"), "1111");
    ASSERT_EQ(_block->query_output_resource("2222"), "2222");
}

TEST_F(test_block, block_create_genesis) {
    std::string account = "Ta0000@0";
    std::string last_block_hash = "111";
    uint64_t _clock = 1000000000;
    base::xbbuild_para_t _para(base::enum_main_chain_id, account, base::enum_xvblock_level_table, base::enum_xvblock_class_light, last_block_hash);
    // _para.set_table_cert_para(_clock, 1, 1, {2,2}, {2,2}, 1, {}, false);
    
    xvblockmaker_t _blockbuild;
    _blockbuild.init_header(_para);
    _blockbuild.init_qcert(_para);
    base::xvaction_t _action("000", "000", "000", "000");
    _blockbuild.set_input_entity(_action);
    _blockbuild.set_input_resource("1111","1111");
    _blockbuild.set_output_resource("2222", "2222");
    _blockbuild.set_output_full_state_hash("3333");
    std::string binlog = "4444";
    _blockbuild.set_output_binlog(binlog);

    base::xauto_ptr<base::xvblock_t> _block = _blockbuild.build_new_block();
    ASSERT_NE(_block, nullptr);

    ASSERT_TRUE(_block->get_header()->is_valid());
    ASSERT_TRUE(_block->get_cert()->is_valid());
    ASSERT_TRUE(_block->is_genesis_block());
}

TEST_F(test_block, block_create_2) {
    std::string account = "Ta0000@0";
    uint64_t height = 1;
    std::string last_block_hash = "111";
    std::string last_full_block_hash = "222";
    uint64_t _clock = 1000000000;
    base::xbbuild_para_t _para(base::enum_main_chain_id, account, height, base::enum_xvblock_class_light, base::enum_xvblock_level_table, base::enum_xvblock_type_general, last_block_hash, last_full_block_hash);
    _para.set_table_cert_para(_clock, 1, 1, {2,2}, {2,2}, 1, {}, false);
    
    xvblockmaker_t _blockbuild;
    _blockbuild.init_header(_para);
    _blockbuild.init_qcert(_para);
    base::xvaction_t _action("000", "000", "000", "000");
    _blockbuild.set_input_entity(_action);
    _blockbuild.set_input_resource("1111","1111");
    _blockbuild.set_output_resource("2222", "2222");
    _blockbuild.set_output_full_state_hash("3333");
    std::string binlog = "4444";
    _blockbuild.set_output_binlog(binlog);

    base::xauto_ptr<base::xvblock_t> _block = _blockbuild.build_new_block();
    ASSERT_NE(_block, nullptr);

    ASSERT_TRUE(_block->get_header()->is_valid());
    ASSERT_TRUE(_block->get_cert()->is_valid());
    _block->get_cert()->set_parent_height(1);
    _block->get_cert()->set_parent_viewid(1);
    _block->get_cert()->set_nonce(1);
    _block->get_cert()->reset_block_flags();
    // _block->get_cert()->set_extend_cert("1");
    // _block->get_cert()->set_extend_data("1");
    _block->set_verify_signature("1");
    _block->set_audit_signature("1");
    _block->set_block_flag(base::enum_xvblock_flag_authenticated);
    ASSERT_NE(_block->get_block_hash(), std::string());
    ASSERT_TRUE(_block->is_valid(true));
    ASSERT_TRUE(_block->is_deliver(true));
    ASSERT_TRUE(_block->is_input_ready(true));
    ASSERT_TRUE(_block->is_output_ready(true));
    ASSERT_TRUE(_block->is_body_and_offdata_ready(true));

    std::string block_object_bin;
    _block->serialize_to_string(block_object_bin);
    base::xauto_ptr<base::xvblock_t> _block2 = base::xvblock_t::create_block_object(block_object_bin);
    _block2->set_input_data(_block->get_input_data());
    _block2->set_output_data(_block->get_output_data());
    _block2->set_output_offdata(_block->get_output_offdata());
    ASSERT_EQ(_block2->query_input_resource("1111"), "1111");
    ASSERT_EQ(_block2->query_output_resource("2222"), "2222");
    ASSERT_FALSE(_block2->is_genesis_block());
}

TEST_F(test_block, bindex_create) {
    std::string account = "Ta0000@0";
    uint64_t height = 1;
    std::string last_block_hash = "111";
    std::string last_full_block_hash = "222";
    uint64_t _clock = 1000000000;
    base::xbbuild_para_t _para(base::enum_main_chain_id, account, height, base::enum_xvblock_class_light, base::enum_xvblock_level_table, base::enum_xvblock_type_general, last_block_hash, last_full_block_hash);
    _para.set_table_cert_para(_clock, 1, 1, {2,2}, {2,2}, 1, {}, false);
    
    xvblockmaker_t _blockbuild;
    _blockbuild.init_header(_para);
    _blockbuild.init_qcert(_para);
    base::xvaction_t _action("000", "000", "000", "000");
    _blockbuild.set_input_entity(_action);
    _blockbuild.set_input_resource("1111","1111");
    _blockbuild.set_output_resource("2222", "2222");
    _blockbuild.set_output_full_state_hash("3333");
    std::string binlog = "4444";
    _blockbuild.set_output_binlog(binlog);

    base::xauto_ptr<base::xvblock_t> _block = _blockbuild.build_new_block();
    ASSERT_NE(_block, nullptr);

    _block->set_verify_signature("1");
    _block->set_audit_signature("1");
    _block->set_block_flag(base::enum_xvblock_flag_authenticated);
    ASSERT_NE(_block->get_block_hash(), std::string());

    base::xauto_ptr<xvbindex_t> _bindex = new xvbindex_t(*_block);
    std::string bindex_bin;
    ASSERT_TRUE(_bindex->serialize_to(bindex_bin) > 0);
    base::xauto_ptr<xvbindex_t> _bindex2 = new base::xvbindex_t();
    ASSERT_TRUE(_bindex2->serialize_from(bindex_bin) > 0);
}