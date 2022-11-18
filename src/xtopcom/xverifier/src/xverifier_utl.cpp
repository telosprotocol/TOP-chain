// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fstream>
#include "xbase/xlog.h"
#include "xdata/xgenesis_data.h"
#include "xverifier/xverifier_utl.h"
#include "xbase/xutl.h"

NS_BEG2(top, xverifier)

int32_t xtx_utl::address_is_valid(const std::string & addr, bool isTransaction) {
    bool res = true;
    std::vector<std::string> parts;
    
    try {
        if (false == top::base::xvaccount_t::check_address(addr, isTransaction)) {
            res = false;
        } else {
            if (base::xstring_utl::split_string(addr, '@', parts) >= 2) {
                if (top::base::xvaccount_t::get_addrtype_from_account(addr) != base::enum_vaccount_addr_type_native_contract) {
                    top::utl::xkeyaddress_t keyaddr{parts[0]};
                    if (!keyaddr.is_valid()) {
                        xwarn("account %s is invalid", addr.c_str());
                        res = false;
                    }
                }
            } else {
                top::utl::xkeyaddress_t keyaddr{addr};
                if (!keyaddr.is_valid()) {
                    res = false;
                }
            }
        }
    } catch (...) {
        res = false;
    }

    if (!res) {
        xwarn("[global_trace][xverifier][address_is_valid][fail] address: %s", addr.c_str());
        return xverifier_error::xverifier_error_addr_invalid;
    }

    xdbg("[global_trace][xverifier][address_is_valid][success] address: %s", addr.c_str());
    return xverifier_error::xverifier_success;
}

int32_t xtx_utl::address_is_valid(common::xnode_id_t const & addr) {

    bool res = true;
    try {
        top::utl::xkeyaddress_t keyaddr{addr.to_string()};
        if (!keyaddr.is_valid()) res = false;

    } catch (...) {
        res = false;
    }

    if (!res) {
        xwarn("[global_trace][xverifier][address_is_valid][fail] address: %s", addr.to_string().c_str());
        return xverifier_error::xverifier_error_addr_invalid;
    }

    xdbg("[global_trace][xverifier][address_is_valid][success] address: %s", addr.to_string().c_str());
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
    //if (data::is_user_contract_address(common::xaccount_address_t{tx_ptr->get_source_addr()})) {
    //    xwarn("[global_trace][xtx_verifier][verify normal contract sendtx][fail]tx hash:%s addr:%s",
    //        tx_ptr->get_digest_hex_str().c_str(), tx_ptr->get_source_addr().c_str());
    //    return xverifier_error::xverifier_error_sendtx_by_normal_contrwact;
    //}

    return xverifier_error::xverifier_success;
}
bool xtx_utl::is_valid_hex_format(std::string const& str) {
    for (size_t i = 0; i < str.size(); ++i) {
        if (-1 == top::data::hex_to_dec(str[i])) {
            return false;
        }
    }
    return true;
}

bool xtx_utl::load_bwlist_content(std::string const& config_file, std::map<std::string, std::string>& result) {
    if (config_file.empty()) {
        xwarn("xtx_utl::load_bwlist_content fail-config_file empty");
        return false;
    }

    std::ifstream in(config_file);
    if (!in.is_open()) {
        xwarn("xtx_utl::load_bwlist_content fail-open local list file %s error", config_file.c_str());
        return false;
    }
    std::ostringstream oss;
    oss << in.rdbuf();
    in.close();
    auto json_content = std::move(oss.str());
    if (json_content.empty()) {
        xwarn("xtx_utl::load_bwlist_content fail-read list config file %s empty", config_file.c_str());
        return false;
    }


    xJson::Value  json_root;
    xJson::Reader  reader;
    bool ret = reader.parse(json_content, json_root);
    if (!ret) {
        xwarn("xtx_utl::load_bwlist_content fail-parse config file %s failed", config_file.c_str());
        return false;
    }


    auto const members = json_root.getMemberNames();
    for(auto const& member: members) {
        if (member == "local_toggle_whitelist") {
            std::string _toggle = json_root[member].asString();
            if (_toggle == "1" || _toggle == "0" || _toggle == "true" || _toggle == "false") {
                result[member] = _toggle;
            } else {
                xwarn("xtx_utl::load_bwlist_content fail-local_toggle_whitelist not correct. config file %s failed", config_file.c_str());
                return false;                
            }
        } else if (member == "local_blacklist" || member == "local_whitelist") {
            if (json_root[member].isArray()) {
                for (unsigned int i = 0; i < json_root[member].size(); ++i) {
                    try {
                        auto const& addr = json_root[member][i].asString();
                        if (addr.size() <= top::base::xvaccount_t::enum_vaccount_address_prefix_size) return false;
                        top::base::xvaccount_t _vaccount(addr);
                        if (!_vaccount.is_unit_address()) {
                            xwarn("xtx_utl::load_bwlist_content fail-address type invalid. addr=%s, config file %s failed", addr.c_str(), config_file.c_str());
                            return false;
                        }
                        if ( top::xverifier::xverifier_success != top::xverifier::xtx_utl::address_is_valid(addr)) {
                            xwarn("xtx_utl::load_bwlist_content fail-address is invalid. addr=%s, config file %s failed", addr.c_str(), config_file.c_str());
                            return false;
                        }
                    } catch (...) {
                        xwarn("xtx_utl::load_bwlist_content fail-json exception. config file %s failed", config_file.c_str());
                        return false;
                    }

                    result[member] += json_root[member][i].asString() + ",";
                }
            } else {
                xwarn("xtx_utl::load_bwlist_content fail-json member not array. config file %s failed", config_file.c_str());
                return false;
            }
        } else {
            xwarn("xtx_utl::load_bwlist_content fail-unknown member. member=%s config file %s failed", member.c_str(), config_file.c_str());
            return false;                  
        }
    }

    xinfo("xtx_utl::load_bwlist_content succ.config file %s,result=%zu",config_file.c_str(),result.size());
    return true;
}

void xtx_utl::parse_bwlist_config_data(std::string const& data, std::set<std::string> & ret_addrs) {
    std::vector<std::string> vec;
    if (!data.empty()) {
        base::xstring_utl::split_string(data, ',', vec);
        for (auto const& v: vec) {
            ret_addrs.insert(v);
        }
    }
}

NS_END2
