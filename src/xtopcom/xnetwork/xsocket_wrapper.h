// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xsocket.h"
#include "xbasic/xthreading/xutility.h"

#include <cassert>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <type_traits>

NS_BEG2(top, network)

template
<
    typename XBaseSocketT,
    typename std::enable_if<std::is_base_of<base::xsocket_t, XBaseSocketT>::value>::type * = nullptr
>
class xtop_socket : public XBaseSocketT
{
public:
    using on_receive_callback_t =
        std::function<std::int32_t(std::uint64_t,
                                   std::uint64_t,
                                   std::uint64_t,
                                   std::uint64_t,
                                   base::xpacket_t &,
                                   std::int32_t,
                                   std::chrono::milliseconds,
                                   base::xendpoint_t *)>;

private:
    using base_t = XBaseSocketT;
    mutable std::mutex m_callback_mutex{};
    on_receive_callback_t m_callback{};

public:
    xtop_socket(xtop_socket const &)             = delete;
    xtop_socket & operator=(xtop_socket const &) = delete;
    xtop_socket(xtop_socket &&)                  = default;
    xtop_socket & operator=(xtop_socket &&)      = default;
    ~xtop_socket() override                      = default;

    xtop_socket(base::xcontext_t & context,
                base::xendpoint_t * parent,
                int32_t const target_thread_id,
                xfd_handle_t native_handle)
        : base_t(context, parent, target_thread_id, native_handle)  // using base_t{ ... } here doesn't work.  you'll get a compiling error here which is a GCC4.8 bug!
    {
    }

private:
    int32_t
    recv(std::uint64_t from_xip_addr_low,
         uint64_t from_xip_addr_high,
         uint64_t to_xip_addr_low,
         uint64_t to_xip_addr_high,
         base::xpacket_t & packet,
         int32_t cur_thread_id,
         uint64_t timenow_ms,
         base::xendpoint_t* from_child_end) override {
        if (m_callback) {
            return m_callback(from_xip_addr_low,
                              from_xip_addr_high,
                              to_xip_addr_low,
                              to_xip_addr_high,
                              packet,
                              cur_thread_id,
                              std::chrono::milliseconds{ timenow_ms },
                              from_child_end);
        }

        return 0;
    }

public:
    void
    register_on_receive_callback(on_receive_callback_t callback) {
        XLOCK_GUARD(m_callback_mutex) {
            assert(m_callback == nullptr);
            m_callback = std::move(callback);
        }
    }

    void
    unregister_on_receiver_callback() {
        XLOCK_GUARD(m_callback_mutex) {
            if (m_callback != nullptr) {
                m_callback = nullptr;
            }
        }
    }
};

template <typename XBaseSocketT>
using xsocket_t = xtop_socket<XBaseSocketT>;

NS_END2
