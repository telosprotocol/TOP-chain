// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#define private public

#include "tests/xsystem_contract/xrec_standby_algorithm/xtest_registration_data_manager_fixture.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xvm/xsystem_contracts/xelection/xrec/xrec_standby_pool_contract.h"

NS_BEG3(top, tests, rec_standby)
using data::election::xstandby_result_store_t;
using top::xvm::system_contracts::rec::xtop_rec_standby_pool_contract;

class xtop_test_rec_standby_contract_fixture : public xtop_test_registration_data_manager_fixture {
public:
    xtop_test_rec_standby_contract_fixture() : rec_standby_contract{common::xnetwork_id_t{255}} {
    }
    // XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_test_rec_standby_contract_fixture);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_test_rec_standby_contract_fixture);
    XDECLARE_DEFAULTED_DESTRUCTOR(xtop_test_rec_standby_contract_fixture);

    xtop_rec_standby_pool_contract rec_standby_contract;

    xstandby_result_store_t standby_result_store;
    data::system_contract::xactivation_record record;
};

using xtest_rec_standby_contract_fixture_t = xtop_test_rec_standby_contract_fixture;
NS_END3
