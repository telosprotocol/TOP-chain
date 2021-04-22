#pragma once

#include <string>
#include <vector>
#include <map>
#include "xdata/xblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xtablestate.h"
#include "xstore/xstore_face.h"
#include "tests/mock/xcertauth_util.hpp"
#include "tests/mock/xdatamock_unit.hpp"

namespace top {
namespace mock {

using namespace top::data;

class xdatamock_table {
 protected:
    enum config_para {
        enum_default_full_table_interval_count = 128,
        enum_default_empty_table_interval_count = 10,
        enum_default_batch_table_include_unit_count = 1,
        enum_default_batch_table_include_tx_count = 1,
        enum_default_full_table_state_count = 2,
    };
 public:
    explicit xdatamock_table(uint16_t tableid = 1, uint32_t user_count = 4, store::xstore_face_t* store = nullptr) {
        m_table_account = xblocktool_t::make_address_shard_table_account(tableid);
        base::xvblock_t* genesis_table = xblocktool_t::create_genesis_empty_table(m_table_account);
        xblock_ptr_t block;
        block.attach((xblock_t*)genesis_table);
        m_history_tables.push_back(block);
        m_offstate = make_object_ptr<xtablestate_t>();

        set_xstore(store);

        m_table_blockchain = make_object_ptr<xblockchain2_t>(m_table_account);
        m_table_blockchain->update_state_by_genesis_block(block.get());

        for (uint32_t i = 0; i < user_count; i++) {
            std::string user = make_user_address(tableid);
            xdatamock_unit datamock_unit(user, xdatamock_unit::enum_default_init_balance, store);
            m_mock_units.push_back(datamock_unit);
        }
        xassert(m_mock_units.size() == user_count);
    }
    void    set_xstore(store::xstore_face_t* store) {m_store = store;}

    const std::string &                 get_account() const {return m_table_account;}
    const xblockchain_ptr_t &           get_blockchain() const {return m_table_blockchain;}
    const std::vector<xblock_ptr_t> &   get_history_tables() const {return m_history_tables;}
    const std::vector<xdatamock_unit> & get_mock_units() const {return m_mock_units;}
    static uint32_t                     get_full_table_interval_count() {return enum_default_full_table_interval_count;}

    void    genrate_table_chain(uint64_t max_block_height) {
        for (uint64_t i = 0; i < max_block_height; i++) {
            xblock_ptr_t block = generate_tableblock();
            xassert(block != nullptr);
            on_table_finish(block);
        }
    }

    xblock_ptr_t generate_tableblock() {
        xblock_ptr_t prev_tableblock = get_prev_block();
        xblock_ptr_t next_table = nullptr;
        uint32_t history_table_count = m_history_tables.size();
        if ( ((prev_tableblock->get_height() + 1) % enum_default_full_table_interval_count) == 0) {
            next_table = generate_full_table();
        } else if (history_table_count % enum_default_empty_table_interval_count == 0) {
            next_table = generate_empty_table();
        } else {
            next_table = generate_batch_table(m_mock_units.size() < 2 ? 1 : m_mock_units.size() / 2, 1);
        }
        return next_table;
    }
    void on_table_finish(const xblock_ptr_t & block) {
        xassert(get_prev_block()->get_height() == (block->get_height() - 1));
        xassert(m_table_blockchain->get_last_height() == (block->get_height() - 1));
        m_history_tables.push_back(block);

        m_table_blockchain->update_state_by_next_height_block(block.get());
        m_offstate->execute_block(block.get());

        auto tableblock = dynamic_cast<data::xtable_block_t*>(block.get());
        if (tableblock != nullptr) {
            auto & units = tableblock->get_tableblock_units(true);
            xassert(!units.empty());
            for (auto & unit : units) {
                on_unit_finish(unit);
            }

            // tableblock->create_txreceipts();  TODO
        }
    }
    void on_unit_finish(const xblock_ptr_t & unit) {
        for (auto & mockunit : m_mock_units) {
            if (unit->get_account() == mockunit.get_account()) {
                mockunit.on_unit_finish(unit);
            }
        }
    }

 private:
    xblock_ptr_t get_prev_block() const {
        return m_history_tables.back();
    }
    xblock_ptr_t generate_empty_table() {
        xblock_ptr_t prev_tableblock = get_prev_block();
        base::xvblock_t* proposal_block = xblocktool_t::create_next_emptyblock(prev_tableblock.get());
        assert(proposal_block != nullptr);
        xcertauth_util::instance().do_multi_sign(proposal_block);
        xblock_ptr_t block;
        block.attach((xblock_t*)proposal_block);
        return block;
    }

    xblock_ptr_t generate_full_table() {
        xblock_ptr_t prev_tableblock = get_prev_block();
        xstatistics_data_t block_statistics;
        xfulltable_block_para_t blockpara(m_offstate, block_statistics);
        base::xvblock_t* proposal_block = xblocktool_t::create_next_fulltable(blockpara, prev_tableblock.get());
        assert(proposal_block != nullptr);
        xcertauth_util::instance().do_multi_sign(proposal_block);
        xblock_ptr_t block;
        block.attach((xblock_t*)proposal_block);
        return block;

    }

    xblock_ptr_t generate_batch_table(uint32_t user_count, uint32_t every_user_tx_count) {
        generate_send_tx(user_count, every_user_tx_count);

        std::vector<xblock_ptr_t>   units;
        for (auto & mockunit : m_mock_units) {
            xblock_ptr_t unit = mockunit.generate_unit();
            if (unit != nullptr) {
                units.push_back(unit);
            }
        }
        xassert(units.size() > 0);

        xtable_block_para_t table_para;
        for (auto& unit : units) {
            table_para.add_unit(unit.get());
        }

        xblock_ptr_t prev_tableblock = get_prev_block();
        xblock_consensus_para_t cs_para{xcertauth_util::instance().get_leader_xip(), prev_tableblock.get()};
        base::xvblock_t* proposal_block = xblocktool_t::create_next_tableblock(table_para, cs_para, prev_tableblock.get());
        xcertauth_util::instance().do_multi_sign(proposal_block);
        assert(!proposal_block->get_cert()->get_verify_signature().empty());
        xblock_ptr_t block;
        block.attach((xblock_t*)proposal_block);
        return block;
    }

    void generate_send_tx(uint32_t user_count, uint32_t every_user_tx_count) {
        xassert(user_count <= m_mock_units.size());
        for (uint32_t i = 0; i < user_count; i++) {
            uint32_t send_index = (m_last_generate_send_tx_user_index + 1) % m_mock_units.size();
            uint32_t recv_index = (send_index + 1) % m_mock_units.size();
            m_last_generate_send_tx_user_index = send_index;
            xdatamock_unit & send_mockunit = m_mock_units[send_index];
            xdatamock_unit & recv_mockunit = m_mock_units[recv_index];
            send_mockunit.generate_transfer_tx(recv_mockunit.get_account(), every_user_tx_count);
        }
    }

    std::string     make_user_address(uint16_t tableid) {
        uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
        uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
        std::string addr = utl::xcrypto_util::make_address_by_random_key(addr_type, ledger_id);
        return addr;
    }

 private:
    std::string                     m_table_account;
    xblockchain_ptr_t               m_table_blockchain{nullptr};
    std::vector<xblock_ptr_t>       m_history_tables;
    uint32_t                        m_last_generate_send_tx_user_index = 0xFFFFFFFF;
    std::vector<xdatamock_unit>     m_mock_units;
    xtablestate_ptr_t               m_offstate{nullptr};
    store::xstore_face_t*           m_store{nullptr};
};

}
}
