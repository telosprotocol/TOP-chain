// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include "xbase/xdata.h"
#include "xbase/xobject_ptr.h"
#include "xbase/xns_macro.h"

NS_BEG2(top, base)


// the block offdata includes some propertys
class xvboffdata_t : public xdataunit_t {
 public:
    static  const std::string   name(){ return std::string("xvboffdata");}
    virtual std::string         get_obj_name() const override {return name();}
    xvboffdata_t();
 protected:
    virtual ~xvboffdata_t() {}
    int32_t         do_write(base::xstream_t & stream) override;
    int32_t         do_read(base::xstream_t & stream) override;

 private://not implement those private construction
    xvboffdata_t(xvboffdata_t &&);
    xvboffdata_t & operator = (const xvboffdata_t & other);

 public:
    std::string     build_root_hash(enum_xhash_type hashtype);  // build root of the whold offdata

 public:
    xobject_ptr_t<xdataunit_t>      query_offdata(const std::string & name) const;

 private:
    std::map<std::string, xobject_ptr_t<xdataunit_t>>  m_propertys;  // 块下数据由多种具体数据组成

 private:  // local member
    uint64_t        m_height{0};
    std::string     m_block_hash;
};

NS_END2
