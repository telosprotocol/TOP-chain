// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "basic_handler.h"

NS_BEG3(top, metrics, handler)

class flow_handler : public xbasic_handler_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(flow_handler);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(flow_handler);
    XDECLARE_DEFAULTED_DESTRUCTOR(flow_handler);

    metrics_variant_ptr init_new_metrics(event_message const & msg) override;
    void dump_metrics_info(metrics_variant_ptr const & metrics_ptr) override;
    void process_message_event(metrics_variant_ptr & metrics_ptr, event_message const & msg) override;
};
using flow_handler_t = flow_handler;
NS_END3