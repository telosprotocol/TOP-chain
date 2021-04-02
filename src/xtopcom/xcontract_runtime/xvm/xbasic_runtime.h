#pragma once

#include "xcontract_runtime/xvm/xruntime_face.h"

NS_BEG3(top, contract_runtime, vm)

class xtop_basic_runtime : public xruntime_face_t {
public:
    xtop_basic_runtime(xtop_basic_runtime const &) = delete;
    xtop_basic_runtime & operator=(xtop_basic_runtime const &) = delete;
    xtop_basic_runtime(xtop_basic_runtime &&) = default;
    xtop_basic_runtime & operator=(xtop_basic_runtime &&) = default;
    ~xtop_basic_runtime() override = default;

protected:
    xtop_basic_runtime() = default;

public:
    std::unique_ptr<xsession_t> new_session(observer_ptr<contract_common::xcontract_state_t> contract_state) override;
};
using xbasic_runtime_t = xtop_basic_runtime;

NS_END3
