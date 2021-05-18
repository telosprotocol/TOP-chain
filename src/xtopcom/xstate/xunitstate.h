// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xstate/xunit_state_face.h"

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

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

#include <cstdint>

namespace top {
namespace state {

class xtop_unit_state : public xunit_state_face_t {
private:
    xobject_ptr_t<base::xvbstate_t> m_state{ nullptr };

public:
    xtop_unit_state() = default;
    xtop_unit_state(xtop_unit_state const &) = delete;
    xtop_unit_state & operator=(xtop_unit_state const &) = delete;
    xtop_unit_state(xtop_unit_state &&) = default;
    xtop_unit_state & operator=(xtop_unit_state &&) = default;
    virtual ~xtop_unit_state() = default;

    uint64_t balance() const override;
    uint64_t balance(std::string const & symbol) const override;
    uint64_t nonce() const override;

    xtoken_t withdraw(uint64_t const amount) override;
    xtoken_t withdraw(std::string const & symbol, uint64_t const amount) override;
};
using xunit_state_t = xtop_unit_state;

}
}
