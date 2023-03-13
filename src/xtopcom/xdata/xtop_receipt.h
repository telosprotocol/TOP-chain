
// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "xcommon/xbloom9.h"
#include "xcommon/xtop_log.h"

NS_BEG2(top, data)

class xtop_store_receipt_t {
public:
    xtop_store_receipt_t() = default;
    xtop_store_receipt_t(const common::xtop_logs_t & _logs): m_logs(_logs){};
    ~xtop_store_receipt_t() = default;

    xbytes_t encodeBytes() const;
    void decodeBytes(xbytes_t const & _d, std::error_code & ec);

public:
    void create_bloom();
    const evm_common::xbloom9_t & get_logsBloom() const {
        return m_logsBloom;
    }
    void set_logs(common::xtop_logs_t const & logs) {
        m_logs = logs;
    }
    const common::xtop_logs_t & get_logs() const {
        return m_logs;
    }

protected:
    void streamRLP(evm_common::RLPStream & _s) const;
    void decodeRLP(evm_common::RLP const & _r, std::error_code & ec);

private:
    evm_common::xbloom9_t m_logsBloom;
    common::xtop_logs_t m_logs;
};

NS_END2