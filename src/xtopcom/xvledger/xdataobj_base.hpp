// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbase/xcontext.h"
#include "xbase/xdata.h"

namespace top {

// the real enum_xdata_type value = enum_xdata_type_max(355) - enum_xtopcom_object_type
enum enum_xtopcom_object_type {
    xdata_type_transaction                  = 0,
    xdata_type_transaction_store            = 1,
    xdata_type_account_binlog               = 2,
    xdata_type_blockchain                   = 3,
    xdata_type_cons_transaction             = 4,
    xdata_type_block_cert                   = 5,
    xdata_type_tx_receipt                   = 6,
    xdata_type_prove_cert                   = 7,
    xdata_type_input                        = 8,
    xdata_type_output                       = 9,
    xdata_type_dummy_entity                 = 10,

    xdata_type_empty_block                  = 11,
    xdata_type_rootblock                    = 12,
    xdata_type_rootblock_input_entity       = 13,

    xdata_type_lightunit_block              = 14,
    xdata_type_lightunit_input_entity       = 15,
    xdata_type_lightunit_output_entity      = 16,
    xdata_type_lightunit_output_resource    = 17,

    xdata_type_fullunit_block               = 18,
    xdata_type_fullunit_input_entity        = 19,
    xdata_type_fullunit_output_entity       = 20,

    xdata_type_table_block                  = 21,
    xdata_type_tableblock_input_entity      = 22,
    xdata_type_tableblock_output_entity     = 23,
    xdata_type_tableblock_unitinput_resource = 24,
    xdata_type_tableblock_unitoutput_resource = 25,

    xdata_type_whole_block_resource         = 26,  // whole block resource
    xdata_type_origin_tx_resource           = 27,  // origin tx resource

    xdata_type_table_mbt                    = 28,
    xdata_type_accountindex_binlog          = 29,
    xdata_type_receiptid                    = 30,
    xdata_type_fulltable_block              = 31,
    xdata_type_fulltable_output_entity      = 32,
    xdata_type_table_proposal_input         = 33,

    xtopcom_object_type_max
};


template <typename T, int type_value>
class xbase_dataobj_t : public base::xdataobj_t {
 protected:
    enum { object_type_value = enum_xdata_type_max - type_value };

 public:
    xbase_dataobj_t()
    : base::xdataobj_t((enum_xdata_type)object_type_value) {
        // XMETRICS_XBASE_DATA_CATEGORY_NEW(object_type_value);
    }

    virtual ~xbase_dataobj_t() {
        // XMETRICS_XBASE_DATA_CATEGORY_DELETE(object_type_value);
    }

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
        return xdataobj_t::query_interface(_enum_xobject_type_);
    }
};

template <typename T, int type_value>
class xbase_dataunit_t : public base::xdataunit_t {
 protected:
    enum { object_type_value = enum_xdata_type_max - type_value };

 public:
    xbase_dataunit_t()
    : base::xdataunit_t((enum_xdata_type)object_type_value) {
        // XMETRICS_XBASE_DATA_CATEGORY_NEW(object_type_value);
    }

 protected:
    virtual ~xbase_dataunit_t() {
        // XMETRICS_XBASE_DATA_CATEGORY_DELETE(object_type_value);
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

template <class data_cls>
class register_xcls {
public:
    register_xcls() { top::base::xcontext_t::register_xobject((top::base::enum_xobject_type)data_cls::get_object_type(), data_cls::create_object); }
};

#ifndef REG_CLS
#    define REG_CLS(CLS)                                                                                                                                                           \
        register_xcls<CLS> g_##CLS
#endif  // REG_CLS

}  // namespace top
