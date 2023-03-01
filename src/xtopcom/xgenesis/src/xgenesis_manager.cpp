// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgenesis/xgenesis_manager.h"

#include "xdata/xblocktool.h"
#include "xdata/xblockbuild.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xrootblock.h"
#include "xdata/xtransaction_v1.h"
#include "xdata/xunit_bstate.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xevm_common/common_data.h"
#include "xevm_contract_runtime/xevm_contract_manager.h"
#include "xgenesis/xerror/xerror.h"
#include "xstore/xaccount_context.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/deploy/xcontract_deploy.h"
#include "xvm/xvm_service.h"
#include "xpbase/base/top_utils.h"

namespace top {
namespace genesis {

#define CHECK_EC_RETURN(ec)                                                                                                                                                        \
    do {                                                                                                                                                                           \
        if (ec) {                                                                                                                                                                  \
            xwarn("[xtop_genesis_manager] error occured, category: %s, msg: %s!", ec.category().name(), ec.message().c_str());                                                     \
            return;                                                                                                                                                                \
        }                                                                                                                                                                          \
    } while (0)

#define SET_EC_RETURN(ec, error)                                                                                                                                                   \
    do {                                                                                                                                                                           \
        ec = error;                                                                                                                                                                \
        xwarn("[xtop_genesis_manager] error occured, category: %s, msg: %s!", ec.category().name(), ec.message().c_str());                                                         \
        return;                                                                                                                                                                    \
    } while (0)

std::set<common::xaccount_address_t>     xtop_genesis_manager::m_all_genesis_accounts;

xtop_genesis_manager::xtop_genesis_manager(observer_ptr<base::xvblockstore_t> const & blockstore)
  : m_blockstore{blockstore} {
    xassert(m_blockstore != nullptr);
    m_blockstore->register_create_genesis_callback(std::bind(&xgenesis_manager_t::create_genesis_block, this, std::placeholders::_1, std::placeholders::_2));

    if (m_blockstore->exist_genesis_block(data::xrootblock_t::get_rootblock_address())) {
        m_root_finish = true;
    }
}

void xtop_genesis_manager::load_accounts() {
    // step1: lock
    std::lock_guard<std::mutex> guard(m_lock);
    // step2: system contract accounts
    auto const & system_contract_map = contract::xcontract_deploy_t::instance().get_map();
    for (auto const & pair : system_contract_map) {
        auto const & address = top::get<common::xaccount_address_t const>(pair);

        if (is_t2_address(address) && address_belongs_to_zone(address, common::xconsensus_zone_id)) {
            for (uint16_t i = 0; i < enum_vbucket_has_tables_count; i++) {
                m_contract_accounts.insert(append_table_id(address, common::xtable_id_t{i}));
            }
        } else {
            m_contract_accounts.insert(pair.first);
        }
    }
    // step3: user accounts with data
    m_user_accounts_data = chain_data::xchain_data_processor_t::get_all_user_data();
    // step4: genesis accounts
    auto const & genesis_accounts_data = data::xrootblock_t::get_all_genesis_accounts();
    for (auto const & pair : genesis_accounts_data) {
        m_genesis_accounts_data.insert({common::xaccount_address_t{pair.first}, pair.second});
    }
    // step5: new evm contract
    auto const & evm_system_contract_map = contract_runtime::evm::xevm_contract_manager_t::instance()->get_sys_contracts();
    for (auto const & pair : evm_system_contract_map) {
        m_evm_contract_accounts.insert(pair.first);
    }
}

void xtop_genesis_manager::release_accounts() {
    // lock
    std::lock_guard<std::mutex> guard(m_lock);
    // clear()
    m_evm_contract_accounts.clear();
    m_genesis_accounts_data.clear();
    m_user_accounts_data.clear();
    m_contract_accounts.clear();
}

void xtop_genesis_manager::create_genesis_of_root_account(std::error_code & ec) {
    if (m_root_finish == true) {
        return;
    }
    base::xvaccount_t account{data::xrootblock_t::get_rootblock_address()};
    xinfo("[xtop_genesis_manager::create_genesis_of_root_account] account %s", account.get_account().c_str());
    // lock
    std::lock_guard<std::mutex> guard(m_lock);
    // check
    if (m_blockstore->exist_genesis_block(account)) {
        xinfo("[xtop_genesis_manager::create_genesis_of_root_account] account: %s, rootblock already exists", account.get_account().c_str());
        return;
    }
    base::xvblock_t * rootblock = data::xrootblock_t::get_rootblock();
    xassert(rootblock != nullptr);
    // store block
    if (false == m_blockstore->store_block(account, rootblock)) {
        xerror("[xtop_genesis_manager::create_genesis_of_root_account] account: %s, store rootblock failed", account.get_account().c_str());
        xassert(false);
        SET_EC_RETURN(ec, error::xenum_errc::genesis_block_store_failed);
    }
    m_root_finish = true;
    xinfo("[xtop_genesis_manager::create_genesis_of_root_account] account: %s, create rootblock success", account.get_account().c_str());
}

base::xauto_ptr<base::xvblock_t> xtop_genesis_manager::create_genesis_of_contract_account(common::xaccount_address_t const & account, xenum_create_src_t src, std::error_code & ec) {
#if 0
    // lock
    std::lock_guard<std::mutex> guard(m_lock);
    // create in contract manager
    contract::xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{account.get_account()}, m_blockstore.get());
#else
    xinfo("[xtop_genesis_manager::create_genesis_of_contract_account] account %s", account.to_string().c_str());
    // lock
    std::lock_guard<std::mutex> guard(m_lock);
    // run setup of contract
    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_v1_t>();
    tx->make_tx_run_contract("setup", "");
    tx->set_same_source_target_address(account.to_string());
    tx->set_digest();
    tx->set_len();
    xobject_ptr_t<base::xvbstate_t> bstate =
        make_object_ptr<base::xvbstate_t>(account.to_string(), uint64_t{0}, uint64_t{0}, std::string{}, std::string{}, uint64_t{0}, uint32_t{0}, uint16_t{0});
    data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(bstate.get());
    store::xaccount_context_t ac(unitstate);
    xvm::xvm_service s;
    s.deal_transaction(tx, &ac);
    store::xtransaction_result_t result;
    ac.get_transaction_result(result);
    // create
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_lightunit(account.to_string(), tx, result);
    xassert(genesis_block != nullptr);
    // check
    xassert(!account.vaccount().get_account().empty());
    if (src == xenum_create_src_t::init && m_blockstore->exist_genesis_block(account.vaccount())) {
        auto const existed_genesis_block = m_blockstore->load_block_object(account.vaccount(), (uint64_t)0, (uint64_t)0, false);
        if (existed_genesis_block->get_block_hash() == genesis_block->get_block_hash()) {
            xinfo("[xtop_genesis_manager::create_genesis_of_contract_account] account %s, genesis block already exists", account.to_string().c_str());
            return nullptr;
        }
        if ((account.to_string().find(sys_contract_rec_elect_fullnode_addr) != std::string::npos) ||
            (account.to_string().find(sys_contract_rec_elect_exchange_addr) != std::string::npos) ||
            (account.to_string().find(sys_contract_zec_elect_eth_addr) != std::string::npos) ||
            (account.to_string().find(sys_contract_eth_table_cross_chain_txs_collection_addr) != std::string::npos) ||
            (account.to_string().find(sys_contract_zec_elect_relay_addr) != std::string::npos) ||
            (account.to_string().find(sys_contract_relay_make_block_addr) != std::string::npos) ||
            (account.to_string().find(sys_contract_sharding_fork_info_addr) != std::string::npos) ||
            (account.to_string().find(sys_contract_eth_fork_info_addr) != std::string::npos) || 
            (account.to_string().find(sys_contract_rec_node_manage_addr) != std::string::npos)) {
            // just delete it here and store new root block after
            xwarn("[xtop_genesis_manager::create_genesis_of_contract_account] account: %s genesis block exist but hash not match, replace it, %s, %s",
                  account.to_string().c_str(),
                  base::xstring_utl::to_hex(existed_genesis_block->get_block_hash()).c_str(),
                  base::xstring_utl::to_hex(genesis_block->get_block_hash()).c_str());
            m_blockstore->delete_block(account.vaccount(), existed_genesis_block.get());
        } else {
            xerror("[xtop_genesis_manager::create_genesis_of_contract_account] account: %s genesis block exist but hash not match, %s, %s",
                   account.to_string().c_str(),
                   base::xstring_utl::to_hex(existed_genesis_block->get_block_hash()).c_str(),
                   base::xstring_utl::to_hex(genesis_block->get_block_hash()).c_str());
            xassert(false);
            ec = error::xenum_errc::genesis_block_hash_mismatch;
            return nullptr;
        }
    }
    xinfo("[xtop_genesis_manager::create_genesis_of_contract_account] account: %s, create genesis block success", account.to_string().c_str());
    return genesis_block;
#endif
}

base::xauto_ptr<base::xvblock_t> xtop_genesis_manager::create_genesis_of_evm_contract_account(base::xvaccount_t const & account, xenum_create_src_t src, std::error_code & ec) {
    xinfo("[xtop_genesis_manager::create_genesis_of_evm_contract_account] account %s", account.get_account().c_str());
    if (account.get_account() == evm_eth_bridge_contract_address.to_string() || account.get_account() == evm_bsc_client_contract_address.to_string() ||
        account.get_account() == evm_heco_client_contract_address.to_string()) {
        xobject_ptr_t<base::xvbstate_t> bstate =
            make_object_ptr<base::xvbstate_t>(account.get_account(), uint64_t{0}, uint64_t{0}, std::string{}, std::string{}, uint64_t{0}, uint32_t{0}, uint16_t{0});
        xobject_ptr_t<base::xvcanvas_t> canvas = make_object_ptr<base::xvcanvas_t>();
        bstate->new_string_map_var(data::system_contract::XPROPERTY_HEADERS, canvas.get());
        bstate->new_string_map_var(data::system_contract::XPROPERTY_HEADERS_SUMMARY, canvas.get());
        bstate->new_string_map_var(data::system_contract::XPROPERTY_ALL_HASHES, canvas.get());
        bstate->new_string_map_var(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_LAST_HASH, canvas.get());
        auto bytes = (evm_common::h256()).asBytes();
        bstate->load_string_var(data::system_contract::XPROPERTY_LAST_HASH)->reset({bytes.begin(), bytes.end()}, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_RESET_FLAG, canvas.get());
        bstate->load_string_var(data::system_contract::XPROPERTY_RESET_FLAG)->reset(top::to_string(0), canvas.get());
        // create
        base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_lightunit(bstate, canvas);
        xassert(genesis_block != nullptr);
        if (src == xenum_create_src_t::init && m_blockstore->exist_genesis_block(account)) {
            auto const existed_genesis_block = m_blockstore->load_block_object(account, (uint64_t)0, (uint64_t)0, false);
            if (existed_genesis_block->get_block_hash() == genesis_block->get_block_hash()) {
                xinfo("[xtop_genesis_manager::create_genesis_of_evm_contract_account] account %s, genesis block already exists", account.get_account().c_str());
                return nullptr;
            } else {
                // just delete it here and store new root block after
                xwarn("[xtop_genesis_manager::create_genesis_of_evm_contract_account] account: %s genesis block exist but hash not match, replace it, %s, %s",
                    account.get_account().c_str(),
                    base::xstring_utl::to_hex(existed_genesis_block->get_block_hash()).c_str(),
                    base::xstring_utl::to_hex(genesis_block->get_block_hash()).c_str());
                m_blockstore->delete_block(account, existed_genesis_block.get());
            }
        }
        xinfo("[xtop_genesis_manager::create_genesis_of_contract_account] account: %s, create genesis block success", account.get_account().c_str());
        return genesis_block;
    } else if (account.get_account() == evm_eth2_client_contract_address.to_string()) {
        xobject_ptr_t<base::xvbstate_t> bstate =
            make_object_ptr<base::xvbstate_t>(account.get_account(), uint64_t{0}, uint64_t{0}, std::string{}, std::string{}, uint64_t{0}, uint32_t{0}, uint16_t{0});
        xobject_ptr_t<base::xvcanvas_t> canvas = make_object_ptr<base::xvcanvas_t>();
        bstate->new_string_map_var(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_BLOCKS, canvas.get());
        bstate->new_string_map_var(data::system_contract::XPROPERTY_UNFINALIZED_HEADERS, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_FINALIZED_BEACON_HEADER, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_HEADER, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_CURRENT_SYNC_COMMITTEE, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_NEXT_SYNC_COMMITTEE, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_RESET_FLAG, canvas.get());
        // create
        base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_lightunit(bstate, canvas);
        xassert(genesis_block != nullptr);
        if (src == xenum_create_src_t::init && m_blockstore->exist_genesis_block(account)) {
            auto const existed_genesis_block = m_blockstore->load_block_object(account, (uint64_t)0, (uint64_t)0, false);
            if (existed_genesis_block->get_block_hash() == genesis_block->get_block_hash()) {
                xinfo("[xtop_genesis_manager::create_genesis_of_evm_contract_account] account %s, genesis block already exists", account.get_account().c_str());
                return nullptr;
            } else {
                // just delete it here and store new root block after
                xwarn("[xtop_genesis_manager::create_genesis_of_evm_contract_account] account: %s genesis block exist but hash not match, replace it, %s, %s",
                    account.get_account().c_str(),
                    base::xstring_utl::to_hex(existed_genesis_block->get_block_hash()).c_str(),
                    base::xstring_utl::to_hex(genesis_block->get_block_hash()).c_str());
                m_blockstore->delete_block(account, existed_genesis_block.get());
            }
        }
        xinfo("[xtop_genesis_manager::create_genesis_of_contract_account] account: %s, create genesis block success", account.get_account().c_str());
        return genesis_block;
    }
    return nullptr;
}

base::xauto_ptr<base::xvblock_t> xtop_genesis_manager::create_genesis_of_datauser_account(base::xvaccount_t const & account,
                                                                                          chain_data::data_processor_t const & data,
                                                                                          xenum_create_src_t src,
                                                                                          std::error_code & ec) {
    xinfo(
        "[xtop_genesis_manager::create_genesis_of_datauser_account] account=%s, balance=%ld, burn_balance=%ld, tgas_balance=%ld, vote_balance=%ld, lock_balance=%ld, "
        "lock_tgas=%ld, unvote_num=%ld, expire_vote=%ld, create_time=%ld, lock_token=%ld, pledge_vote_str_cnt=%ld",
        account.get_account().c_str(),
        data.top_balance,
        data.burn_balance,
        data.tgas_balance,
        data.vote_balance,
        data.lock_balance,
        data.lock_tgas,
        data.unvote_num,
        data.expire_vote,
        data.create_time,
        data.lock_token,
        data.pledge_vote.size());
    // lock
    std::lock_guard<std::mutex> guard(m_lock);
    // check
    if (src == xenum_create_src_t::init && m_blockstore->exist_genesis_block(account)) {
        xinfo("[xtop_genesis_manager::create_genesis_of_datauser_account] account: %s, genesis block already exists", account.get_account().c_str());
        return nullptr;
    }
    // create
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_lightunit(account.get_account(), data);
    xassert(genesis_block != nullptr);
    xinfo("[xtop_genesis_manager::create_genesis_of_datauser_account] account: %s, create genesis block success", account.get_account().c_str());
    return genesis_block;
}

base::xauto_ptr<base::xvblock_t> xtop_genesis_manager::create_genesis_of_genesis_account(base::xvaccount_t const & account,
                                                                                         uint64_t const data,
                                                                                         xenum_create_src_t src,
                                                                                         std::error_code & ec) {
    xinfo("[xtop_genesis_manager::create_genesis_of_genesis_account] account: %s, balance: %lu", account.get_account().c_str(), data);
    // lock
    std::lock_guard<std::mutex> guard(m_lock);
    // check
    if (src == xenum_create_src_t::init && m_blockstore->exist_genesis_block(account)) {
        xinfo("[xtop_genesis_manager::create_genesis_of_genesis_account] account: %s, genesis block already exists", account.get_account().c_str());
        return nullptr;
    }
    // create
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_lightunit(account.get_account(), data);
    xassert(genesis_block != nullptr);
    xinfo("[xtop_genesis_manager::create_genesis_of_genesis_account] account: %s, create genesis block success", account.get_account().c_str());
    return genesis_block;
}

base::xauto_ptr<base::xvblock_t> xtop_genesis_manager::create_genesis_of_common_account(base::xvaccount_t const & account, xenum_create_src_t src, std::error_code & ec) {
    xdbg("[xtop_genesis_manager::create_genesis_of_common_account] account: %s", account.get_account().c_str());
    // lock
    std::lock_guard<std::mutex> guard(m_lock);
    // check
    // blockstore can create root multi times
    // create
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_empty_table(account.get_account());
    xassert(genesis_block != nullptr);
    genesis_block->reset_modified_count();
    xdbg("[xtop_genesis_manager::create_genesis_of_common_account] account: %s, create empty genesis block success", account.get_account().c_str());
    return genesis_block;
}

void xtop_genesis_manager::store_block(base::xvaccount_t const & account, base::xvblock_t * block, std::error_code & ec) {
    if (false == m_blockstore->store_block(account, block)) {
        xerror("[xtop_genesis_manager::store_block] account: %s, store genesis block failed", account.get_account().c_str());
        xassert(false);
        SET_EC_RETURN(ec, error::xenum_errc::genesis_block_store_failed);
    }
}

void xtop_genesis_manager::init_genesis_block(std::error_code & ec) {
    create_genesis_of_root_account(ec);
    CHECK_EC_RETURN(ec);

    auto src = xenum_create_src_t::init;
    // step1: load accounts
    load_accounts();
    // step2: system contract accounts(reset)
    for (auto const & account : m_contract_accounts) {
        auto vblock = create_genesis_of_contract_account(account, src, ec);
        CHECK_EC_RETURN(ec);
        if (vblock != nullptr) {
            store_block(base::xvaccount_t{account.to_string()}, vblock.get(), ec);
            CHECK_EC_RETURN(ec);
        }
        m_all_genesis_accounts.insert(account);
    }
    for (auto const & account : m_evm_contract_accounts) {
        auto vblock = create_genesis_of_evm_contract_account(base::xvaccount_t{account.to_string()}, src, ec);
        CHECK_EC_RETURN(ec);
        if (vblock != nullptr) {
            store_block(base::xvaccount_t{account.to_string()}, vblock.get(), ec);
            CHECK_EC_RETURN(ec);
        }
        m_all_genesis_accounts.insert(account);
    }
    {
        common::xaccount_address_t account(sys_contract_relay_block_addr);
        auto vblock = create_genesis_of_relay_account(base::xvaccount_t{account.to_string()}, src, ec);
        CHECK_EC_RETURN(ec);
        if (vblock != nullptr) {
            store_block(base::xvaccount_t{account.to_string()}, vblock.get(), ec);
            CHECK_EC_RETURN(ec);
        }
        m_all_genesis_accounts.insert(account);
    }
    // step3: user accounts with data(reset)
    if (false == chain_data::xtop_chain_data_processor::check_state()) {
        for (auto const & pair : m_user_accounts_data) {
            auto vblock = create_genesis_of_datauser_account(base::xvaccount_t{pair.first.to_string()}, pair.second, src, ec);
            CHECK_EC_RETURN(ec);
            if (vblock != nullptr) {
                store_block(base::xvaccount_t{pair.first.to_string()}, vblock.get(), ec);
                CHECK_EC_RETURN(ec);
            }
            m_all_genesis_accounts.insert(pair.first);
        }
        if (false == chain_data::xtop_chain_data_processor::set_state()) {
            xerror("[xtop_genesis_manager::init_genesis_block] chain_data_processor set state failed");
            SET_EC_RETURN(ec, error::xenum_errc::genesis_set_data_state_failed);
        }
    }
    // step4: genesis accounts(almost included in step3, so set it at last)
    for (auto const & pair : m_genesis_accounts_data) {
        auto vblock = create_genesis_of_genesis_account(base::xvaccount_t{pair.first.to_string()}, pair.second, src, ec);
        CHECK_EC_RETURN(ec);
        if (vblock != nullptr) {
            store_block(base::xvaccount_t{pair.first.to_string()}, vblock.get(), ec);
            CHECK_EC_RETURN(ec);
        }
        m_all_genesis_accounts.insert(pair.first);
    }
}

base::xauto_ptr<base::xvblock_t> xtop_genesis_manager::create_genesis_block(base::xvaccount_t const & account, std::error_code & ec) {
    if (!m_root_finish) {
        ec = error::xenum_errc::genesis_root_has_not_ready;
        return nullptr;
    }

    if (account.get_account() == data::xrootblock_t::get_rootblock_address()) {
        ec = error::xenum_errc::genesis_account_invalid;
        return nullptr;
    }

    auto constexpr src = xenum_create_src_t::blockstore;
    common::xaccount_address_t const account_address{account.get_account()};

    if (is_t2_address(account_address)) {
        // check system contract account(reset)
        return create_genesis_of_contract_account(account_address, src, ec);
    }

    if (m_user_accounts_data.count(account_address)) {
        // check user account with data(reset)
        return create_genesis_of_datauser_account(account, m_user_accounts_data[account_address], src, ec);
    }

    if (m_genesis_accounts_data.count(account_address)) {
        // check genesis account
        return create_genesis_of_genesis_account(account, m_genesis_accounts_data[account_address], src, ec);
    }

    // common account => empty genesis block
    return create_genesis_of_common_account(account, src, ec);
}
base::xauto_ptr<base::xvblock_t> xtop_genesis_manager::create_genesis_of_relay_account(base::xvaccount_t const & account, xenum_create_src_t src, std::error_code & ec) {
    xinfo("[xtop_genesis_manager::create_genesis_of_relay_account] account: %s", account.get_account().c_str());
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_wrap_relayblock();
    xassert(nullptr != genesis_block);
    return genesis_block;
}
}  // namespace genesis
}  // namespace top
