// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xproperties/xproperty_access_control.h"

#include "xbasic/xerror/xerror.h"
#include "xdata/xproperty.h"
#include "xvledger/xvledger.h"

#include <cassert>

NS_BEG3(top, contract_common, properties)


void xtop_property_utl::property_assert(bool condition, error::xerrc_t error_enum, std::string const& exception_msg) {
    if (!condition) {
        std::error_code ec{ error_enum };
        top::error::throw_error(ec, exception_msg);
    }
}

NS_END3
