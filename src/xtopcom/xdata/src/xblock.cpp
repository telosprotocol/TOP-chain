// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xblock.h"
#include "xdata/xmemcheck_dbg.h"
#include "xdata/xfullunit.h"
#include "xdata/xlightunit.h"
#include "xdata/xemptyblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xrootblock.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xtransaction_v1.h"
#include "xdata/xtransaction_v2.h"
#include "xdata/xtransaction_v3.h"
#include "xdata/xblockbuild.h"
#include "xvledger/xtxreceipt.h"
#include "xvledger/xvpropertyprove.h"

#include "xbase/xhash.h"
#include "xbase/xobject.h"
#include "xbase/xutl.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xmetrics/xmetrics.h"
#include "xbasic/xdbg.h"
#include "xvledger/xvblockbuild.h"

#include <cinttypes>
#include <string>
NS_BEG2(top, data)

xobject_ptr_t<xblock_t> xblock_t::raw_vblock_to_object_ptr(base::xvblock_t* vblock) {
    xblock_ptr_t object_ptr;
    xblock_t* block_ptr = dynamic_cast<xblock_t*>(vblock);
    xassert(block_ptr != nullptr);
    block_ptr->add_ref();
    object_ptr.attach(block_ptr);
    return object_ptr;
}

void  xblock_t::txs_to_receiptids(const std::vector<xlightunit_tx_info_ptr_t> & txs_info, base::xreceiptid_check_t & receiptid_check) {
    for (auto & tx : txs_info) {
        base::xtable_shortid_t tableid = tx->get_receipt_id_peer_tableid();
        uint64_t receiptid = tx->get_receipt_id();
        uint64_t rspid = tx->get_rsp_id();
        if (tx->is_send_tx()) {
            receiptid_check.set_sendid(tableid, receiptid);
            if (rspid != 0) {
                receiptid_check.set_send_rsp_id(tableid, rspid);
            }
        } else if (tx->is_recv_tx()) {
            receiptid_check.set_recvid(tableid, receiptid);
        } else if (tx->is_confirm_tx()) {
            receiptid_check.set_confirmid(tableid, receiptid);
            if (rspid != 0) {
                receiptid_check.set_confirm_rsp_id(tableid, rspid);
            }
        }
    }
}

void  xblock_t::register_object(base::xcontext_t & _context) {
#ifdef DEBUG    // should only call once
    static int32_t static_registered_flag = 0;
    ++static_registered_flag;
    xassert(static_registered_flag == 1);
#endif

    auto lambda_new_rootblock= [](const int type)->xobject_t*{
        return new xrootblock_t();
    };
    auto lambda_new_fullunit= [](const int type)->xobject_t*{
        return new xfullunit_block_t();
    };
    auto lambda_new_lightunit= [](const int type)->xobject_t*{
        return new xlightunit_block_t();
    };
    auto lambda_new_cons_transaction = [](const int type)->xobject_t*{
        return new xcons_transaction_t();
    };
    auto lambda_new_lighttable = [](const int type)->xobject_t*{
        return new xtable_block_t();
    };
    auto lambda_new_fulltable = [](const int type)->xobject_t*{
        return new xfull_tableblock_t();
    };
    auto lambda_new_emptyblock = [](const int type)->xobject_t*{
        return new xemptyblock_t();
    };
    auto lambda_new_txreceipt = [](const int type)->xobject_t*{
        return new base::xtx_receipt_t();
    };    
    auto lambda_new_transactionv1 = [](const int type)->xobject_t*{
        return new xtransaction_v1_t();
    };    
    auto lambda_new_transactionv2 = [](const int type)->xobject_t*{
        return new xtransaction_v2_t();
    };      

    auto lambda_new_transactionv3 = [](const int type) -> xobject_t * {
        return new xtransaction_v3_t();
    };

    auto lambda_new_property_prove = [](const int type)->xobject_t*{
        return new base::xvproperty_prove_t();
    };
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xrootblock_t::get_object_type(),lambda_new_rootblock);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xfullunit_block_t::get_object_type(),lambda_new_fullunit);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xlightunit_block_t::get_object_type(),lambda_new_lightunit);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xcons_transaction_t::get_object_type(),lambda_new_cons_transaction);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xtable_block_t::get_object_type(),lambda_new_lighttable);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xfull_tableblock_t::get_object_type(),lambda_new_fulltable);    
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xemptyblock_t::get_object_type(),lambda_new_emptyblock);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)base::xtx_receipt_t::get_object_type(),lambda_new_txreceipt);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xtransaction_v1_t::get_object_type(),lambda_new_transactionv1);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xtransaction_v2_t::get_object_type(),lambda_new_transactionv2);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)xtransaction_v3_t::get_object_type(), lambda_new_transactionv3);
    base::xcontext_t::register_xobject2(_context,(base::enum_xobject_type)base::xvproperty_prove_t::get_object_type(),lambda_new_property_prove);
    xkinfo("xblock_t::register_object,finish");    
}

xblock_t::xblock_t(base::xvheader_t & header, base::xvqcert_t & cert, enum_xdata_type type)
    : base::xvblock_t(header, cert, nullptr, nullptr, type) {
    MEMCHECK_ADD_TRACE(this, "block_create");
}

xblock_t::xblock_t(enum_xdata_type type) : base::xvblock_t(type) {
    MEMCHECK_ADD_TRACE(this, "block_create");
}

xblock_t::xblock_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output, enum_xdata_type type)
  : base::xvblock_t(header, cert, input, output, type) {
    MEMCHECK_ADD_TRACE(this, "block_create");
}

xblock_t::~xblock_t() {
    xdbg("xblock_t::~xblock_t %s", dump().c_str());
    MEMCHECK_REMOVE_TRACE(this);
}

#ifdef XENABLE_PSTACK  // tracking memory
int32_t xblock_t::add_ref() {
    MEMCHECK_ADD_TRACE(this, "block_add");
    return base::xvblock_t::add_ref();
}
int32_t xblock_t::release_ref() {
    MEMCHECK_ADD_TRACE(this, "block_release");
    xdbgassert(get_refcount() > 0);
    return base::xvblock_t::release_ref();
}
#endif

int32_t xblock_t::full_block_serialize_to(base::xstream_t & stream) {
    if ( false == is_body_and_offdata_ready(false) ) {
        xerror("xblock_t::full_block_serialize_to not valid block.%s, %d,%d,%d", dump().c_str(), is_input_ready(false), is_output_ready(false), is_output_offdata_ready(false));
        return -1;
    }

    KEEP_SIZE();
    std::string block_object_bin;
    this->serialize_to_string(block_object_bin);
    stream << block_object_bin;
    if (get_header()->get_block_class() != base::enum_xvblock_class_nil) {
        stream << get_input_data();
        stream << get_output_data();
        if (!get_output_offdata_hash().empty()) {
            stream << get_output_offdata();
        }
    }

    xdbg("xblock_t::full_block_serialize_to,succ.block=%s,size=%zu,%zu,%zu,%zu,%d",
        dump().c_str(), block_object_bin.size(), get_input_data().size(), get_output_data().size(), get_output_offdata().size(), stream.size());
    return CALC_LEN();
}

base::xvblock_t* xblock_t::full_block_read_from(base::xstream_t & stream) {
    xassert(stream.size() > 0);
    std::string block_object_bin;
    stream >> block_object_bin;
    base::xvblock_t* new_block = base::xvblock_t::create_block_object(block_object_bin);
    if (new_block == nullptr) {
        xerror("xblock_t::full_block_read_from not valid block.object_bin=%ld", base::xhash64_t::digest(block_object_bin));
        return nullptr;
    }
    xassert(new_block != nullptr);
    if (new_block != nullptr && new_block->get_header()->get_block_class() != base::enum_xvblock_class_nil) {
        std::string _input_content;
        std::string _output_content;
        stream >> _input_content;
        stream >> _output_content;
        if (false == new_block->set_input_data(_input_content)) {
            xerror("xblock_t::full_block_read_from set_input_data fail.block=%s,ir=%ld",new_block->dump().c_str(),base::xhash64_t::digest(_input_content));
            new_block->release_ref();
            return nullptr;
        }
        if (false == new_block->set_output_data(_output_content)) {
            xerror("xblock_t::full_block_read_from set_output_data failblock=%s,or=%ld",new_block->dump().c_str(),base::xhash64_t::digest(_output_content));
            new_block->release_ref();
            return nullptr;
        }

        if (!new_block->get_output_offdata_hash().empty()) {
            if (stream.size() == 0) {
                xerror("xblock_t::full_block_read_from not include output offdata failblock=%s",new_block->dump().c_str());
                new_block->release_ref();
                return nullptr;
            }
            std::string _out_offdata;
            stream >> _out_offdata;
            if (false == new_block->set_output_offdata(_out_offdata)) {
                xerror("xblock_t::full_block_read_from set_output_offdata failblock=%s,offdata_size=%zu",new_block->dump().c_str(),_out_offdata.size());
                new_block->release_ref();
                return nullptr;
            }
        }
    }
    return new_block;
}

xtransaction_ptr_t  xblock_t::query_raw_transaction(const std::string & txhash) const {
    std::string value = query_input_resource(txhash);
    if (!value.empty()) {
        xtransaction_ptr_t raw_tx;
        xtransaction_t::set_tx_by_serialized_data(raw_tx, value);
        return raw_tx;
    }
    return nullptr;
}

uint32_t  xblock_t::query_tx_size(const std::string & txhash) const {
    std::string value = query_input_resource(txhash);
    return value.size();
}


NS_END2
