// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <atomic>
#include "xbase/xdata.h"
#include "xbasic/xmemory.hpp"

#include <string>

NS_BEG2(top, statestore)

class statestore_prune_dispatcher_t
  :  public base::xiobject_t {
public:
    statestore_prune_dispatcher_t(base::xcontext_t & _context, int32_t thread_id)
      : base::xiobject_t(_context, thread_id, base::enum_xobject_type_woker) {
    }

    void dispatch(base::xcall_t & call) {
        send_call(call);
    }
protected:
    ~statestore_prune_dispatcher_t() override {
    }
};

class xstatestore_resources_t {
public:
    xstatestore_resources_t();
    virtual ~xstatestore_resources_t();

public:
    void set_prune_dispatcher(const observer_ptr<statestore_prune_dispatcher_t> & prune_dispatcher);
    statestore_prune_dispatcher_t * get_prune_dispatcher();

private:
    observer_ptr<statestore_prune_dispatcher_t> m_prune_dispatcher{nullptr};
};

NS_END2
