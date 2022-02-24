// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <utility>
#include "xbase/xns_macro.h"
#include "xbasic/xmulti_arg_macro.h"
#include "xcontract_base.h"
#include "xcontract_exec.h"
NS_BEG3(top, xvm, xcontract)
using std::string;
using std::shared_ptr;

/**
 * @brief contract register
 *
 */
class xcontract_register {
public:

    /**
     * @brief get the registered contract map object
     *
     * @return std::unordered_map<string, shared_ptr<xcontract_base>>&
     */
    static std::unordered_map<string, shared_ptr<xcontract_base>>& get_contract_map() {
        static std::unordered_map<string, shared_ptr<xcontract_base>> contract_map;
        return contract_map;
    }

    /**
     * @brief Construct a new xcontract register object
     *
     * @param name  the contract name
     * @param contract_ptr  the contract object ptr
     */
    xcontract_register(const string& name, shared_ptr<xcontract_base> contract_ptr) {
        printf("%s\n", name.c_str());
        get_contract_map()[name]= contract_ptr;
    }

    /**
     * @brief Get the contract object
     *
     * @param name  the contract name
     * @return shared_ptr<xcontract_base>
     */
    static shared_ptr<xcontract_base> get_contract(const std::string& name) {
        auto contract = get_contract_map().find(name);
        if (contract != get_contract_map().end()) {
            return contract->second;
        }
        return nullptr;
    }
};

#define REGISTER_NAME_CONTRACT(...)     OVERLOADED_MACRO(REGISTER_NAME_CONTRACT, __VA_ARGS__)

#define REGISTER_NAME_CONTRACT2(name, contract) static xcontract_register COMBINE_NAME(__contract_, __COUNTER__) (name, std::make_shared<contract>())

#define REGISTER_NAME_CONTRACT3(name, contract, helper) static xcontract_register COMBINE_NAME(__contract_, __COUNTER__) (name, std::make_shared<contract>(helper))

NS_END3
