#pragma once

#include <string>
#include <vector>
#include <map>
#include "xdata/xblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xblocktool.h"
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
    explicit xdatamock_table(uint16_t tableid = 1, uint32_t user_count = 4)
    : base::xvaccount_t(xblocktool_t::make_address_shard_table_account(tableid)) {
        m_fulltable_builder = std::make_shared<xfulltable_builder_t>();
        m_lighttable_builder = std::make_shared<xlighttable_builder_t>();

        xblock_ptr_t _block = build_genesis_table_block();
        on_table_finish(_block);

        for (uint32_t i = 0; i < user_count; i++) {
            std::string user = xdatamock_address::make_user_address_random(tableid);
            xdatamock_unit datamock_unit(user, xdatamock_unit::enum_default_init_balance);
            m_mock_units.push_back(datamock_unit);
        }
        xassert(m_mock_units.size() == user_count);
    }

    const base::xvaccount_t &           get_vaccount() const {return *this;}
    const xtablestate_ptr_t &           get_table_state() const {return m_table_state;}
    const std::vector<xblock_ptr_t> &   get_history_tables() const {return m_history_tables;}
    const std::vector<xdatamock_unit> & get_mock_units() const {return m_mock_units;}
    static uint32_t                     get_full_table_interval_count() {return enum_default_full_table_interval_count;}
    xblock_ptr_t                        get_cert_block() const {return m_history_tables.back();}
    xblock_ptr_t                        get_lock_block() const { return m_history_tables.size() == 1 ? m_history_tables[0] : m_history_tables[m_history_tables.size() - 2];}    

    void    genrate_table_chain(uint64_t max_block_height) {
        for (uint64_t i = 0; i < max_block_height; i++) {
            xblock_ptr_t block = generate_tableblock();
            xassert(block != nullptr);
            on_table_finish(block);
        }
    }

    xblock_ptr_t generate_tableblock() {
        xblock_ptr_t prev_tableblock = get_cert_block();
        xblock_consensus_para_t cs_para(xcertauth_util::instance().get_leader_xip(), prev_tableblock.get());
        
        xblock_ptr_t proposal_block = nullptr;
        uint32_t history_table_count = m_history_tables.size();
        if ( ((prev_tableblock->get_height() + 1) % enum_default_full_table_interval_count) == 0) {
            proposal_block = generate_full_table(cs_para);
        } else {
            cs_para.set_tableblock_consensus_para(1, "1", 1, "1"); // TODO(jimmy) for light-table
            proposal_block = generate_batch_table(cs_para);
        }
        if (proposal_block != nullptr) {
            xcertauth_util::instance().do_multi_sign(proposal_block.get());
        }
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
            xassert(sub_blocks.size() > 0);
            for (auto & unit : sub_blocks) {
                on_unit_finish(unit);
            }
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
        generate_send_tx();

        const base::xreceiptid_state_ptr_t & receiptid_state = m_table_state->get_receiptid_state();
        receiptid_state->clear_pair_modified();

        std::vector<xblock_ptr_t>   units;
        for (auto & mockunit : m_mock_units) {
            xblock_ptr_t unit = mockunit.generate_unit(receiptid_state, cs_para);
            if (unit != nullptr) {
                units.push_back(unit);
            }
        }
        xassert(units.size() > 0);

        cs_para.set_justify_cert_hash(get_lock_block()->get_input_root_hash());
        cs_para.set_parent_height(0);
        xblock_builder_para_ptr_t build_para = std::make_shared<xlighttable_builder_para_t>(units, m_default_resources);
        xblock_ptr_t proposal_block = m_lighttable_builder->build_block(get_cert_block(), m_table_state->get_bstate(), cs_para, build_para);
        return proposal_block;
    }

    void generate_send_tx() {
        uint32_t txs_count = 0;
        for (uint32_t i = 0; i < m_mock_units.size(); i++) {
            uint32_t send_index = (m_last_generate_send_tx_user_index + 1) % m_mock_units.size();
            uint32_t recv_index = (send_index + 1) % m_mock_units.size();
            m_last_generate_send_tx_user_index = send_index;
            xdatamock_unit & send_mockunit = m_mock_units[send_index];
            xdatamock_unit & recv_mockunit = m_mock_units[recv_index];
            std::vector<xcons_transaction_ptr_t> txs = send_mockunit.generate_transfer_tx(recv_mockunit.get_account(), 1);
            send_mockunit.push_txs(txs);

            txs_count++;
            if (txs_count >= enum_default_batch_table_include_tx_count_max) {
                break;
            }
        }
    }

 private:
    xtablestate_ptr_t               m_table_state{nullptr};
    std::vector<xblock_ptr_t>       m_history_tables;
    uint32_t                        m_last_generate_send_tx_user_index = 0xFFFFFFFF;
    std::vector<xdatamock_unit>     m_mock_units;
    xblock_builder_face_ptr_t       m_fulltable_builder;
    xblock_builder_face_ptr_t       m_lighttable_builder;
    xblockmaker_resources_ptr_t     m_default_resources{nullptr};
};

}
}
