#pragma once

#include <string>

#include "xdata/xtransaction.h"
#include "xstore/xaccount_context.h"

namespace top {
namespace mock {

using namespace top::store;

class xdatamock_tx {
 public:
    void init_account_context() {
        {
            xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(m_source_account, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
            data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(bstate.get());
            m_source_context = std::make_shared<xaccount_context_t>(xaccount_context_t(unitstate));
        }

        {
            xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(m_target_account, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
            data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(bstate.get());
            m_target_context = std::make_shared<xaccount_context_t>(xaccount_context_t(unitstate));
        }
    }

    // only for transfer now
    void construct_tx(data::xtransaction_ptr_t & tx, bool is_eth_tx = false) {
        init_account_context();
        data::xproperty_asset asset{m_transfer_out_amount};
        if (is_eth_tx) {
            asset.m_token_name = data::XPROPERTY_ASSET_ETH;
        }
        tx->make_tx_transfer(asset);
        tx->set_different_source_target_address(m_source_account, m_target_account);
        tx->set_deposit(m_deposit);
        tx->set_digest();
    }

    std::shared_ptr<xaccount_context_t> get_source_context() {return m_source_context;}
    std::shared_ptr<xaccount_context_t> get_target_context() {return m_target_context;}
    uint64_t get_transfer_out_amount() {return m_transfer_out_amount;}
    uint32_t get_deposit() {return m_deposit;}

    void set_target_account(const std::string & target_account) {m_target_account = target_account;}

private:
    std::string m_source_account{"T8000037d4fbc08bf4513a68a287ed218b0adbd497ef30"};
    std::string m_target_account{"T80000fb8f4c7c8f3c1d58adf2173372a6ccda1350d10b"};
    std::shared_ptr<xaccount_context_t> m_source_context;
    std::shared_ptr<xaccount_context_t> m_target_context;
    uint64_t m_transfer_out_amount{10};
    uint32_t m_deposit{100000};
};

}
}
