// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbasic/xns_macro.h"
#include "xelection/xcache/xdata_accessor_face.h"
#include "xvnetwork/xvhost_face_fwd.h"
#include "xvnetwork/xvnetwork_message.h"
NS_BEG2(top, vnetwork)

class xtop_message_filter_base {
public:
    virtual void filt(xvnetwork_message_t & vnetwork_message) = 0;
    virtual ~xtop_message_filter_base(){}
};
using xmessage_filter_base_t = xtop_message_filter_base;

NS_END2