#pragma once

#include <string>
#include <vector>
#include "xdata/xblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xcons_transaction.h"
#include "xstore/xstore_face.h"
#include "xcrypto/xckey.h"
#include "xblockmaker/xblockmaker_face.h"
#include "tests/mock/xcertauth_util.hpp"

namespace top {
namespace mock {

using namespace top::data;
using namespace top::utl;
using namespace top::blockmaker;
class xdatamock_tx {
 protected:
    enum config_para {
        enum_default_transfer_amount = 1,
        enum_default_tx_deposit = 1000000,
        enum_default_tx_duration = 100,
        enum_default_fullunit_interval_unit_count = 21,
        enum_default_init_balance = 10000000000,
    };

 public:
    xdatamock_tx(const xblockmaker_resources_ptr_t & resouces, const std::string & account, uint64_t init_balance = enum_default_init_balance)
    : m_resource(resouces), m_account(account), m_init_balance(init_balance) {
        base::xvblock_t* genesis_unit = xblocktool_t::create_genesis_lightunit(account, init_balance);
        xblock_ptr_t block;
        block.attach((xblock_t*)genesis_unit);

        m_blockchain = make_object_ptr<xblockchain2_t>(account);
        m_blockchain->update_state_by_genesis_block(block.get());
        xassert(m_blockchain->get_account_mstate().get_latest_send_trans_number() == 1);
        base::xvaccount_t _addr(account);
        m_resource->get_blockstore()->store_block(_addr, block.get());
        m_resource->get_blockstore()->execute_block(_addr, block.get());
    }

    xdatamock_tx(const xblockmaker_resources_ptr_t & resouces, const std::string & account, const xecprikey_t & pri_key_obj, uint64_t init_balance = enum_default_init_balance)
    : m_resource(resouces), m_account(account), m_pri_key_obj(pri_key_obj), m_init_balance(init_balance) {
        base::xvblock_t* genesis_unit = xblocktool_t::create_genesis_lightunit(account, init_balance);
        xblock_ptr_t block;
        block.attach((xblock_t*)genesis_unit);

        m_blockchain = make_object_ptr<xblockchain2_t>(account);
        m_blockchain->update_state_by_genesis_block(block.get());
        xassert(m_blockchain->get_account_mstate().get_latest_send_trans_number() == 1);
        base::xvaccount_t _addr(account);
        m_resource->get_blockstore()->store_block(_addr, block.get());
        m_resource->get_blockstore()->execute_block(_addr, block.get());
    }

    xdatamock_tx(const xblockmaker_resources_ptr_t & resouces, uint64_t init_balance = enum_default_init_balance) {
        uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
        uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
        xecpubkey_t pub_key_obj = m_pri_key_obj.get_public_key();
        m_account = pub_key_obj.to_address(addr_type, ledger_id);
        m_enable_sign = true;

        m_resource = resouces;
        m_init_balance = init_balance;

        base::xvblock_t* genesis_unit = xblocktool_t::create_genesis_lightunit(m_account, init_balance);
        xblock_ptr_t block;
        block.attach((xblock_t*)genesis_unit);

        m_blockchain = make_object_ptr<xblockchain2_t>(m_account);
        m_blockchain->update_state_by_genesis_block(block.get());
        xassert(m_blockchain->get_account_mstate().get_latest_send_trans_number() == 1);
        base::xvaccount_t _addr(m_account);
        m_resource->get_blockstore()->store_block(_addr, block.get());
        m_resource->get_blockstore()->execute_block(_addr, block.get());
    }

    const std::string & get_account() const {return m_account;}

    std::vector<xcons_transaction_ptr_t> generate_transfer_tx(const std::string & to, uint32_t count) {
        struct timeval val;
        base::xtime_utl::gettimeofday(&val);
        uint64_t amount = enum_default_transfer_amount;
        uint32_t deposit = enum_default_tx_deposit;

        std::vector<xcons_transaction_ptr_t> txs;

        xblockchain_ptr_t blockchain = m_blockchain;
        for (uint32_t i = 0; i < count; i++) {
            xtransaction_ptr_t tx = blockchain->make_transfer_tx(to, amount, (uint64_t)val.tv_sec, enum_default_tx_duration, deposit);
            if (m_enable_sign) {
                utl::xecdsasig_t signature_obj = m_pri_key_obj.sign(tx->digest());
                auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
                tx->set_signature(signature);
                tx->set_len();
            }
            xcons_transaction_ptr_t constx = make_object_ptr<xcons_transaction_t>(tx.get());
            txs.push_back(constx);
        }
        return txs;
    }

    std::vector<xcons_transaction_ptr_t> generate_contract_tx(const std::string & to, uint32_t count) {
        struct timeval val;
        base::xtime_utl::gettimeofday(&val);
        uint64_t amount = enum_default_transfer_amount;
        uint32_t deposit = enum_default_tx_deposit;

        std::vector<xcons_transaction_ptr_t> txs;

        xblockchain_ptr_t blockchain = m_blockchain;
        for (uint32_t i = 0; i < count; i++) {
            xtransaction_ptr_t tx = blockchain->make_run_contract_tx(to, "aaa", "bbb", 1, (uint64_t)val.tv_sec, enum_default_tx_duration, deposit);
            if (m_enable_sign) {
                utl::xecdsasig_t signature_obj = m_pri_key_obj.sign(tx->digest());
                auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
                tx->set_signature(signature);
                tx->set_len();
            }
            xcons_transaction_ptr_t constx = make_object_ptr<xcons_transaction_t>(tx.get());
            txs.push_back(constx);
        }
        return txs;
    }

    std::vector<xcons_transaction_ptr_t> generate_transfer_tx_with_db_state(const std::string & to, uint32_t count) {
        struct timeval val;
        base::xtime_utl::gettimeofday(&val);
        uint64_t amount = enum_default_transfer_amount;
        uint32_t deposit = enum_default_tx_deposit;

        std::vector<xcons_transaction_ptr_t> txs;

        xblockchain_ptr_t blockchain = m_resource->get_store()->query_account(get_account());
        xassert(blockchain != nullptr);
        for (uint32_t i = 0; i < count; i++) {
            xtransaction_ptr_t tx = blockchain->make_transfer_tx(to, amount, (uint64_t)val.tv_sec, enum_default_tx_duration, deposit);
            if (m_enable_sign) {
                utl::xecdsasig_t signature_obj = m_pri_key_obj.sign(tx->digest());
                auto signature = std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
                tx->set_signature(signature);
                tx->set_len();
            }
            xcons_transaction_ptr_t constx = make_object_ptr<xcons_transaction_t>(tx.get());
            txs.push_back(constx);
        }
        return txs;
    }

    static void do_mock_signature(base::xvblock_t* block, uint64_t clock = 0, uint64_t viewid = 0) {
        xassert(block->get_block_hash().empty());
        xassert(block->get_block_hash().empty());
        if (block->get_cert()->get_consensus_flags() & base::enum_xconsensus_flag_extend_cert) {
            block->set_extend_cert("1");
            block->set_extend_data("1");
        } else {
            block->set_verify_signature("1");
        }
        block->set_block_flag(base::enum_xvblock_flag_authenticated);
        xassert(!block->get_block_hash().empty());
    }

 private:
    xblockmaker_resources_ptr_t m_resource;
    std::string                 m_account;
    xecprikey_t                 m_pri_key_obj;
    uint64_t                    m_init_balance{0};
    xblockchain_ptr_t           m_blockchain{nullptr};
    bool                        m_enable_sign{false};
};

}
}
