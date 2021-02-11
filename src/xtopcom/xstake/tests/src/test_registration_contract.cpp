// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xip.h"
#include "xdata/xelect_transaction.hpp"
#include "xstore/xstore_face.h"
#include <gtest/gtest.h>
#include <memory>
#include <cmath>

#define private public
#include "xvm/xrec/xrec_registration_contract.h"
#include "xstake/xstake_algorithm.h"

using namespace top;
using namespace base;
using namespace xvm;
using namespace xstake;

TEST(test_registration_contract, all) {
    xobject_ptr_t<xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::shared_ptr<xaccount_context_t> account_ctx_ptr = make_shared<xaccount_context_t>(sys_contract_rec_registration_addr, store.get());
    std::shared_ptr<xcontract_helper>  contract_helper = make_shared<xcontract_helper>(account_ctx_ptr.get(), sys_contract_rec_registration_addr, sys_contract_rec_registration_addr);
    xrec_registration_contract contract(contract_helper);

    std::string account = "T0000014XLkV2uq649Xuom3SUiGb91g8HobxV8Cu";
    uint64_t mortgage   = 5000;
    uint64_t node_type  = xstake::enum_node_type_t::edge;
    contract.registerNode(account, mortgage, node_type);
}
