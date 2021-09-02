// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xpara_proxy/xchain_para_proxy.h"
#include "xbase/xthread.h"
#include "xbasic/xmemory.hpp"

NS_BEG2(top, para_proxy)

class xtop_beacon_chain_para_proxy final : public xchain_para_proxy_t {
private:
    using xbase_t = xchain_para_proxy_t;

public:
    xtop_beacon_chain_para_proxy(xtop_beacon_chain_para_proxy const &)             = delete;
    xtop_beacon_chain_para_proxy & operator=(xtop_beacon_chain_para_proxy const &) = delete;
    xtop_beacon_chain_para_proxy(xtop_beacon_chain_para_proxy &&)                  = default;
    xtop_beacon_chain_para_proxy & operator=(xtop_beacon_chain_para_proxy &&)      = default;
    ~xtop_beacon_chain_para_proxy() override                                        = default;

    xtop_beacon_chain_para_proxy(observer_ptr<xpara_proxy_t> const & para_proxy,
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
using xbeacon_chain_para_proxy_t = xtop_beacon_chain_para_proxy;

NS_END2
