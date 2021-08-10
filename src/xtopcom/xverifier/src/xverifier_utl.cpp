// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xlog.h"
#include "xdata/xgenesis_data.h"
#include "xverifier/xverifier_utl.h"
#include "xbase/xutl.h"

NS_BEG2(top, xverifier)

int32_t xtx_utl::address_is_valid(const std::string & addr) {
    bool res = true;
    std::vector<std::string> parts;
    
    try {
        if (base::xstring_utl::split_string(addr,'@',parts) >= 2) {
            int32_t subaddr = base::xstring_utl::toint32(parts[1]);
            if (subaddr >= enum_vledger_const::enum_vbucket_has_tables_count || subaddr < 0) {
                res = false;
            }
            top::utl::xkeyaddress_t  keyaddr{ parts[0] };
            if (!keyaddr.is_valid()) res = false;
        } else {
            top::utl::xkeyaddress_t  keyaddr{ addr };
            if (!keyaddr.is_valid()) res = false;
        }
    } catch (...) {
        res = false;
    }

    if (!res) {
        xwarn("[global_trace][xverifier][address_is_valid][fail] address: %s", addr.c_str());
        return xverifier_error::xverifier_error_addr_invalid;
    }

    if (addr[0] != 'T') {
            xwarn("[global_trace][verifier][address_is_valid]addr invalid: %s", addr.c_str());
            return  xverifier_error::xverifier_error_addr_invalid;
    }

    base::enum_vaccount_addr_type addr_type = base::xvaccount_t::get_addrtype_from_account(addr);
    std::string addrtemp(addr);
    if (addr_type == top::base::enum_vaccount_addr_type_secp256k1_eth_user_account
        && addrtemp.size() > top::base::xvaccount_t::enum_vaccount_address_prefix_size) {
        addrtemp = addrtemp.substr(top::base::xvaccount_t::enum_vaccount_address_prefix_size);
        std::string addrtemp2(addrtemp);
        std::transform(addrtemp.begin(), addrtemp.end(), addrtemp.begin(), ::tolower);
        if (addrtemp != addrtemp2 || is_valid_hex_format(addrtemp) == false) {
            xwarn("[global_trace][verifier][address_is_valid]addr invalid: %s", addr.c_str());
            return  xverifier_error::xverifier_error_addr_invalid;
        }
    }

    xdbg("[global_trace][xverifier][address_is_valid][success] address: %s", addr.c_str());
    return xverifier_error::xverifier_success;
}

int32_t xtx_utl::address_is_valid(common::xnode_id_t const & addr) {

    bool res = true;
    try {
        top::utl::xkeyaddress_t  keyaddr{ addr.value() };
        if (!keyaddr.is_valid()) res = false;

    } catch (...) {
        res = false;
    }

    if (!res) {
        xwarn("[global_trace][xverifier][address_is_valid][fail] address: %s", addr.value().c_str());
        return xverifier_error::xverifier_error_addr_invalid;
    }

    xdbg("[global_trace][xverifier][address_is_valid][success] address: %s", addr.value().c_str());
    return xverifier_error::xverifier_success;
}

int32_t xtx_utl::privkey_pubkey_is_match(std::string const& privkey, std::string const& pubkey) {

    if (PRIKEY_LEN != privkey.size()) {
        xwarn("[global_trace][xverifier][privkey_pubkey_is_match][fail] privkey: %s, pubkey: %s", privkey.c_str(), pubkey.c_str());
        return false;
    }

    bool res = true;
    try {

        uint8_t priv_content[PRIKEY_LEN];
        memcpy(priv_content, privkey.data(), privkey.size());
        top::utl::xecprikey_t ecpriv(priv_content);

        if (COMPRESSED_PUBKEY_LEN == pubkey.size() && pubkey != ecpriv.get_compress_public_key())  res = false;

        if (UNCOMPRESSED_PUBKEY_LEN == pubkey.size()) {

            std::string tmp_pubkey_str = std::string(reinterpret_cast<char*>(ecpriv.get_public_key().data()), UNCOMPRESSED_PUBKEY_LEN);
            if (pubkey != tmp_pubkey_str) res = false;
        }

    } catch (...) {

        res = false;

    }

    if (!res) {
        xwarn("[global_trace][xverifier][privkey_pubkey_is_match][fail] privkey: %s, pubkey: %s", privkey.c_str(), pubkey.c_str());
        return xverifier_error::xverifier_error_priv_pub_not_match;
    }

    xdbg("[global_trace][xverifier][privkey_pubkey_is_match][success] privkey: %s, pubkey: %s", privkey.c_str(), pubkey.c_str());
    return xverifier_error::xverifier_success;

}

uint64_t xtx_utl::get_gmttime_s() {
    struct timeval val;
    base::xtime_utl::gettimeofday(&val);
    return (uint64_t)val.tv_sec;
}

// judge sendtx whether by normal contract(T-3)
int32_t  xtx_utl::judge_normal_contract_sendtx(data::xtransaction_ptr_t const& tx_ptr) {
    // filter out T-3 normal contract send trx
    if (data::is_user_contract_address(common::xaccount_address_t{tx_ptr->get_source_addr()})) {
        xwarn("[global_trace][xtx_verifier][verify normal contract sendtx][fail]tx hash:%s addr:%s",
            tx_ptr->get_digest_hex_str().c_str(), tx_ptr->get_source_addr().c_str());
        return xverifier_error::xverifier_error_sendtx_by_normal_contrwact;
    }

    return xverifier_error::xverifier_success;
}
bool xtx_utl::is_valid_hex_format(std::string const& str) {
    for (size_t i = 0; i < str.size(); ++i) {
        if (-1 == top::data::xaction_t::hex_to_dec(str[i])) {
            return false;
        }
    }
    return true;
}
NS_END2
