#include "tests/mock/xcertauth_util.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "xchain_upgrade/xchain_checkpoint_center.h"

#include <gtest/gtest.h>

NS_BEG3(top, tests, xcheckpoint)

#define HEIGHT_1 10
#define HEIGHT_2 50
#define HEIGHT_3 200

#define TABLE_0 "Ta0000@0"
#define TABLE_1 "Ta0000@1"

class xtop_test_checkpoint_fixture : public testing::Test {
public:
    void SetUp() override {
    }

    void TearDown() override {
    }
};
using xtest_checkpoint_fixture_t = xtop_test_checkpoint_fixture;

class xtop_checkpoint_datamock_table : public mock::xdatamock_table {
public:
    explicit xtop_checkpoint_datamock_table(uint16_t tableid) : mock::xdatamock_table(tableid, 4) {
    }

    void genrate_table_chain(uint64_t max_block_height) {
        for (uint64_t i = 0; i < max_block_height; i++) {
            generate_send_tx();  // auto generate txs internal
            xblock_ptr_t block = generate_tableblock();
            xassert(block != nullptr);
            on_table_finish(block);
        }
    }

    xblock_ptr_t generate_tableblock() {
        xblock_ptr_t prev_tableblock = get_cert_block();
        xblock_consensus_para_t cs_para = init_consensus_para();

        if (prev_tableblock->get_account() == TABLE_0) {
            if (prev_tableblock->get_height() == 1 - 1) {
                cs_para.m_clock = HEIGHT_1;
            } else if (prev_tableblock->get_height() == 5 - 1) {
                cs_para.m_clock = HEIGHT_2;
            } else if (prev_tableblock->get_height() == 20 - 1) {
                cs_para.m_clock = HEIGHT_3;
            }
        } else if (prev_tableblock->get_account() == TABLE_1) {
            if (prev_tableblock->get_height() == 6 - 1) {
                cs_para.m_clock = HEIGHT_2;
            } else if (prev_tableblock->get_height() == 21 - 1) {
                cs_para.m_clock = HEIGHT_3;
            }
        }

        xblock_ptr_t proposal_block = nullptr;
        uint32_t history_table_count = m_history_tables.size();
        if ((m_config_fulltable_interval != 0) && (((prev_tableblock->get_height() + 1) % m_config_fulltable_interval) == 0)) {
            proposal_block = generate_full_table(cs_para);
        } else {
            cs_para.set_tableblock_consensus_para(1, "1", 1, "1");  // TODO(jimmy) for light-table
            proposal_block = generate_batch_table(cs_para);
        }
        do_multi_sign(proposal_block);
        return proposal_block;
    }
};
using xcheckpoint_datamock_table_t = xtop_checkpoint_datamock_table;

NS_END3