// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xblockmaker/xblockmaker_face.h"
#include "xstatectx/xstatectx.h"
#include "xdata/xcons_transaction.h"

NS_BEG2(top, blockmaker)

class xrelayblock_plugin_t : public xblock_resource_plugin_face_t {  // TODO(jimmy) define face
 public:
    xrelayblock_plugin_t() {}

    virtual std::string                            get_face_name() const override {return "relayblock_plugin";}
    virtual void                                   init(statectx::xstatectx_ptr_t const& statectx_ptr, std::error_code & ec) override;
    virtual std::vector<xcons_transaction_ptr_t>   make_contract_txs(statectx::xstatectx_ptr_t const& statectx_ptr, uint64_t timestamp, std::error_code & ec) override;
    virtual xblock_resource_description_t          make_resource(const data::xblock_consensus_para_t & cs_para, std::error_code & ec) const override;

 private:
    std::string get_new_relay_election_data(statectx::xstatectx_ptr_t const& statectx_ptr, uint64_t timestamp) const;
    data::xcons_transaction_ptr_t   make_relay_make_block_contract_tx(statectx::xstatectx_ptr_t const& statectx_ptr, uint64_t timestamp, std::error_code & ec);

 private:   
   data::xunitstate_ptr_t  m_relay_make_block_contract_state{nullptr};
   std::string             m_last_phase;
   mutable bool            m_time_to_check_election{false};
};

NS_END2
