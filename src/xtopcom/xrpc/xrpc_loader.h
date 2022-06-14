#pragma once

#include "json/json.h"
#include "xvledger/xvtxindex.h"
#include "xvledger/xvaction.h"
#include "xdata/xlightunit_info.h"
#include "xdata/xtransaction.h"
#include "xdata/xethreceipt.h"

namespace top {

namespace xrpc {

class xtxindex_detail_t {
public:
    xtxindex_detail_t(const base::xvtxindex_ptr & txindex, const base::xvaction_t & txaction, uint64_t transaction_index);
    ~xtxindex_detail_t() {}

    void            set_raw_tx(base::xdataunit_t* tx);
    const base::xvtxindex_ptr &         get_txindex() const {return m_txindex;}
    const data::xlightunit_action_t &   get_txaction() const {return m_txaction;}
    const data::xtransaction_ptr_t &    get_raw_tx() const {return m_raw_tx;}
    uint64_t                            get_transaction_index() const {return m_transaction_index;}

private:
    base::xvtxindex_ptr         m_txindex{nullptr};
    data::xlightunit_action_t   m_txaction;
    data::xtransaction_ptr_t    m_raw_tx{nullptr};
    uint64_t                    m_transaction_index{0};
};
using xtxindex_detail_ptr_t = std::shared_ptr<xtxindex_detail_t>;

class xrpc_loader_t {
 public:
    static  xtxindex_detail_ptr_t   load_tx_indx_detail(const std::string & raw_tx_hash,base::enum_transaction_subtype type);
 public:  // json transfer
    static  xJson::Value            parse_send_tx(const xtxindex_detail_ptr_t & txindex_detail);
    static  xJson::Value            load_and_parse_recv_tx(const std::string & raw_tx_hash, const xtxindex_detail_ptr_t & sendindex, data::enum_xunit_tx_exec_status & recvtx_status);
    static  xJson::Value            load_and_parse_confirm_tx(const std::string & raw_tx_hash, const xtxindex_detail_ptr_t & sendindex, data::enum_xunit_tx_exec_status recvtx_status);
 private:
    static  xJson::Value            parse_recv_tx(const xtxindex_detail_ptr_t & sendindex, const xtxindex_detail_ptr_t & recvindex);
    static  xJson::Value            parse_confirm_tx(const xtxindex_detail_ptr_t & sendindex, data::enum_xunit_tx_exec_status recvtx_status, const xtxindex_detail_ptr_t & confirmindex);
    static  void                    parse_common_info(const xtxindex_detail_ptr_t & txindex, xJson::Value & jv);

 public: // load ethdata
    static  xtxindex_detail_ptr_t   load_ethtx_indx_detail(const std::string & raw_tx_hash);
};

}  // namespace chain_info
}  // namespace top
