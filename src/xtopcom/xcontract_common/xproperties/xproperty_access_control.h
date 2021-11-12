// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xvledger/xvstate.h"
#include "xbase/xvmethod.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xenable_to_string.h"
#include "xbasic/xerror/xerror.h"
#include "xbasic/xmemory.hpp"
#include "xbasic/xserializable_based_on.h"
#include "xcommon/xaddress.h"
#include "xcontract_common/xcontract_execution_param.h"
#include "xcontract_common/xerror/xerror.h"
#include "xstate_accessor/xaccess_control_data.h"
#include "xstate_accessor/xproperties/xproperty_identifier.h"
#include "xvledger/xvcanvas.h"
#include "xvledger/xvstate.h"

#include <map>
#include <memory>
#include <string>
#include <type_traits>

NS_BEG3(top, contract_common, properties)


class xtop_property_utl {
public:
    static void property_assert(bool condition, error::xerrc_t error_enum, std::string const& exception_msg);
};
using xproperty_utl_t = xtop_property_utl;

NS_END3
