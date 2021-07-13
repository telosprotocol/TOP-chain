// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include "xbase/xdata.h"
#include "xvledger/xvblock.h"
#include "xdata/xtransaction.h"

NS_BEG2(top, data)

// blockpara should has small key
// string value can change to any data type
// value zero will not save actually
class xblockpara_base_t {
 public:
    xblockpara_base_t();
    xblockpara_base_t(const std::map<std::string, std::string> & values);
    virtual std::string dump() const;
    const std::map<std::string, std::string> & get_map_para() const {return m_values;}

    virtual int32_t do_write(base::xstream_t & stream);
    virtual int32_t do_read(base::xstream_t & stream);

 protected:
    void            delete_value(const std::string & key);
    void            set_value(const std::string & key, bool value);
    void            set_value(const std::string & key, uint16_t value);
    void            set_value(const std::string & key, uint32_t value);
    void            set_value(const std::string & key, uint64_t value);
    void            set_value(const std::string & key, const std::string & value);

    bool            get_value_bool(const std::string & key) const;
    uint16_t        get_value_uint16(const std::string & key) const;
    uint32_t        get_value_uint32(const std::string & key) const;
    uint64_t        get_value_uint64(const std::string & key) const;
    std::string     get_value(const std::string & key) const;

 private:
    std::map<std::string, std::string>  m_values;
};

NS_END2
