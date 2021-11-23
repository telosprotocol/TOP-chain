#pragma once

#include <string>
#include <vector>
#include <map>
#include "xdata/xblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xblockbuild.h"
#include "xdata/xcons_transaction.h"
#include "xstore/xstore_face.h"
#include "xblockmaker/xtable_builder.h"
#include "tests/mock/xcertauth_util.hpp"
#include "tests/mock/xdatamock_unit.hpp"
#include "tests/mock/xdatamock_address.hpp"

namespace top {
namespace mock {

using namespace top::data;

class xdatamock_table : public base::xvaccount_t {
 protected:
    enum config_para {
        enum_default_full_table_interval_count = 128,
        enum_default_batch_table_include_tx_count_max = 40,
        enum_default_full_table_state_count = 2,
    };
 public:
    explicit xdatamock_table(uint16_t tableid = 1, uint32_t user_count = 4, bool is_fixed = false)
    : base::xvaccount_t(xblocktool_t::make_address_shard_table_account(tableid)) {
        m_fulltable_builder = std::make_shared<xfulltable_builder_t>();
        m_lighttable_builder = std::make_shared<xlighttable_builder_t>();

        xblock_ptr_t _block = build_genesis_table_block();
        on_table_finish(_block);

        if (is_fixed) {
            // two account is enough as reference block
            {
                xaddress_key_pair_t addr_pair;
                addr_pair.m_address = send_account;
                xdatamock_unit datamock_unit(addr_pair, xdatamock_unit::enum_default_init_balance);
                m_mock_units.push_back(datamock_unit);
            }
            {
                xaddress_key_pair_t addr_pair;
                addr_pair.m_address = recv_account;
                xdatamock_unit datamock_unit(addr_pair, xdatamock_unit::enum_default_init_balance);
                m_mock_units.push_back(datamock_unit);
            }
        } else {
            for (uint32_t i = 0; i < user_count; i++) {
                xaddress_key_pair_t addr_pair = xdatamock_address::make_unit_address_with_key(tableid);
                xdatamock_unit datamock_unit(addr_pair, xdatamock_unit::enum_default_init_balance);
                m_mock_units.push_back(datamock_unit);
            }
        }
        xassert(m_mock_units.size() == user_count);
    }

    const std::string send_account{"T00000LZbJfje6hW6MmG43h6EaTkme3ZqggGoUvF"};
    const std::string recv_account{"T00000LS5oCNzqoCFe5MTkGBJDyCHCM2wCkfFBLM"};

    void                                disable_fulltable() {m_config_fulltable_interval = 0;}
    const base::xvaccount_t &           get_vaccount() const {return *this;}
    const xtablestate_ptr_t &           get_table_state() const {return m_table_state;}
    const xtablestate_ptr_t &           get_commit_table_state() const {return m_table_states.front();}
    const std::vector<xblock_ptr_t> &   get_history_tables() const {return m_history_tables;}
    const std::vector<xdatamock_unit> & get_mock_units() const {return m_mock_units;}    
    static uint32_t                     get_full_table_interval_count() {return enum_default_full_table_interval_count;}
    xblock_ptr_t                        get_cert_block() const {return m_history_tables.back();}
    xblock_ptr_t                        get_lock_block() const { return m_history_tables.size() == 1 ? m_history_tables[0] : m_history_tables[m_history_tables.size() - 2];}
    xblock_ptr_t                        get_commit_block() const { return m_history_tables.size() < 3 ? get_lock_block() : m_history_tables[m_history_tables.size() - 3];}    
    std::vector<xblock_ptr_t>           get_all_genesis_units() const {
        std::vector<xblock_ptr_t> units;
        for (auto & mockunit : m_mock_units) {
            units.push_back(mockunit.get_history_units()[0]);
        }
        return units;
    }
    std::vector<std::string>            get_unit_accounts() const {
        std::vector<std::string> accounts;
        for (auto & mockunit : m_mock_units) {
            accounts.push_back(mockunit.get_account());
        }
        return accounts;
    }

    std::vector<xcons_transaction_ptr_t>   create_send_txs(const std::string & from, const std::string & to, uint32_t count) {
        for (auto & mockunit : m_mock_units) {
            if (from == mockunit.get_account()) {
                return mockunit.generate_transfer_tx(to, count);
            }
        }
        xassert(false);
        return {};
    }

    std::vector<xcons_transaction_ptr_t> create_receipts(const xblock_ptr_t & commit_block) {
        uint64_t cert_height = commit_block->get_height() + 2;
        if (m_history_tables.size() < cert_height + 1) {
            xassert(false);
            return {};
        }
        xblock_ptr_t cert_block = m_history_tables[cert_height];

        std::vector<data::xcons_transaction_ptr_t> all_cons_txs;
        std::vector<xfull_txreceipt_t> all_receipts = base::xtxreceipt_build_t::create_all_txreceipts(commit_block.get(), cert_block.get());
        for (auto & receipt : all_receipts) {
            data::xcons_transaction_ptr_t constx = make_object_ptr<data::xcons_transaction_t>(receipt);
            all_cons_txs.push_back(constx);
        }
        return all_cons_txs;
    }

    void    push_txs(const std::vector<xcons_transaction_ptr_t> & txs) {        
        for (auto & tx : txs) {
            push_tx(tx);
                xtransaction_ptr_t raw_tx_ptr;
                auto raw_tx = tx->get_transaction();
                raw_tx->add_ref();
                raw_tx_ptr.attach(raw_tx);
                m_raw_txs[tx->get_tx_hash()] = raw_tx_ptr;
        }
    }
    void    push_tx(const xcons_transaction_ptr_t & tx) {
        auto account_addr = tx->get_account_addr();
        for (auto & mockunit : m_mock_units) {
            if (account_addr == mockunit.get_account()) {
                mockunit.push_tx(tx, m_raw_txs);
                return;
            }
        }
        xdatamock_unit datamock_unit(account_addr, xdatamock_unit::enum_default_init_balance);
        m_mock_units.push_back(datamock_unit);
        datamock_unit.push_tx(tx, m_raw_txs);
    }
    xblock_ptr_t    generate_one_table() {
        xblock_ptr_t block = generate_tableblock();
        xassert(block != nullptr);
        on_table_finish(block);
        return block;
    }

    void    genrate_table_chain(uint64_t max_block_height) {
        for (uint64_t i = 0; i < max_block_height; i++) {
            generate_send_tx(); // auto generate txs internal
            xblock_ptr_t block = generate_tableblock();
            xassert(block != nullptr);
            on_table_finish(block);
        }
    }

    xblock_consensus_para_t  init_consensus_para(uint64_t clock = 10000000) {
        xblock_consensus_para_t cs_para(xcertauth_util::instance().get_leader_xip(), get_cert_block().get());
        cs_para.update_latest_cert_block(get_cert_block());
        cs_para.update_latest_lock_block(get_lock_block());
        cs_para.update_latest_commit_block(get_commit_block());
        cs_para.set_tableblock_consensus_para(1,"1",1,"1");
        cs_para.set_clock(clock);
        return cs_para;
    }

    void    do_multi_sign(const xblock_ptr_t & proposal_block, bool is_fixed = false) {
        if (proposal_block != nullptr) {
            xcertauth_util::instance().do_multi_sign(proposal_block.get(), is_fixed);
        }        
    }

    xblock_ptr_t generate_tableblock() {
        xblock_ptr_t prev_tableblock = get_cert_block();
        xblock_consensus_para_t cs_para = init_consensus_para();
        
        xblock_ptr_t proposal_block = nullptr;
        uint32_t history_table_count = m_history_tables.size();
        if ( (m_config_fulltable_interval != 0) && (((prev_tableblock->get_height() + 1) % m_config_fulltable_interval) == 0) ) {
            proposal_block = generate_full_table(cs_para);
        } else {
            cs_para.set_tableblock_consensus_para(1, "1", 1, "1"); // TODO(jimmy) for light-table
            proposal_block = generate_batch_table(cs_para);
        }
        do_multi_sign(proposal_block);
        return proposal_block;
    }
    void on_table_finish(const xblock_ptr_t & block) {
        if (block->get_height() > 0) {
            xassert(block->get_account() == get_account());
            xassert(block->get_height() == (m_history_tables.back()->get_height() + 1));
            xassert(block->get_last_block_hash() == (m_history_tables.back()->get_block_hash()));
        }
        m_history_tables.push_back(block);
        execute_block(block);

        if (block->get_block_class() == base::enum_xvblock_class_light) {
            std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks;
            block->extract_sub_blocks(sub_blocks);
            for (auto & unit : sub_blocks) {
                on_unit_finish(unit);
            }
        }

        //clear all unit mock txs
        for (auto & mockunit : m_mock_units) {
            mockunit.clear_txs();
        }
    }

    void on_unit_finish(const xobject_ptr_t<base::xvblock_t> & unit) {
        xblock_ptr_t _unit_ptr = dynamic_xobject_ptr_cast<xblock_t>(unit);
        for (auto & mockunit : m_mock_units) {
            if (unit->get_account() == mockunit.get_account()) {
                mockunit.on_unit_finish(_unit_ptr);
            }
        }
    }

 private:
    xblock_ptr_t    build_genesis_table_block() {
        base::xvblock_t* genesis_table = xblocktool_t::create_genesis_empty_table(get_account());
        xblock_ptr_t block;
        block.attach((xblock_t*)genesis_table);
        return block;
    }

    bool  execute_block(const xblock_ptr_t & _block) {
        xobject_ptr_t<base::xvbstate_t> current_state = nullptr;
        if (_block->get_height() == 0) {
            current_state = make_object_ptr<base::xvbstate_t>(*_block.get());
            if (_block->get_block_class() != base::enum_xvblock_class_nil) {
                std::string binlog = _block->get_block_class() == enum_xvblock_class_light ? _block->get_binlog() : _block->get_full_state();
                xassert(!binlog.empty());
                if(false == current_state->apply_changes_of_binlog(binlog)) {
                    xerror("execute_block,invalid binlog and abort it for block(%s)",_block->dump().c_str());
                    return false;
                }
            }
        } else {            
            const xobject_ptr_t<base::xvbstate_t> & base_state = m_table_state->get_bstate();
            xassert(_block->get_height() == base_state->get_block_height() + 1);
            current_state = make_object_ptr<xvbstate_t>(*_block.get(), *base_state.get());
            if (_block->get_block_class() == enum_xvblock_class_light) {
                std::string binlog = _block->get_binlog();
                xassert(!binlog.empty());
                if(false == current_state->apply_changes_of_binlog(binlog)) {
                    xerror("execute_block,invalid binlog and abort it for block(%s)",_block->dump().c_str());
                    return false;
                }
            }            
        }
        m_table_state = std::make_shared<xtable_bstate_t>(current_state.get());
        m_table_states.push_back(m_table_state);
        if (m_table_states.size() > 3) {
            m_table_states.pop_front();
        }
        return true;
    }

    xblock_ptr_t generate_full_table(const data::xblock_consensus_para_t & cs_para) {
        cs_para.set_justify_cert_hash(get_lock_block()->get_input_root_hash());
        cs_para.set_parent_height(0);

        std::vector<xblock_ptr_t> _form_highest_blocks;
        for (auto iter = m_history_tables.rbegin(); iter != m_history_tables.rend(); iter++) {
            auto & _block = *iter;
            if (_block->get_block_class() == base::enum_xvblock_class_full || _block->get_height() == 0) {
                break;
            }
            _form_highest_blocks.push_back(_block);
        }

        xblock_builder_para_ptr_t build_para = std::make_shared<xfulltable_builder_para_t>(m_table_state, _form_highest_blocks, m_default_resources);
        xblock_ptr_t proposal_block = m_fulltable_builder->build_block(get_cert_block(), m_table_state->get_bstate(), cs_para, build_para);
        return proposal_block;        

    }

    xblock_ptr_t generate_batch_table(const data::xblock_consensus_para_t & cs_para) {
        const base::xreceiptid_state_ptr_t & receiptid_state = m_table_state->get_receiptid_state();
        receiptid_state->clear_pair_modified();

        std::vector<xblock_ptr_t>   units;
        std::vector<xlightunit_tx_info_ptr_t> txs_info;
        for (auto & mockunit : m_mock_units) {
            xblock_ptr_t unit = mockunit.generate_unit(receiptid_state, cs_para);
            if (unit != nullptr) {
                units.push_back(unit);
            }
            auto txs = mockunit.get_exec_txs();
            // if (unit != nullptr && unit->get_block_class() == enum_xvblock_class_full) {
            //     EXPECT_EQ(txs.size(), 1);
            // }
            for (auto & tx : txs) {
                base::xvaction_t _action = data::make_action(tx);
                xlightunit_tx_info_ptr_t txinfo = std::make_shared<xlightunit_tx_info_t>(_action, tx->get_transaction());
                txs_info.push_back(txinfo);
            }
            mockunit.clear_exec_txs();
        }
        xassert(units.size() > 0);

        cs_para.set_justify_cert_hash(get_lock_block()->get_input_root_hash());
        cs_para.set_parent_height(0);
        xblock_builder_para_ptr_t build_para = std::make_shared<xlighttable_builder_para_t>(units, m_default_resources, txs_info);
        xblock_ptr_t proposal_block = m_lighttable_builder->build_block(get_cert_block(), m_table_state->get_bstate(), cs_para, build_para);
        return proposal_block;
    }

    void generate_send_tx() {
        xassert(m_mock_units.size() > 1);
        uint32_t txs_count = 0;
        for (uint32_t i = 0; i < m_mock_units.size(); i++) {
            uint32_t send_index = (m_last_generate_send_tx_user_index) % m_mock_units.size();
            uint32_t recv_index = (send_index + 1) % m_mock_units.size();
            xdatamock_unit & send_mockunit = m_mock_units[send_index];
            xdatamock_unit & recv_mockunit = m_mock_units[recv_index];
            std::vector<xcons_transaction_ptr_t> txs = send_mockunit.generate_transfer_tx(recv_mockunit.get_account(), 1);
            send_mockunit.push_txs(txs);

            m_last_generate_send_tx_user_index = (m_last_generate_send_tx_user_index + 1) % m_mock_units.size();

            txs_count++;
            if (txs_count >= enum_default_batch_table_include_tx_count_max) {
                break;
            }
        }
    }

 private:
    std::map<std::string, xtransaction_ptr_t> m_raw_txs;
    xtablestate_ptr_t               m_table_state{nullptr};
    std::deque<xtablestate_ptr_t>   m_table_states;  // save cert/lock/commit table states
    std::vector<xblock_ptr_t>       m_history_tables;
    uint32_t                        m_last_generate_send_tx_user_index{0};
    std::vector<xdatamock_unit>     m_mock_units;
    xblock_builder_face_ptr_t       m_fulltable_builder;
    xblock_builder_face_ptr_t       m_lighttable_builder;
    xblockmaker_resources_ptr_t     m_default_resources{nullptr};
    uint32_t                        m_config_fulltable_interval{enum_default_full_table_interval_count};
};

}
}
