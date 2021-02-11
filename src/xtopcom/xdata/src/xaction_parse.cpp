// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <sys/time.h>

#include "xdata/xdata_error.h"
#include "xdata/xaction_parse.h"
#include "xdata/xdata_defines.h"

#include "xbase/xlog.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xutility/xhash.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/reader.h"
#include "rapidjson/writer.h"

namespace top { namespace data {
using base::xstream_t;
using base::xcontext_t;

bool xaction_asset_out::operator == (const xaction_asset_out &other) {
    return m_asset_out.m_token_name == other.m_asset_out.m_token_name &&
            m_asset_out.m_amount == other.m_asset_out.m_amount;
}

bool xaction_asset_out::operator != (const xaction_asset_out &other) {
    return !(*this == other);
}

bool xaction_asset_out::is_top_token() const {
    return m_asset_out.is_top_token();
}

int32_t xaction_asset_out::parse(const xaction_t & action) {
    if (action.get_action_type() != xaction_type_asset_out) {
        return xchain_error_action_type_invalid;
    }
    if (action.get_action_param().empty()) {
        return xchain_error_action_param_empty;
    }

    try {
        xstream_t stream(xcontext_t::instance(), (uint8_t*)action.get_action_param().data(), action.get_action_param().size());
        stream >> m_asset_out.m_token_name;
        stream >> m_asset_out.m_amount;
    } catch(enum_xerror_code& e) {
        return xchain_error_action_param_empty;
    } catch(...) {
        return xchain_error_action_param_empty;
    }
    return xsuccess;
}

int32_t xaction_asset_out::serialze_to(xaction_t & action, const data::xproperty_asset & asset_out) {
    action.set_action_type(xaction_type_asset_out);
    xstream_t stream(xcontext_t::instance());
    stream << asset_out.m_token_name;
    stream << asset_out.m_amount;
    std::string param((char*)stream.data(), stream.size());
    action.set_action_param(std::move(param));
    return xsuccess;
}

bool xaction_asset_proc::is_top_token() const {
    return m_asset.is_top_token();
}

int32_t xaction_asset_proc::parse(const xaction_t & action) {
    if (action.get_action_type() != action_type()) {
        return xchain_error_action_type_invalid;
    }
    if (action.get_action_param().empty()) {
        return xchain_error_action_param_empty;
    }

    try {
        xstream_t stream(xcontext_t::instance(), (uint8_t*)action.get_action_param().data(), action.get_action_param().size());
        stream >> m_asset.m_token_name;
        stream >> m_asset.m_amount;
    } catch(enum_xerror_code& e) {
        return xchain_error_action_param_empty;
    } catch(...) {
        return xchain_error_action_param_empty;
    }
    return xsuccess;
}

int32_t xaction_asset_proc::serialze_to(xaction_t & action, const data::xproperty_asset & asset, uint16_t action_type) {
    // if (amount == 0) {
    //     return xchain_error_action_param_zero;
    // }
    action.set_action_type(action_type);
    xstream_t stream(xcontext_t::instance());
    stream << asset.m_token_name;
    stream << asset.m_amount;
    std::string param((char*)stream.data(), stream.size());
    action.set_action_param(std::move(param));
    return xsuccess;
}

int32_t xaction_source_null::parse(const xaction_t & action) {
    if (action.get_action_type() != xaction_type_source_null) {
        return xchain_error_action_type_invalid;
    }
    if (!action.get_action_param().empty()) {
        return xchain_error_action_param_not_empty;
    }
    return xsuccess;
}

int32_t xaction_source_null::serialze_to(xaction_t & action) {
    action.set_action_type(xaction_type_source_null);
    action.clear_action_param();
    return xsuccess;
}

int32_t xaction_create_user_account::parse(const xaction_t & action) {
    if (action.get_action_type() != xaction_type_create_user_account) {
        return xchain_error_action_type_invalid;
    }
    if (action.get_action_param().empty()) {
        return xchain_error_action_param_empty;
    }
    try {
        xstream_t stream(xcontext_t::instance(), (uint8_t*)action.get_action_param().data(), action.get_action_param().size());
        xinfo("action_params:%s\n",action.get_action_param().c_str());
        stream >> m_address;
    } catch(enum_xerror_code& e) {
        return xchain_error_action_param_empty;
    } catch(...) {
        return xchain_error_action_param_empty;
    }
    return xsuccess;
}

int32_t xaction_create_user_account::serialze_to(xaction_t & action, const std::string& address) {
    action.set_action_type(xaction_type_create_user_account);
    xstream_t stream(xcontext_t::instance());
    stream << address;
    std::string param((char*)stream.data(), stream.size());
    action.set_action_param(std::move(param));
    return xsuccess;
}

int32_t xaction_deploy_contract::parse(const xaction_t & action) {
    if (action.get_action_type() != xaction_type_create_contract_account) {
        return xchain_error_action_type_invalid;
    }
    if (action.get_action_param().empty()) {
        return xchain_error_action_param_empty;
    }
    //  check code is valid
    try {
        xstream_t stream(xcontext_t::instance(), (uint8_t*)action.get_action_param().data(), action.get_action_param().size());
        stream >> m_tgas_limit;
        stream >> m_code;
        if (m_code.size() > MAX_CONTRACT_CODE_SIZE) {
            return xchain_error_action_param_code_length_too_long;
        }
    } catch(enum_xerror_code& e) {
        return xchain_error_action_param_empty;
    } catch(...) {
        return xchain_error_action_param_empty;
    }
    return xsuccess;
}

int32_t xaction_deploy_contract::serialze_to(xaction_t & action, uint64_t tgas_limit, const std::string& code) {
    if (code.empty()) {
        return xchain_error_action_param_empty;
    }
    action.set_action_type(xaction_type_create_contract_account);
    xstream_t stream(xcontext_t::instance());
    stream << tgas_limit;
    stream << code;
    std::string param((char*)stream.data(), stream.size());
    action.set_action_param(std::move(param));
    return xsuccess;
}

int32_t xaction_run_contract::parse(const xaction_t & action) {
    if (action.get_action_type() != xaction_type_run_contract) {
        return xchain_error_action_type_invalid;
    }
    m_function_name = action.get_action_name();
    m_para = action.get_action_param();
    return xsuccess;
}
int32_t xaction_run_contract::serialze_to(xaction_t & action, const std::string& function_name, const std::string& para) {
    action.set_action_type(xaction_type_run_contract);
    action.set_action_name(function_name);
    action.set_action_param(para);
    return xsuccess;
}

int32_t xaction_alias_name::parse(const xaction_t & action) {
    if (action.get_action_type() != xaction_type_alias_name) {
        return xchain_error_action_type_invalid;
    }
    if (action.get_action_param().empty()) {
        return xchain_error_action_param_empty;
    }
    if (action.get_action_param().size() > 32) {  //  move to config
        return xchain_error_action_param_size_too_large;
    }
    m_name = action.get_action_param();
    return xsuccess;
}

int32_t xaction_alias_name::serialze_to(xaction_t & action, const std::string& name) {
    action.set_action_type(xaction_type_alias_name);
    if (action.get_action_param().empty()) {
        return xchain_error_action_param_empty;
    }
    if (action.get_action_param().size() > 32) {
        return xchain_error_action_param_size_too_large;
    }
    action.set_action_param(name);
    return xsuccess;
}

int32_t xaction_set_account_keys::parse(const xaction_t & action) {
    if (action.get_action_type() != xaction_type_set_account_keys) {
        return xchain_error_action_type_invalid;
    }

    base::xstream_t stream(base::xcontext_t::instance(),
                           (uint8_t *)action.get_action_param().c_str(),
                           (uint32_t)action.get_action_param().size());

    stream >> m_account_key;
    stream >> m_key_value;

    if (m_key_value.size() != 65) {
        return xchain_error_set_account_key_len_invalid;
    }

    return xsuccess;
}

int32_t xaction_lock_account_token::parse_param(const std::string &param) {

    base::xstream_t stream(base::xcontext_t::instance(),
                          (uint8_t *)param.c_str(),
                          (uint32_t)param.size());

    stream >> m_version;
    stream >> m_amount;
    stream >> m_unlock_type;

    assert(m_unlock_type == UT_time ||
           m_unlock_type == UT_alone_key ||
           m_unlock_type == UT_muti_keys);
    uint32_t size;
    std::string sign;
    stream >> size;
    assert(size <= 8);
    for (uint32_t i = 0; i < size; i++) {
        stream >> sign;
        m_unlock_values.push_back(sign);
    }

    if (m_unlock_type == UT_time) {
        if (m_unlock_values.size() != 1) {
            return xchain_error_unlock_keys_num_invalid;
        }

        // add time limit, negative value and too large value(uint64_t 20)
        if (m_unlock_values.at(0)[0] == '-' || m_unlock_values.at(0).size() > 19) {
            xinfo("parse lock token param, time param invalid");
            return xchain_error_lock_token_time_param_invalid;
        }

    }else if(m_unlock_type == UT_alone_key) {
        if (m_unlock_values.size() != 1) {
            return xchain_error_unlock_keys_num_invalid;
        }
        if (m_unlock_values.at(0).size() != 65) {
            return xchain_error_unlock_keys_len_invalid;
        }
    } else {
        if (m_unlock_values.empty()) {
            return xchain_error_unlock_keys_num_invalid;
        }
        if (m_unlock_values.size() > 8) {
            xerror("lock token unlock pubkey size %d beyond limit 8", m_unlock_values.size());
            return xchain_error_lock_token_pubkey_size_beyond_limit;
        }
        for (auto &i : m_unlock_values) {
            if (i.size() != 65) {
                return xchain_error_unlock_keys_len_invalid;
            }
        }
    }

    xinfo("parse lock token param %s, version %d, amount %d, unlocke_type %d, unlock_value size %d",
          param.c_str(), m_version, m_amount, m_unlock_type, m_unlock_values.size());
    m_params = param;

    return xsuccess;
}

int32_t xaction_lock_account_token::parse(const xaction_t & action) {
    if (action.get_action_type() != xaction_type_lock_token) {
        return xchain_error_action_type_invalid;
    }
    return parse_param(action.get_action_param());
}

int32_t xaction_unlock_account_token::parse_param(const std::string &param) {

    base::xstream_t stream(base::xcontext_t::instance(),
                          (uint8_t *)param.c_str(),
                          (uint32_t)param.size());

    stream >> m_version;
    stream >> m_lock_tran_hash;

    uint32_t size;
    std::string sign;
    stream >> size;
    assert(size <= 8);
    for (uint32_t i = 0; i < size; i++) {
        stream >> sign;
        m_signatures.push_back(sign);
    }

    xinfo("parse unlock token param %s, version %d, lock_tx_hash %s, sign num %d",
          param.c_str(), m_version, m_lock_tran_hash.c_str(), m_signatures.size());

    return xsuccess;
}

int32_t xaction_unlock_account_token::parse(const xaction_t & action) {
    if (action.get_action_type() != xaction_type_unlock_token) {
        return xchain_error_action_type_invalid;
    }
    return parse_param(action.get_action_param());
}

int32_t xaction_pledge_token_vote::parse(const xaction_t & action) {
    if (action.get_action_type() != xaction_type_pledge_token_vote) {
        return xchain_error_action_type_invalid;
    }
    auto & param = action.get_action_param();
    base::xstream_t stream(base::xcontext_t::instance(),
                          (uint8_t *)param.c_str(),
                          (uint32_t)param.size());

    stream >> m_vote_num;
    stream >> m_lock_duration;

    return 0;
}

int32_t xaction_pledge_token_vote::serialze_to(xaction_t & action, const uint64_t num, const uint16_t duration) {
    action.set_action_type(xaction_type_pledge_token_vote);
    xstream_t stream(xcontext_t::instance());
    stream << num;
    stream << duration;
    std::string param((char*)stream.data(), stream.size());
    action.set_action_param(std::move(param));
    return xsuccess;
}

int32_t xaction_redeem_token_vote::parse(const xaction_t & action) {
    if (action.get_action_type() != xaction_type_redeem_token_vote) {
        return xchain_error_action_type_invalid;
    }
    auto & param = action.get_action_param();
    base::xstream_t stream(base::xcontext_t::instance(),
                          (uint8_t *)param.c_str(),
                          (uint32_t)param.size());

    stream >> m_vote_num;

    return 0;
}

int32_t xaction_redeem_token_vote::serialze_to(xaction_t & action, const uint64_t num) {
    action.set_action_type(xaction_type_redeem_token_vote);
    xstream_t stream(xcontext_t::instance());
    stream << num;
    std::string param((char*)stream.data(), stream.size());
    action.set_action_param(std::move(param));
    return xsuccess;
}

}  // namespace data
}  // namespace top
