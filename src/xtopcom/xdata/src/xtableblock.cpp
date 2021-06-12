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

void xtable_block_t::cache_units_set_parent_cert(std::vector<xblock_ptr_t> & units, base::xvqcert_t* parent_cert) const {
    xassert(!units.empty());
    std::vector<xobject_ptr_t<xvblock_t>> _batch_units;  // TODO(jimmy) change to base class vector
    for (auto & v : units) {
        _batch_units.push_back(v);
    }
    base::xvtableblock_maker_t::units_set_parent_cert(_batch_units, parent_cert);
}

void xtable_block_t::create_txreceipts(std::vector<xcons_transaction_ptr_t> & sendtx_receipts,
    std::vector<xcons_transaction_ptr_t> & recvtx_receipts) {
    auto & units = get_tableblock_units(true);
    for (auto & cache_unit : units) {
        if (cache_unit->is_lightunit()) {
            (dynamic_cast<xlightunit_block_t*>(cache_unit.get()))->create_txreceipts(sendtx_receipts, recvtx_receipts);
        }
    }
}

const std::vector<xblock_ptr_t> & xtable_block_t::get_tableblock_units(bool need_parent_cert) const {
    return unpack_and_get_units(need_parent_cert);
}

xblock_ptr_t xtable_block_t::create_whole_unit(const std::string & header,
                                                    const std::string & input,
                                                    const std::string & input_res,
                                                    const std::string & output,
                                                    const std::string & output_res,
                                                    const base::xbbuild_para_t & build_para) {
    // recreate whole block
    base::xauto_ptr<base::xvheader_t> _header = base::xvblock_t::create_header_object(header);
    xassert(_header != nullptr);
    base::xauto_ptr<base::xvinput_t> _input = base::xvblock_t::create_input_object(input);
    xassert(_input != nullptr);
    base::xauto_ptr<base::xvoutput_t> _output = base::xvblock_t::create_output_object(output);
    xassert(_output != nullptr);

    // TODO(jimmy)
    xobject_ptr_t<base::xvinput_t> _new_input = make_object_ptr<base::xvinput_t>(_input->get_entitys(), input_res);
    xobject_ptr_t<base::xvoutput_t> _new_output = make_object_ptr<base::xvoutput_t>(_output->get_entitys(), output_res);

    std::shared_ptr<base::xvblockmaker_t> vbbuild = nullptr;

    if (_header->get_block_class() == base::enum_xvblock_class_light) {
        vbbuild = std::make_shared<xlightunit_build_t>(_header.get(), _new_input.get(), _new_output.get());
    } else if (_header->get_block_class() == base::enum_xvblock_class_full) {
        vbbuild = std::make_shared<xfullunit_build_t>(_header.get(), _new_input.get(), _new_output.get());
    } else if (_header->get_block_class() == base::enum_xvblock_class_nil) {
        vbbuild = std::make_shared<xemptyblock_build_t>(_header.get());
    }
    vbbuild->init_qcert(build_para);
    base::xauto_ptr<base::xvblock_t> _vblock = vbbuild->build_new_block();
    xassert(_vblock != nullptr);
    xblock_ptr_t _block = xblock_t::raw_vblock_to_object_ptr(_vblock());
    return _block;
}

xblock_ptr_t xtable_block_t::recreate_unit_from_unit_input_output_resource(uint32_t index) const {
    base::xauto_ptr<base::xtable_unit_resource_t> _unit_res = base::xvtableblock_maker_t::query_unit_resource(this, index);
    xassert(_unit_res != nullptr);

    base::xbbuild_para_t build_para;
    build_para.set_unit_cert_para(get_cert()->get_clock(), get_cert()->get_viewtoken(), get_cert()->get_viewid(), get_cert()->get_validator(),
                                 get_cert()->get_auditor(), get_cert()->get_drand_height(), get_height(), _unit_res->get_unit_justify_hash());
    xblock_ptr_t _unit_block = xtable_block_t::create_whole_unit(_unit_res->get_unit_header(),
                                                                                    _unit_res->get_unit_input(),
                                                                                    _unit_res->get_unit_input_resources(),
                                                                                    _unit_res->get_unit_output(),
                                                                                    _unit_res->get_unit_output_resources(),
                                                                                    build_para);
    xassert(_unit_block != nullptr);
    _unit_block->reset_block_flags();
    xassert(_unit_block->get_block_hash().empty());
    return _unit_block;
}

void xtable_block_t::unpack_proposal_units(std::vector<xblock_ptr_t> & units) const {
    xlighttable_action_t _tableaction(get_input()->get_primary_entity()->get_actions()[0]);
    uint32_t count = _tableaction.get_unit_number();
    xassert(count != 0);

    for (uint32_t index = 0; index < count; index++) {
        // recreate whole block
        xblock_ptr_t _block_ptr = recreate_unit_from_unit_input_output_resource(index);
        units.push_back(_block_ptr);
    }
}

const std::vector<xblock_ptr_t> & xtable_block_t::unpack_and_get_units(bool need_parent_cert) const {
    if (check_block_flag(base::enum_xvblock_flag_authenticated)) {
        std::call_once(m_once_unpack_flag, [this] () {
            unpack_proposal_units(m_cache_units);
            cache_units_set_parent_cert(m_cache_units, get_cert());
            for (auto & unit : m_cache_units) {
                if(   (false == unit->is_input_ready(false))
                    || (false == unit->is_output_ready(false))
                    || (false == unit->is_deliver(false)) )//must have full valid data and has mark as enum_xvblock_flag_authenticated
                {
                    xerror("block=%s,cert:%s,header:%s",
                        unit->dump().c_str(), unit->dump_cert().c_str(), unit->dump_header().c_str());
                }
            }
        });
    }
    return m_cache_units;
}

uint32_t xtable_block_t::get_txs_count() const {
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
