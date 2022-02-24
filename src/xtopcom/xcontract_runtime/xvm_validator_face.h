// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvstate.h"
#include "xbase/xobject_ptr.h"
#include "xcommon/xaddress.h"
#include "xdata/xtransaction.h"

NS_BEG2(top, contract_runtime)

class xtop_vm_validator_face {
public:
    xtop_vm_validator_face() = default;
    xtop_vm_validator_face(xtop_vm_validator_face const &) = delete;
    xtop_vm_validator_face & operator=(xtop_vm_validator_face const &) = delete;
    xtop_vm_validator_face(xtop_vm_validator_face &&) = default;
    xtop_vm_validator_face & operator=(xtop_vm_validator_face &&) = default;
    virtual ~xtop_vm_validator_face() = default;

    virtual validate_transaction(data::xtransaction_ptr_t const & tx, xobject_ptr_t<base::xvbstate_t> block_state) = 0;
};
using xvm_validator_face_t = xtop_vm_validator_face;

NS_END2
