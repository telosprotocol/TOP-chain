#include <iostream>
#include <string>
#include "gtest/gtest.h"
#include "xcrypto/xckey.h"
#include "xbase/xint.h"
#include "xdata/xgenesis_data.h"



using std::string;
using namespace top::utl;
using namespace top;
class test_signer : public testing::Test
{
protected:
  void SetUp() override
  {
  }

  void TearDown() override
  {
  }
};

string binary_2_hex(uint8_t *src, int size)
{
  string dest;
  int i;
  unsigned char symbol[16] = {
      '0',
      '1',
      '2',
      '3',
      '4',
      '5',
      '6',
      '7',
      '8',
      '9',
      'a',
      'b',
      'c',
      'd',
      'e',
      'f',
  };

  dest.resize(size * 2);
  char *append_ptr = &dest[0];

  for (i = 0; i < size; i++)
  {
    *append_ptr++ = symbol[(src[i] & 0xf0) >> 4];
    *append_ptr++ = symbol[src[i] & 0x0f];
  }

  return dest;
}
const char kHexAlphabet[] = "0123456789abcdef";
const char kHexLookup[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15};
std::string HexEncode(const std::string &str)
{
  auto size(str.size());
  std::string hex_output(size * 2, 0);
  for (size_t i(0), j(0); i != size; ++i)
  {
    hex_output[j++] = kHexAlphabet[static_cast<unsigned char>(str[i]) / 16];
    hex_output[j++] = kHexAlphabet[static_cast<unsigned char>(str[i]) % 16];
  }
  return hex_output;
}

std::string HexDecode(const std::string &str)
{
  auto size(str.size());
  if (size % 2)
    return "";

  std::string non_hex_output(size / 2, 0);
  for (size_t i(0), j(0); i != size / 2; ++i)
  {
    non_hex_output[i] = (kHexLookup[static_cast<int>(str[j++])] << 4);
    non_hex_output[i] |= kHexLookup[static_cast<int>(str[j++])];
  }
  return non_hex_output;
}

TEST_F(test_signer, create_private_key)
{
  xecprikey_t privk;
  uint256_t data;
  memset(&data, 0, sizeof(uint256_t));

  uint8_t *p_priv_data = privk.data();
  int priv_size = privk.size();

  string priv_hex_string = binary_2_hex(p_priv_data, priv_size);
  std::cout << "private hexstring:" << priv_hex_string.c_str() << std::endl;

  xecpubkey_t pub_key = privk.get_public_key();
  uint8_t *p_pub_data = pub_key.data();
  int pub_size = pub_key.size();

  string pub_hex_string = binary_2_hex(p_pub_data, pub_size);
  std::cout << "pub hexstring:" << pub_hex_string.c_str() << " address:" << pub_key.to_address((uint8_t)base::enum_vaccount_addr_type_secp256k1_user_account, 1) << std::endl;

  xecdsasig_t sign = privk.sign(data);
  uint8_t *p_comp_data = sign.get_compact_signature();
  int size = sign.get_compact_signature_size();
  string hexString = binary_2_hex(p_comp_data, size);
  std::cout << "hexstring:" << hexString.c_str() << " recover:" << sign.get_recover_id() << std::endl;

  uint8_t *p_raw_data = sign.get_raw_signature();
  size = sign.get_raw_signature_size();
  hexString = binary_2_hex(p_raw_data, size);
  std::cout << "hex raw string:" << hexString.c_str() << std::endl;

  ASSERT_TRUE(!hexString.empty());
}

TEST_F(test_signer, top_key)
{
  uint16_t ledger_id = 0;
  uint8_t address_type = '0';
  for (int i = 0; i < 5; i++)
  {
    utl::xecprikey_t pri_key_obj;
    utl::xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();

    pub_key_obj = pri_key_obj.get_public_key();
    std::string sub_addr = pub_key_obj.to_address(address_type, ledger_id);

    std::cout << "pri key:" << HexEncode(std::string((char *)pri_key_obj.data(), pri_key_obj.size())).c_str() << std::endl;
    std::cout << "pub key:" << HexEncode(std::string((char *)pub_key_obj.data(), pub_key_obj.size())).c_str() << std::endl;
    std::cout << "sub_addr:" << sub_addr.c_str() << std::endl;

    uint256_t hash;
    utl::xecdsasig_t signature = pri_key_obj.sign(hash);

    utl::xkeyaddress_t key_address2(sub_addr);

    bool valid = key_address2.verify_signature(signature, hash);
    EXPECT_EQ(true, valid);
  }
}

TEST_F(test_signer, eth_key)
{
  uint16_t ledger_id = 0;
  uint8_t address_type = '4';
  for (int i = 0; i < 5; i++)
  {
    utl::xecprikey_t pri_key_obj;
    utl::xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();

    pub_key_obj = pri_key_obj.get_public_key();

    std::string sub_addr = pub_key_obj.to_eth_address(address_type, ledger_id);

    std::cout << "pri key:" << HexEncode(std::string((char *)pri_key_obj.data(), pri_key_obj.size())).c_str() << std::endl;
    std::cout << "pub key:" << HexEncode(std::string((char *)pub_key_obj.data(), pub_key_obj.size())).c_str() << std::endl;
    std::cout << "top sub_addr:" << sub_addr.c_str() << std::endl;

    uint256_t hash;
    utl::xecdsasig_t signature = pri_key_obj.sign(hash);

    utl::xkeyaddress_t key_address2(sub_addr);

    bool valid = key_address2.verify_signature(signature, hash);
    EXPECT_EQ(true, valid);
  }
}

TEST_F(test_signer, eth_to_top_key)
{
  uint16_t ledger_id = 1;
  uint8_t address_type = 1;
  std::string pri = HexDecode("aa11f86c198aa30c9d8db628ca7d4c52f81f3cf8168a70e83ff739312e92ccbb");
  utl::xecprikey_t pri_key_obj((uint8_t *)pri.c_str());
  utl::xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
  std::string sub_addr = pub_key_obj.to_address(address_type, ledger_id);
  std::cout << "from eth to top, sub_addr:" << sub_addr.c_str() << std::endl;
  uint256_t hash;
  utl::xecdsasig_t signature = pri_key_obj.sign(hash);

  utl::xkeyaddress_t key_address2(sub_addr);

  bool valid = key_address2.verify_signature(signature, hash);
  EXPECT_EQ(true, valid);
}

TEST_F(test_signer, eth_to_eth_address2)
{
  std::string pri = HexDecode("eaa0ec632007d14c9bbe4c24bed737d384fcc794915743e39e8952b4b5e66ddc");
  utl::xecprikey_t pri_key_obj((uint8_t *)pri.c_str());
  utl::xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();

  //xsha3_256_t hasher;
  //hasher.update();
  std::cout << "pri key:" << HexEncode(std::string((char *)pri.data(), pri.size())).c_str() << std::endl;
  std::cout << "pub key:" << HexEncode(std::string((char *)pub_key_obj.data(), pub_key_obj.size())).c_str() << std::endl;
  //uint256_t hash_value = xsha3_256_t::digest(pub_key_obj.data()+1, pub_key_obj.size()-1);
  //std::cout << "sha3:" << HexEncode(std::string((char *)&hash_value, sizeof(uint256_t))).c_str() << std::endl;

  uint256_t hash_value = xkeccak256_t::digest(pub_key_obj.data()+1, pub_key_obj.size()-1);
  std::cout << "xkeccak256_t, 64bytes:" << HexEncode(std::string((char *)&hash_value, sizeof(uint256_t))).c_str() << std::endl;
 
  std::string eth_address((char*)&hash_value+12, sizeof(hash_value) - 12);
  eth_address = HexEncode(eth_address);
  std::cout << "eth address: 0x" << eth_address.c_str() << std::endl;

  uint16_t ledger_id = 0;
  uint8_t address_type = 1;
  std::string sub_addr = pub_key_obj.to_eth_address(address_type, ledger_id);
  std::cout << "top sub_addr:" << sub_addr.c_str() << std::endl;

  std::string data("17c55b0ea315eca4bdbadbb706e5f9e76050c1b1a55e495d5edc47798f3b273e");
  data = HexDecode(data);
  uint256_t message((uint8_t*)data.c_str()) ; //xsha3_256_t::digest(data.data(), data.size());
  utl::xecdsasig_t signature = pri_key_obj.sign(message);
  std::cout << "eth sign: "<< HexEncode(std::string((char*)signature.get_compact_signature(), 65)) <<std::endl;

  utl::xkeyaddress_t key_address2(sub_addr);
  bool valid = key_address2.verify_signature(signature, message);

  std::cout<<"verify_signature: "<<valid<<std::endl;
  EXPECT_EQ(true, valid);  
}
TEST_F(test_signer, is_valid)
{
  {
    utl::xkeyaddress_t a(std::string("T00000Lfvt98NjvRfnwTFsEpsiFHQ4CxexknFciR"));
    bool valid = a.is_valid();
    std::cout << "valid: " << valid << std::endl;
  }
  {
    utl::xkeyaddress_t a(std::string("T4000066ab344963eaa071f9636faac26b0d1a39900325"));
    bool valid = a.is_valid();
    std::cout << "valid: " << valid << std::endl;
  }
}