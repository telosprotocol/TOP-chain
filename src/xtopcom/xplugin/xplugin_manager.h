// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once
#include "xbase/xns_macro.h"
#include "xdata/xpreprocess_data.h"

NS_BEG2(top, data)
class xplugin_manager : public xhandler_face_t {
public:
    xplugin_manager() = default;
    ~xplugin_manager() override = default;
    bool handle(xmessage_t & msg) override;
    void handle(xmessage_t & msg, std::error_code & ec) override;
};
NS_END2
