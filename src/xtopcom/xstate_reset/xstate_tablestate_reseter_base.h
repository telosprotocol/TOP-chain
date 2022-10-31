// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xns_macro.h"
#include "xstatectx/xstatectx_face.h"

#include <memory>

NS_BEG2(top, state_reset)

/// tablestate reseter base hold statectx relevant
/// provide api to reset each specific properties.
/// while not care how this data come from (no json parse code here)
class xstate_tablestate_reseter_base {
public:
    xstate_tablestate_reseter_base(statectx::xstatectx_face_ptr_t statectx_ptr);

    virtual bool exec_reset_tablestate() = 0;

private:
    statectx::xstatectx_face_ptr_t m_statectx_ptr;
    base::xvaccount_t m_table_account;  // todo use xtable_address_t
};

using xstate_tablestate_reseter_base_ptr = std::unique_ptr<xstate_tablestate_reseter_base>;

NS_END2