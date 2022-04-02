// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>

#include "xdata/xcons_transaction.h"
#include "xstore/xaccount_context.h"
#include "xtxexecutor/xvm_face.h"

NS_BEG2(top, txexecutor)

class xtvm_t : public xvm_face_t {
 public:
    virtual enum_execute_result_type execute(const xvm_input_t & input, xvm_output_t & output) override;
};

NS_END2
