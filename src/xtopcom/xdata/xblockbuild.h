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

class xlightunit_build_t : public base::xvblockbuild_t {
 public:
    xlightunit_build_t(const std::string & account, const xlightunit_block_para_t & bodypara);
    xlightunit_build_t(base::xvblock_t* prev_block, const xlightunit_block_para_t & bodypara, const xblock_consensus_para_t & para);
    xlightunit_build_t(base::xvheader_t* header, base::xvinput_t* input, base::xvoutput_t* output);

    base::xauto_ptr<base::xvblock_t> create_new_block() override;

 private:
    bool build_block_body(const xlightunit_block_para_t & para);
};

class xemptyblock_build_t : public base::xvblockbuild_t {
 public:
    xemptyblock_build_t(const std::string & account);
    xemptyblock_build_t(base::xvblock_t* prev_block, const xblock_consensus_para_t & para);
    xemptyblock_build_t(base::xvblock_t* prev_block);  // TODO(jimmy) not valid block
    xemptyblock_build_t(base::xvheader_t* header);
    xemptyblock_build_t(const std::string & tc_account, uint64_t _tc_height);  // for tc block


    base::xauto_ptr<base::xvblock_t> create_new_block() override;
};

class xfullunit_build_t : public base::xvblockbuild_t {
 public:
    xfullunit_build_t(base::xvblock_t* prev_block, const xfullunit_block_para_t & bodypara, const xblock_consensus_para_t & para);
    xfullunit_build_t(base::xvheader_t* header, base::xvinput_t* input, base::xvoutput_t* output);

    base::xauto_ptr<base::xvblock_t> create_new_block() override;

 private:
    bool build_block_body(const xfullunit_block_para_t & para);
};

class xlighttable_build_t : public base::xvblockbuild_t {
 public:
    xlighttable_build_t(base::xvblock_t* prev_block, const xtable_block_para_t & bodypara, const xblock_consensus_para_t & para);

    base::xauto_ptr<base::xvblock_t> create_new_block() override;

 private:
    bool build_block_body(const xtable_block_para_t & para);
};

class xfulltable_build_t : public base::xvblockbuild_t {
 public:
    xfulltable_build_t(base::xvblock_t* prev_block, const xfulltable_block_para_t & bodypara, const xblock_consensus_para_t & para);

    base::xauto_ptr<base::xvblock_t> create_new_block() override;

 private:
    bool build_block_body(const xfulltable_block_para_t & para);
};

class xrootblock_build_t : public base::xvblockbuild_t {
 public:
    // root block is a root genesis block
    xrootblock_build_t(base::enum_xchain_id chainid, const std::string & account, const xrootblock_para_t & bodypara);

    base::xauto_ptr<base::xvblock_t> create_new_block() override;

 private:
    bool build_block_body(const xrootblock_para_t & para);
};

NS_END2
