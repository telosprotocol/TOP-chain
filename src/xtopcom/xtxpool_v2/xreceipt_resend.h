// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"
#include "xbase/xbase.h"

NS_BEG2(top, xtxpool_v2)

#define send_tx_receipt_first_retry_timeout (200)
#define send_tx_receipt_common_retry_timeout (800)

static uint32_t get_receipt_send_times(uint64_t cert_time, uint64_t now) {
    if (now < cert_time) {
        xdbg("xtxpool_table_t::get_receipt_resend_time receipt gmtime(%llu) is ahead now(%llu)", cert_time, now);
        return 0;
    }

    uint64_t delay_time = now - cert_time;
    if (delay_time < send_tx_receipt_first_retry_timeout) {
        return 0;
    } else if (delay_time < send_tx_receipt_first_retry_timeout + send_tx_receipt_common_retry_timeout) {
        return 1;
    } else {
        return 1 + (delay_time - send_tx_receipt_first_retry_timeout)/send_tx_receipt_common_retry_timeout;
    }
}

static uint64_t get_next_resend_time(uint64_t cert_time, uint64_t now) {
    if (now < cert_time + send_tx_receipt_first_retry_timeout) {
        return cert_time + send_tx_receipt_first_retry_timeout;
    } else {
        return now + send_tx_receipt_common_retry_timeout - (now - cert_time - send_tx_receipt_first_retry_timeout)%send_tx_receipt_common_retry_timeout;
    }
}

NS_END2
