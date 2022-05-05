#pragma once

#include <string>
#include <vector>
#include "xdata/xblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xunit_bstate.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xtransaction_v1.h"
#include "xblockmaker/xblock_builder.h"
#include "xblockmaker/xblockmaker_face.h"
#include "xstore/xaccount_context.h"
#include "tests/mock/xcertauth_util.hpp"
#include "tests/mock/xdatamock_address.hpp"

namespace top {
namespace mock {

using namespace top::data;
using namespace top::blockmaker;

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
    xdatamock_unit(const std::string & address, uint64_t init_balance = enum_default_init_balance)
    : m_account(address) {
        m_enable_sign = false;
        xblock_ptr_t genesis_unit = build_genesis_block(init_balance);
        on_unit_finish(genesis_unit);
    }

    xdatamock_unit(const xaddress_key_pair_t & addr_pair, uint64_t init_balance = enum_default_init_balance)
    : m_account(addr_pair.m_address) {
        m_enable_sign = true;
        memcpy(m_private_key, addr_pair.m_private_key, 32);
        xblock_ptr_t genesis_unit = build_genesis_block(init_balance);
        on_unit_finish(genesis_unit);
    }

    const std::string &     get_account() const {return m_account;}
    xblock_ptr_t            get_cert_block() const {return m_history_units.back();}
    xblock_ptr_t            get_lock_block() const { return m_history_units.size() == 1 ? m_history_units[0] : m_history_units[m_history_units.size() - 2];}
    const std::vector<xblock_ptr_t> &   get_history_units() const {return m_history_units;}
    const std::vector<xcons_transaction_ptr_t> & get_txs() const {return m_current_txs;}
    xaccount_ptr_t          get_account_state() const {return m_unit_bstate;}

 public:
    xtransaction_ptr_t make_transfer_tx(uint256_t last_tx_hash, uint64_t last_tx_nonce, const std::string & from, const std::string & to,
        uint64_t amount, uint64_t firestamp, uint16_t duration, uint32_t deposit) {
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_v1_t>();
        data::xproperty_asset asset(amount);
        tx->make_tx_transfer(asset);
        tx->set_last_trans_hash_and_nonce(last_tx_hash, last_tx_nonce);
        tx->set_different_source_target_address(from, to);
        tx->set_fire_timestamp(firestamp);
        tx->set_expire_duration(duration);
        tx->set_deposit(deposit);
        tx->set_digest();
        if (m_enable_sign) {
            std::string signature = utl::xcrypto_util::digest_sign(tx->digest(), m_private_key);
            tx->set_authorization(signature);
        }
        tx->set_len();
        return tx;
    }
    
    static xtransaction_ptr_t make_fixed_transfer_tx(const std::string & from, const std::string & to, uint64_t last_nonce) {
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_v1_t>();
        data::xproperty_asset asset(100);
        tx->make_tx_transfer(asset);
        tx->set_last_nonce(last_nonce);
        tx->set_last_hash(100);
        tx->set_different_source_target_address(from, to);
        tx->set_fire_timestamp(10000);
        tx->set_expire_duration(100);
        tx->set_deposit(100000);
        tx->set_digest();
        std::string signature = "fixed for block hash";
        tx->set_authorization(signature);
        tx->set_len();
        return tx;
    }

    std::vector<xcons_transaction_ptr_t> generate_transfer_tx(const std::string & to, uint32_t count) {
        struct timeval val;
        base::xtime_utl::gettimeofday(&val);
        uint64_t amount = enum_default_transfer_amount;
        uint32_t deposit = enum_default_tx_deposit;

        uint256_t last_tx_hash = m_unit_bstate->account_send_trans_hash();
        uint64_t last_tx_nonce = m_unit_bstate->account_send_trans_number();

        std::vector<xcons_transaction_ptr_t> txs;
        for (uint32_t i = 0; i < count; i++) {
            xtransaction_ptr_t tx = make_transfer_tx(last_tx_hash, last_tx_nonce, get_account(), to, amount, (uint64_t)val.tv_sec, enum_default_tx_duration, deposit);
            xcons_transaction_ptr_t constx = make_object_ptr<xcons_transaction_t>(tx.get());
            txs.push_back(constx);
            last_tx_hash = tx->digest();
            last_tx_nonce = tx->get_tx_nonce();
        }
        return txs;
    }

    void clear_txs() {m_current_txs.clear();}

    void push_txs(const std::vector<xcons_transaction_ptr_t> & txs) {
        for (auto & tx : txs) {
            push_tx(tx, m_raw_txs);
        }
    }
    void push_tx(const xcons_transaction_ptr_t & tx, const std::map<std::string, xtransaction_ptr_t> & raw_txs) {
        std::string account = tx->get_account_addr();
        xassert(account == get_account());
        if (tx->is_confirm_tx()) {
            auto iter = raw_txs.find(tx->get_tx_hash());
            if (iter == raw_txs.end()) {
                xassert(false);
            } else {
                tx->set_raw_tx(iter->second.get());
            }
        }
        m_current_txs.push_back(tx);
    }

    void clear_exec_txs() {m_exec_txs.clear();}
    const std::vector<xcons_transaction_ptr_t> & get_exec_txs() const {return m_exec_txs;}

    xblock_ptr_t generate_unit(const base::xreceiptid_state_ptr_t & receiptid_state, const data::xblock_consensus_para_t & cs_para) {
        xblock_ptr_t prev_block = get_cert_block();
        xblock_ptr_t proposal_block = nullptr;

        // cs_para.set_justify_cert_hash(get_lock_block()->get_input_root_hash());
        proposal_block = build_block(prev_block, m_unit_bstate->get_bstate(), cs_para, m_current_txs);
        m_current_txs.clear();  // clear txs after make unit
        return proposal_block;
    }

    void on_unit_finish(const xblock_ptr_t & block) {
        if (block->get_height() > 0) {
            xassert(block->get_account() == get_account());
            xassert(block->get_height() == (m_history_units.back()->get_height() + 1));
            xassert(block->get_last_block_hash() == (m_history_units.back()->get_block_hash()));
        }
        m_history_units.push_back(block);

        // save raw tx
        const std::vector<xlightunit_tx_info_ptr_t> & sub_txs = block->get_txs();
        for (auto & v : sub_txs) {
            xtransaction_ptr_t raw_tx_ptr = v->get_raw_tx();
            if (raw_tx_ptr != nullptr) {
                m_raw_txs[v->get_tx_hash()] = raw_tx_ptr;
            }
        }

        execute_block(block);
    }

    virtual xblock_ptr_t        build_block(const xblock_ptr_t & prev_block,
                                            const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                            const data::xblock_consensus_para_t & cs_para,
                                            const std::vector<xcons_transaction_ptr_t> & input_txs) {
        base::xauto_ptr<base::xvheader_t> _temp_header = base::xvblockbuild_t::build_proposal_header(prev_block.get(), cs_para.get_clock());
        xobject_ptr_t<base::xvbstate_t> proposal_bstate = make_object_ptr<base::xvbstate_t>(*_temp_header.get(), *prev_bstate.get());
        xaccount_ptr_t proposal_state = std::make_shared<xunit_bstate_t>(proposal_bstate.get(), false);

        // TODO(jimmy) mock transfer only
        for (auto & tx : input_txs) {
            if (tx->is_send_tx()) {
                proposal_state->token_withdraw(data::XPROPERTY_BALANCE_AVAILABLE, base::vtoken_t(1));
                proposal_state->set_tx_info_latest_sendtx_num(tx->get_tx_nonce());
            } else if (tx->is_recv_tx()) {
                proposal_state->token_deposit(data::XPROPERTY_BALANCE_AVAILABLE, base::vtoken_t(1));
            } else if (tx->is_confirm_tx()) {
                
            }
            tx->set_current_exec_status(enum_xunit_tx_exec_status_success);
        }

        xunitbuilder_para_t unit_para({});
        data::xblock_ptr_t unitblock = xunitbuilder_t::make_block(prev_block, proposal_state, unit_para, cs_para);
        return unitblock;
    }    

 private:
    xblock_ptr_t    build_genesis_block(uint64_t init_balance) {
        base::xvblock_t* genesis_unit = xblocktool_t::create_genesis_lightunit(get_account(), init_balance);
        xblock_ptr_t block;
        block.attach((xblock_t*)genesis_unit);
        return block;
    }

    bool  execute_block(const xblock_ptr_t & _block) {
        xobject_ptr_t<base::xvbstate_t> current_state = nullptr;
        if (_block->get_height() == 0 || _block->get_block_class() == base::enum_xvblock_class_full) {
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
            xassert(m_unit_bstate != nullptr);
            xobject_ptr_t<base::xvbstate_t> base_state = m_unit_bstate->get_bstate();
            xassert(_block->get_height() == base_state->get_block_height() + 1);
            current_state = make_object_ptr<xvbstate_t>(*_block.get(), *base_state.get());
            if (_block->get_block_class() != enum_xvblock_class_nil) {
                std::string binlog = _block->get_block_class() == enum_xvblock_class_light ? _block->get_binlog() : _block->get_full_state();
                xassert(!binlog.empty());
                if(false == current_state->apply_changes_of_binlog(binlog)) {
                    xerror("execute_block,invalid binlog and abort it for block(%s)",_block->dump().c_str());
                    return false;
                }
            }
        }

        m_unit_bstate = std::make_shared<xunit_bstate_t>(current_state.get());
        return true;
    }

 private:
    bool                                    m_enable_sign{false};
    uint8_t                                 m_private_key[32];
    std::string                             m_account;
    xaccount_ptr_t                          m_unit_bstate{nullptr};
    std::vector<xcons_transaction_ptr_t>    m_current_txs;
    std::vector<xcons_transaction_ptr_t>    m_exec_txs;
    std::map<std::string, xtransaction_ptr_t> m_raw_txs;
    std::vector<xblock_ptr_t>               m_history_units;
    xblockmaker_resources_ptr_t             m_default_resources{nullptr};
};


}
}
