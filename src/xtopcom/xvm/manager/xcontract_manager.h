// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <json/json.h>
#include "xbase/xlock.h"
#include "xbasic/xmemory.hpp"
#include "xblockstore/xsyncvstore_face.h"
#include "xcommon/xaddress.h"
#include "xmbus/xbase_sync_event_monitor.hpp"
#include "xmbus/xevent_vnode.h"
#include "xmbus/xevent_store.h"

#include "xvledger/xvaccount.h"
#include "xvledger/xvcnode.h"
#include "xvm/manager/xcontract_register.h"
#include "xvm/manager/xrole_context.h"
#include "xvnetwork/xmessage_callback_hub.h"
#include "xvnetwork/xvhost_face.h"

#include <cstdint>
#include <list>
#include <mutex>
#include <string>
#include <vector>

NS_BEG2(top, contract)

using xrole_map_t = std::unordered_map<vnetwork::xvnetwork_driver_face_t *, xrole_context_t *>;
enum class xtop_enum_json_format : uint8_t {
    invalid,
    simple,
    detail
};
using xjson_format_t = xtop_enum_json_format;

class xtop_contract_manager final : public mbus::xbase_sync_event_monitor_t {
public:
    xtop_contract_manager(){
    }
    virtual ~xtop_contract_manager();

    /**
     * @brief get an instance
     *
     * @return xtop_contract_manager&
     */
    static xtop_contract_manager & instance();

    /**
     * @brief register system contracts
     *
     */
    void instantiate_sys_contracts();
    void init(xobject_ptr_t<store::xsyncvstore_t> const & syncstore);

    /**
     * @brief register_address
     */
    void register_address();

    /**
     * @brief install monitors
     *
     * @param bus message bus
     * @param msg_callback_hub callback function
     * @param store store
     * @param syncstore sync store
     */
    void install_monitors(observer_ptr<mbus::xmessage_bus_face_t> const &               bus,
                          observer_ptr<vnetwork::xmessage_callback_hub_t> const & msg_callback_hub,
                          xobject_ptr_t<store::xsyncvstore_t> const&              syncstore);

    /**
     * @brief clear
     *
     */
    void clear();

    /**
     * @brief register the contract
     *
     * @tparam T
     * @param name contract name
     * @param network_id network id
     */
    template <typename T>
    void register_contract(common::xaccount_address_t const & name, common::xnetwork_id_t const & network_id) {
        m_contract_register.add<T>(name, network_id);
    }

    /**
     * @brief Get the contract object
     *
     * @param address contract address
     * @return xcontract_base*
     */
    xvm::xcontract::xcontract_base * get_contract(common::xaccount_address_t const & address);
    /**
     * @brief register the contract object
     *
     * @param address contract address
     * @param cluster_address contract cluster address
     */
    void register_contract_cluster_address(common::xaccount_address_t const & address, common::xaccount_address_t const & cluster_address);
    /**
     * @brief Get the node service object
     *
     * @return base::xvnodesrv_t*
     */
    base::xvnodesrv_t * get_node_service() const noexcept;
    /**
     * @brief Get the thread object
     *
     * @return observer_ptr<xiothread_t>
     */
    observer_ptr<base::xiothread_t> get_thread() const noexcept { return m_observed_thread; }

    // for tests
    /**
     * @brief Get the m_map object
     *
     * @return std::unordered_map<common::xaccount_address_t, xrole_map_t *> const&
     */
    std::unordered_map<common::xaccount_address_t, xrole_map_t *> const & get_map() const noexcept { return m_map; }

    /**
     * @brief Get the contract inst map object
     *
     * @return std::unordered_map<common::xaccount_address_t, xcontract_base *> const&
     */
    std::unordered_map<common::xaccount_address_t, xvm::xcontract::xcontract_base *> const & get_contract_inst_map() const noexcept { return m_contract_inst_map; }

    /**
     * @brief Set the nodesrv ptr object
     *
     * @param nodesrv nodesrv ptr
     */
    static void set_nodesrv_ptr(const xobject_ptr_t<base::xvnodesrv_t> &nodesrv) {m_nodesvr_ptr = nodesrv.get();}
    /**
     * @brief Get the account from xip object
     *
     * @param target_node target node xvip2
     * @param target_addr target node address
     * @return int32_t
     */
    static int32_t get_account_from_xip(const xvip2_t & target_node, std::string& target_addr);

    void get_contract_data(common::xaccount_address_t const & contract_address, xjson_format_t const json_format, bool compatible_mode, Json::Value & json) const;
    void get_contract_data(common::xaccount_address_t const & contract_address, std::uint64_t const height, xjson_format_t const json_format, Json::Value & json, std::error_code & ec) const;
    void get_contract_data(common::xaccount_address_t const & contract_address,
                           std::string const & property_name,
                           xjson_format_t const json_format,
                           bool compatible_mode,
                           Json::Value & json) const;
    void get_contract_data(common::xaccount_address_t const & contract_address,
                           data::xunitstate_ptr_t const & unitstate,
                           std::string const & property_name,
                           xjson_format_t const json_format,
                           bool compatible_mode,
                           Json::Value & json) const;
    // void get_contract_data(common::xaccount_address_t const & contract_address, std::string const & property_name, std::string const & key, xjson_format_t const json_format, Json::Value & json) const;

    void get_election_data(common::xaccount_address_t const & contract_address, const data::xunitstate_ptr_t unitstate, std::string const & property_name, std::vector<std::pair<xpublic_key_t, uint64_t>> & election_data) const;
private:
    /**
     * @brief filter the event
     *
     * @param e event prt
     * @return true
     * @return false
     */
    bool filter_event(const mbus::xevent_ptr_t & e) override;
    /**
     * @brief process the event
     *
     * @param e event prt
     */
    void process_event(const mbus::xevent_ptr_t & e) override;
    /**
     * @brief hook function, invoke after event pushed
     *
     * @param e event prt
     */
    void after_event_pushed(const mbus::xevent_ptr_t & e) override;

    /**
     * @brief process new vnode event
     *
     * @param e event prt
     */
    void do_new_vnode(const mbus::xevent_vnode_ptr_t & e);
    /**
     * @brief process destroy vnode event
     *
     * @param e event prt
     */
    void do_destory_vnode(const mbus::xevent_vnode_ptr_t & e);
    /**
     * @brief process store and chain timer event
     *
     * @param e event prt
     */
    void do_on_block(const mbus::xevent_ptr_t & e);
    /**
     * @brief add to map
     *
     * @param m role map ptr
     * @param rc role context prt
     * @param driver driver
     */
    void add_to_map(xrole_map_t & m, xrole_context_t * rc, vnetwork::xvnetwork_driver_face_t * driver);
    /**
     * @brief add role context
     *
     * @param e event prt
     * @param type node type
     * @param disable_broadcasts if disabling broadcasts
     */
    void add_role_contexts_by_type(const mbus::xevent_vnode_ptr_t & e, common::xnode_type_t type, bool disable_broadcasts);

    /**
     * @brief Set up contract
     *
     * @param contract_cluster_address contract cluster address
     * @param store store
     */
    void setup_chain(common::xaccount_address_t const & contract_cluster_address, base::xvblockstore_t * blockstore);
    bool is_need_process_commit_event(const mbus::xevent_store_block_committed_ptr_t & store_event) const;

    std::unordered_map<common::xaccount_address_t, xrole_map_t *>    m_map;
    xcontract_register_t                                             m_contract_register;
    observer_ptr<store::xsyncvstore_t>                               m_syncstore{};
    std::unordered_map<common::xaccount_address_t, xvm::xcontract::xcontract_base *> m_contract_inst_map;
    base::xrwlock_t                                                  m_rwlock;

    static base::xvnodesrv_t                                         *m_nodesvr_ptr;

    uint64_t                                                         m_latest_timer{};
};
using xcontract_manager_t = xtop_contract_manager;

NS_END2
