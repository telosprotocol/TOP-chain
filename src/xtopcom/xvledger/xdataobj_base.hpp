// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xmetrics/xmetrics.h"

namespace top {

// the real enum_xdata_type value = enum_xdata_type_max(355) - enum_xtopcom_object_type
enum enum_xtopcom_object_type {
    xdata_type_transaction                  = 0,  // 355
    xdata_type_cons_transaction             = 1,  // 354
    xdata_type_tx_receipt                   = 2,  // 353
    xdata_type_prove_cert                   = 3,  // 352
    xdata_type_empty_block                  = 4,  // 351
    xdata_type_rootblock                    = 5,  // 350
    xdata_type_rootblock_input_entity       = 6,  // 349
    xdata_type_lightunit_block              = 7,  // 348
    xdata_type_fullunit_block               = 8,  // 347
    xdata_type_table_block                  = 9,  // 346
    xdata_type_fulltable_block              = 10, // 345
    xdata_type_table_proposal_input         = 11, // 344
    xdata_type_transaction_v2               = 12, // 343

    xtopcom_object_type_max
};

template <typename T, int type_value>
class xbase_dataunit_t : public base::xdataunit_t {
 protected:
    enum { object_type_value = enum_xdata_type_max - type_value };

 public:
    xbase_dataunit_t()
    : base::xdataunit_t((enum_xdata_type)object_type_value) {
    }

 protected:
    virtual ~xbase_dataunit_t() {
    }

 public:
    static int32_t get_object_type() {
        return object_type_value;
    }

    static xobject_t *create_object(int type) {
        (void)type;
        return new T;
    }

    void *query_interface(const int32_t _enum_xobject_type_) override {
        if (object_type_value == _enum_xobject_type_)
            return this;
        return base::xdataunit_t::query_interface(_enum_xobject_type_);
    }
};


}  // namespace top
