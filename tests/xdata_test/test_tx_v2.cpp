#include "gtest/gtest.h"
#include "xdata/xtransaction_v2.h"
#include "xdata/xtransaction_v1.h"
#include "xdata/xnative_contract_address.h"
#include "xconfig/xconfig_register.h"
#include "xbase/xmem.h"
#include "xcrypto/xckey.h"

using namespace top;
using namespace top::base;
using namespace top::data;

class test_tx_v2 : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_tx_v2, serialize) {
    xtransaction_v2_ptr_t tx = make_object_ptr<xtransaction_v2_t>();
    std::string param = "do not panic";
    std::string target_action_name = "nodeJoinNetwork2";
    tx->make_tx_run_contract(target_action_name, param);
    std::string source_addr = "T8000037d4fbc08bf4513a68a287ed218b0adbd497ef30";
    tx->set_different_source_target_address(source_addr, sys_contract_rec_standby_pool_addr);
    tx->set_fire_and_expire_time(600);
    const uint32_t min_tx_deposit = 100000;
    tx->set_deposit(min_tx_deposit);

    tx->set_last_nonce(0);
    tx->set_digest();
    std::string authorization = "0105c2ba9cd7d9a9b6c27b1c503ffc045846698cdff1492e4b";
    tx->set_authorization(authorization);
    auto tx_hash = tx->get_digest_hex_str();

    base::xstream_t stream(base::xcontext_t::instance());
    tx->do_write(stream);
    std::cout << "run contract tx v2 size: " << stream.size() << std::endl;

    xtransaction_v2_ptr_t tx_r = make_object_ptr<xtransaction_v2_t>();
    tx_r->do_read(stream);
    EXPECT_EQ(tx_r->get_digest_hex_str(), tx_hash);
    EXPECT_EQ(tx_r->source_address().to_string(), source_addr);
    EXPECT_EQ(tx_r->target_address(), rec_standby_pool_contract_address);
    EXPECT_EQ(tx_r->get_deposit(), min_tx_deposit);
    EXPECT_EQ(tx_r->get_authorization(), authorization);
    EXPECT_EQ(tx_r->get_target_action_name(), target_action_name);
}

TEST_F(test_tx_v2, serialize_by_base) {
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_v2_t>();
    std::string param = "do not panic";
    std::string target_action_name = "nodeJoinNetwork2";
    tx->make_tx_run_contract(target_action_name, param);
    std::string source_addr = "T8000037d4fbc08bf4513a68a287ed218b0adbd497ef30";
    tx->set_different_source_target_address(source_addr, sys_contract_rec_standby_pool_addr);
    tx->set_fire_and_expire_time(600);
    const uint32_t min_tx_deposit = 100000;
    tx->set_deposit(min_tx_deposit);

    tx->set_last_nonce(0);
    tx->set_digest();
    std::string authorization = "0105c2ba9cd7d9a9b6c27b1c503ffc045846698cdff1492e4b";
    tx->set_authorization(authorization);
    auto tx_hash = tx->get_digest_hex_str();

    base::xstream_t stream(base::xcontext_t::instance());
    tx->serialize_to(stream);

    std::string data((char*)stream.data(), stream.size());
    xtransaction_ptr_t tx_r;
    auto ret = xtransaction_t::set_tx_by_serialized_data(tx_r, data);
    EXPECT_EQ(tx_r->get_digest_hex_str(), tx_hash);
    EXPECT_EQ(tx_r->source_address().to_string(), source_addr);
    EXPECT_EQ(tx_r->target_address(), rec_standby_pool_contract_address);
    EXPECT_EQ(tx_r->get_deposit(), min_tx_deposit);
    EXPECT_EQ(tx_r->get_authorization(), authorization);
    EXPECT_EQ(tx_r->get_target_action_name(), target_action_name);
}

TEST_F(test_tx_v2, serialize_by_base_transfer) {
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_v2_t>();
    data::xproperty_asset asset_out{0};
    tx->make_tx_transfer(asset_out);
    std::string source_addr = "T8000037d4fbc08bf4513a68a287ed218b0adbd497ef30";
    std::string target_addr = "T80000077ae60e9d17e4f59fd614a09eae3d1312b2041a";
    tx->set_different_source_target_address(source_addr, target_addr);
    tx->set_fire_and_expire_time(600);
    const uint32_t min_tx_deposit = 100000;
    tx->set_deposit(min_tx_deposit);

    tx->set_last_nonce(0);
    std::string last_trans_hash = "0xce27bac30e9d5dd1";
    tx->set_last_hash(data::hex_to_uint64(last_trans_hash));
    tx->set_digest();
    std::string authorization = "0105c2ba9cd7d9a9b6c27b1c503ffc045846698cdff1492e4b";
    tx->set_authorization(authorization);
    auto tx_hash = tx->get_digest_hex_str();

    base::xstream_t stream(base::xcontext_t::instance());
    tx->serialize_to(stream);
    std::cout << "transfer v2 tx size: " << stream.size() << std::endl;

    std::string data((char*)stream.data(), stream.size());
    xtransaction_ptr_t tx_r;
    auto ret = xtransaction_t::set_tx_by_serialized_data(tx_r, data);
    EXPECT_EQ(tx_r->get_digest_hex_str(), tx_hash);
    EXPECT_EQ(tx_r->source_address().to_string(), source_addr);
    EXPECT_EQ(tx_r->target_address().to_string(), target_addr);
    EXPECT_EQ(tx_r->get_deposit(), min_tx_deposit);
    EXPECT_EQ(tx_r->get_authorization(), authorization);
}

TEST_F(test_tx_v2, json) {
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_v2_t>();
    std::string source_addr = "T8000037d4fbc08bf4513a68a287ed218b0adbd497ef30";
    std::string source_action_name = "sss";
    std::string source_action_para = "spspsp";
    std::string target_addr = "T80000077ae60e9d17e4f59fd614a09eae3d1312b2041a";
    std::string target_action_name = "ttt";
    std::string target_action_para = "tptptp";
    xtx_action_info tx_info(source_addr, source_action_name, source_action_para, target_addr, target_action_name, target_action_para);

    enum_xtransaction_type tx_type = xtransaction_type_run_contract;
    uint16_t expire = 100;
    uint32_t deposit = 1000000;
    uint32_t nonce = 0;
    std::string memo = "json test";

    tx->construct_tx(tx_type, expire, deposit, nonce, memo, tx_info);
    tx->set_digest();

    Json::Value jv;
    tx->parse_to_json(jv);
    std::cout << jv.toStyledString() << std::endl;

    xtransaction_ptr_t tx2 = make_object_ptr<xtransaction_v2_t>();
    tx2->construct_from_json(jv);
    EXPECT_EQ(tx2->source_address().to_string(), source_addr);
    EXPECT_EQ(tx2->get_source_action_para(), source_action_para);
    EXPECT_EQ(tx2->get_tx_type(), tx_type);

    tx2->set_digest();
    EXPECT_EQ(tx2->get_digest_hex_str(), tx->get_digest_hex_str());
}


TEST_F(test_tx_v2, person_sign) {
    Json::Value tx_json;
    tx_json["amount"] = 1000000;
    tx_json["edge_nodeid"] = "";
    tx_json["ext"] = "0x01";
    tx_json["note"] = "";
    tx_json["premium_price"] = 0;
    tx_json["last_tx_nonce"] = 0;
    tx_json["receiver_account"] = "T80000328db230b5bb8d5bbe06ae78f365789cce4917f1";
    tx_json["receiver_action_name"] = "";
    tx_json["receiver_action_param"] = "0x";
    tx_json["send_timestamp"] = 1696990677;
    tx_json["sender_account"] = "T80000a7c3afb0803a59b472036a5ddf0d8f0f9bfba290";
    tx_json["sender_action_name"] = "";
    tx_json["sender_action_param"] = "0x";
    tx_json["token_name"] = "";
    tx_json["tx_deposit"] = 100000;
    tx_json["tx_expire_duration"] = 100;
    tx_json["tx_hash"] = "0x998a3665c881b50f8265a2b2bb6502bedb47da212c5bf82b1753ae9dcf4baabb";
    tx_json["tx_len"] = 0;
    tx_json["tx_structure_version"] = 2;
    tx_json["tx_type"] = 4;
    tx_json["authorization"] = "0x0024c9799dc74c9caf6449cc6e9b01c9fb0c596d51e109fb5eab59b18f85d8c56e4c10beac259303a65c5d646b97d8b1c98134f69006f02800f98da1912507913e";

    data::xtransaction_v2_ptr_t tx2 = make_object_ptr<xtransaction_v2_t>();
    tx2->construct_from_json(tx_json);
    auto signature = hex_to_uint(tx_json["authorization"].asString());
    std::string signature_str((char *)signature.data(), signature.size());  // hex_to_uint client send xstream_t data 0xaaaa => string EXPECT_EQ(tx2->get_authorization(),
    EXPECT_EQ(tx2->get_authorization(), signature_str);
    tx2->set_digest(top::data::hex_to_uint256(tx_json["tx_hash"].asString()));
    EXPECT_EQ(tx2->sign_check(), true);
}

TEST_F(test_tx_v2, v2_sign_test) {

    Json::Value tx_json;
    tx_json["amount"] = 100;
    tx_json["edge_nodeid"] = "";
    tx_json["ext"] = "";
    tx_json["note"] = "";
    tx_json["premium_price"] = 0;
    tx_json["last_tx_nonce"] = 1;
    tx_json["receiver_account"] = "T800005ccba4ce46ca0e59174e30259643536d49c72ee8";
    tx_json["receiver_action_name"] =  "";
    tx_json["receiver_action_param"] ="0x";
    tx_json["send_timestamp"] = 1695797312;
    tx_json["sender_account"] = "T800009f05d555fa6cba97549f192a79015a8b0f63918d";
    tx_json["sender_action_name"] = "";
    tx_json["sender_action_param"] = "0x";
    tx_json["token_name"] =  "";
    tx_json["tx_deposit"] = 100000;
    tx_json["tx_expire_duration"] = 100;
    tx_json["tx_hash"] = "0x7af3bdce098d40bc5ea7546aa4a60bdf68fbb314f0330337afe5eed14d29452c";
    tx_json["tx_len"] = 0;
    tx_json["tx_structure_version"] = 2;
    tx_json["tx_type"] = 4;
    tx_json["authorization"] = "0x016935f7410f8fc6a79bf56fb67ed5782d79ae5654c3018ac2a465b3113fceea1d5439ce012cfa73ba28e97036114c9f8f726234b556e67159121c0caaacc9b96b";

    data::xtransaction_v2_ptr_t tx2 = make_object_ptr<xtransaction_v2_t>();
    tx2->construct_from_json(tx_json);
    tx2->set_digest();
    auto signature = hex_to_uint(tx_json["authorization"].asString());
    std::string signature_str((char *)signature.data(), signature.size());  // hex_to_uint client send xstream_t data 0xaaaa => string
    EXPECT_EQ(tx2->get_authorization(), signature_str);
    //EXPECT_EQ(tx2->get_authorization(), tx_json["authorization"].asString());
    EXPECT_EQ("0x"+tx2->get_digest_hex_str(), tx_json["tx_hash"].asString());
    EXPECT_EQ(tx2->sign_check(), true);

}
