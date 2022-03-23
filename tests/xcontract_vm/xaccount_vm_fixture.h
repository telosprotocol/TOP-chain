#pragma once
#include <sstream>
#define private public
#define protected public

#include "tests/mock/xvchain_creator.hpp"
#include "xbase/xutl.h"
#include "xdata/xblock.h"
#include "xsystem_contract_runtime/xsystem_contract_manager.h"
#include "xvledger/xvblockstore.h"

#include <gtest/gtest.h>

NS_BEG3(top, tests, contract_vm)

class test_contract_vm : public testing::Test {
public:
    void SetUp() override {
        m_blockstore = creator.get_blockstore();
        m_manager = new top::contract_runtime::system::xsystem_contract_manager_t();
        cs_para.m_clock = 5796740;
        cs_para.m_total_lock_tgas_token = 0;
        cs_para.m_proposal_height = 7;
        cs_para.m_account = "Ta0001@0";
        cs_para.m_random_seed = base::xstring_utl::base64_decode("ODI3OTg4ODkxOTMzOTU3NDk3OA==");
    }

    void TearDown() override {
        delete m_manager;
    }

    top::mock::xvchain_creator creator;
    base::xvblockstore_t * m_blockstore;
    top::contract_runtime::system::xsystem_contract_manager_t * m_manager{nullptr};
    top::data::xblock_consensus_para_t cs_para;
};

NS_END3
