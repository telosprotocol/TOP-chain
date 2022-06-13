// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <vector>
#include "xbase/xlog.h"
#include "xdata/xcons_transaction.h"
#include "xtxexecutor/xbatchtx_executor.h"
#include "xtxexecutor/xunit_service_error.h"
#include "xtxexecutor/xatomictx_executor.h"

NS_BEG2(top, txexecutor)

xbatchtx_executor_t::xbatchtx_executor_t(const statectx::xstatectx_face_ptr_t & statectx, const xvm_para_t & para)
: m_statectx(statectx), m_para(para) {

}

int32_t xbatchtx_executor_t::execute(const std::vector<xcons_transaction_ptr_t> & txs, xexecute_output_t & outputs) {
    xatomictx_executor_t atomic_executor(m_statectx, m_para);
    uint64_t gas_used = 0;
    xassert(!txs.empty());
    for (auto & tx : txs) {
        xatomictx_output_t output;
        output.m_tx = tx;
        if (tx->get_transaction()->get_gaslimit() + gas_used > m_para.get_gas_limit()) {
            output.m_is_pack = false;
            xwarn("xbatchtx_executor_t::execute gas reach limit,tx:%s,tx gaslimit:%llu,gas_used:%llu,gaslimit:%llu",
                tx->dump().c_str(), (uint64_t)tx->get_transaction()->get_gaslimit(), gas_used, m_para.get_gas_limit());
            if (gas_used == 0) {
                outputs.drop_outputs.push_back(output);
            } else {
                outputs.nopack_outputs.push_back(output);
            }
            continue;
        }

        enum_execute_result_type result = atomic_executor.execute(tx, output, gas_used);
        if (output.m_is_pack) {
            gas_used += output.m_vm_output.m_tx_result.used_gas;
            outputs.pack_outputs.push_back(output);
        } else {
            if (output.m_tx->is_send_or_self_tx()) {
                outputs.drop_outputs.push_back(output);
            } else {
                outputs.nopack_outputs.push_back(output);
            }
        }
        output.m_result = result;
        // xinfo("xbatchtx_executor_t::execute %s,batch_result=%d", output.dump().c_str(),result);
    }

    return xsuccess;
}

NS_END2
