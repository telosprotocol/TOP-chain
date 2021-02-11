// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"
#include "xstore/xstore_face.h"
#include "xtxpool_service/xrequest_tx_receiver_face.h"
#include "xvm/xcontract_info.h"
#include "xvnetwork/xvnetwork_driver_face.h"
#include "xblockstore/xsyncvstore_face.h"

NS_BEG2(top, contract)

using namespace top::mbus;
using namespace top::data;
using namespace top::store;
using namespace top::vnetwork;

struct xtable_schedule_info_t {
    uint16_t    cur_interval{0};
    uint16_t    target_interval{0};
    bool        clock_or_table{false}; ///< defualt false, means interval is clock interval, otherwise means table num
    uint16_t    cur_table{0};

    xtable_schedule_info_t() = default;
    xtable_schedule_info_t(uint16_t clock_interval, uint16_t start_table): target_interval{clock_interval}, cur_table{start_table}{}
};

class xrole_context_t {
public:
    xrole_context_t(const observer_ptr<xstore_face_t> &                                 store,
                    const xobject_ptr_t<store::xsyncvstore_t> &                         syncstore,
                    const std::shared_ptr<xtxpool_service::xrequest_tx_receiver_face> & unit_service,
                    const std::shared_ptr<xvnetwork_driver_face_t> &                    driver,
                    xcontract_info_t *                                                  info);
    virtual ~xrole_context_t();

    /**
     * @brief process store and chain timer event
     *
     * @param e Event ojbect.
     */
    void on_block(const xevent_ptr_t & e, bool & event_broadcasted);

    /**
     * @brief check if this timer round is valid
     *
     * @param onchain_timer_round
     * @return true
     * @return false
     */
    bool valid_call(const uint64_t onchain_timer_round);

protected:
    /**
     * @brief call the contract
     *
     * @param onchain_timer_round
     * @param info
     * @param block_timestamp
     */
    void call_contract(const uint64_t onchain_timer_round, xblock_monitor_info_t * info, const uint64_t block_timestamp);
    /**
     * @brief call the contract
     *
     * @param onchain_timer_round
     * @param info
     * @param block_timestamp
     * @param table_id
     */
    void call_contract(const uint64_t onchain_timer_round, xblock_monitor_info_t * info, const uint64_t block_timestamp, uint16_t table_id);
    /**
     * @brief call the contract
     *
     * @param action_params
     * @param timestamp
     * @param info
     */
    void call_contract(const std::string & action_params, uint64_t timestamp, xblock_monitor_info_t * info);
    /**
     * @brief call the contract
     *
     * @param action_params
     * @param timestamp
     * @param info
     * @param table_id
     */
    void call_contract(const std::string & action_params, uint64_t timestamp, xblock_monitor_info_t * info, uint16_t table_id);
    /**
     * @brief check if timer is unorder
     *
     * @param address
     * @param timestamp
     * @return true
     * @return false
     */
    bool is_timer_unorder(common::xaccount_address_t const & address, uint64_t timestamp);
    /**
     * @brief broadcast the block
     *
     * @param block_ptr
     * @param types
     */
    void broadcast(const xblock_ptr_t & block_ptr, common::xnode_type_t types);
    /**
     * @brief check if sys_addr is election contract and do not produce block
     *
     * @param timer_round
     * @param sys_addr
     * @return true
     * @return false
     */
    bool runtime_stand_alone(const uint64_t timer_round, common::xaccount_address_t const & sys_addr) const;

    /**
     * @brief check if the addr is table contract
     *
     * @param addr
     * @return true
     * @return false
     */
    bool is_scheduled_table_contract(common::xaccount_address_t const& addr) const;

protected:
    observer_ptr<xstore_face_t>                                                 m_store{};
    xobject_ptr_t<store::xsyncvstore_t>                                         m_syncstore{};
    std::shared_ptr<xtxpool_service::xrequest_tx_receiver_face>                 m_unit_service{};
    std::shared_ptr<xvnetwork_driver_face_t>                                    m_driver{};
    xcontract_info_t *                                                          m_contract_info{};
    std::unordered_map<common::xaccount_address_t, uint64_t>                    m_address_round_map;  // record address and timer round
    std::unordered_map<common::xaccount_address_t, xtable_schedule_info_t>      m_table_contract_schedule; // table schedule
};

NS_END2
