// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xutility.h"
#include "xcontract_common/xreceipt_data_store.h"


NS_BEG2(top, contract_common)

void xtop_receipt_data_store::receipt_data(std::map<std::string, xbyte_buffer_t> const& receipt_data) {
    m_receipt_data = receipt_data;
}

std::map<std::string, xbyte_buffer_t>   xtop_receipt_data_store::receipt_data() const {
    return m_receipt_data;
}

xbyte_buffer_t xtop_receipt_data_store::receipt_data_item(std::string const& key) const {
    xbyte_buffer_t empty;
    auto const it = m_receipt_data.find(key);

    if (it != std::end(m_receipt_data)) {
        return top::get<xbyte_buffer_t>(*it);
    }

    return empty;
}


void xtop_receipt_data_store::remove_item(std::string const& key) {
    m_receipt_data.erase(key);
}

void xtop_receipt_data_store::add_item(std::string const& key,  xbyte_buffer_t value) {
    m_receipt_data.emplace(key, std::move(value));
}

bool  xtop_receipt_data_store::item_exist(std::string const& key) const {
    return m_receipt_data.find(key) != std::end(m_receipt_data);
}

bool  xtop_receipt_data_store::empty() const {
    return m_receipt_data.empty();
}

NS_END2