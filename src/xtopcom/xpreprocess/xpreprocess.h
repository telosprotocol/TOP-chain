// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbase/xns_macro.h"
#include "xdata/xpreprocess_data.h"

#include <memory>
#include <string>

NS_BEG2(top, data)
class xpreprocess {
public:
    xpreprocess();
    xpreprocess(const xpreprocess &) = delete;
    xpreprocess & operator=(const xpreprocess &) = delete;
    ~xpreprocess() = default;
    static xpreprocess & instance();
    // if the value is false,  the following logic can continue.
    bool send(xmessage_t & msg);

private:
    std::unique_ptr<xhandler_face_t> m_handler;
};

NS_END2