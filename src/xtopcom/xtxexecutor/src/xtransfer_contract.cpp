#include "xtxexecutor/xcontract/xtransfer_contract.h"

#include "xdata/xgenesis_data.h"

NS_BEG3(top, txexecutor, contract)

void xtop_transfer_contract::deposit(const std::string & token_name, const base::vtoken_t token) {
    auto unitstate = unitstate_owned();
    xassert(unitstate != nullptr);
    if (token_name == data::XPROPERTY_ASSET_TOP) {
        auto ret = unitstate->token_deposit(data::XPROPERTY_BALANCE_AVAILABLE, token);
        if (ret) {
            xwarn("[xtop_transfer_contract::deposit] %s, deposit %lu, token: %s, failed", unitstate->get_account().c_str(), token, token_name.c_str());
            std::error_code ec{contract_runtime::error::xenum_errc::enum_vm_exception};
            top::error::throw_error(ec, "tep_token_deposit failed");
        }
    } else if (token_name == data::XPROPERTY_ASSET_ETH) {
        auto ret = unitstate->tep_token_deposit(data::XPROPERTY_TEP1_BALANCE_KEY, token_name, token);
        if (ret) {
            xwarn("[xtop_transfer_contract::deposit] %s, deposit %lu, token %s failed", unitstate->get_account().c_str(), token, token_name.c_str());
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

void xtop_transfer_contract::withdraw(const std::string & token_name, const base::vtoken_t token) {
    auto unitstate = unitstate_owned();
    xassert(unitstate != nullptr);
    if (token_name == data::XPROPERTY_ASSET_TOP) {
        auto ret = unitstate->token_withdraw(data::XPROPERTY_BALANCE_AVAILABLE, token);
        if (ret) {
            xwarn("[xtop_transfer_contract::withdraw] %s, withdraw %lu, token: %s, failed", unitstate->get_account().c_str(), token, token_name.c_str());
            std::error_code ec{contract_runtime::error::xenum_errc::enum_vm_exception};
            top::error::throw_error(ec, "tep_token_withdraw failed");
        }
    } else if (token_name == data::XPROPERTY_ASSET_ETH) {
        auto ret = unitstate->tep_token_withdraw(data::XPROPERTY_TEP1_BALANCE_KEY, token_name, token);
        if (ret) {
            xwarn("[xtop_transfer_contract::withdraw] %s, withdraw %lu, token %s failed", unitstate->get_account().c_str(), token, token_name.c_str());
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

void xtop_transfer_contract::transfer(const std::string & token_name, const base::vtoken_t token) {
    auto sender_unitstate = unitstate_owned();
    auto recver_unitstate = unitstate_other();
    xassert(sender_unitstate != nullptr);
    xassert(recver_unitstate != nullptr);
    if (token_name == data::XPROPERTY_ASSET_TOP) {
        auto ret = sender_unitstate->token_withdraw(data::XPROPERTY_BALANCE_AVAILABLE, token);
        if (ret) {
            xwarn("[xtop_transfer_contract::transfer] %s, withdraw %lu, token: %s, failed", sender_unitstate->get_account().c_str(), token, token_name.c_str());
            std::error_code ec{contract_runtime::error::xenum_errc::enum_vm_exception};
            top::error::throw_error(ec, "tep_token_withdraw failed");
        }
        ret = recver_unitstate->token_deposit(data::XPROPERTY_BALANCE_AVAILABLE, token);
        if (ret) {
            xwarn("[xtop_transfer_contract::transfer] %s, deposit %lu, token: %s, failed", recver_unitstate->get_account().c_str(), token, token_name.c_str());
            std::error_code ec{contract_runtime::error::xenum_errc::enum_vm_exception};
            top::error::throw_error(ec, "tep_token_deposit failed");
        }
    } else if (token_name == data::XPROPERTY_ASSET_ETH) {
        auto ret = sender_unitstate->tep_token_withdraw(data::XPROPERTY_TEP1_BALANCE_KEY, token_name, token);
        if (ret) {
            xwarn("[xtop_transfer_contract::transfer] %s, withdraw %lu, token %s failed", sender_unitstate->get_account().c_str(), token, token_name.c_str());
            std::error_code ec{contract_runtime::error::xenum_errc::enum_vm_exception};
            top::error::throw_error(ec, "tep_token_withdraw failed");
        }
        ret = recver_unitstate->tep_token_deposit(data::XPROPERTY_TEP1_BALANCE_KEY, token_name, token);
        if (ret) {
            xwarn("[xtop_transfer_contract::transfer] %s, deposit %lu, token %s failed", recver_unitstate->get_account().c_str(), token, token_name.c_str());
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
