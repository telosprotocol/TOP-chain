#include "xapplication/xgenesis_manager.h"

#include "xblockstore/src/xvgenesis.h"
#include "xdata/xblocktool.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xrootblock.h"
#include "xdata/xtransaction_v1.h"
#include "xdata/xunit_bstate.h"
#include "xstore/xaccount_context.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xsystem_contracts/deploy/xcontract_deploy.h"
#include "xvm/xvm_service.h"

#include <chrono>

NS_BEG2(top, application)

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

xtop_genesis_manager::xtop_genesis_manager(observer_ptr<base::xvblockstore_t> const & blockstore, observer_ptr<store::xstore_face_t> const & store)
  : m_blockstore{blockstore}, m_store{store} {
    xassert(m_blockstore != nullptr);
    xassert(store != nullptr);
    m_blockstore->register_create_genesis_callback(std::bind(&xgenesis_manager_t::create_genesis_block, this, std::placeholders::_1, std::placeholders::_2));
}

void xtop_genesis_manager::load_accounts() {
    // lock
    std::lock_guard<std::mutex> guard(m_lock);
    // step1: root account
    m_root_account = common::xaccount_address_t{data::xrootblock_t::get_rootblock_address()};
    // step2: system contract accounts
    auto const & system_contract_map = contract::xcontract_deploy_t::instance().get_map();
    for (auto const & pair : system_contract_map) {
        if (data::is_sys_sharding_contract_address(pair.first)) {
            for (auto i = 0; i < enum_vbucket_has_tables_count; i++) {
                m_contract_accounts.insert(data::make_address_by_prefix_and_subaddr(pair.first.value(), i));
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
}

void xtop_genesis_manager::release_accounts() {
    // lock
    std::lock_guard<std::mutex> guard(m_lock);
    // clear()
    m_contract_accounts.clear();
    m_genesis_accounts_data.clear();
    m_user_accounts_data.clear();
}

void xtop_genesis_manager::create_genesis_of_root_account(base::xvaccount_t const & account, xenum_create_src_t src, std::error_code & ec) {
    xinfo("[xtop_genesis_manager::create_genesis_of_root_account] address", account.get_account().c_str());
    // lock
    std::lock_guard<std::mutex> guard(m_lock);
    // check
    if (m_exist_accounts.count(m_root_account) || (src == xenum_create_src_t::init && m_blockstore->exist_genesis_block(base::xvaccount_t{m_root_account.value()}))) {
        if (!m_exist_accounts.count(m_root_account)) {
            m_exist_accounts.insert(m_root_account);
        }
        xinfo("[xtop_genesis_manager::create_genesis_of_root_account] account: %s, rootblock already exists", m_root_account.c_str());
        return;
    }
    base::xvblock_t * rootblock = data::xrootblock_t::get_rootblock();
    assert(rootblock != nullptr);
    // store block
    if (false == m_blockstore->store_block(account, rootblock)) {
        xerror("[xtop_genesis_manager::create_genesis_of_root_account] account: %s, store rootblock failed", account.get_account().c_str());
        xassert(false);
        SET_EC_RETURN(ec, error::xenum_errc::genesis_block_store_failed);
    }
    m_exist_accounts.insert(m_root_account);
    xinfo("[xtop_genesis_manager::create_genesis_of_root_account] account: %s, create rootblock success", m_root_account.c_str());
}


void xtop_genesis_manager::create_genesis_of_contract_account(base::xvaccount_t const & account, xenum_create_src_t src, std::error_code & ec) {
#if 1
    // lock
    std::lock_guard<std::mutex> guard(m_lock);
    // check
    if (m_exist_accounts.count(common::xaccount_address_t{account.get_account()})) {
        xinfo("[xtop_genesis_manager::create_genesis_of_contract_account] account: %s, system contract already exists", m_root_account.c_str());
        return;
    }
    // create in contract manager
    contract::xcontract_manager_t::instance().setup_chain(common::xaccount_address_t{account.get_account()}, m_blockstore.get());
    m_exist_accounts.insert(common::xaccount_address_t{account.get_account()});
#else
    xinfo("[xtop_genesis_manager::create_genesis_of_contract_account] address", account.get_account().c_str());
    // lock
    std::lock_guard<std::mutex> guard(m_lock);
    // check
    if (m_blockstore->exist_genesis_block(account)) {
        xinfo("[xtop_genesis_manager::create_genesis_of_contract_account] account %s, genesis block already exists", account.get_account().c_str());
        return;
    }
    // run setup of contract
    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_v1_t>();
    tx->make_tx_run_contract("setup", "");
    tx->set_same_source_target_address(account.get_account());
    tx->set_digest();
    tx->set_len();
    xobject_ptr_t<base::xvbstate_t> bstate =
        make_object_ptr<base::xvbstate_t>(account.get_account(), uint64_t{0}, uint64_t{0}, std::string{}, std::string{}, uint64_t{0}, uint32_t{0}, uint16_t{0});
    data::xaccount_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(bstate.get());
    store::xaccount_context_t ac(unitstate, m_store.get());
    xvm::xvm_service s;
    s.deal_transaction(tx, &ac);
    store::xtransaction_result_t result;
    ac.get_transaction_result(result);
    // create
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_lightunit(account.get_account(), tx, result);
    xassert(genesis_block != nullptr);
    // store block
    if (false == m_blockstore->store_block(account, genesis_block.get())) {
        xerror("[xtop_genesis_manager::create_genesis_of_contract_account] account: %s, store genesis block failed", account.get_account().c_str());
        xassert(false);
        SET_EC_RETURN(ec, error::xenum_errc::genesis_block_store_failed);
    }
    xdbg("[xtop_genesis_manager::create_genesis_of_contract_account] account: %s, create genesis block success", account.get_account().c_str());
#endif
}

void xtop_genesis_manager::create_genesis_of_datauser_account(base::xvaccount_t const & account, chain_data::data_processor_t const & data, xenum_create_src_t src, std::error_code & ec) {
    xinfo(
        "[xtop_genesis_manager::create_genesis_of_datauser_account] address=%s, balance=%ld, burn_balance=%ld, tgas_balance=%ld, vote_balance=%ld, lock_balance=%ld, "
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
    if (m_exist_accounts.count(common::xaccount_address_t{account.get_account()}) || (src == xenum_create_src_t::init && m_blockstore->exist_genesis_block(account))) {
        if (!m_exist_accounts.count(common::xaccount_address_t{account.get_account()})) {
            m_exist_accounts.insert(common::xaccount_address_t{account.get_account()});
        }
        xinfo("[xtop_genesis_manager::create_genesis_of_datauser_account] account: %s, genesis block already exists", account.get_account().c_str());
        return;
    }
    // create
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_lightunit(account.get_account(), data);
    xassert(genesis_block != nullptr);
    // store block
    if (false == m_blockstore->store_block(account, genesis_block.get())) {
        xerror("[xtop_genesis_manager::create_genesis_of_datauser_account] account: %s, store genesis block failed", account.get_account().c_str());
        xassert(false);
        SET_EC_RETURN(ec, error::xenum_errc::genesis_block_store_failed);
    }
    m_exist_accounts.insert(common::xaccount_address_t{account.get_account()});
    xinfo("[xtop_genesis_manager::create_genesis_of_datauser_account] account: %s, create genesis block success", account.get_account().c_str());
}

void xtop_genesis_manager::create_genesis_of_genesis_account(base::xvaccount_t const & account, uint64_t const data, xenum_create_src_t src, std::error_code & ec) {
    xinfo("[xtop_genesis_manager::create_genesis_of_genesis_account] address: %s, balance: %lu", account.get_account().c_str(), data);
    // lock
    std::lock_guard<std::mutex> guard(m_lock);
    // check
    if (m_exist_accounts.count(common::xaccount_address_t{account.get_account()}) || (src == xenum_create_src_t::init && m_blockstore->exist_genesis_block(account))) {
        if (!m_exist_accounts.count(common::xaccount_address_t{account.get_account()})) {
            m_exist_accounts.insert(common::xaccount_address_t{account.get_account()});
        }
        xinfo("[xtop_genesis_manager::create_genesis_of_genesis_account] account: %s, genesis block already exists", account.get_account().c_str());
        return;
    }
    // create
    base::xauto_ptr<base::xvblock_t> genesis_block = data::xblocktool_t::create_genesis_lightunit(account.get_account(), data);
    xassert(genesis_block != nullptr);
    // store block
    if (false == m_blockstore->store_block(account, genesis_block.get())) {
        xerror("[xtop_genesis_manager::create_genesis_of_genesis_account] account: %s, store genesis block failed", account.get_account().c_str());
        xassert(false);
        SET_EC_RETURN(ec, error::xenum_errc::genesis_block_store_failed);
    }
    m_exist_accounts.insert(common::xaccount_address_t{account.get_account()});
    xinfo("[xtop_genesis_manager::create_genesis_of_genesis_account] account: %s, create genesis block success", account.get_account().c_str());
}

void xtop_genesis_manager::create_genesis_of_common_account(base::xvaccount_t const & account, xenum_create_src_t src, std::error_code & ec) {
    xinfo("[xtop_genesis_manager::create_genesis_of_common_account] address: %s", account.get_account().c_str());
    // lock
    std::lock_guard<std::mutex> guard(m_lock);
    // // check
    if (src == xenum_create_src_t::init && m_blockstore->exist_genesis_block(account)) {
        xinfo("[xtop_genesis_manager::create_genesis_of_common_account] account: %s, genesis block already exists", account.get_account().c_str());
        return;
    }
    // create
    base::xauto_ptr<base::xvblock_t> genesis_block = store::xgenesis_block::create_genesis_block(account.get_account());
    // data::xblocktool_t::create_genesis_empty_block(account.get_account());
    xassert(genesis_block != nullptr);
    // store block
    if (false == m_blockstore->store_block(account, genesis_block.get())) {
        xerror("[xtop_genesis_manager::create_genesis_of_common_account] account: %s, store empty genesis block failed", account.get_account().c_str());
        xassert(false);
        SET_EC_RETURN(ec, error::xenum_errc::genesis_block_store_failed);
    }
    xinfo("[xtop_genesis_manager::create_genesis_of_common_account] account: %s, create empty genesis block success", account.get_account().c_str());
}

void start_blockstore_test_thread(std::string const & account, observer_ptr<base::xvblockstore_t> blockstore) {
    auto blockstore_test_thread = [](std::string const & account, observer_ptr<base::xvblockstore_t> blockstore) {
        printf("blockstore_test_thread for %s waiting...\n", account.c_str());
        std::this_thread::sleep_for(std::chrono::seconds(5));
        printf("blockstore_test_thread for %s start\n", account.c_str());
        auto vblock = blockstore->load_block_object(account, 0, 0, false);
        if (vblock == nullptr) {
            printf("load %s height 0 block null!\n", account.c_str());
        } else {
            printf("load %s height 0 block not null!\n", account.c_str());
        }
    };

    std::thread th(blockstore_test_thread, account, blockstore);
    th.join();
}

void xtop_genesis_manager::init_genesis_block(std::error_code & ec) {
    auto src = xenum_create_src_t::init;
    // step0: load accounts
    load_accounts();
    // step1: root account
    create_genesis_of_root_account(base::xvaccount_t{m_root_account.value()}, src, ec);
    CHECK_EC_RETURN(ec);

    // printf("test before root init...\n");
    // start_blockstore_test_thread(sys_contract_rec_registration_addr, m_blockstore);
    // printf("genesis_main_thread wating...\n");
    // std::this_thread::sleep_for(std::chrono::seconds(15));
    // // start_blockstore_test_thread(test_account, m_blockstore);
    // // printf("genesis_main_thread wating...\n");
    // // std::this_thread::sleep_for(std::chrono::seconds(30));
    // start_blockstore_test_thread("T00000LfhWJA5JPcKPJovoBVtN4seYnnsVjx2VuB", m_blockstore);
    // printf("genesis_main_thread wating...\n");
    // std::this_thread::sleep_for(std::chrono::seconds(15));
    // start_blockstore_test_thread("T00000LhyqGeUzUCAqfDbhTGcu5ToaReQy4BQNos", m_blockstore);
    // printf("genesis_main_thread wating...\n");
    // std::this_thread::sleep_for(std::chrono::seconds(15));

    m_root_finish = true;

    // printf("test after root init...\n");
    // start_blockstore_test_thread(sys_contract_rec_registration_addr, m_blockstore);
    // printf("genesis_main_thread wating...\n");
    // std::this_thread::sleep_for(std::chrono::seconds(15));
    // // start_blockstore_test_thread(test_account, m_blockstore);
    // // printf("genesis_main_thread wating...\n");
    // // std::this_thread::sleep_for(std::chrono::seconds(30));
    // start_blockstore_test_thread("T00000LfhWJA5JPcKPJovoBVtN4seYnnsVjx2VuB", m_blockstore);
    // printf("genesis_main_thread wating...\n");
    // std::this_thread::sleep_for(std::chrono::seconds(15));
    // start_blockstore_test_thread("T00000LhyqGeUzUCAqfDbhTGcu5ToaReQy4BQNos", m_blockstore);
    // printf("genesis_main_thread wating...\n");
    // std::this_thread::sleep_for(std::chrono::seconds(15));

    // step2: system contract accounts(reset)
    for (auto const & account : m_contract_accounts) {
        create_genesis_of_contract_account(base::xvaccount_t{account.value()}, src, ec);
        CHECK_EC_RETURN(ec);
    }
    // step3: user accounts with data(reset)
    if (false == chain_data::xtop_chain_data_processor::check_state()) {
        for (auto const & pair : m_user_accounts_data) {
            create_genesis_of_datauser_account(base::xvaccount_t{pair.first.value()}, pair.second, src, ec);
            CHECK_EC_RETURN(ec);
        }
        if (false == chain_data::xtop_chain_data_processor::set_state()) {
            xerror("[xtop_genesis_manager::init_genesis_block] chain_data_processor set state failed");
            SET_EC_RETURN(ec, error::xenum_errc::genesis_set_data_state_failed);
        }
    }
    // step4: genesis accounts(almost included in step3, so set it at last)
    for (auto const & pair : m_genesis_accounts_data) {
        create_genesis_of_genesis_account(base::xvaccount_t{pair.first.value()}, pair.second, src, ec);
        CHECK_EC_RETURN(ec);
    }
    // step5: set finish flag
    m_init_finish = true;
    // step6: release resource used by step2 and step3
    chain_data::xchain_data_processor_t::release();
    release_accounts();
}

void xtop_genesis_manager::create_genesis_block(base::xvaccount_t const & account, std::error_code & ec) {
    if (!m_root_finish) {
        SET_EC_RETURN(ec, error::xenum_errc::genesis_root_has_not_ready);
    }
    auto src = xenum_create_src_t::blockstore;
    if (!m_init_finish) {
        common::xaccount_address_t account_address{account.get_account()};
        if (account.is_contract_address()) {
            // check system contract account(reset)
            create_genesis_of_contract_account(account, src, ec);
            CHECK_EC_RETURN(ec);
        } else if (m_user_accounts_data.count(account_address)) {
            // check user account with data(reset)
            create_genesis_of_datauser_account(account, m_user_accounts_data[account_address], src, ec);
            CHECK_EC_RETURN(ec);
        } else if (m_genesis_accounts_data.count(account_address)) {
            // check genesis account
            create_genesis_of_genesis_account(account, m_genesis_accounts_data[account_address], src, ec);
            CHECK_EC_RETURN(ec);
        } else {
            // common account => empty genesis block
            create_genesis_of_common_account(account, src, ec);
            CHECK_EC_RETURN(ec);
        }
    } else {
        // empty genesis block
        create_genesis_of_common_account(account, src, ec);
        CHECK_EC_RETURN(ec);
    }
}

NS_END2