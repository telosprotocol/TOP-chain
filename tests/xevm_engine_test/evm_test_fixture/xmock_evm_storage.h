#pragma once

#include "xevm_contract_runtime/xevm_storage_base.h"

#include <map>

NS_BEG4(top, contract_runtime, evm, tests)

class xmock_evm_storage : public xtop_evm_storage_base {
public:
    xmock_evm_storage() = default;
    xmock_evm_storage(xmock_evm_storage const &) = delete;
    xmock_evm_storage & operator=(xmock_evm_storage const &) = delete;
    xmock_evm_storage(xmock_evm_storage &&) = default;
    xmock_evm_storage & operator=(xmock_evm_storage &&) = default;
    ~xmock_evm_storage() override = default;

    xbytes_t storage_get(xbytes_t const & key) override;

    void storage_set(xbytes_t const & key, xbytes_t const & value) override;

    void storage_remove(xbytes_t const & key) override;

    void debug(storage_key_type const & debug_type = storage_key_type::ALL) {
        for (auto & pair : ext_kv_datas) {
            auto decode_key = decode_key_type(pair.first);
            if (debug_type != storage_key_type::ALL && decode_key.key_type != debug_type) {
                continue;
            }
            switch (decode_key.key_type) {
            case storage_key_type::Nonce:
                printf("[key - nonce]: %s ", decode_key.address.c_str());
                break;
            case storage_key_type::Balance:
                printf("[key - balance]: %s ", decode_key.address.c_str());
                break;
            case storage_key_type::Code:
                printf("[key - code]: %s ", decode_key.address.c_str());
                break;
            case storage_key_type::Storage:
                printf("[key - storage]: %s ", decode_key.address.c_str());
                break;
            case storage_key_type::Generation:
                printf("[key - generation]: %s ", decode_key.address.c_str());
                break;
            default:
                printf("[key - unknown]: ");
            }

            printf(" extra_key.size(): %zu %s \n", decode_key.extra_key.size(), decode_key.extra_key.c_str());
            printf("[     value]: ");
            for (auto & c : pair.second) {
                printf("%02x", c);
            }
            printf("\n");
        }
    }

private:
    std::map<std::vector<uint8_t>, std::vector<uint8_t>> ext_kv_datas;
};

NS_END4