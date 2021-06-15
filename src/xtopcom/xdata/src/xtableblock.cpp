// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbase/xutl.h"
#include "xvledger/xvblockbuild.h"
#include "xdata/xtableblock.h"
#include "xdata/xlightunit.h"
#include "xdata/xrootblock.h"
#include "xdata/xemptyblock.h"
#include "xdata/xblockbuild.h"
#include "xdata/xblockaction.h"

NS_BEG2(top, data)

REG_CLS(xtable_block_t);

xtable_block_t::xtable_block_t(base::xvheader_t & header, base::xvqcert_t & cert, base::xvinput_t* input, base::xvoutput_t* output)
: xblock_t(header, cert, input, output, (enum_xdata_type)object_type_value) {

}

xtable_block_t::xtable_block_t()
: xblock_t((enum_xdata_type)object_type_value) {

}

xtable_block_t::~xtable_block_t() {
}

base::xobject_t * xtable_block_t::create_object(int type) {
    (void)type;
    return new xtable_block_t;
}

void * xtable_block_t::query_interface(const int32_t _enum_xobject_type_) {
    if (object_type_value == _enum_xobject_type_)
        return this;
    return xvblock_t::query_interface(_enum_xobject_type_);
}

const std::vector<xblock_ptr_t> & xtable_block_t::get_tableblock_units(bool need_parent_cert) const {
    return unpack_and_get_units(need_parent_cert);
}

void xtable_block_t::unpack_proposal_units(std::vector<xblock_ptr_t> & units) const {
    std::vector<xobject_ptr_t<base::xvblock_t>> _units = xlighttable_build_t::unpack_units_from_table(this);
    xassert(!_units.empty());
    for (auto & v : _units) {  // TODO(jimmy)
        units.push_back(dynamic_xobject_ptr_cast<xblock_t>(v));
    }
}

const std::vector<xblock_ptr_t> & xtable_block_t::unpack_and_get_units(bool need_parent_cert) const {
    if (check_block_flag(base::enum_xvblock_flag_authenticated)) {
        std::call_once(m_once_unpack_flag, [this] () {
            unpack_proposal_units(m_cache_units);
        });
    }
    return m_cache_units;
}

uint32_t xtable_block_t::get_txs_count() const {
    // TODO(jimmy) tx count == actions count ?
    uint32_t tx_count = 0;
    auto & units = get_tableblock_units();
    for (auto & unit : units) {
        tx_count += unit->get_txs_count();
    }
    return tx_count;
}

int64_t xtable_block_t::get_pledge_balance_change_tgas() const {
    int64_t pledge_tgas_change = 0;
    auto & units = get_tableblock_units();
    for (auto & unit : units) {
        pledge_tgas_change += unit->get_pledge_balance_change_tgas();
    }
    return pledge_tgas_change;
}

bool  xtable_block_t::extract_sub_blocks(std::vector<xobject_ptr_t<base::xvblock_t>> & sub_blocks) {
    const std::vector<xblock_ptr_t> & subblocks = get_tableblock_units(true);
    xassert(!subblocks.empty());
    for (auto & v : subblocks) {
        sub_blocks.push_back(v);
    }
    return true;
}

NS_END2
