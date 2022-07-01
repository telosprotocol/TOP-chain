// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include <map>
#include "json/json.h"

#if defined(__clang__)

#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"

#elif defined(__GNUC__)

#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"

#elif defined(_MSC_VER)

#    pragma warning(push, 0)

#endif

#include "xvledger/xvblock.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xaccountindex.h"
#include "xvledger/xreceiptid.h"
#include "xvledger/xmerkle.hpp"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbase/xobject_ptr.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xdata_common.h"
#include "xdata/xlightunit_info.h"
#include "xcommon/xaccount_address.h"
#include "xevm_common/common.h"

NS_BEG2(top, data)

using base::xaccount_index_t;
using xvheader_ptr_t = xobject_ptr_t<base::xvheader_t>;

class xblock_t : public base::xvblock_t {
 public:
    static std::string get_block_base_path(base::xvblock_t* block) {return block->get_account() + ':' + std::to_string(block->get_height());}
    static xobject_ptr_t<xblock_t> raw_vblock_to_object_ptr(base::xvblock_t* block);
    static void  txs_to_receiptids(const std::vector<xlightunit_tx_info_ptr_t> & txs_info, base::xreceiptid_check_t & receiptid_check);
    static std::string dump_header(base::xvheader_t* header);
    static void  register_object(base::xcontext_t & _context);
public:
    xblock_t(enum_xdata_type type);
    xblock_t(base::xvheader_t & header, base::xvqcert_t & cert, enum_xdata_type type);
    xblock_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output, enum_xdata_type type);

#ifdef XENABLE_PSTACK  // tracking memory
    virtual int32_t add_ref() override;
    virtual int32_t release_ref() override;
#endif

 protected:
    virtual ~xblock_t();
 private:
    xblock_t(const xblock_t &);
    xblock_t & operator = (const xblock_t &);

 public:
    virtual int32_t     full_block_serialize_to(base::xstream_t & stream);  // for block sync
    static  base::xvblock_t*    full_block_read_from(base::xstream_t & stream);  // for block sync
    virtual void parse_to_json(xJson::Value & root, const std::string & rpc_version) {};
 public:
    inline base::enum_xvblock_level get_block_level() const {return get_header()->get_block_level();}
    inline base::enum_xvblock_class get_block_class() const {return get_header()->get_block_class();}
    inline bool     is_unitblock() const {return get_block_level() == base::enum_xvblock_level_unit;}
    inline bool     is_tableblock() const {return get_block_level() == base::enum_xvblock_level_table;}
    inline bool     is_fullblock() const {return get_block_class() == base::enum_xvblock_class_full;}
    inline bool     is_lightunit() const {return get_block_level() == base::enum_xvblock_level_unit && get_block_class() == base::enum_xvblock_class_light;}
    inline bool     is_fullunit() const {return get_block_level() == base::enum_xvblock_level_unit && get_block_class() == base::enum_xvblock_class_full;}
    inline bool     is_emptyunit() const {return get_block_level() == base::enum_xvblock_level_unit && get_block_class() == base::enum_xvblock_class_nil;}
    inline bool     is_emptyblock() const {return get_block_class() == base::enum_xvblock_class_nil;}
    inline bool     is_fulltable() const {return get_block_level() == base::enum_xvblock_level_table && get_block_class() == base::enum_xvblock_class_full;}
    inline bool     is_lighttable() const {return get_block_level() == base::enum_xvblock_level_table && get_block_class() == base::enum_xvblock_class_light;}
    inline bool     is_emptytable() const {return get_block_level() == base::enum_xvblock_level_table && get_block_class() == base::enum_xvblock_class_nil;}
    std::string     get_block_hash_hex_str() const;
    std::string     dump_header() const;
    std::string     dump_cert() const;
    virtual std::string     dump_body() const;
    std::string     dump_cert(base::xvqcert_t* qcert) const;

 public:
    virtual int64_t                     get_pledge_balance_change_tgas() const {return 0;}
    virtual void                        dump_block_data(xJson::Value & json) const {return;}
    virtual uint32_t                    get_unconfirm_sendtx_num() const {return 0;}
    virtual uint64_t                    get_second_level_gmtime() const override;
    xtransaction_ptr_t                  query_raw_transaction(const std::string & txhash) const;
    uint32_t                            query_tx_size(const std::string & txhash) const;
    std::vector<base::xvtxkey_t>        get_txkeys() const;

 public:
    uint64_t    get_timerblock_height() const {return get_clock();}
    std::string get_block_owner()const {return get_account();}
};

using xblock_ptr_t = xobject_ptr_t<xblock_t>;
using xvblock_ptr_t = xobject_ptr_t<base::xvblock_t>;

NS_END2
