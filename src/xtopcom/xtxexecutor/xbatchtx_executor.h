// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>

#include "xdata/xcons_transaction.h"
#include "xtxexecutor/xatomictx_executor.h"
#include "xstatectx/xstatectx_face.h"

NS_BEG2(top, txexecutor)

struct xexecute_output_t {
   int64_t                         tgas_balance_change{0};
   std::vector<xatomictx_output_t> pack_outputs;
   std::vector<xatomictx_output_t> nopack_outputs;
   std::vector<xatomictx_output_t> drop_outputs;
   bool empty() const {return pack_outputs.empty() && nopack_outputs.empty() && drop_outputs.empty();}
};

class xbatchtx_executor_t {
 public:
    xbatchtx_executor_t(const statectx::xstatectx_face_ptr_t & statectx, const xvm_para_t & para);

 public:
    int32_t execute(const std::vector<xcons_transaction_ptr_t> & txs, xexecute_output_t & outputs);
 private:
    statectx::xstatectx_face_ptr_t  m_statectx{nullptr};
    xvm_para_t                      m_para;
};

NS_END2
