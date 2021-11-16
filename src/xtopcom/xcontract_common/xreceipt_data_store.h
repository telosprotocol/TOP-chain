// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xbyte_buffer.h"

#include <map>

NS_BEG2(top, contract_common)

class xtop_receipt_data_store {
public:
    xtop_receipt_data_store() =  default;
    xtop_receipt_data_store(xtop_receipt_data_store const&) = default;
    xtop_receipt_data_store& operator=(xtop_receipt_data_store const&) = default;
    xtop_receipt_data_store(xtop_receipt_data_store&&) = default;
    xtop_receipt_data_store& operator=(xtop_receipt_data_store&&) = default;
    ~xtop_receipt_data_store() =  default;

    void receipt_data(std::map<std::string, xbyte_buffer_t> const& receipt_data);
    // std::map<std::string, xbyte_buffer_t>  receipt_data() const;

    xbyte_buffer_t  receipt_data_item(std::string const& key) const;
    void remove_item(std::string const& key);
    void add_item(std::string const& key, xbyte_buffer_t value);

    bool  item_exist(std::string const& key) const;
    bool  empty() const;


private:
    std::map<std::string, xbyte_buffer_t> m_receipt_data;
};

using xreceipt_data_store_t = xtop_receipt_data_store;

NS_END2