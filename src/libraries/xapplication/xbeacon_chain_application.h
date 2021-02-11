// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xapplication/xchain_application.h"
#include "xbase/xthread.h"
#include "xbasic/xmemory.hpp"

NS_BEG2(top, application)

class xtop_beacon_chain_application final : public xchain_application_t {
private:
    using xbase_t = xchain_application_t;

public:
    xtop_beacon_chain_application(xtop_beacon_chain_application const &)             = delete;
    xtop_beacon_chain_application & operator=(xtop_beacon_chain_application const &) = delete;
    xtop_beacon_chain_application(xtop_beacon_chain_application &&)                  = default;
    xtop_beacon_chain_application & operator=(xtop_beacon_chain_application &&)      = default;
    ~xtop_beacon_chain_application() override                                        = default;

    xtop_beacon_chain_application(observer_ptr<xapplication_t> const & application,
                                  xobject_ptr_t<base::xvblockstore_t> &blockstore,
                                  xobject_ptr_t<base::xvnodesrv_t> &nodesvr_ptr,
                                  xobject_ptr_t<base::xvcertauth_t> &cert_ptr,
                                  observer_ptr<base::xiothread_t> const & grpc_thread,
                                  observer_ptr<base::xiothread_t> const & sync_thread,
                                  std::vector<observer_ptr<base::xiothread_t>> const & sync_account_thread_pool,
                                  std::vector<observer_ptr<base::xiothread_t>> const & sync_handler_thread_pool);
protected:
    void
    load_last_election_data() override;

    bool
    enable_grpc_service() const noexcept override { return true; }
};
using xbeacon_chain_application_t = xtop_beacon_chain_application;

NS_END2
