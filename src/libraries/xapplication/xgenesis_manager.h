#include "xapplication/xerror/xerror.h"
#include "xbasic/xmemory.hpp"
#include "xchain_upgrade/xchain_data_processor.h"
#include "xcommon/xaddress.h"
#include "xstore/xstore_face.h"
#include "xvledger/xvblockstore.h"

#include <mutex>
#include <system_error>

NS_BEG2(top, application)

class xtop_genesis_manager {
public:
    explicit xtop_genesis_manager(observer_ptr<base::xvblockstore_t> const & blockstore, observer_ptr<store::xstore_face_t> const & store);
    xtop_genesis_manager(xtop_genesis_manager const &) = delete;
    xtop_genesis_manager & operator=(xtop_genesis_manager const &) = delete;
    xtop_genesis_manager(xtop_genesis_manager &&) = default;
    xtop_genesis_manager & operator=(xtop_genesis_manager &&) = default;
    ~xtop_genesis_manager() = default;

    /// @brief Create genesis block of accounts in different types necessary when application start. Throws xtop_error_t when any error occurs.
    void init_genesis_block();

    /// @brief Create genesis block of the specific account.
    /// @param account Account which need to create genesis block.
    void create_genesis_block(base::xvaccount_t const & account);

private:
    /// @brief Create genesis block of root account.
    /// @param account Root account.
    /// @param ec Log the error code.
    void create_genesis_of_root_account(base::xvaccount_t const & account, std::error_code & ec);

    /// @brief Create genesis block of contract account.
    /// @param account Contract account.
    /// @param ec Log the error code.
    void create_genesis_of_contract_account(base::xvaccount_t const & account, std::error_code & ec);

    /// @brief Create genesis block of genesis account.
    /// @param account Genesis account.
    /// @param data Data of genesis account, which means balance.
    /// @param ec Log the error code.
    void create_genesis_of_genesis_account(base::xvaccount_t const & account, uint64_t const data, std::error_code & ec);

    /// @brief Create genesis block of user account who has specific data.
    /// @param account User account.
    /// @param data Data of user account which recorded in file "xchain_data_new_horizons.h".
    /// @param ec Log the error code.
    void create_genesis_of_datauser_account(base::xvaccount_t const & account, chain_data::data_processor_t const & data, std::error_code & ec);

    /// @brief Create genesis block of none data account.
    /// @param account Common account.
    /// @param ec Log the error code.
    void create_genesis_of_common_account(base::xvaccount_t const & account, std::error_code & ec);

    observer_ptr<base::xvblockstore_t> m_blockstore;
    observer_ptr<store::xstore_face_t> m_store;
    std::mutex m_lock;
    bool init_finish{false};

    common::xaccount_address_t m_root_account;
    std::set<common::xaccount_address_t> m_contract_accounts;
    std::map<common::xaccount_address_t, uint64_t> m_genesis_accounts_data;
    std::map<common::xaccount_address_t, chain_data::data_processor_t> m_user_accounts_data;
};
using xgenesis_manager_t = xtop_genesis_manager;

NS_END2