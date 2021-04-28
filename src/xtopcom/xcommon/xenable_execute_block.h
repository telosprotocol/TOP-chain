// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.#pragma once

#pragma once

#include "xbase/xobject_ptr.h"
#include "xbasic/xerror/xthrow_error.h"
#include "xdata/xblock.h"

#include <system_error>

NS_BEG2(top, common)

template <class T, template <typename> class SmartPtrT>
class xtop_enable_execute_block {
public:
    xtop_enable_execute_block() = default;
    xtop_enable_execute_block(xtop_enable_execute_block const &) = delete;
    xtop_enable_execute_block & operator=(xtop_enable_execute_block const &) = delete;
    xtop_enable_execute_block(xtop_enable_execute_block &&) = default;
    xtop_enable_execute_block & operator=(xtop_enable_execute_block &&) = default;
    virtual ~xtop_enable_execute_block() = default;

    /// @brief Apply block data onto current object of type T.
    /// @param block The block object to be applied.
    /// @param ec The error code that will be set in the execute process.
    virtual void execute_block(SmartPtrT<data::xblock_t const> block, std::error_code & ec) = 0;

    /// @brief Apply block data onto current object of type T.
    ///        If any error is seen, top::error::xchain_error_t exception will be thrown.
    /// @param block The block object ot be applied.
    virtual void execute_block(SmartPtrT<data::xblock_t const> block) {
        std::error_code ec;
        execute_block(std::move(block), ec);
        top::error::throw_error(ec);
    }
};

template <class T, template <typename> class SmartPtrT = xobject_ptr_t>
using xenable_execute_block_t = xtop_enable_execute_block<T, SmartPtrT>;

NS_END2
