#include "cryptopp/aes.h"
#include "json/json.h"

#include <string>

namespace xChainSDK {
namespace xcrypto {
using namespace CryptoPP;
using std::string;
using bytes = std::vector<byte>;

// for eth key: salt(32), pri_key(32), pub_key(64), aes_key(16), mac(32)
// for top key: salt(32), pri_key(32), pub_key(64), aes_key(32), mac(32)
const uint32_t salt_len = 32;
const uint32_t derived_key_len = 64;
const uint32_t aes_key_len = derived_key_len >> 1;
const uint32_t aes_iv_len = 32;
const uint32_t hkdf_info_len = 8;
const uint32_t mac_len = 32;

class AES_INFO {
public:
    uint8_t salt[salt_len];
    uint8_t info[hkdf_info_len];
    CryptoPP::byte iv[aes_iv_len];
    CryptoPP::byte mac[mac_len];
    std::string ciphertext;
};

void fill_aes_info(const std::string & pw, const string & raw_text, AES_INFO & aes_info);
std::string get_keystore_filepath(string & dir, const string & account);
string create_new_keystore(const string & pw, string & dir, bool is_key = false, string account = "");
string create_new_keystore(const string & pw, string & dir, const string & base64_pri, bool is_key = false, string account = "");
void generate_keystore_mac(CryptoPP::byte * mac, CryptoPP::byte * key, const string & ciphertext);
void generate_eth_keystore_mac(CryptoPP::byte * mac, CryptoPP::byte * key, const string & ciphertext);
void aes256_cbc_encrypt(const std::string & pw, const string & raw_text, std::ofstream & key_file);
void eth_aes256_cbc_encrypt(const std::string & pw, const string & raw_text, std::ofstream & key_file);
void writeKeystoreFile(std::ofstream & key_file, byte * iv, const string & ciphertext, byte * info, byte * salt, byte * mac);
void writeEthKeystoreFile(std::ofstream & key_file, byte * iv, const string & ciphertext, byte * info, byte * salt, byte * mac);
void update_keystore_file(const std::string & pw, const string & raw_text, std::ofstream & key_file, xJson::Value &key_info);

string get_symmetric_ed_key(const string & pw, const string & path);
//string get_eth_symmetric_ed_key(const string & pw, const string & path);
string decrypt_keystore_by_key(const string & hex_ed_key, const string & path);
string get_eth_key(const string & ed_key, const xJson::Value & key_info);
string get_top_key(const string & ed_key, const xJson::Value & key_info);
int get_top_ed_key(const string & pw, const xJson::Value & key_info, CryptoPP::byte* key);
int get_eth_ed_key(const string & pw, const xJson::Value & key_info, CryptoPP::byte* key);

string import_existing_keystore(const string & cache_pw, const string & path, bool auto_dec = false);
string attach_import_existing_keystore(const string & cache_pw, const string & path, std::ostringstream & out_str);
xJson::Value parse_keystore(const string & path);
xJson::Value attach_parse_keystore(const string & path, std::ostringstream & out_str);
string aes256_cbc_decrypt(const string & pw, const xJson::Value & key_info);
string eth_aes_decrypt(const string & pw, const xJson::Value & key_info);

void attach_reset_keystore_pw(const string & old_pw, const string & pw, const string & path, std::ostringstream & out_str);
string reset_keystore_pw(const string & pw, const string & path);
std::vector<std::string> scan_key_dir(const std::string & path);
bool search_key_by_account(const string & account, const string & dir);
bool set_g_userinfo(const string & base64_pri);
}  // namespace xcrypto
}  // namespace xChainSDK
