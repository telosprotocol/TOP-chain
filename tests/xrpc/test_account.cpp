#include <memory>
#include "gtest/gtest.h"
#include "xrpc/xedge/xedge_local_method.hpp"
#include "xrpc/xedge/xedge_rpc_handler.h"

using namespace top;
using namespace top::xrpc;
using std::unique_ptr;
class test_account : public testing::Test {
 protected:
    void SetUp() override {
        m_edge_local_method_ptr = top::make_unique<xedge_local_method<xedge_http_handler>>();
    }

    void TearDown() override {
    }
 public:
    unique_ptr<xedge_local_method<xedge_http_handler>> m_edge_local_method_ptr;
};

TEST_F(test_account, account) {
    xjson_proc_t json_proc;
    xJson::Value jsonValue;
    uint16_t network_id = 1;
    uint8_t  address_type = 1;
    jsonValue["network_id"] = network_id;
    jsonValue["address_type"] = address_type;
    json_proc.m_request_json["params"] = jsonValue;
    m_edge_local_method_ptr->account_method(json_proc);
    EXPECT_EQ(1, xedge_local_method<xedge_http_handler>::m_account_key_map.size());

    uint16_t tmp_network_id{0};
    uint8_t  tmp_address_type{0};
    for(auto& key : xedge_local_method<xedge_http_handler>::m_account_key_map) {
        auto& addr = key.first;
        utl::xkeyaddress_t key_address(addr);
        EXPECT_EQ(true, key_address.get_type_and_netid(tmp_address_type, tmp_network_id));
        EXPECT_EQ(network_id, tmp_network_id);
        EXPECT_EQ(address_type, tmp_address_type);

                // parent account;
        utl::xecprikey_t pri_key_obj;
        utl::xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
        xJson::Value dataJson;
        dataJson["private_key"] = uint_to_str(pri_key_obj.data(), pri_key_obj.size());
        auto sub_addr = pub_key_obj.to_address(addr, address_type, network_id);

        uint256_t hash;
        utl::xecdsasig_t signature = pri_key_obj.sign(hash);
        //string signature_str((char*)signature.get_compact_signature(), signature.get_compact_signature_size());

        utl::xkeyaddress_t key_address2(sub_addr);

        bool valid = key_address2.verify_signature(signature, hash, addr);
        EXPECT_EQ(true , valid);
        valid = key_address2.verify_signature(signature, hash);
        EXPECT_EQ(false , valid);
    }



}
