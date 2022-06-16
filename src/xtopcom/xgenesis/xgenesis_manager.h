// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "xapplication/xerror/xerror.h"
#include "xbasic/xmemory.hpp"
#include "xchain_upgrade/xchain_data_processor.h"
#include "xcommon/xaddress.h"
#include "xstore/xstore_face.h"
#include "xvledger/xvblockstore.h"

#include <mutex>
#include <system_error>

namespace top {
namespace genesis {

enum xtop_enum_create_src {
    init,
    blockstore,
};
using xenum_create_src_t = xtop_enum_create_src;

class xtop_genesis_manager {
public:
    xtop_genesis_manager(observer_ptr<base::xvblockstore_t> const & blockstore, observer_ptr<store::xstore_face_t> const & store);
    xtop_genesis_manager(xtop_genesis_manager const &) = delete;
    xtop_genesis_manager & operator=(xtop_genesis_manager const &) = delete;
    xtop_genesis_manager(xtop_genesis_manager &&) = default;
    xtop_genesis_manager & operator=(xtop_genesis_manager &&) = default;
    ~xtop_genesis_manager() = default;

    /// @brief Create genesis block of accounts in different types necessary when application start.
    /// @param ec Log the error code.
    void init_genesis_block(std::error_code & ec);

    /// @brief Create genesis block of the specific account.
    /// @param account Account which need to create genesis block.
    /// @param ec Log the error code.
    /// @return Block created.
    base::xauto_ptr<base::xvblock_t> create_genesis_block(base::xvaccount_t const & account, std::error_code & ec);

private:
    /// @brief Create genesis block of root account.
    /// @param ec Log the error code.
    void create_genesis_of_root_account(std::error_code & ec);

    /// @brief Create genesis block of contract account.
    /// @param account Contract account.
    /// @param ec Log the error code.
    /// @return Block created.
    base::xauto_ptr<base::xvblock_t> create_genesis_of_contract_account(base::xvaccount_t const & account, xenum_create_src_t src, std::error_code & ec);

    /// @brief Create genesis block of evm contract account.
    /// @param account Evm contract account.
    /// @param ec Log the error code.
    /// @return Block created.
    base::xauto_ptr<base::xvblock_t> create_genesis_of_evm_contract_account(base::xvaccount_t const & account, xenum_create_src_t src, std::error_code & ec);

    /// @brief Create genesis block of relay account.
    /// @param account Relay account.
    /// @param ec Log the error code.
    /// @return Block created.
    base::xauto_ptr<base::xvblock_t> create_genesis_of_relay_account(base::xvaccount_t const & account, xenum_create_src_t src, std::error_code & ec);

    /// @brief Create genesis block of genesis account.
    /// @param account Genesis account.
    /// @param data Data of genesis account, which means balance.
    /// @param ec Log the error code.
    /// @return Block created.
    base::xauto_ptr<base::xvblock_t> create_genesis_of_genesis_account(base::xvaccount_t const & account, uint64_t const data, xenum_create_src_t src, std::error_code & ec);

    /// @brief Create genesis block of user account who has specific data.
    /// @param account User account.
    /// @param data Data of user account which recorded in file "xchain_data_new_horizons.h".
    /// @param ec Log the error code.
    /// @return Block created.
    base::xauto_ptr<base::xvblock_t> create_genesis_of_datauser_account(base::xvaccount_t const & account,
                                                                        chain_data::data_processor_t const & data,
                                                                        xenum_create_src_t src,
                                                                        std::error_code & ec);

    /// @brief Create genesis block of none data account.
    /// @param account Common account.
    /// @param ec Log the error code.
    /// @return Block created.
    base::xauto_ptr<base::xvblock_t> create_genesis_of_common_account(base::xvaccount_t const & account, xenum_create_src_t src, std::error_code & ec);

    /// @brief Store genesis block created.
    /// @param account Account.
    /// @param block Block to store.
    /// @param ec Log the error code.
    void store_block(base::xvaccount_t const & account, base::xvblock_t * block, std::error_code & ec);

    /// @brief Load accounts of different types.
    void load_accounts();

    /// @brief Release resources by load_accounts.
    void release_accounts();

    observer_ptr<base::xvblockstore_t> m_blockstore;
    observer_ptr<store::xstore_face_t> m_store;
    bool m_init_finish{false};
    bool m_root_finish{false};
    std::mutex m_lock;

    std::set<common::xaccount_address_t> m_contract_accounts;
    std::set<common::xaccount_address_t> m_evm_contract_accounts;
    std::map<common::xaccount_address_t, uint64_t> m_genesis_accounts_data;
    std::map<common::xaccount_address_t, chain_data::data_processor_t> m_user_accounts_data;
};
using xgenesis_manager_t = xtop_genesis_manager;

}  // namespace genesis
}  // namespace top