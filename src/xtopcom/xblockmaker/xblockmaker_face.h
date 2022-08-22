// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include "xvledger/xvstate.h"
#include "xvledger/xvstatestore.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvcertauth.h"
#include "xdata/xblock.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xblock_cs_para.h"
#include "xstore/xstore_face.h"
#include "xblockstore/xblockstore_face.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xblockmaker/xblock_maker_para.h"
#include "xstatectx/xstatectx.h"

NS_BEG2(top, blockmaker)

using data::xblock_ptr_t;

class xblockmaker_resources_t {
 public:
    virtual base::xvblockstore_t*       get_blockstore() const = 0;
    virtual xtxpool_v2::xtxpool_face_t* get_txpool() const = 0;
    virtual mbus::xmessage_bus_face_t*  get_bus() const = 0;
    virtual base::xvblkstatestore_t*    get_xblkstatestore() const = 0;
    virtual base::xvcertauth_t*         get_certauth() const {return m_ca;}
    virtual void                        set_certauth(base::xvcertauth_t* _ca) {m_ca = _ca;}
 private:
    base::xvcertauth_t* m_ca{nullptr};
};
using xblockmaker_resources_ptr_t = std::shared_ptr<xblockmaker_resources_t>;

class xblockmaker_resources_impl_t : public xblockmaker_resources_t {
 public:
    xblockmaker_resources_impl_t(const observer_ptr<store::xstore_face_t> & store,
                                 const observer_ptr<base::xvblockstore_t> & blockstore,
                                 const observer_ptr<xtxpool_v2::xtxpool_face_t> & txpool,
                                 const observer_ptr<mbus::xmessage_bus_face_t> & bus)
    : m_blockstore(blockstore), m_txpool(txpool), m_bus(bus) {}

    virtual base::xvblockstore_t*       get_blockstore() const {return m_blockstore.get();}
    virtual xtxpool_v2::xtxpool_face_t* get_txpool() const {return m_txpool.get();}
    virtual mbus::xmessage_bus_face_t*  get_bus() const {return m_bus.get();}
    virtual base::xvblkstatestore_t*    get_xblkstatestore() const {return base::xvchain_t::instance().get_xstatestore()->get_blkstate_store();}

 private:
    observer_ptr<base::xvblockstore_t>          m_blockstore{nullptr};
    observer_ptr<xtxpool_v2::xtxpool_face_t>    m_txpool{nullptr};
    observer_ptr<mbus::xmessage_bus_face_t>     m_bus{nullptr};
};

struct xunitmaker_para_t {
    xunitmaker_para_t(const data::xtablestate_ptr_t & tablestate, bool is_leader)
    : m_tablestate(tablestate), m_is_leader(is_leader) {}
    xunitmaker_para_t(const data::xtablestate_ptr_t & tablestate, const xunit_proposal_input_t & unit_input)
    : m_tablestate(tablestate), m_unit_input(unit_input) {}

    data::xtablestate_ptr_t                 m_tablestate{nullptr};
    xunit_proposal_input_t                  m_unit_input;
    bool                                    m_is_leader{false};
};

class xtablemaker_result_t {
 public:
    xblock_ptr_t                            m_block{nullptr};
    int32_t                                 m_make_block_error_code{0};

    uint32_t                                m_total_tx_num{0};
    uint32_t                                m_self_tx_num{0};
    uint32_t                                m_send_tx_num{0};
    uint32_t                                m_recv_tx_num{0};
    uint32_t                                m_confirm_tx_num{0};

    uint32_t                                m_total_unit_num{0};
    uint32_t                                m_fail_unit_num{0};
    uint32_t                                m_succ_unit_num{0};
    uint32_t                                m_empty_unit_num{0};
    uint32_t                                m_light_unit_num{0};
    uint32_t                                m_full_unit_num{0};
    std::vector<xcons_transaction_ptr_t>    m_unchange_txs;
};

class xtablemaker_para_t {
 public:
    xtablemaker_para_t() {
        m_proposal = make_object_ptr<xtable_proposal_input_t>();
    }
    xtablemaker_para_t(const data::xtablestate_ptr_t & tablestate, const data::xtablestate_ptr_t & commit_tablestate)
      : m_tablestate(tablestate), m_commit_tablestate(commit_tablestate) {
        m_proposal = make_object_ptr<xtable_proposal_input_t>();
    }
    // xtablemaker_para_t(const std::vector<xcons_transaction_ptr_t> & origin_txs)
    // : m_origin_txs(origin_txs) {
    //     m_proposal = make_object_ptr<xtable_proposal_input_t>();
    // }

 public:
    void    set_origin_txs(const std::vector<xcons_transaction_ptr_t> & origin_txs) {
        m_origin_txs = origin_txs;
    }
    void    set_other_accounts(const std::vector<std::string> & accounts) {
        m_other_accounts = accounts;
    }
    void    set_table_state(const data::xtablestate_ptr_t & tablestate) const {
        m_tablestate = tablestate;
    }
    void    push_tx_to_proposal(const xcons_transaction_ptr_t & input_tx) const {
        m_proposal->set_input_tx(input_tx);
    }
    void    push_receiptid_state_prove(const base::xvproperty_prove_ptr_t receiptid_state_prove) {
        m_proposal->set_receiptid_state_prove(receiptid_state_prove);
    }
    bool    delete_fail_tx_from_proposal(const std::vector<xcons_transaction_ptr_t> & fail_txs) const {
        for (auto & tx : fail_txs) {
            if (false == m_proposal->delete_fail_tx(tx)) {
                return false;
            }
        }
        return true;
    }
    void    set_pack_resource(const xtxpool_v2::xpack_resource & pack_resource) {
        m_origin_txs = pack_resource.m_txs;
        m_receiptid_info_map = pack_resource.m_receiptid_info_map;
    }

    void    set_relay_extra_data(const std::string & relay_extra_data) {
        m_relay_extra_data = relay_extra_data;
    }

    void    set_need_relay_prove(bool is_need) {
        m_need_relay_prove = is_need;
    }

    void    set_relay_evm_height(uint64_t height) {
        m_relay_evm_height = height;
    }

    void    set_relay_elect_height(uint64_t height) {
        m_relay_elect_height = height;
    }

    const std::vector<xcons_transaction_ptr_t> &    get_origin_txs() const {return m_origin_txs;}
    const std::map<base::xtable_shortid_t, xtxpool_v2::xreceiptid_state_and_prove> & get_receiptid_info_map() const {return m_receiptid_info_map;}
    const std::vector<std::string> &                get_other_accounts() const {return m_other_accounts;}
    const data::xtablestate_ptr_t &                 get_tablestate() const {return m_tablestate;}
    const data::xtablestate_ptr_t &                 get_commit_tablestate() const {return m_commit_tablestate;}
    const xtable_proposal_input_ptr_t &             get_proposal() const {return m_proposal;}
    const std::string &                             get_relay_extra_data() const {return m_relay_extra_data;}
    bool                                            need_relay_prove() const {return m_need_relay_prove;}
    uint64_t                                        get_relay_evm_height() const {return m_relay_evm_height;}
    uint64_t                                        get_relay_elect_height() const {return m_relay_elect_height;}

 private:
    std::vector<xcons_transaction_ptr_t>    m_origin_txs;
    std::map<base::xtable_shortid_t, xtxpool_v2::xreceiptid_state_and_prove> m_receiptid_info_map;
    std::vector<std::string>                m_other_accounts;  // for empty or full unit accounts
    std::string                             m_relay_extra_data;
    bool                                    m_need_relay_prove{false};
    uint64_t                                m_relay_evm_height;
    uint64_t                                m_relay_elect_height;

    mutable xtable_proposal_input_ptr_t     m_proposal;  // leader should make proposal input; backup should verify proposal input
    mutable data::xtablestate_ptr_t         m_tablestate{nullptr};
    mutable data::xtablestate_ptr_t         m_commit_tablestate{nullptr};
};

class xblock_maker_t : public base::xvaccount_t {
 public:
    explicit xblock_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources);
    virtual ~xblock_maker_t();

 public:
    base::xvblockstore_t*       get_blockstore() const {return m_resources->get_blockstore();}
    xtxpool_v2::xtxpool_face_t*    get_txpool() const {return m_resources->get_txpool();}
    mbus::xmessage_bus_face_t*  get_bus() const {return m_resources->get_bus();}
    const xblockmaker_resources_ptr_t & get_resources() const {return m_resources;}
 private:
    xblockmaker_resources_ptr_t             m_resources{nullptr};
};


class xblock_builder_para_face_t {
 public:
    xblock_builder_para_face_t() = default;
     xblock_builder_para_face_t(const xblockmaker_resources_ptr_t & resources,
                                const std::vector<data::xlightunit_tx_info_ptr_t> & txs_info = std::vector<data::xlightunit_tx_info_ptr_t>{})
    : m_resources(resources), m_txs_info(txs_info) {}

 public:
    virtual base::xvblockstore_t*       get_blockstore() const {return m_resources->get_blockstore();}
    virtual xtxpool_v2::xtxpool_face_t* get_txpool() const {return m_resources->get_txpool();}
    virtual int32_t                     get_error_code() const {return m_error_code;}
    virtual void                        set_error_code(int32_t error_code) {m_error_code = error_code;}
    int64_t get_tgas_balance_change() const { return m_tgas_balance_change; }
    void set_tgas_balance_change(const int64_t amount) { m_tgas_balance_change = amount; }
    const std::vector<data::xlightunit_tx_info_ptr_t> & get_txs() const {return m_txs_info;}
    void set_changed_confirm_ids(const std::map<base::xtable_shortid_t, uint64_t> & changed_confirm_ids) {m_changed_confirm_ids = changed_confirm_ids;}
    const std::map<base::xtable_shortid_t, uint64_t> & get_changed_confirm_ids() const {return m_changed_confirm_ids;}
    void set_relay_extra_data(const std::string & relay_extra_data) {m_relay_extra_data = relay_extra_data;}
    const std::string & get_relay_extra_data() const {return m_relay_extra_data;}
    void set_need_relay_prove(bool is_need) {m_need_relay_prove = is_need;}
    bool need_relay_prove() const {return m_need_relay_prove;}

 private:
    xblockmaker_resources_ptr_t m_resources{nullptr};
    int32_t                     m_error_code{0};
    int64_t                     m_tgas_balance_change{0};
    std::vector<data::xlightunit_tx_info_ptr_t> m_txs_info;
    std::map<base::xtable_shortid_t, uint64_t> m_changed_confirm_ids;
    std::string                 m_relay_extra_data;
    bool                        m_need_relay_prove{false};
};
using xblock_builder_para_ptr_t = std::shared_ptr<xblock_builder_para_face_t>;

class xblock_builder_face_t {
 public:
    virtual data::xblock_ptr_t          build_block(const data::xblock_ptr_t & prev_block,
                                                  const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                                  const data::xblock_consensus_para_t & cs_para,
                                                  xblock_builder_para_ptr_t & build_para) = 0;
};

using xblock_builder_face_ptr_t = std::shared_ptr<xblock_builder_face_t>;


struct xblock_resource_description_t {
    bool            is_input_resource{false};
    std::string     resource_key_name;
    std::string     resource_value;
    bool            need_signature{false};
    uint256_t       signature_hash; 
};
class xblock_resource_plugin_face_t {
 public:
    virtual std::string                             get_face_name() const {return std::string();}
    virtual void                                    init(statectx::xstatectx_ptr_t const& statectx_ptr, std::error_code & ec) {}
    virtual std::vector<xcons_transaction_ptr_t>    make_contract_txs(statectx::xstatectx_ptr_t const& statectx_ptr, uint64_t timestamp, std::error_code & ec) {return {};}
    virtual xblock_resource_description_t           make_resource(const data::xblock_consensus_para_t & cs_para, std::error_code & ec) const {return {};}
};
using xblock_resource_plugin_face_ptr_t = std::shared_ptr<xblock_resource_plugin_face_t>;

NS_END2
