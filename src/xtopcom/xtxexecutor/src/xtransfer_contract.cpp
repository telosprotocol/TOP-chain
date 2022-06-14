#include "xtxexecutor/xcontract/xtransfer_contract.h"

#include "xdata/xgenesis_data.h"
#include "xevm_common/common_data.h"

NS_BEG3(top, txexecutor, contract)

void xtop_transfer_contract::deposit(const std::string & token_name, const std::string & amount_256_str) {
    xbytes_t amount_256_bytes{amount_256_str.begin(), amount_256_str.end()}; 
    auto token = evm_common::fromBigEndian<evm_common::u256>(amount_256_bytes);
    auto unitstate = unitstate_owned();
    xassert(unitstate != nullptr);
    if (token_name == data::XPROPERTY_ASSET_TOP) {
        auto vtoken = base::vtoken_t(static_cast<uint64_t>(token));
        xassert(token <= UINT64_MAX);
        auto ret = unitstate->token_deposit(data::XPROPERTY_BALANCE_AVAILABLE, vtoken);
        if (ret) {
            xwarn("[xtop_transfer_contract::deposit] %s, deposit %lu, token: %s, failed", unitstate->get_account().c_str(), vtoken, token_name.c_str());
            std::error_code ec{contract_runtime::error::xenum_errc::enum_vm_exception};
            top::error::throw_error(ec, "tep_token_deposit failed");
        }
    } else if (token_name == data::XPROPERTY_ASSET_ETH) {
        auto ret = unitstate->tep_token_deposit(common::xtoken_id_t::eth, token);
        if (ret) {
            xwarn("[xtop_transfer_contract::deposit] %s, deposit %s, token %s failed", unitstate->get_account().c_str(), token.str().c_str(), token_name.c_str());
            std::error_code ec{contract_runtime::error::xenum_errc::enum_vm_exception};
            top::error::throw_error(ec, "tep_token_deposit failed");
        }
    } else {
        xassert(false);
        xwarn("[xtop_transfer_contract::deposit] bad token name: %s", token_name.c_str());
        std::error_code ec{contract_runtime::error::xenum_errc::enum_vm_exception};
        top::error::throw_error(ec, "tep_token_deposit failed");
    }

    return;
}

void xtop_transfer_contract::withdraw(const std::string & token_name, const std::string & amount_256_str) {
    xbytes_t amount_256_bytes{amount_256_str.begin(), amount_256_str.end()}; 
    auto token = evm_common::fromBigEndian<evm_common::u256>(amount_256_bytes);
    auto unitstate = unitstate_owned();
    xassert(unitstate != nullptr);
    xdbg("[xtop_transfer_contract::withdraw] token name: %s,amount: %ld", token_name.c_str(), (uint64_t)token);
    if (token_name == data::XPROPERTY_ASSET_TOP) {
        xassert(token <= UINT64_MAX);
        auto vtoken = base::vtoken_t(static_cast<uint64_t>(token));
        auto ret = unitstate->token_withdraw(data::XPROPERTY_BALANCE_AVAILABLE, vtoken);
        if (ret) {
            xwarn("[xtop_transfer_contract::withdraw] %s, withdraw %lu, token: %s, failed", unitstate->get_account().c_str(), vtoken, token_name.c_str());
            std::error_code ec{contract_runtime::error::xenum_errc::enum_vm_exception};
            top::error::throw_error(ec, "tep_token_withdraw failed");
        }
    } else if (token_name == data::XPROPERTY_ASSET_ETH) {
        auto ret = unitstate->tep_token_withdraw(common::xtoken_id_t::eth, token);
        if (ret) {
            xwarn("[xtop_transfer_contract::withdraw] %s, withdraw %s, token %s failed", unitstate->get_account().c_str(), token.str().c_str(), token_name.c_str());
            std::error_code ec{contract_runtime::error::xenum_errc::enum_vm_exception};
            top::error::throw_error(ec, "tep_token_withdraw failed");
        }
    } else {
        xassert(false);
        xwarn("[xtop_transfer_contract::withdraw] bad token name: %s", token_name.c_str());
        std::error_code ec{contract_runtime::error::xenum_errc::enum_vm_exception};
        top::error::throw_error(ec, "tep_token_withdraw failed");
    }

    return;
}

void xtop_transfer_contract::transfer(const std::string & token_name, const std::string & amount_256_str) {
    xbytes_t amount_256_bytes{amount_256_str.begin(), amount_256_str.end()}; 
    auto token = evm_common::fromBigEndian<evm_common::u256>(amount_256_bytes);
    auto sender_unitstate = unitstate_owned();
    auto recver_unitstate = unitstate_other();
    xassert(sender_unitstate != nullptr);
    xassert(recver_unitstate != nullptr);
    xdbg("[xtop_transfer_contract::transfer] token name: %s,amount: %ld", token_name.c_str(), (uint64_t)token);
    if (token_name == data::XPROPERTY_ASSET_TOP) {
        xassert(token <= UINT64_MAX);
        auto vtoken = base::vtoken_t(static_cast<uint64_t>(token));
        auto ret = sender_unitstate->token_withdraw(data::XPROPERTY_BALANCE_AVAILABLE, vtoken);
        if (ret) {
            xwarn("[xtop_transfer_contract::transfer] %s, withdraw %lu, token: %s, failed", sender_unitstate->get_account().c_str(), vtoken, token_name.c_str());
            std::error_code ec{contract_runtime::error::xenum_errc::enum_vm_exception};
            top::error::throw_error(ec, "tep_token_withdraw failed");
        }
        ret = recver_unitstate->token_deposit(data::XPROPERTY_BALANCE_AVAILABLE, vtoken);
        if (ret) {
            xwarn("[xtop_transfer_contract::transfer] %s, deposit %lu, token: %s, failed", recver_unitstate->get_account().c_str(), vtoken, token_name.c_str());
            std::error_code ec{contract_runtime::error::xenum_errc::enum_vm_exception};
            top::error::throw_error(ec, "tep_token_deposit failed");
        }
    } else if (token_name == data::XPROPERTY_ASSET_ETH) {
        auto ret = sender_unitstate->tep_token_withdraw(common::xtoken_id_t::eth, token);
        if (ret) {
            xwarn("[xtop_transfer_contract::transfer] %s, withdraw %s, token %s failed", sender_unitstate->get_account().c_str(), token.str().c_str(), token_name.c_str());
            std::error_code ec{contract_runtime::error::xenum_errc::enum_vm_exception};
            top::error::throw_error(ec, "tep_token_withdraw failed");
        }
        ret = recver_unitstate->tep_token_deposit(common::xtoken_id_t::eth, token);
        if (ret) {
            xwarn("[xtop_transfer_contract::transfer] %s, deposit %s, token %s failed", recver_unitstate->get_account().c_str(), token.str().c_str(), token_name.c_str());
            std::error_code ec{contract_runtime::error::xenum_errc::enum_vm_exception};
            top::error::throw_error(ec, "tep_token_deposit failed");
        }
    } else {
        xassert(false);
        xwarn("[xtop_transfer_contract::transfer] bad token name: %s", token_name.c_str());
        std::error_code ec{contract_runtime::error::xenum_errc::enum_vm_exception};
        top::error::throw_error(ec, "tep_token_transfer failed");
    }

    return;
}

NS_END3
