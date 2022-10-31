// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_reset/xstate_tablestate_reseter_base.h"

#include "xstatectx/xstatectx.h"

NS_BEG2(top, state_reset)

xstate_tablestate_reseter_base::xstate_tablestate_reseter_base(statectx::xstatectx_face_ptr_t statectx_ptr)
  : m_statectx_ptr{statectx_ptr}, m_table_account{m_statectx_ptr->get_table_address()} {
}

NS_END2