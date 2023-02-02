// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xstate_reset/xstate_json_parser.h"
#include "xstate_reset/xstate_tablestate_reseter_base.h"
#include "xstatectx/xstatectx_face.h"

NS_BEG2(top, state_reset)

class xstate_tablestate_reseter_sample : public xstate_tablestate_reseter_base {
public:
    xstate_tablestate_reseter_sample(statectx::xstatectx_face_ptr_t statectx_ptr, std::string const & fork_name);
    ~xstate_tablestate_reseter_sample() override = default;

    bool exec_reset_tablestate(std::size_t cnt = 0) override;

private:
    xstate_json_parser m_json_parser;
};

NS_END2