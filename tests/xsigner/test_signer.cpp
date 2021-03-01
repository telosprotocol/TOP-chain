#include <iostream>
#include <string>
#include "gtest/gtest.h"
#include "xcrypto/xckey.h"
#include "xbase/xint.h"
#include "xdata/xgenesis_data.h"

using std::string;
using namespace top::utl;
using namespace top;
class test_signer : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

string binary_2_hex(uint8_t* src, int size) {
  string dest;
  int i;
  unsigned char symbol[16] = {
    '0', '1', '2', '3',
    '4', '5', '6', '7',
    '8', '9', 'a', 'b',
    'c', 'd', 'e', 'f',
  };

  dest.resize(size * 2);
  char* append_ptr = &dest[0];

  for (i = 0; i < size; i++) {
    *append_ptr++ = symbol[(src[i] & 0xf0) >> 4];
    *append_ptr++ = symbol[src[i] & 0x0f];
  }

  return dest;
}

TEST_F(test_signer, create_private_key) {
    xecprikey_t privk;
    uint256_t data;
    memset(&data, 0, sizeof(uint256_t));

    uint8_t* p_priv_data = privk.data();
    int priv_size = privk.size();

    string priv_hex_string = binary_2_hex(p_priv_data, priv_size);
    std::cout << "private hexstring:" << priv_hex_string.c_str() << std::endl;

    xecpubkey_t pub_key = privk.get_public_key();
    uint8_t* p_pub_data = pub_key.data();
    int pub_size = pub_key.size();

    string pub_hex_string = binary_2_hex(p_pub_data, pub_size);
    std::cout << "pub hexstring:" << pub_hex_string.c_str() << " address:" << pub_key.to_address((uint8_t) base::enum_vaccount_addr_type_secp256k1_user_account, 1) << std::endl;

    xecdsasig_t sign = privk.sign(data);
    uint8_t* p_comp_data = sign.get_compact_signature();
    int size = sign.get_compact_signature_size();
    string hexString = binary_2_hex(p_comp_data, size);
    std::cout << "hexstring:" << hexString.c_str() << " recover:" << sign.get_recover_id() << std::endl;


    uint8_t* p_raw_data = sign.get_raw_signature();
    size = sign.get_raw_signature_size();
    hexString = binary_2_hex(p_raw_data, size);
    std::cout << "hex raw string:" << hexString.c_str() << std::endl;

    ASSERT_TRUE(!hexString.empty());
}
