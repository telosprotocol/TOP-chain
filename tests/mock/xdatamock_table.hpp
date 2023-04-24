#pragma once

#include <string>
#include <vector>
#include <map>
#include "xdata/xblock_cs_para.h"
#include "xdata/xblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xblockbuild.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xethbuild.h"
#include "xblockmaker/xtable_builder.h"
#include "xblockmaker/xblock_builder.h"
#include "xtxexecutor/xbatchtx_executor.h"
#include "xtxexecutor/xunit_service_error.h"
#include "xtxexecutor/xatomictx_executor.h"
#include "xvledger/xvblockstore.h"
#include "xstate_mpt/xstate_mpt.h"
#include "tests/mock/xcertauth_util.hpp"
#include "tests/mock/xdatamock_unit.hpp"
#include "tests/mock/xdatamock_address.hpp"
#include "xdbstore/xstore_face.h"

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
    explicit xdatamock_table(base::enum_xchain_zone_index zone, uint16_t tableid, uint32_t user_count = 4)
    : base::xvaccount_t(xblocktool_t::make_address_table_account(zone, tableid)) {
        init(*this, user_count);
    }

    explicit xdatamock_table(uint16_t tableid = 1, uint32_t user_count = 4, bool is_fixed = false)
    : base::xvaccount_t(xblocktool_t::make_address_shard_table_account(tableid)) {
        if (is_fixed) {
            user_count = 2;// two account is enough as reference block
        }
        init(*this, user_count);
    }
    void init(base::xvaccount_t const& table_addr, uint32_t user_count) {
        m_fulltable_builder = std::make_shared<blockmaker::xfulltable_builder_t>();

        xblock_ptr_t _block = build_genesis_table_block();
        on_table_finish(_block);

        for (uint32_t i = 0; i < user_count; i++) {
            xaddress_key_pair_t addr_pair = xdatamock_address::make_unit_address_with_key(get_ledger_subaddr());
            xdatamock_unit datamock_unit(addr_pair, xdatamock_unit::enum_default_init_balance);
            m_mock_units.push_back(datamock_unit);
        }        
        xassert(m_mock_units.size() == user_count);

        m_db = db::xdb_factory_t::create_memdb();
        m_store = store::xstore_factory::create_store_with_static_kvdb(m_db);
    }

    void                                disable_fulltable() {m_config_fulltable_interval = 0;}
    const base::xvaccount_t &           get_vaccount() const {return *this;}
    const xtablestate_ptr_t &           get_table_state() const {return m_table_state;}
    const xtablestate_ptr_t &           get_commit_table_state() const {return m_table_states.front();}
    const std::vector<xblock_ptr_t> &   get_history_tables() const {return m_history_tables;}
    const std::vector<xdatamock_unit> & get_mock_units() const {return m_mock_units;} 

    xdatamock_unit                      find_mock_unit(std::string const& addr) {
        for (auto & mockunit : m_mock_units) {
            if (mockunit.get_account() == addr) {
                return mockunit;
            }
        }
        return xdatamock_unit(addr, 0);
    }

    static uint32_t                     get_full_table_interval_count() {return enum_default_full_table_interval_count;}
    xblock_ptr_t                        get_cert_block() const {return m_history_tables.back();}
    xblock_ptr_t                        get_lock_block() const { return m_history_tables.size() == 1 ? m_history_tables[0] : m_history_tables[m_history_tables.size() - 2];}
    xblock_ptr_t                        get_commit_block() const { return m_history_tables.size() < 3 ? get_lock_block() : m_history_tables[m_history_tables.size() - 3];}    

    void                                store_genesis_units(base::xvblockstore_t* blockstore) {
        for (auto & mockunit : m_mock_units) {
            auto gene_block = mockunit.get_history_units()[0];
            blockstore->store_unit(base::xvaccount_t(mockunit.get_account()), gene_block.get());
        }
    }

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

        std::vector<data::xcons_transaction_ptr_t> all_cons_txs = data::xblocktool_t::create_all_txreceipts(commit_block.get(), cert_block.get());
        return all_cons_txs;
    }

    void    push_txs(std::vector<xcons_transaction_ptr_t> const& txs) {
        m_proposal_txs = txs;
    }

    xblock_ptr_t    generate_one_table() {        
        xblock_ptr_t block = generate_tableblock();
        xassert(block != nullptr);
        on_table_finish(block);
        return block;
    }

    void    genrate_table_chain(uint64_t max_block_height, base::xvblockstore_t* blockstore) {
        for (uint64_t i = 0; i < max_block_height; i++) {
            generate_send_tx(); // auto generate txs internal
            generate_one_table();
        }

        // store genesis units
        if (blockstore != nullptr) {
            store_genesis_units(blockstore);
        }        
    }

    xblock_consensus_para_t  init_consensus_para(uint64_t clock = 100000000) {
        xblock_consensus_para_t cs_para(xcertauth_util::instance().get_leader_xip(), get_cert_block().get());
        cs_para.set_latest_blocks(get_cert_block(), get_lock_block(), get_commit_block());
        cs_para.set_tableblock_consensus_para(1,"1",1,1);
        cs_para.set_clock(clock);
        cs_para.set_timeofday_s((uint64_t)base::xtime_utl::gettimeofday());
        cs_para.set_parent_height(get_cert_block()->get_height() + 1);
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
        cs_para.set_parent_height(prev_tableblock->get_height()+1);
        
        xblock_ptr_t proposal_block = nullptr;
        uint32_t history_table_count = m_history_tables.size();
        if ( (m_config_fulltable_interval != 0) && (((prev_tableblock->get_height() + 1) % m_config_fulltable_interval) == 0) ) {
            proposal_block = generate_full_table(cs_para);
        } else {
            cs_para.set_tableblock_consensus_para(1, "1", 1, 1); // TODO(jimmy) for light-table
            proposal_block = generate_batch_table(cs_para);
        }
        do_multi_sign(proposal_block);
        return proposal_block;
    }
    void on_table_finish(const xblock_ptr_t & block) {
        // std::cout << "this:" << this << "  on_table_finish:" << block->dump() << std::endl;
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
                xassert(!unit->get_block_hash().empty());                           
                on_unit_finish(unit);
            }
        }

        m_batch_units.clear();
        m_proposal_txs.clear();
    }

    void on_unit_finish(const xobject_ptr_t<base::xvblock_t> & unit) {
        xblock_ptr_t _unit_ptr = dynamic_xobject_ptr_cast<xblock_t>(unit);
        for (auto & mockunit : m_mock_units) {
            if (unit->get_account() == mockunit.get_account()) {
                mockunit.on_unit_finish(_unit_ptr);
                return;
            }
        }
        xdatamock_unit s_mockunit = find_mock_unit(unit->get_account());
        s_mockunit.on_unit_finish(_unit_ptr);
        m_mock_units.push_back(s_mockunit);
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

        std::error_code ec;
        auto const & last_state_root = data::xblockextract_t::get_state_root(get_cert_block().get(), ec);
        xassert(!ec);
        data::xeth_header_t eth_header;
        eth_header.set_state_root(last_state_root);
        std::string _ethheader_str = eth_header.serialize_to_string();
        cs_para.set_ethheader(_ethheader_str);

        blockmaker::xblock_builder_para_ptr_t build_para = std::make_shared<blockmaker::xfulltable_builder_para_t>(m_table_state, _form_highest_blocks, m_default_resources);
        xblock_ptr_t proposal_block = m_fulltable_builder->build_block(get_cert_block(), m_table_state->get_bstate(), cs_para, build_para);
        return proposal_block;        

    }

    bool set_tx_table_state(const data::xtablestate_ptr_t & tablestate, const xcons_transaction_ptr_t & tx) {
        if ((tx->is_self_tx() || tx->get_inner_table_flag())) {
            xdbg("xatomictx_executor_t::set_tx_table_state not need.tx=%s", tx->dump().c_str());
            return true;
        }

        base::xtable_shortid_t peer_tableid = tx->get_peer_tableid();
        base::xreceiptid_pair_t receiptid_pair;
        tablestate->find_receiptid_pair(peer_tableid, receiptid_pair);

        if (data::xblocktool_t::alloc_transaction_receiptid(tx, receiptid_pair)) {
            tablestate->set_receiptid_pair(peer_tableid, receiptid_pair);  // save to modified pairs
            xinfo("xatomictx_executor_t::set_tx_table_state succ.tx=%s,pair=%s", tx->dump().c_str(), receiptid_pair.dump().c_str());
        } else {
            xerror("xatomictx_executor_t::set_tx_table_state fail.tx=%s,pair=%s", tx->dump().c_str(), receiptid_pair.dump().c_str());
        }
        return true;
    }

    data::xtablestate_ptr_t  build_proposal_state() {
        // create proposal header, clock use to set block version
        base::xauto_ptr<base::xvheader_t> _temp_header = base::xvblockbuild_t::build_proposal_header(get_cert_block().get(), get_cert_block()->get_clock()+1);
        // always clone new state
        xobject_ptr_t<base::xvbstate_t> proposal_bstate = make_object_ptr<base::xvbstate_t>(*_temp_header.get(), *(m_table_state->get_bstate()));

        return std::make_shared<data::xtable_bstate_t>(proposal_bstate.get(), m_table_state->get_bstate().get());
    }

    xblock_ptr_t generate_batch_table(const data::xblock_consensus_para_t & cs_para) {
        const base::xreceiptid_state_ptr_t & receiptid_state = m_table_state->get_receiptid_state();
        receiptid_state->clear_pair_modified();
        // create units
        m_batch_units.clear();        

        data::xtablestate_ptr_t proposal_table_state = build_proposal_state();
        std::map<std::string, data::xaccountstate_ptr_t>  proposal_states;

        txexecutor::xexecute_output_t execute_output;
        for (auto & tx : m_proposal_txs) {                
            std::string state_addr;
            if (tx->is_send_or_self_tx() || tx->is_confirm_tx()) {
                state_addr = tx->get_source_addr();
            } else {
                state_addr = tx->get_target_addr();
            }

            xdatamock_unit s_mockunit = find_mock_unit(state_addr);
            data::xaccountstate_ptr_t pstate;
            auto iter = proposal_states.find(s_mockunit.get_account());
            if (iter != proposal_states.end()) {
                pstate = iter->second;
            } else {
                pstate = s_mockunit.build_proposal_account_state();
                proposal_states[s_mockunit.get_account()] = pstate;
            }


            if (tx->is_send_or_self_tx() || tx->is_confirm_tx()){               
                pstate->get_unitstate()->token_withdraw(data::XPROPERTY_BALANCE_AVAILABLE, base::vtoken_t(1));    
                if (tx->is_send_or_self_tx()) {
                    pstate->set_tx_nonce(tx->get_tx_nonce());
                }
            }

            if (tx->is_recv_tx()) {
                pstate->get_unitstate()->token_deposit(data::XPROPERTY_BALANCE_AVAILABLE, base::vtoken_t(1));      
            }
            pstate->do_snapshot();

            txexecutor::xatomictx_output_t output;
            output.m_tx = tx;
            execute_output.pack_outputs.push_back(output);         

            set_tx_table_state(proposal_table_state, tx);   
        }

        for (auto & v : proposal_states) {
            xdatamock_unit s_mockunit = find_mock_unit(v.first);
            blockmaker::xunit_build_result_t unit_result;
            blockmaker::xunitbuilder_t::make_unitblock_and_unitstate(v.second, cs_para, unit_result);
            m_batch_units.push_back(std::make_pair(unit_result.unitblock, unit_result.accountindex));
        }

        cs_para.set_justify_cert_hash(get_lock_block()->get_input_root_hash());
        cs_para.set_parent_height(0);

        std::error_code ec;
        auto const & last_state_root = data::xblockextract_t::get_state_root(get_cert_block().get(), ec);
        xassert(!ec);
        auto table_mpt = state_mpt::xstate_mpt_t::create(common::xtable_address_t::build_from(get_cert_block()->get_account()), last_state_root, m_store.get(), ec);
        if (ec) {
            xassert(false);
        }

        data::xtable_block_para_t lighttable_para;
        for (auto & unit_and_index : m_batch_units) {
            auto & unit = unit_and_index.first;
            auto & index = unit_and_index.second;

            lighttable_para.set_unit(unit);
            lighttable_para.set_accountindex(unit->get_account(), index);       
            table_mpt->set_account_index(common::xaccount_address_t{unit->get_account()}, index, ec);
            if (ec) {
                xassert(false);
            }
        }
        auto root_hash = table_mpt->get_root_hash(ec);
        if (ec) {
            xassert(false);
        }
        xh256_t state_root = xh256_t(root_hash.to_bytes());
        data::xeth_header_t eth_header;
        eth_header.set_state_root(state_root);
        std::string _ethheader_str = eth_header.serialize_to_string();
        cs_para.set_ethheader(_ethheader_str);
 
        blockmaker::xtablebuilder_t::make_table_block_para(proposal_table_state, execute_output, lighttable_para);
        data::xblock_ptr_t proposal_block = blockmaker::xtablebuilder_t::make_light_block(get_cert_block(),
                                                                            cs_para,
                                                                            lighttable_para);
        xassert(nullptr != proposal_block);    
        table_mpt->commit(ec);                                                           
        return proposal_block;
    }

    void generate_send_tx() {
        xassert(m_mock_units.size() > 1);
        m_proposal_txs.clear();
        // uint32_t txs_count = 0;
        for (uint32_t i = 0; i < m_mock_units.size(); i++) {
            uint32_t send_index = (m_last_generate_send_tx_user_index) % m_mock_units.size();
            uint32_t recv_index = (send_index + 1) % m_mock_units.size();
            xdatamock_unit & send_mockunit = m_mock_units[send_index];
            xdatamock_unit & recv_mockunit = m_mock_units[recv_index];
            std::vector<xcons_transaction_ptr_t> txs = send_mockunit.generate_transfer_tx(recv_mockunit.get_account(), 1);
            m_proposal_txs.insert(m_proposal_txs.end(), txs.begin(), txs.end());

            m_last_generate_send_tx_user_index = (m_last_generate_send_tx_user_index + 1) % m_mock_units.size();

            // txs_count++;
            // if (txs_count >= enum_default_batch_table_include_tx_count_max) {
            //     break;
            // }
        }
    }

 private:
    std::vector<xcons_transaction_ptr_t>    m_proposal_txs;
    std::vector<std::pair<base::xvblock_ptr_t, data::xaccount_index_t>> m_batch_units;

    std::map<std::string, xtransaction_ptr_t> m_raw_txs;
    xtablestate_ptr_t               m_table_state{nullptr};
    std::deque<xtablestate_ptr_t>   m_table_states;  // save cert/lock/commit table states
    std::vector<xblock_ptr_t>       m_history_tables;
    uint32_t                        m_last_generate_send_tx_user_index{0};
    std::vector<xdatamock_unit>     m_mock_units;
    blockmaker::xblock_builder_face_ptr_t       m_fulltable_builder;
    blockmaker::xblockmaker_resources_ptr_t     m_default_resources{nullptr};
    uint32_t                        m_config_fulltable_interval{enum_default_full_table_interval_count};

    std::shared_ptr<db::xdb_face_t>      m_db{nullptr};
    xobject_ptr_t<store::xstore_face_t>  m_store{nullptr};
};

}
}
