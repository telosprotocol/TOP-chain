#pragma once

#include <string>
#include <vector>
#include "xdata/xblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xcons_transaction.h"
#include "xstore/xstore_face.h"
#include "tests/mock/xcertauth_util.hpp"

namespace top {
namespace mock {

using namespace top::data;

class xdatamock_unit {
 public:
    enum config_para {
        enum_default_transfer_amount = 1,
        enum_default_tx_deposit = 1000000,
        enum_default_tx_duration = 100,
        enum_default_fullunit_interval_unit_count = 21,
        enum_default_init_balance = 10000000000,
    };

 public:
    xdatamock_unit(const std::string & account, uint64_t init_balance = enum_default_init_balance, store::xstore_face_t* store = nullptr)
    : m_account(account), m_init_balance(init_balance), m_store(store) {
        base::xvblock_t* genesis_unit = xblocktool_t::create_genesis_lightunit(account, init_balance);
        xblock_ptr_t block;
        block.attach((xblock_t*)genesis_unit);
        m_history_units.push_back(block);

        if (m_store != nullptr) {
            xassert(m_store->set_vblock({}, genesis_unit) == true);
            xassert(m_store->execute_block(genesis_unit) == true);
        }

        m_blockchain = make_object_ptr<xblockchain2_t>(account);
        m_blockchain->update_state_by_genesis_block(block.get());
    }

    const std::string & get_account() const {return m_account;}
    xblock_ptr_t get_latest_unit() const {return m_history_units.back();}
    const std::vector<xblock_ptr_t> &   get_history_units() const {return m_history_units;}
    const std::vector<xcons_transaction_ptr_t> & get_txs() const {return m_current_txs;}

 public:
    void generate_transfer_tx(const std::string & to, uint32_t count) {
        struct timeval val;
        base::xtime_utl::gettimeofday(&val);
        uint64_t amount = enum_default_transfer_amount;
        uint32_t deposit = enum_default_tx_deposit;

        xblockchain_ptr_t blockchain = clone_blockchain();
        for (uint32_t i = 0; i < count; i++) {
            xtransaction_ptr_t tx = blockchain->make_transfer_tx(to, amount, (uint64_t)val.tv_sec, enum_default_tx_duration, deposit);
            xcons_transaction_ptr_t constx = make_object_ptr<xcons_transaction_t>(tx.get());
            m_current_txs.push_back(constx);
        }
    }

    void push_receipt() {

    }

    xblock_ptr_t generate_unit(const base::xreceiptid_state_ptr_t & receiptid_state) {
        xblock_ptr_t prev_block = m_history_units.back();
        base::xvblock_t* unit;
        if (prev_block->is_lightunit() && prev_block->get_height() > (prev_block->get_last_full_block_height() + enum_default_fullunit_interval_unit_count)) {
            xfullunit_block_para_t fullunit_para;
            fullunit_para.m_account_state = m_blockchain->get_account_mstate();  // TODO(jimmy) account ptr pass for performance
            fullunit_para.m_first_unit_hash = m_blockchain->get_last_full_unit_hash();
            fullunit_para.m_first_unit_height = m_blockchain->get_last_full_unit_height();
            unit = xblocktool_t::create_next_fullunit(fullunit_para, prev_block.get());
        } else {
            if (m_current_txs.empty()) {
                return nullptr;
            }
            int64_t balance_change = 0;
            for (auto & tx : m_current_txs) {
                if (tx->is_send_tx()) {
                    balance_change -= enum_default_transfer_amount;
                } else if (tx->is_recv_tx()) {
                    balance_change += enum_default_transfer_amount;
                }
                data::xblocktool_t::alloc_transaction_receiptid(tx, receiptid_state);
            }
            xlightunit_block_para_t lightunit_para;
            lightunit_para.set_input_txs(m_current_txs);
            lightunit_para.set_balance_change(balance_change);
            lightunit_para.set_account_unconfirm_sendtx_num(m_blockchain->get_unconfirm_sendtx_num());
            unit = xblocktool_t::create_next_lightunit(lightunit_para, prev_block.get());

            m_current_txs.clear();  // clear txs after make unit
        }
        xblock_ptr_t block;
        block.attach((xblock_t*)unit);
        return block;
    }

    void on_unit_finish(const xblock_ptr_t & block) {
        xassert(block->get_account() == get_account());
        xassert(block->get_height() == (m_history_units.back()->get_height() + 1));
        m_history_units.push_back(block);
        m_blockchain->update_state_by_next_height_block(block.get());

        data::xlightunit_block_t * lightunit = dynamic_cast<data::xlightunit_block_t *>(block.get());
        if (lightunit != nullptr) {
            // const std::vector<xlightunit_tx_info_ptr_t> & txs = lightunit->get_txs();
            // for (auto & tx : txs) {
            //     pop_tx_by_hash(tx->get_tx_hash_256(), tx->get_tx_subtype(), 0);
            // }
        }
    }

 private:
    xblockchain_ptr_t clone_blockchain() {
        base::xstream_t stream(base::xcontext_t::instance());
        m_blockchain->serialize_to(stream);
        xblockchain_ptr_t blockchain = make_object_ptr<xblockchain2_t>();
        blockchain->serialize_from(stream);
        return blockchain;
    }

 private:
    std::string                             m_account;
    uint64_t                                m_init_balance{0};
    xblockchain_ptr_t                       m_blockchain{nullptr};
    std::vector<xcons_transaction_ptr_t>    m_current_txs;
    std::vector<xblock_ptr_t>               m_history_units;
    store::xstore_face_t*                   m_store{nullptr};
};

}
}
