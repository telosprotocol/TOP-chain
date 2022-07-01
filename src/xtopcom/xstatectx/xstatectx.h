// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xmemory.hpp"
#include "xvledger/xvstate.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvstatestore.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"
#include "xstatectx/xtablestate_ctx.h"
#include "xstatectx/xunitstate_ctx.h"
#include "xstatectx/xstatectx_face.h"
#include "xstatectx/xstatectx_base.h"

NS_BEG2(top, statectx)

// the table world state context
class xstatectx_t : public xstatectx_face_t {
 public:
    xstatectx_t(base::xvblock_t* prev_block, const data::xtablestate_ptr_t & prev_table_state, const data::xtablestate_ptr_t & commit_table_state, const xstatectx_para_t & para);
 public:// APIs for vm & tx executor
    const data::xtablestate_ptr_t &     get_table_state() const override;
    data::xunitstate_ptr_t              load_unit_state(const base::xvaccount_t & addr) override;    
    bool                                do_rollback() override;
    size_t                              do_snapshot() override;
    const std::string &                 get_table_address() const override {return m_table_ctx->get_table_address();}
    bool                                is_state_dirty() const override;
    base::xtable_shortid_t              get_tableid() const {return m_table_ctx->get_tableid();}
    std::vector<xunitstate_ctx_ptr_t>   get_modified_unit_ctx() const;

 private:
    xunitstate_ctx_ptr_t    load_unit_ctx(const base::xvaccount_t & addr);
    xunitstate_ctx_ptr_t    find_unit_ctx(const std::string & addr, bool is_same_table);
    void                    add_unit_ctx(const std::string & addr, bool is_same_table, const xunitstate_ctx_ptr_t & unit_ctx);
    bool                    is_same_table(const base::xvaccount_t & addr) const;
    const xstatectx_para_t & get_ctx_para() const {return m_statectx_para;}
 private:
    xstatectx_base_t        m_statectx_base;
    xstatectx_para_t        m_statectx_para;
    xtablestate_ctx_ptr_t   m_table_ctx{nullptr};
    std::map<std::string, xunitstate_ctx_ptr_t>   m_unit_ctxs;
    std::map<std::string, xunitstate_ctx_ptr_t>   m_other_table_unit_ctxs;
};
using xstatectx_ptr_t = std::shared_ptr<xstatectx_t>;

class xstatectx_factory_t {
 public:
    static xstatectx_ptr_t create_statectx(const base::xvaccount_t & table_addr, base::xvblock_t* _block);
    static xstatectx_ptr_t create_latest_cert_statectx(base::xvblock_t* prev_block, const data::xtablestate_ptr_t & prev_table_state, const data::xtablestate_ptr_t & commit_table_state, const xstatectx_para_t & para);
};

NS_END2
