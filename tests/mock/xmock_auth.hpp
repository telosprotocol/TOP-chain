// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvblock.h"
#include "xbase/xutl.h"

namespace top { namespace mock {

class xmock_auth_t : public base::xvcertauth_t {
public:
    xmock_auth_t(const uint32_t total_nodes) {
        m_total_nodes = total_nodes;
    }
    virtual ~xmock_auth_t() {}

    const uint32_t get_total_nodes() const { return m_total_nodes; }  // just for test purpose ,we put total nodes here

    const std::string get_signer(const xvip2_t & signer) override {
        return base::xstring_utl::tostring(signer.low_addr);
    }

    xvip_t get_validator_addr(const std::string & account_addr) override {
        return 0;
    }
    bool verify_validator_addr(const base::xvblock_t * test_for_block) override {
        return true;
    }
    bool verify_validator_addr(const std::string & for_account,const base::xvqcert_t * for_cert) override {
        return true;
    }

    const std::string do_sign(const xvip2_t & signer,const base::xvqcert_t * sign_for_cert,const uint64_t random_seed) override {
        std::string random_seed_string = base::xstring_utl::tostring(base::xtime_utl::get_fast_random64());
        if (random_seed != 0)
            random_seed_string += base::xstring_utl::tostring(random_seed);

        std::string bin;
        ((base::xvqcert_t *) sign_for_cert)->serialize_to_string(bin);
        return bin + random_seed_string;
    }

    const std::string do_sign(const xvip2_t & signer,const base::xvqcert_t * sign_for_cert,
                              const uint64_t random_seed, const std::string sign_hash) override {
        return do_sign(signer, sign_for_cert, random_seed);
    }

    const std::string do_sign(const xvip2_t & signer,const base::xvblock_t * sign_for_block,const uint64_t random_seed) override {
        return do_sign(signer, sign_for_block->get_cert(), random_seed);
    }

    base::enum_vcert_auth_result verify_sign(const xvip2_t & signer,const base::xvqcert_t * test_for_cert,const std::string & block_account) override {
        return base::enum_vcert_auth_result::enum_successful;
    }

    base::enum_vcert_auth_result verify_sign(const xvip2_t & signer,const xvqcert_t * test_for_cert,
                                             const std::string & block_account, const std::string sign_hash) override {
        return base::enum_vcert_auth_result::enum_successful;
    } 

    base::enum_vcert_auth_result verify_sign(const xvip2_t & signer,const base::xvblock_t * test_for_block) override {
        return base::enum_vcert_auth_result::enum_successful;
    }

    base::enum_vcert_auth_result verify_muti_sign(const base::xvqcert_t * test_for_cert, const std::string & block_account) override {
        return base::enum_vcert_auth_result::enum_successful;
    }

    base::enum_vcert_auth_result verify_muti_sign(const base::xvblock_t * test_for_block) override {
        return base::enum_vcert_auth_result::enum_successful;
    }

    const std::string merge_muti_sign(const std::vector<xvip2_t> & muti_nodes,const std::vector<std::string> & muti_signatures,const base::xvqcert_t * for_cert) override {
        std::string bin;
        ((base::xvqcert_t *)for_cert)->serialize_to_string(bin);
        for(auto const& s : muti_signatures) {
            bin += s;
        }
        return bin;
    }

    const std::string merge_muti_sign(const std::map<xvip2_t,std::string,xvip2_compare> & muti_nodes_signatures,const base::xvqcert_t * for_cert) override {
        std::string bin;
        ((base::xvqcert_t *)for_cert)->serialize_to_string(bin);
        for(auto const& pair : muti_nodes_signatures) {
            bin += pair.second;
        }
        return bin;
    }

    const std::string merge_muti_sign(const std::map<xvip2_t,std::string,xvip2_compare> & muti_nodes_signatures,const base::xvblock_t * for_block) override {
        return merge_muti_sign(muti_nodes_signatures, for_block->get_cert());
    }

    const std::string get_pubkey(const xvip2_t & signer) override {
        return {};
    }

private:
    uint32_t m_total_nodes;
};


}
}
