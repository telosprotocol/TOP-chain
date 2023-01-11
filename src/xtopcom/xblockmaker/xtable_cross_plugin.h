// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xblockmaker/xblockmaker_face.h"
#include "xstatectx/xstatectx.h"
#include "xdata/xcons_transaction.h"

NS_BEG2(top, blockmaker)

class xtable_cross_plugin_t : public xblock_resource_plugin_face_t {  
 public:
    xtable_cross_plugin_t() {}

    virtual std::string                                      get_face_name() const override {return "xtable_cross_plugin";}
    virtual void                                             init(statectx::xstatectx_ptr_t const& statectx_ptr, std::error_code & ec) override;
    virtual std::vector<data::xcons_transaction_ptr_t>       make_contract_txs_after_execution(statectx::xstatectx_ptr_t const& statectx_ptr, uint64_t timestamp,
                                                                              std::vector<top::txexecutor::xatomictx_output_t> const& pack_outputs, std::error_code& ec) override;
 
 private:   
   data::xaccountstate_ptr_t  m_cross_block_contract_state{nullptr};

};


NS_END2
