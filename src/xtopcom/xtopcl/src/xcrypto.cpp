#include "xcrypto.h"

#include "base/utility.h"
#include "cryptopp/filters.h"
#include "cryptopp/hkdf.h"
#include "cryptopp/modes.h"
#include "cryptopp/osrng.h"
#include "cryptopp/sha3.h"
#include "global_definition.h"
#include "user_info.h"
#include "xcrypto_util.h"
#include "xrpc/xuint_format.h"
#include "xpbase/base/top_utils.h"

#include <dirent.h>
#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

namespace xChainSDK {
namespace xcrypto {

using namespace top::utl;
using namespace top::xrpc;
using std::cout;
using std::endl;
using std::string;

static bool g_is_key = false;
static string g_owner_account;

std::string get_keystore_filepath(string & dir, const string & account) {
    // create keystore directory
    std::string cmd = "mkdir -p ";
    if (dir == "") {
        cmd += g_keystore_dir;
        dir = g_keystore_dir;
    } else {
        cmd += dir;
    }
    std::system(cmd.c_str());

    // create keystore file
    std::string file_name = account;
    if (g_is_key) {
        file_name = file_name.substr(TOP_ACCOUNT_PREFIX.size());
    }
    auto path = dir + "/" + file_name;

    return path;
}

string create_new_keystore(const string & pw, string & dir, bool is_key, string account) {
    g_is_key = is_key;
    g_owner_account = account;

    // create new pub pri key pair
    if (g_userinfo.account.size() != 0) {
        copy_g_userinfo = g_userinfo;
    }

    xcrypto_util::make_private_key(g_userinfo.private_key);
    g_userinfo.account = xcrypto_util::make_eth_address_by_assigned_key(g_userinfo.private_key);
    //auto base64_pri = utility::base64_encode(g_userinfo.private_key.data(), PRI_KEY_LEN);
    std::string hex_pri((char*)g_userinfo.private_key.data(), PRI_KEY_LEN);
    hex_pri = top::HexEncode(hex_pri);

    auto path = get_keystore_filepath(dir, g_userinfo.account);
    std::ofstream key_file(path, std::ios::out | std::ios::trunc);
    if (!key_file) {
        std::cout << "Open Key File: " << path << " Error" << std::endl;
    }

    aes256_cbc_encrypt(pw, hex_pri, key_file);
    return path;
}

string create_new_keystore(const string & pw, string & dir, const string & str_pri, bool is_key, string account) {
    g_is_key = is_key;

    if (g_userinfo.account.size() != 0) {
        copy_g_userinfo = g_userinfo;
    }

    set_g_userinfo(str_pri);

    auto path = get_keystore_filepath(dir, g_userinfo.account);
    std::ofstream key_file(path, std::ios::out | std::ios::trunc);
    if (!key_file) {
        std::cout << "Open Key File: " << path << " Error" << std::endl;
    }

    aes256_cbc_encrypt(pw, str_pri, key_file);
    return path;
}

void generate_keystore_mac(CryptoPP::byte * mac, CryptoPP::byte * key, const string & ciphertext) {
    CryptoPP::SHA3_Final<mac_len> sha3;
    std::vector<uint8_t> vec;
    for (size_t i = aes_key_len; i < derived_key_len; ++i) {
        vec.push_back(key[i]);
    }
    for (auto c : ciphertext) {
        vec.push_back(c);
    }
    sha3.Update(vec.data(), vec.size());
    sha3.TruncatedFinal(mac, mac_len);
}

void fill_aes_info(const std::string & pw, const string & raw_text, AES_INFO & aes_info) {
    CryptoPP::AutoSeededRandomPool rnd;

    // generate salt
    rnd.GenerateBlock(aes_info.salt, salt_len);
    // generate hkdf param: info
    rnd.GenerateBlock(aes_info.info, hkdf_info_len);

    // generate derived key by hdkf using input password
    CryptoPP::byte key[derived_key_len];
    CryptoPP::HKDF<CryptoPP::SHA3_Final<derived_key_len>> hkdf;
    hkdf.DeriveKey(key, derived_key_len, (const uint8_t *)(pw.data()), pw.size(), aes_info.salt, salt_len, aes_info.info, hkdf_info_len);

    // generate aes iv
    rnd.GenerateBlock(aes_info.iv, aes_iv_len);
    // encrypt raw text
    CryptoPP::AES::Encryption aesEncryption(key, aes_key_len);
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, aes_info.iv);
    CryptoPP::StreamTransformationFilter stfEncryptor(cbcEncryption, new CryptoPP::StringSink(aes_info.ciphertext));
    stfEncryptor.Put(reinterpret_cast<const unsigned char *>(raw_text.c_str()), raw_text.length());
    stfEncryptor.MessageEnd();

    // generate mac
    generate_keystore_mac(aes_info.mac, key, aes_info.ciphertext);
}

void aes256_cbc_encrypt(const std::string & pw, const string & raw_text, std::ofstream & key_file) {
    AES_INFO aes_info;
    fill_aes_info(pw, raw_text, aes_info);
    writeKeystoreFile(key_file, aes_info.iv, aes_info.ciphertext, aes_info.info, aes_info.salt, aes_info.mac);
}

void writeKeystoreFile(std::ofstream & key_file, byte * iv, const string & ciphertext, byte * info, byte * salt, byte * mac) {
    xJson::Value key_info;
    if (!g_is_key) {
        key_info["account address"] = g_userinfo.account;
        key_info["key_type"] = "owner";
    } else {
        key_info["account address"] = g_owner_account;
        key_info["key_type"] = "worker";
    }
    api_method_imp api;

    xecprikey_t pri_key_obj(g_userinfo.private_key.data());
    xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
    //auto base64_pub = utility::base64_encode(pub_key_obj.data(), pub_key_obj.size());
    std::string str_pub = top::HexEncode(std::string((char*)pub_key_obj.data()+1, pub_key_obj.size()-1));
    key_info["public_key"] = str_pub;

    // cipher
    key_info["crypto"]["cipher"] = "aes-256-cbc";
    key_info["crypto"]["cipherparams"]["iv"] = uint_to_str(iv, aes_iv_len);
    auto cipher_hex = uint_to_str(ciphertext.data(), ciphertext.size());
    key_info["crypto"]["ciphertext"] = cipher_hex;

    // kdf
    key_info["crypto"]["kdf"] = "hkdf";
    key_info["crypto"]["kdfparams"]["prf"] = "sha3-256";
    key_info["crypto"]["kdfparams"]["salt"] = uint_to_str(salt, salt_len);
    key_info["crypto"]["kdfparams"]["info"] = uint_to_str(info, hkdf_info_len);
    key_info["crypto"]["kdfparams"]["dklen"] = derived_key_len;

    // mac
    auto mac_hex = uint_to_str(mac, mac_len);
    key_info["crypto"]["mac"] = mac_hex;

    // hint
    key_info["hint"] = g_pw_hint;

    key_file << key_info.toStyledString();
}

void update_keystore_file(const std::string & pw, const string & raw_text, std::ofstream & key_file, xJson::Value & key_info) {
    AES_INFO aes_info;
    fill_aes_info(pw, raw_text, aes_info);

    key_info["crypto"]["cipherparams"]["iv"] = uint_to_str(aes_info.iv, aes_iv_len);

    key_info["crypto"]["kdfparams"]["salt"] = uint_to_str(aes_info.salt, salt_len);

    key_info["crypto"]["kdfparams"]["info"] = uint_to_str(aes_info.info, hkdf_info_len);

    auto mac_hex = uint_to_str(aes_info.mac, mac_len);
    key_info["crypto"]["mac"] = mac_hex;

    auto cipher_hex = uint_to_str(aes_info.ciphertext.data(), aes_info.ciphertext.size());
    key_info["crypto"]["ciphertext"] = cipher_hex;

    key_info["hint"] = g_pw_hint;

    key_file << key_info.toStyledString();
}

bool set_g_userinfo(const string & str_pri) {
    if (str_pri.empty()) {
        return false;
    }
    if (str_pri.size() == BASE64_PRI_KEY_LEN)
    {
        auto sign_key = utility::base64_decode(str_pri);
        xecprikey_t pri_key_obj((uint8_t *)sign_key.data());
        memcpy(g_userinfo.private_key.data(), pri_key_obj.data(), pri_key_obj.size());
        g_userinfo.account = xcrypto_util::make_address_by_assigned_key(g_userinfo.private_key);
    } else 
    {
        std::string sign_key = top::HexDecode(str_pri);
        xecprikey_t pri_key_obj((uint8_t *)sign_key.data());
        memcpy(g_userinfo.private_key.data(), pri_key_obj.data(), pri_key_obj.size());
        g_userinfo.account = xcrypto_util::make_eth_address_by_assigned_key(g_userinfo.private_key);
    }

    return g_userinfo.account.size() > 0;
}

string import_existing_keystore(const string & cache_pw, const string & path, bool auto_dec) {
    auto key_info = parse_keystore(path);
    if (key_info.empty()) {
        return "";
    }
    auto decrypttext = aes256_cbc_decrypt(cache_pw, key_info);
    if (decrypttext.empty()) {
        if (!auto_dec) {
            cout << "Password error！" << endl;
            cout << "Hint: " << key_info["hint"].asString() << endl;
        }
    }
    return decrypttext;
}

string get_symmetric_ed_key(const string & pw, const string & path) {
    auto key_info = parse_keystore(path);
    if (key_info.empty()) {
        return "";
    }
    // parse hkdf params
    auto salt_hex = key_info["crypto"]["kdfparams"]["salt"].asString();
    auto salt_vec = hex_to_uint(salt_hex);
    auto info_hex = key_info["crypto"]["kdfparams"]["info"].asString();
    auto info_vec = hex_to_uint(info_hex);

    // derive hkdf key
    CryptoPP::byte key[derived_key_len];
    CryptoPP::HKDF<CryptoPP::SHA3_Final<derived_key_len>> hkdf;
    hkdf.DeriveKey(key, derived_key_len, (const uint8_t *)(pw.data()), pw.size(), salt_vec.data(), salt_len, info_vec.data(), hkdf_info_len);

    auto hex_ed_key = uint_to_str(key, aes_key_len);
    return hex_ed_key;
}

string decrypt_keystore_by_key(const string & ed_key, const string & path) {
    auto key_info = parse_keystore(path);
    if (key_info.size() == 0) {
        return "";
    }

    // derive ciphertext
    auto cipher_hex = key_info["crypto"]["ciphertext"].asString();
    auto ciphertext = hex_to_uint8_str(cipher_hex);

    // parse aes param iv
    auto iv_str = key_info["crypto"]["cipherparams"]["iv"].asString();
    auto iv = hex_to_uint(iv_str);
    auto key = hex_to_uint(ed_key);

    CryptoPP::AES::Decryption aesDecryption(key.data(), aes_key_len);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv.data());
    std::string decryptedtext;
    try {
        CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedtext));
        stfDecryptor.Put(reinterpret_cast<const unsigned char *>(ciphertext.c_str()), ciphertext.size());
        stfDecryptor.MessageEnd();
    } catch (std::exception & e) {
        cout << "Decryption Error: " << e.what() << endl;
        return "";
    }

    return decryptedtext;
}

string attach_import_existing_keystore(const string & cache_pw, const string & path, std::ostringstream & out_str) {
    auto key_info = attach_parse_keystore(path, out_str);
    if (key_info.size() == 0) {
        return "";
    }
    auto decrypttext = aes256_cbc_decrypt(cache_pw, key_info);
    if (decrypttext.size() == 0) {
        out_str << "Password error！" << endl;
        out_str << "Hint: " << key_info["hint"].asString() << endl;
    }
    return decrypttext;
}

xJson::Value attach_parse_keystore(const string & path, std::ostringstream & out_str) {
    xJson::Value key_info_js;
    std::ifstream keyfile(path, std::ios::in);
    if (!keyfile) {
        out_str << path << " Not Exist" << std::endl;
        return key_info_js;
    }

    std::stringstream buffer;
    buffer << keyfile.rdbuf();
    string key_info = buffer.str();

    xJson::Reader reader;
    if (!reader.parse(key_info, key_info_js)) {
        out_str << "keyfile " << path << " parse error" << endl;
    }

    return key_info_js;
}

xJson::Value parse_keystore(const string & path) {
    xJson::Value key_info_js;
    std::ifstream keyfile(path, std::ios::in);
    if (!keyfile) {
        std::cout << path << " Not Exist" << std::endl;
        return key_info_js;
    }

    std::stringstream buffer;
    buffer << keyfile.rdbuf();
    string key_info = buffer.str();

    xJson::Reader reader;
    if (!reader.parse(key_info, key_info_js)) {
        std::cout << "keyfile " << path << " parse error" << endl;
    }

    return key_info_js;
}

string aes256_cbc_decrypt(const string & pw, const xJson::Value & key_info) {
    // parse hkdf params
    auto salt_hex = key_info["crypto"]["kdfparams"]["salt"].asString();
    auto salt_vec = hex_to_uint(salt_hex);
    auto info_hex = key_info["crypto"]["kdfparams"]["info"].asString();
    auto info_vec = hex_to_uint(info_hex);

    // derive hkdf key
    CryptoPP::byte key[derived_key_len];
    CryptoPP::HKDF<CryptoPP::SHA3_Final<derived_key_len>> hkdf;
    hkdf.DeriveKey(key, derived_key_len, (const uint8_t *)(pw.data()), pw.size(), salt_vec.data(), salt_len, info_vec.data(), hkdf_info_len);

    // derive ciphertext
    auto cipher_hex = key_info["crypto"]["ciphertext"].asString();
    auto ciphertext = hex_to_uint8_str(cipher_hex);

    // validate mac
    CryptoPP::byte mac[mac_len];
    generate_keystore_mac(mac, key, ciphertext);
    auto mac_hex = uint_to_str(mac, mac_len);
    if (mac_hex != key_info["crypto"]["mac"].asString()) {
        // cout << "Invalid key - MAC mismatch" << endl;
        return "";
    }

    // parse aes param iv
    auto iv_str = key_info["crypto"]["cipherparams"]["iv"].asString();
    auto iv = hex_to_uint(iv_str);

    CryptoPP::AES::Decryption aesDecryption(key, aes_key_len);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, iv.data());
    std::string decryptedtext;
    CryptoPP::StreamTransformationFilter stfDecryptor(cbcDecryption, new CryptoPP::StringSink(decryptedtext));
    stfDecryptor.Put(reinterpret_cast<const unsigned char *>(ciphertext.c_str()), ciphertext.size());
    stfDecryptor.MessageEnd();

    return decryptedtext;
}

bool search_key_by_account(const string & account, const string & dir) {
    auto key_paths = scan_key_dir(dir);
    string key_path;
    xJson::Value key_info;
    bool key_found = false;
    for (auto path : key_paths) {
        key_path = dir + "/" + path;
        key_info = parse_keystore(key_path);
        if (key_info["address"] == account) {
            key_found = true;
            break;
        }
    }
    return key_found;
}

string reset_keystore_pw(const string & old_pw, const string & key_path) {
    xJson::Value key_info = parse_keystore(key_path);

    auto decryptext = aes256_cbc_decrypt(old_pw, key_info);
    if (decryptext.empty()) {
        cout << "Password error！" << endl;
        cout << "Hint: " << key_info["hint"].asString() << endl;
        cout << "Reset password failed." << endl;
        return "";
    }

    ApiMethod api;
    api.is_reset_pw = true;
    if (!api.check_password()) {
        return "";
    }

    std::ofstream key_file(key_path, std::ios::out | std::ios::trunc);
    auto new_pw = api.get_password();

    // reset g_userinfo key
    api_method_imp api_imp;
    if (!set_g_userinfo(decryptext)) {
        std::cout << "reset private key fail." << std::endl;
        return "";
    }

    update_keystore_file(new_pw, decryptext, key_file, key_info);
    std::cout << "Reset password successfully!." << std::endl;
    return new_pw;
}

void attach_reset_keystore_pw(const string & old_pw, const string & pw, const string & key_path, std::ostringstream & out_str) {
    xJson::Value key_info = attach_parse_keystore(key_path, out_str);

    auto decryptext = aes256_cbc_decrypt(old_pw, key_info);
    if (decryptext.size() == 0) {
        out_str << "Password error！" << endl;
        out_str << "Hint: " << key_info["hint"].asString() << endl;
        return;
    }

    std::ofstream key_file(key_path, std::ios::out | std::ios::trunc);

    // reset g_userinfo key
    if (!set_g_userinfo(decryptext)) {
        out_str << "reset private key fail." << std::endl;
        return;
    }

    aes256_cbc_encrypt(pw, decryptext, key_file);
    out_str << "Reset password successfully!." << std::endl;
}

std::vector<std::string> scan_key_dir(const std::string & path) {
    std::vector<std::string> keys;
    DIR * dir = opendir(path.c_str());
    if (dir == nullptr) {
        return keys;
    }
    struct dirent * filename;
    while ((filename = readdir(dir)) != nullptr) {
        if (strcmp(filename->d_name, ".") == 0 || strcmp(filename->d_name, "..") == 0)
            continue;
        struct ::stat fs;
        std::string file_path = path + "/" + filename->d_name;
        if (::stat(file_path.c_str(), &fs) == 0 && S_ISREG(fs.st_mode)) {
            keys.push_back(std::string(filename->d_name));
        }
    }
    return keys;
}

}  // namespace xcrypto
}  // namespace xChainSDK
