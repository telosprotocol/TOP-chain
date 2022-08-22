#pragma once

#include <string>
#include <vector>
#include "xdata/xblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xunit_bstate.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xtransaction_v1.h"
// #include "xstore/xaccount_context.h"
#include "tests/mock/xcertauth_util.hpp"
#include "tests/mock/xdatamock_address.hpp"

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
    xaccount_ptr_t          get_account_state() const {return m_unit_bstate;}
    data::xunitstate_ptr_t  build_proposal_state() {
        // create proposal header, clock use to set block version
        base::xauto_ptr<base::xvheader_t> _temp_header = base::xvblockbuild_t::build_proposal_header(get_cert_block().get(), get_cert_block()->get_clock()+1);
        // always clone new state
        xobject_ptr_t<base::xvbstate_t> proposal_bstate = make_object_ptr<base::xvbstate_t>(*_temp_header.get(), *(get_account_state()->get_bstate()));

        return std::make_shared<data::xunit_bstate_t>(proposal_bstate.get(), false);
    }

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

    void on_unit_finish(const xblock_ptr_t & block) {
        // std::cout << "-----on_unit_finish-:" << block->dump() << std::endl; 
        if (block->get_height() > 0) {
            xassert(block->get_account() == get_account());
            xassert(block->get_height() == (m_history_units.back()->get_height() + 1));
            xassert(block->get_last_block_hash() == (m_history_units.back()->get_block_hash()));
        }
        m_history_units.push_back(block);
        execute_block(block);
    }

 private:
    xblock_ptr_t    build_genesis_block(uint64_t init_balance) {
        base::xvblock_t* genesis_unit;
        if (init_balance == 0) {
            genesis_unit = xblocktool_t::create_genesis_empty_unit(get_account());
        } else {
            genesis_unit = xblocktool_t::create_genesis_lightunit(get_account(), init_balance);
        }        
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

        m_unit_bstate = std::make_shared<xunit_bstate_t>(current_state.get(), false);
        return true;
    }
 private:
    bool                                    m_enable_sign{false};
    uint8_t                                 m_private_key[32];
    std::string                             m_account;
    std::vector<xblock_ptr_t>               m_history_units;
    data::xunitstate_ptr_t                  m_unit_bstate{nullptr};
};


}
}
