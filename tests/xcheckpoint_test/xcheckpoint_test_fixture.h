#include "tests/mock/xcertauth_util.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "xblockstore/src/xvunithub.h"

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
            data::xblock_ptr_t block = generate_tableblock();
            xassert(block != nullptr);
            on_table_finish(block);
        }
    }

    data::xblock_ptr_t generate_tableblock() {
        data::xblock_ptr_t prev_tableblock = get_cert_block();
        data::xblock_consensus_para_t cs_para = init_consensus_para();

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

        data::xblock_ptr_t proposal_block = nullptr;
        uint32_t history_table_count = m_history_tables.size();
        if ((m_config_fulltable_interval != 0) && (((prev_tableblock->get_height() + 1) % m_config_fulltable_interval) == 0)) {
            proposal_block = generate_full_table(cs_para);
        } else {
            cs_para.set_tableblock_consensus_para(1, "1", 1, 1);  // TODO(jimmy) for light-table
            proposal_block = generate_batch_table(cs_para);
        }
        do_multi_sign(proposal_block);
        return proposal_block;
    }
};
using xcheckpoint_datamock_table_t = xtop_checkpoint_datamock_table;

class xtop_old_blockstore_impl : public store::xvblockstore_impl {
public:
    xtop_old_blockstore_impl(base::xcontext_t & _context, const int32_t target_thread_id, base::xvdbstore_t * xvdb_ptr) : xvblockstore_impl(_context, target_thread_id, xvdb_ptr) {
    }

    bool store_block_old(base::xauto_ptr<store::xblockacct_t> & container_account,
                     base::xvblock_t * container_block,
                     bool execute_block = true)  // store table/book blocks if they are
    {
        xdbg("jimmy xvblockstore_impl::store_block enter,store block(%s)", container_block->dump().c_str());

        // first do store block
        bool ret = container_account->store_block(container_block);
        if (!ret) {
            xwarn("xvblockstore_impl::store_block,fail-store block(%s)", container_block->dump().c_str());
            // return false;
        }

        bool did_stored = false;  // inited as false
        // then try extract for container if that is
        if ((container_block->get_block_class() == base::enum_xvblock_class_light)  // skip nil block
            && (container_block->get_block_level() == base::enum_xvblock_level_table) && (container_block->get_height() != 0)) {
            base::xauto_ptr<base::xvbindex_t> existing_index(container_account->load_index(container_block->get_height(), container_block->get_block_hash()));
            if (existing_index && (existing_index->get_block_flags() & base::enum_xvblock_flag_unpacked) == 0)  // unpacked yet
            {
                xassert(container_block->is_input_ready(true));
                xassert(container_block->is_output_ready(true));

                std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks;
                if (container_block->extract_sub_blocks(sub_blocks)) {
                    xdbg("xvblockstore_impl::store_block,table block(%s) carry unit num=%d", container_block->dump().c_str(), (int)sub_blocks.size());

                    bool table_extract_all_unit_successful = true;
                    for (auto & unit_block : sub_blocks) {
                        base::xvaccount_t unit_account(unit_block->get_account());
                        if (false == store_block_old(unit_account, unit_block.get()))  // any fail resultin  re-unpack whole table again
                        {
                            // table_extract_all_unit_successful = false;//reset to false for any failure of unit  // TODO(jimmy) always true if stored
                            xwarn("xvblockstore_impl::store_block,fail-store unit-block=%s", unit_block->dump().c_str());
                        } else {
                            xdbg("xvblockstore_impl::store_block,stored unit-block=%s", unit_block->dump().c_str());

                            on_block_stored(unit_block.get());  // throw event for sub blocks
                        }
                    }

                    // update to block'flag acccording table_extract_all_unit_successful
                    if (table_extract_all_unit_successful) {
                        existing_index->set_block_flag(base::enum_xvblock_flag_unpacked);
                        xinfo("xvblockstore_impl::store_block,extract_sub_blocks done for table block, %s", container_block->dump().c_str());
                    }
                } else {
                    xerror("xvblockstore_impl::store_block,fail-extract_sub_blocks for table block(%s)", container_block->dump().c_str(), (int)sub_blocks.size());
                }
            } else {
                did_stored = true;
            }
        }

        if (false == did_stored) {
            // move clean logic here to reduce risk of reenter process that might clean up some index too early
            if (container_block->get_block_level() == base::enum_xvblock_level_table)
                container_account->clean_caches(false, false);  // cache raw block londer for table with better performance
            else
                container_account->clean_caches(false, true);
        }
#if 0  // TODO(jimmy)
        if(execute_block)
        {
            container_account->try_execute_all_block(container_block);  // try to push execute block, ignore store result
        }
#endif
        return true;  // still return true since tableblock has been stored successful
    }

    #define LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account_vid) \
        if(is_close())\
        {\
            xwarn_err("xvblockstore has closed at store_path=%s",m_store_path.c_str());\
            return false;\
        }\
        base::xvtable_t * target_table = base::xvchain_t::instance().get_table(account_vid.get_xvid()); \
        if (target_table == nullptr) { \
            xwarn_err("xvblockstore invalid account=%s",account_vid.get_address().c_str());\
            return false;\
        }\
        store::auto_xblockacct_ptr account_obj(target_table->get_lock(),this); \
        get_block_account(target_table,account_vid.get_address(),account_obj); \

    #define LOAD_BLOCKACCOUNT_PLUGIN2(account_obj,account_vid) \
        if(is_close())\
        {\
            xwarn_err("xvblockstore has closed at store_path=%s",m_store_path.c_str());\
            return 0;\
        }\
        base::xvtable_t * target_table = base::xvchain_t::instance().get_table(account_vid.get_xvid()); \
        if (target_table == nullptr) { \
            xwarn_err("xvblockstore invalid account=%s",account_vid.get_address().c_str());\
            return 0;\
        }\
        store::auto_xblockacct_ptr account_obj(target_table->get_lock(),this); \
        get_block_account(target_table,account_vid.get_address(),account_obj); \

    bool store_block_old(const base::xvaccount_t & account, base::xvblock_t * block, const int atag = 0) {
        if ((nullptr == block) || (account.get_account() != block->get_account())) {
            xerror("xvblockstore_impl::store_block,block NOT match account:%", account.get_account().c_str());
            return false;
        }

        if (block->check_block_flag(base::enum_xvblock_flag_authenticated) == false ||
            (block->get_height() == 0 && block->check_block_flag(base::enum_xvblock_flag_committed) == false)) {
            xerror("xvblockstore_impl::store_block,unauthorized block(%s)", block->dump().c_str());
            return false;
        }

        bool ret = false;
        {
            LOAD_BLOCKACCOUNT_PLUGIN2(account_obj, account);
            ret = store_block_old(account_obj, block);
        }

        // XTODO only tabletable need execute immediately after stored
        // if (block->get_block_level() == base::enum_xvblock_level_table) {
        //     // TODO(jimmy) commit tableblock try to update table state
        //     base::auto_reference<base::xvblock_t> auto_hold_block_ptr(block);
        //     base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->execute_block(block, metrics::statestore_access_from_blockstore);
        // }

        return ret;
    }
};
using xold_blockstore_impl_t = xtop_old_blockstore_impl;

NS_END3
