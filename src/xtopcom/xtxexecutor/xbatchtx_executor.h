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

class xbatchtx_executor_t {
 public:
    xbatchtx_executor_t(const statectx::xstatectx_face_ptr_t & statectx, const xvm_para_t & para);

 public:
    int32_t execute(const std::vector<xcons_transaction_ptr_t> & txs, std::vector<xatomictx_output_t> & outputs);
 private:
    statectx::xstatectx_face_ptr_t  m_statectx{nullptr};
    xvm_para_t                      m_para;
};

NS_END2
