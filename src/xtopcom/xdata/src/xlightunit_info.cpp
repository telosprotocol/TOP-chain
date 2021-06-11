// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbase/xutl.h"
#include "xdata/xlightunit_info.h"
#include "xdata/xdata_common.h"

namespace top { namespace data {

void xtransaction_exec_state_t::set_tx_exec_status(enum_xunit_tx_exec_status value) {
    set_value(XTX_STATE_TX_EXEC_STATUS, (uint32_t)value);
}

void xtransaction_exec_state_t::set_receipt_id(base::xtable_shortid_t sid, uint64_t value) {
    set_value(XTX_RECEIPT_ID, value);
    set_value(XTX_RECEIPT_ID_TABLE_ID, sid);
}

enum_xunit_tx_exec_status xtransaction_exec_state_t::get_tx_exec_status() const {
    enum_xunit_tx_exec_status status = static_cast<enum_xunit_tx_exec_status>(get_value_uint32(XTX_STATE_TX_EXEC_STATUS));
    return status;
}

uint64_t xlightunit_tx_info_t::get_last_trans_nonce() const {
    if (is_self_tx() || is_send_tx()) {
        if (m_raw_tx != nullptr) {
            return m_raw_tx->get_last_nonce();
        }
    }
    return 0;
}



}  // namespace data
}  // namespace top
