#include "xchaininit/xinit.h"
#include "xchaininit/xchain_command.h"
#include "xchaininit/version.h"

#include <string>
#include <fstream>
#include <algorithm>

extern "C" {
    int init_component(const char *config_file, const char *config_file_extra);
    int init_noparams_component(
            const char *pub_key,
            const uint16_t pub_len,
            const char *pri_key,
            const uint16_t pri_len,
            const char *node_id,
            const char *datadir,
            const char *config_file_extra);
	int get_version();
    int parse_execute_command(const char *config_file_extra, int argc, char **argv);
    int decrypt_keystore(
            const char *keystore_path,
            const char *password,
            uint8_t *pri_key,
            uint32_t &pri_key_size);
    int decrypt_keystore_by_key(
            const char *keystore_path,
            const char *token,
            uint8_t *pri_key,
            uint32_t &pri_key_size);
    int check_miner_info(
            const char *pub_key,
            const uint16_t pub_len,
            const char *node_id,
            uint8_t *miner_type);
}

int init_component(const char *config_file, const char *config_file_extra) {
    top::print_version();
    return top::topchain_init(config_file, config_file_extra);
}

int init_noparams_component(
        const char *pub_key,
        const uint16_t pub_len,
        const char *pri_key,
        const uint16_t pri_len,
        const char *node_id,
        const char *datadir,
        const char *config_file_extra) {
    top::print_version();

    return top::topchain_noparams_init(
            std::string(pub_key, pub_len),
            std::string(pri_key, pri_len),
            std::string(node_id),
            std::string(datadir),
            std::string(config_file_extra));
}

int get_version() {
    top::print_version();
    _exit(0);
}

int parse_execute_command(const char *config_file_extra, int argc, char **argv) {
    return top::parse_execute_command(config_file_extra, argc, argv);
}

int decrypt_keystore(
        const char *keystore_path,
        const char *password,
        uint8_t *pri_key,
        uint32_t &pri_key_size) {
    auto private_key = top::decrypt_keystore(keystore_path, password);
    if (private_key.empty()) {
        pri_key_size = 0;
        return -1;
    }
    memcpy(pri_key, private_key.data(), private_key.size());
    pri_key_size = private_key.size();
    return 0;
}

int decrypt_keystore_by_key(
        const char *keystore_path,
        const char *token,
        uint8_t *pri_key,
        uint32_t &pri_key_size) {
    auto private_key = top::decrypt_keystore_by_key(keystore_path, token);
    if (private_key.empty()) {
        pri_key_size = 0;
        return -1;
    }
    memcpy(pri_key, private_key.data(), private_key.size());
    pri_key_size = private_key.size();
    return 0;
}

int check_miner_info(
        const char *pub_key,
        const uint16_t pub_len,
        const char *node_id,
        uint8_t *miner_type) {
    std::string type;
    bool status = top::check_miner_info(std::string(pub_key, pub_len), node_id, type);
    if (!status) {
        return -1;
    }
    strcpy((char*)miner_type, (char*)type.c_str());
    return 0;
}
