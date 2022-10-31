// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_reset/xstate_tablestate_reseter_sample.h"

#include "xstatectx/xstatectx.h"

NS_BEG2(top, state_reset)
xstate_tablestate_reseter_sample::xstate_tablestate_reseter_sample(statectx::xstatectx_face_ptr_t statectx_ptr, std::string const & fork_name)
  : xstate_tablestate_reseter_base{statectx_ptr}, m_json_parser{statectx_ptr->get_table_address(), fork_name} {
}

bool xstate_tablestate_reseter_sample::exec_reset_tablestate() {
    // todo
    return true;
}

NS_END2