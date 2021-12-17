// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xvledger/xvblockbuild.h"
#include "xdata/xlightunit.h"
#include "xdata/xfullunit.h"
#include "xdata/xfull_tableblock.h"
#include "xdata/xtableblock.h"
#include "xdata/xrootblock.h"
#include "xdata/xblock.h"

NS_BEG2(top, data)

base::xvaction_t make_action(const xcons_transaction_ptr_t & tx);
xlightunit_tx_info_ptr_t build_tx_info(const xcons_transaction_ptr_t & tx);
std::string get_header_extra(const xlightunit_block_para_t & bodypara);

class xlightunit_build_t : public base::xvblockmaker_t {
 public:
    static bool should_build_unit_opt(const uint64_t clock, const uint64_t height);
    xlightunit_build_t(const std::string & account, const xlightunit_block_para_t & bodypara);
    xlightunit_build_t(base::xvblock_t* prev_block, const xlightunit_block_para_t & bodypara, const xblock_consensus_para_t & para);
    xlightunit_build_t(base::xvheader_t* header, base::xvinput_t* input, base::xvoutput_t* output);

    base::xauto_ptr<base::xvblock_t> create_new_block() override;
private:
    bool build_block_body(const xlightunit_block_para_t & para);
};

class xemptyblock_build_t : public base::xvblockmaker_t {
 public:
    xemptyblock_build_t(const std::string & account);
    xemptyblock_build_t(base::xvblock_t* prev_block, const xblock_consensus_para_t & para);
    xemptyblock_build_t(base::xvblock_t* prev_block);  // TODO(jimmy) not valid block
    xemptyblock_build_t(base::xvheader_t* header);
    xemptyblock_build_t(const std::string & tc_account, uint64_t _tc_height);  // for tc block


    base::xauto_ptr<base::xvblock_t> create_new_block() override;
};

class xfullunit_build_t : public base::xvblockmaker_t {
 public:
    xfullunit_build_t(base::xvblock_t* prev_block, const xfullunit_block_para_t & bodypara, const xblock_consensus_para_t & para);
    xfullunit_build_t(base::xvheader_t* header, base::xvinput_t* input, base::xvoutput_t* output);

    base::xauto_ptr<base::xvblock_t> create_new_block() override;

 private:
    bool build_block_body(const xfullunit_block_para_t & para);
};

class xlighttable_build_t : public base::xvtableblock_maker_t {
 public:
    static std::vector<xobject_ptr_t<base::xvblock_t>> unpack_units_from_table(const base::xvblock_t* _tableblock);
    static xobject_ptr_t<base::xvblock_t> unpack_one_unit_from_table(const base::xvblock_t* _tableblock, uint32_t entity_id, const std::string & extend_cert, const std::string & extend_data);
 public:
    xlighttable_build_t(base::xvblock_t* prev_block, const xtable_block_para_t & bodypara, const xblock_consensus_para_t & para);

    base::xauto_ptr<base::xvblock_t> create_new_block() override;

 private:
    bool                build_block_body(const xtable_block_para_t & para, const base::xvaccount_t & account, uint64_t height);
    base::xvaction_t    make_action(const std::vector<xobject_ptr_t<base::xvblock_t>> & batch_units);
    static base::xauto_ptr<base::xvinput_t>     make_unit_input_from_table(const base::xvblock_t* _tableblock, const base::xtable_inentity_extend_t & extend, base::xvinentity_t* _table_unit_inentity);
    static base::xauto_ptr<base::xvoutput_t>    make_unit_output_from_table(const base::xvblock_t* _tableblock, const base::xtable_inentity_extend_t & extend, base::xvoutentity_t* _table_unit_outentity);
};

class xfulltable_build_t : public base::xvblockmaker_t {
 public:
    xfulltable_build_t(base::xvblock_t* prev_block, const xfulltable_block_para_t & bodypara, const xblock_consensus_para_t & para);

    base::xauto_ptr<base::xvblock_t> create_new_block() override;

 private:
    bool build_block_body(const xfulltable_block_para_t & para, const base::xvaccount_t & account, uint64_t height);
};

class xrootblock_build_t : public base::xvblockmaker_t {
 public:
    // root block is a root genesis block
    xrootblock_build_t(base::enum_xchain_id chainid, const std::string & account, const xrootblock_para_t & bodypara);

    base::xauto_ptr<base::xvblock_t> create_new_block() override;

 private:
    bool build_block_body(const xrootblock_para_t & para);
};

NS_END2
