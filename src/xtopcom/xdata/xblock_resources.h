// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xbasic/xns_macro.h"
#include "xbasic/xdataobj_base.hpp"
#include "xbase/xdata.h"
#include "xvledger/xvblock.h"
#include "xdata/xtransaction.h"

NS_BEG2(top, data)

class xresource_wholeblock_t : public xbase_dataunit_t<xresource_wholeblock_t, xdata_type_whole_block_resource>{
 public:
    xresource_wholeblock_t() = default;
    explicit xresource_wholeblock_t(base::xvblock_t* block);

 protected:
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 public:
    base::xauto_ptr<base::xvblock_t>    create_whole_block() const;

 private:
    std::string     m_block_object;
    std::string     m_input_resource;
    std::string     m_output_resource;
};

class xresource_origintx_t : public xbase_dataunit_t<xresource_origintx_t, xdata_type_origin_tx_resource>{
 public:
    xresource_origintx_t() = default;
    explicit xresource_origintx_t(xtransaction_t* tx);

 protected:
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 public:
    xtransaction_ptr_t create_origin_tx() const;

 private:
    std::string     m_origin_tx;
};

class xresource_unit_input_t : public xbase_dataunit_t<xresource_unit_input_t, xdata_type_tableblock_unitinput_resource>{
 public:
    xresource_unit_input_t() = default;
    explicit xresource_unit_input_t(base::xvblock_t* block);

 protected:
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 public:
    const std::string & get_unit_header() const {return m_unit_header;}
    const std::string & get_unit_input() const {return m_unit_input;}
    const std::string & get_unit_input_resources() const {return m_unit_input_resources;}

 private:
    std::string     m_unit_header;
    std::string     m_unit_input;
    std::string     m_unit_input_resources;
};

class xresource_unit_output_t : public xbase_dataunit_t<xresource_unit_output_t, xdata_type_tableblock_unitoutput_resource>{
 public:
    xresource_unit_output_t() = default;
    explicit xresource_unit_output_t(base::xvblock_t* block);

 protected:
    int32_t do_write(base::xstream_t & stream) override;
    int32_t do_read(base::xstream_t & stream) override;

 public:
    const std::string & get_unit_output() const {return m_unit_output;}
    const std::string & get_unit_output_resources() const {return m_unit_output_resources;}
    const std::string & get_unit_justify_hash() const {return m_unit_justify_hash;}

 private:
    std::string     m_unit_output;
    std::string     m_unit_output_resources;
    std::string     m_unit_justify_hash;
};

NS_END2
