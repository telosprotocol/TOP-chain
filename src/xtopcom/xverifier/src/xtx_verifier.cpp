// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xverifier/xtx_verifier.h"

#include "xbase/xutl.h"
#include "xbasic/xmodule_type.h"
#include "xchain_fork/xchain_upgrade_center.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xstake/xstake_algorithm.h"
#include "xverifier/xverifier_utl.h"
#include "xverifier/xwhitelist_verifier.h"
#include "xverifier/xblacklist_verifier.h"
#include "xvledger/xvblock.h"

#include <cinttypes>

NS_BEG2(top, xverifier)

REG_XMODULE_LOG(chainbase::enum_xmodule_type::xmodule_type_xverifier, xverifier_error_to_string, xverifier_error_base+1, xverifier_error_max);

int32_t xtx_verifier::verify_address(data::xtransaction_t const * trx) {
    const auto & src_addr = trx->get_source_addr();
    const auto & dst_addr = trx->get_target_addr();

    if (src_addr.empty() || dst_addr.empty()) {
        xwarn("[global_trace][xtx_verifier][address_verify][fail], tx:%s", trx->dump().c_str());
        return xverifier_error::xverifier_error_addr_invalid;
    }

    if (xverifier_error::xverifier_success != xverifier::xtx_utl::address_is_valid(src_addr) ||
        xverifier_error::xverifier_success != xverifier::xtx_utl::address_is_valid(dst_addr)) {
        xwarn("[global_trace][xtx_verifier][address_verify][address invalid], tx:%s,%s,%s", trx->dump().c_str(), src_addr.c_str(), dst_addr.c_str());
        return  xverifier_error::xverifier_error_addr_invalid;
    }
    return xverifier_error::xverifier_success;
}

int32_t xtx_verifier::verify_burn_tx(data::xtransaction_t const * trx) {
    const auto & src_addr = trx->get_source_addr();
    const auto & dst_addr = trx->get_target_addr();
    bool is_src_black_hole_addr = data::is_black_hole_address(common::xaccount_address_t{src_addr});
    bool is_dst_black_hole_addr = data::is_black_hole_address(common::xaccount_address_t{dst_addr});

    if ( (!is_src_black_hole_addr) && (!is_dst_black_hole_addr) ) {
        // not burn tx
        return xverifier_error::xverifier_success;
    }
    // src addr should be user addr; must be transfer type;
    if ( ( (!data::is_account_address(common::xaccount_address_t{src_addr})) && (!data::is_sub_account_address(common::xaccount_address_t{src_addr})) )
        || (!is_dst_black_hole_addr)
        || (trx->get_tx_type() != data::enum_xtransaction_type::xtransaction_type_transfer) ) {
        xwarn("[global_trace][xtx_verifier][verify_burn_tx] fail, tx:%s", trx->dump().c_str());
        return xverifier_error_burn_tx_invalid;
    }
    return xverifier_error::xverifier_success;
}

int32_t xtx_verifier::verify_local_tx(data::xtransaction_t const * trx) {
    const auto & src_addr = trx->get_source_addr();
    const auto & dst_addr = trx->get_target_addr();
    if (!data::is_sys_contract_address(common::xaccount_address_t{src_addr})) {
        // not local tx
        return xverifier_error::xverifier_success;
    }

    if ( (src_addr != dst_addr)
        || (trx->get_tx_type() != data::enum_xtransaction_type::xtransaction_type_run_contract)
        || (!trx->get_authorization().empty()) ) {
        xwarn("[global_trace][xtx_verifier][verify_local_tx][fail], tx:%s,invalid local tx", trx->dump().c_str());
        return xverifier_error_local_tx_invalid;
    }

    return xverifier_error::xverifier_success;
}

int32_t xtx_verifier::verify_address_type(data::xtransaction_t const * trx) {
    const auto & src_addr = trx->get_source_addr();
    const auto & dst_addr = trx->get_target_addr();
    base::enum_vaccount_addr_type src_addr_type = base::xvaccount_t::get_addrtype_from_account(src_addr);
    base::enum_vaccount_addr_type dst_addr_type = base::xvaccount_t::get_addrtype_from_account(dst_addr);

    if ( (src_addr_type != base::enum_vaccount_addr_type_secp256k1_user_account)
        && (src_addr_type != base::enum_vaccount_addr_type_secp256k1_user_sub_account)
        && (src_addr_type != base::enum_vaccount_addr_type_secp256k1_eth_user_account)
        && (src_addr_type != base::enum_vaccount_addr_type_native_contract) ) {
        xwarn("[global_trace][xtx_verifier][address_verify]src addr invalid, tx:%s", trx->dump().c_str());
        return  xverifier_error::xverifier_error_addr_invalid;
    }

    if ( (dst_addr_type != base::enum_vaccount_addr_type_secp256k1_user_account)
        && (dst_addr_type != base::enum_vaccount_addr_type_secp256k1_user_sub_account)
        && (dst_addr_type != base::enum_vaccount_addr_type_native_contract)
        && (dst_addr_type != base::enum_vaccount_addr_type_custom_contract)
        && (dst_addr_type != base::enum_vaccount_addr_type_secp256k1_eth_user_account)
        && (dst_addr_type != base::enum_vaccount_addr_type_black_hole) ) {
        xwarn("[global_trace][xtx_verifier][address_verify]dst addr invalid, tx:%s", trx->dump().c_str());
        return  xverifier_error::xverifier_error_addr_invalid;
    }

    if (trx->get_tx_type() == data::enum_xtransaction_type::xtransaction_type_transfer) {
        if ( (src_addr_type != base::enum_vaccount_addr_type_secp256k1_user_account)
            && (src_addr_type != base::enum_vaccount_addr_type_secp256k1_eth_user_account)
            && (src_addr_type != base::enum_vaccount_addr_type_secp256k1_user_sub_account) ) {
            xwarn("[global_trace][xtx_verifier][address_verify]src addr invalid , tx:%s", trx->dump().c_str());
            return  xverifier_error::xverifier_error_addr_invalid;
        }
        if ( (dst_addr_type != base::enum_vaccount_addr_type_secp256k1_user_account)
            && (dst_addr_type != base::enum_vaccount_addr_type_secp256k1_user_sub_account)
            && (dst_addr_type != base::enum_vaccount_addr_type_secp256k1_eth_user_account)
            && (dst_addr_type != base::enum_vaccount_addr_type_black_hole) ) {
            xwarn("[global_trace][xtx_verifier][address_verify]dst addr invalid , tx:%s", trx->dump().c_str());
            return  xverifier_error::xverifier_error_addr_invalid;
        }
    }

    return xverifier_error::xverifier_success;
}

int32_t xtx_verifier::verify_tx_signature(data::xtransaction_t const * trx, observer_ptr<store::xstore_face_t> const & store) {
    // verify signature
    if (!data::is_sys_contract_address(common::xaccount_address_t{trx->get_source_addr()}) && !data::is_user_contract_address(common::xaccount_address_t{trx->get_source_addr()})) {
        bool check_success = false;
        if (trx->get_target_addr() != sys_contract_rec_standby_pool_addr) {
            xdbg("[global_trace][xtx_verifier][verify_tx_signature][sign_check], tx:%s", trx->dump().c_str());
            check_success = trx->sign_check();
        } else {
#ifdef XENABLE_MOCK_ZEC_STAKE
            check_success = true;
#else
            assert(store != nullptr);
            xpublic_key_t pub_key = top::xstake::get_reg_info(store, common::xaccount_address_t{trx->get_source_addr()}).consensus_public_key;
            xdbg("[global_trace][xtx_verifier][verify_tx_signature][pub_key_sign_check], tx:%s, pub_key(base64):%s", trx->dump().c_str(), pub_key.to_string().c_str());

            check_success = !pub_key.empty() && trx->pub_key_sign_check(pub_key);
            xdbg("[global_trace][xtx_verifier][verify_tx_signature][pub_key_sign_check]:%s .tx:%s, pub_key(base64):%s",
                 check_success ? "success" : "fail",
                 trx->dump().c_str(),
                 pub_key.to_string().c_str());
#endif
        }
        if (!check_success) {
            xwarn("[global_trace][xtx_verifier][signature_verify][fail], tx:%s", trx->dump().c_str());
            return xverifier_error::xverifier_error_tx_signature_invalid;
        }
    }

    xdbg("[global_trace][xtx_verifier][verify_tx_signature][success], tx:%s", trx->dump().c_str());
    return xverifier_error::xverifier_success;
}

// verify trx fire expiration
int32_t xtx_verifier::verify_tx_fire_expiration(data::xtransaction_t const * trx, uint64_t now) {
    uint32_t trx_fire_tolerance_time = XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_send_timestamp_tolerance);

    uint64_t fire_expire = trx->get_fire_timestamp() + trx_fire_tolerance_time;
    if (fire_expire < now) {
        xwarn("[global_trace][xtx_verifier][verify_tx_fire_expiration][fail], tx:%s, fire_timestamp:%ld, now:%ld",
            trx->dump().c_str(), trx->get_fire_timestamp(), now);
        return xverifier_error::xverifier_error_tx_fire_expired;
    }

    fire_expire = now + trx_fire_tolerance_time;
    if (fire_expire < trx->get_fire_timestamp()) {
        xwarn("[global_trace][xtx_verifier][verify_tx_fire_expiration][fail], tx:%s, fire_timestamp:%ld, now:%ld",
            trx->dump().c_str(), trx->get_fire_timestamp(), now);
        return xverifier_error::xverifier_error_tx_fire_expired;
    }

    xdbg("[global_trace][xtx_verifier][verify_tx_fire_expiration][success], tx:%s", trx->dump().c_str());
    return xverifier_error::xverifier_success;

}

int32_t xtx_verifier::sys_contract_tx_check(data::xtransaction_t const * trx_ptr) {
    static std::vector<std::string> open_sys_contracts = {
        sys_contract_rec_registration_addr,
        sys_contract_rec_standby_pool_addr,
        sys_contract_sharding_vote_addr,
        sys_contract_rec_tcc_addr,
        sys_contract_sharding_reward_claiming_addr
    };

    if (trx_ptr->get_target_action_name() == "nodeJoinNetwork") {
        return xverifier_error::xverifier_error_contract_not_allowed;
    }

    auto source_addr = trx_ptr->get_source_addr();
    auto target_addr = trx_ptr->get_origin_target_addr();

    bool source_is_user_addr            = data::is_account_address(common::xaccount_address_t{source_addr}) || data::is_sub_account_address(common::xaccount_address_t{source_addr});
    bool target_is_sys_contract_addr    = data::is_sys_contract_address(common::xaccount_address_t{target_addr});
    if (source_is_user_addr && target_is_sys_contract_addr) {
        for (const auto & addr : open_sys_contracts) {
            if (addr == target_addr) {
                xdbg("[global_trace][xtx_verifier][sys_contract_tx_check][success], tx:%s", trx_ptr->dump().c_str());
                return xverifier_error::xverifier_success;
            }
        }
        xwarn("[global_trace][xtx_verifier][sys_contract_tx_check][fail], tx:%s,target_origin_addr=%s", trx_ptr->dump().c_str(), target_addr.c_str());
        return xverifier_error::xverifier_error_contract_not_allowed;
    }

    xdbg("[global_trace][xtx_verifier][sys_contract_tx_check][success], tx:%s", trx_ptr->dump().c_str());
    return xverifier_error::xverifier_success;
}

// verify trx duration expiration
int32_t xtx_verifier::verify_tx_duration_expiration(const data::xtransaction_t * trx_ptr, uint64_t now) {
    uint32_t trx_fire_tolerance_time = XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_send_timestamp_tolerance);
    uint64_t fire_expire = trx_ptr->get_fire_timestamp() + trx_ptr->get_expire_duration() + trx_fire_tolerance_time;
    if (fire_expire < now) {
        xwarn("[global_trace][xtx_verifier][verify_tx_duration_expiration][fail], tx:%s, fire_timestamp:%" PRIu64 ", fire_tolerance_time:%" PRIu32 ", expire_duration:%" PRIu16 ", now:%" PRIu64,
            trx_ptr->dump().c_str(), trx_ptr->get_fire_timestamp(), trx_fire_tolerance_time, trx_ptr->get_expire_duration(), now);
        return xverifier_error::xverifier_error_tx_duration_expired;
    }

    xdbg("[global_trace][xtx_verifier][verify_tx_duration_expiration][success], tx hash: %s", trx_ptr->get_digest_hex_str().c_str());
    return xverifier_error::xverifier_success;
}

int32_t xtx_verifier::verify_account_min_deposit(uint64_t amount) {
    auto account_min_deposit = XGET_CONFIG(min_account_deposit);
    if (amount < account_min_deposit) {
        xwarn("[global_trace][xtx_verifier][verify_account_min_deposit][fail], trx min deposit:%d, amount:%ld", account_min_deposit, amount);
        return xverifier_error_account_min_deposit_invalid;
    }
    return xverifier_error::xverifier_success;
}

int32_t xtx_verifier::verify_tx_min_deposit(uint64_t deposit) {
     auto trx_min_deposit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit);
    if (deposit < trx_min_deposit) {
        xwarn("[global_trace][xtx_verifier][verify_tx_min_deposit][fail], trx min deposit:%d, tx deposit:%ld", trx_min_deposit, deposit);
        return xverifier_error_tx_min_deposit_invalid;
    }
    return xverifier_error::xverifier_success;
}

int32_t xtx_verifier::verify_send_tx_source(data::xtransaction_t const * trx_ptr, bool local) {
    base::enum_vaccount_addr_type addr_type = base::xvaccount_t::get_addrtype_from_account(trx_ptr->get_source_addr());
    if (local) {
        if (addr_type != base::enum_vaccount_addr_type_native_contract) {
            xwarn("[global_trace][xtx_verifier][verify_send_tx_source][fail], tx:%s,local tx not sys contract tx", trx_ptr->dump().c_str());
            return xverifier_error_send_tx_source_invalid;
        }
        int32_t ret = verify_local_tx(trx_ptr);
        if (ret) {
            return ret;
        }
    } else {
        bool valid_addr_type = (addr_type == base::enum_vaccount_addr_type_secp256k1_user_account)
                                || (addr_type == base::enum_vaccount_addr_type_secp256k1_user_sub_account)
                                || (addr_type == base::enum_vaccount_addr_type_secp256k1_eth_user_account);
        if (!valid_addr_type) {
            xwarn("[global_trace][xtx_verifier][verify_send_tx_source][fail], tx:%s,non_local tx addr type invalid", trx_ptr->dump().c_str());
            return xverifier_error_send_tx_source_invalid;
        }
        if (trx_ptr->get_authorization().empty()) {
            xwarn("[global_trace][xtx_verifier][verify_send_tx_source][fail], tx:%s,non_local tx must has signature", trx_ptr->dump().c_str());
            return xverifier_error_send_tx_source_invalid;
        }
    }
    return xverifier_error::xverifier_success;
}

int32_t xtx_verifier::verify_send_tx_validation(data::xtransaction_t const * trx_ptr) {
    if (!trx_ptr->transaction_type_check()) {
        xwarn("[global_trace][xtx_verifier][verify_send_tx_validation][fail], tx:%s,tx type invalid", trx_ptr->dump().c_str());
        return xverifier_error_tx_basic_validation_invalid;
    }
    if (!trx_ptr->unuse_member_check()) {
        xwarn("[global_trace][xtx_verifier][verify_send_tx_validation][fail], tx:%s,unuse member check invalid", trx_ptr->dump().c_str());
        return xverifier_error_tx_basic_validation_invalid;
    }
    if (!trx_ptr->transaction_len_check()) {
        xwarn("[global_trace][xtx_verifier][verify_send_tx_validation][fail], tx:%s,len check invalid", trx_ptr->dump().c_str());
        return xverifier_error::xverifier_error_tx_param_invalid;
    }
    int32_t ret = verify_address_type(trx_ptr);
    if (ret) {
        return ret;
    }
    ret = verify_address(trx_ptr);
    if (ret) {
        return ret;
    }
    ret = verify_burn_tx(trx_ptr);
    if (ret) {
        return ret;
    }
    ret = verify_local_tx(trx_ptr);
    if (ret) {
        return ret;
    }
    ret = sys_contract_tx_check(trx_ptr);
    if (ret) {
        return ret;
    }
    ret = verify_shard_contract_addr(trx_ptr);
    if (ret) {
        return ret;
    }
    // verify hash
    if (!trx_ptr->digest_check()) {
        xwarn("[global_trace][xtx_verifier][verify_send_tx_validation][fail], tx:%s digest check invalid", trx_ptr->dump().c_str());
        return xverifier_error::xverifier_error_tx_hash_invalid;
    }
    return xverifier_error::xverifier_success;
}

int32_t xtx_verifier::verify_send_tx_legitimacy(data::xtransaction_t const * trx_ptr, observer_ptr<store::xstore_face_t> const & store) {
    int32_t ret = verify_tx_signature(trx_ptr, store);
    if (ret) {
        return ret;
    }

    auto const& fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
    auto logic_clock = (top::base::xtime_utl::gmttime() - top::base::TOP_BEGIN_GMTIME) / 10;
    if (chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.blacklist_function_fork_point, logic_clock)) {
        xdbg("[xtx_verifier::verify_send_tx_legitimacy] in blacklist fork point time, clock height: %" PRIu64, logic_clock);
        if (xverifier::xblacklist_utl_t::is_black_address(trx_ptr->get_source_addr())) {
            xdbg("[xtx_verifier::verify_send_tx_legitimacy] in black address:%s, %s, %s", trx_ptr->get_digest_hex_str().c_str(), trx_ptr->get_target_addr().c_str(), trx_ptr->get_source_addr().c_str());
            return xverifier_error::xverifier_error_tx_blacklist_invalid;
        }
    } else {
        xdbg("[xtx_verifier::verify_send_tx_legitimacy] not up to blacklist fork point time, clock height: %" PRIu64, logic_clock);
    }

    if (xwhitelist_utl::check_whitelist_limit_tx(trx_ptr)) {
        return xverifier_error::xverifier_error_tx_whitelist_invalid;
    }
    return xverifier_error::xverifier_success;
}

int32_t xtx_verifier::verify_shard_contract_addr(data::xtransaction_t const * trx_ptr) {
    const auto & source_addr = trx_ptr->get_source_addr();
    const auto & origin_target_addr = trx_ptr->get_origin_target_addr();
    // user call sys sharding contract, always auto set target addresss
    if (is_sys_sharding_contract_address(common::xaccount_address_t{origin_target_addr})) {
        if (is_account_address(common::xaccount_address_t{source_addr})) {
            if (std::string::npos != origin_target_addr.find("@")) {
                xwarn("[global_trace][xtx_verifier][verify_shard_contract_addr] fail-already set tableid, tx:%s,origin_target_addr=%s", trx_ptr->dump().c_str(), origin_target_addr.c_str());
                return xverifier_error_tx_basic_validation_invalid;
            }

            const auto & target_addr = trx_ptr->get_target_addr();
            if (std::string::npos == target_addr.find("@")) {
                xwarn("[global_trace][xtx_verifier][verify_shard_contract_addr] fail-not set tableid, tx:%s,target_addr=%s", trx_ptr->dump().c_str(), target_addr.c_str());
                return xverifier_error_tx_basic_validation_invalid;
            }

            base::xvaccount_t _src_vaddr(source_addr);
            base::xvaccount_t _dst_vaddr(target_addr);
            if (_src_vaddr.get_ledger_subaddr() != _dst_vaddr.get_ledger_subaddr()
                || _src_vaddr.get_ledger_id() != _dst_vaddr.get_ledger_id()) {
                xerror("[global_trace][xtx_verifier][verify_shard_contract_addr] fail-src and dst not match, tx:%s,target_addr=%s", trx_ptr->dump().c_str(), target_addr.c_str());
                return xverifier_error_tx_basic_validation_invalid;
            }
        } else {
            if (std::string::npos == origin_target_addr.find("@")) {
                xwarn("[global_trace][xtx_verifier][verify_shard_contract_addr] fail-sys contract all should set full addr, tx:%s,origin_target_addr=%s", trx_ptr->dump().c_str(), origin_target_addr.c_str());
                return xverifier_error_tx_basic_validation_invalid;
            }
        }
    }
    return xverifier_error::xverifier_success;
}

NS_END2
