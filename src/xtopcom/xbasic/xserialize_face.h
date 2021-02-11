// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"
#include "xbase/xdata.h"


NS_BEG2(top, basic)

class xserialize_face_t : public base::xdataunit_t {
protected:
    xserialize_face_t() : 
    base::xdataunit_t(base::xdataunit_t::enum_xdata_type_unknow) {}
    
    virtual ~xserialize_face_t() {}   
};

NS_END2