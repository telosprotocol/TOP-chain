// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xbase/xns_macro.h"
#include "xvledger/xvblock.h"

NS_BEG3(top, vnode, components)
using xvblock_class_t = base::enum_xvblock_class;
using xvblock_level_t = base::enum_xvblock_level;

class xtop_block_process_face {
public:
    xtop_block_process_face() = default;
    ~xtop_block_process_face() =  default;

protected:
    xtop_block_process_face(xtop_block_process_face const&) = delete;
    xtop_block_process_face& operator=(xtop_block_process_face const&) = delete;
    xtop_block_process_face(xtop_block_process_face&&) = delete;
    xtop_block_process_face& operator=(xtop_block_process_face&&) = delete;


public:
    virtual xvblock_class_t block_class(xobject_ptr_t<base::xvblock_t> const & vblock) = 0;
    virtual xvblock_level_t block_level(xobject_ptr_t<base::xvblock_t> const& vblock) = 0;
    virtual void process_block(xobject_ptr_t<base::xvblock_t> const& vblock) = 0;

};

NS_END3