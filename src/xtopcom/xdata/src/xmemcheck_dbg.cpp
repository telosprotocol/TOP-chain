// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xmemcheck_dbg.h"
#include "xbase/xutl.h"
//#include "xbase/xdata.h"
#include "xbase/xcontext.h"
#include "xbasic/xdbg.h"

NS_BEG2(top, data)

#if defined XENABLE_MEMCHECK_DBG

xmemcheck_common_t::xmemcheck_common_t() {
    m_last_print_timestamp = (uint64_t)base::xtime_utl::gmttime_ms() / 1000;
}

xmemcheck_common_t::~xmemcheck_common_t() {
}

void xmemcheck_common_t::add_trace(uint64_t pointer, const std::string & tag, int obj_type) {
    std::lock_guard<std::mutex> l(m_lock);
    m_obj_type = obj_type;
    uint64_t now = (uint64_t)base::xtime_utl::gmttime_ms() / 1000;

    auto iter = m_alloc_pointers.find(pointer);
    if (iter == m_alloc_pointers.end()) {
        xmemcheck_common_info_t info;
        info.m_this_pointer = pointer;
        info.m_add_timestamp = now;
#if defined XENABLE_PSTACK
        xmemcheck_trace_info_t trace_info;
        trace_info.m_tag = tag;
        trace_info.m_traces = xdbg_helper::get_trace();
        info.m_backtraces.push_back(trace_info);
#endif
        m_alloc_pointers[pointer] = info;
        xdbg("xmemcheck_common_t::add_trace,create obj_type=%d,this=%p,add_timestamp=%ld", obj_type, pointer, info.m_add_timestamp);
    } else {
        xdbg("xmemcheck_common_t::add_trace,add obj_type=%d,this=%p", obj_type, pointer);
#if defined XENABLE_PSTACK
        xmemcheck_trace_info_t trace_info;
        trace_info.m_tag = tag;
        trace_info.m_traces = xdbg_helper::get_trace();
        iter->second.m_backtraces.push_back(trace_info);
#endif
    }

    // check and print leak blocks
    print_leak_pointers(now);
}

void xmemcheck_common_t::print_trace(const xmemcheck_common_info_t & trace_info) {
    for (auto & v : trace_info.m_backtraces) {
        uint32_t index = 0;
        for (auto & trace : v.m_traces) {
            xdbg("xmemcheck_common_t::print_trace %p-%s[%d]-%s", trace_info.m_this_pointer, v.m_tag.c_str(), index++, trace.c_str());
        }
    }
}

void xmemcheck_common_t::remove_trace(uint64_t pointer) {
    std::lock_guard<std::mutex> l(m_lock);
    auto iter = m_alloc_pointers.find(pointer);
    if (iter == m_alloc_pointers.end()) {
        xerror("xmemcheck_common_t::remove_trace not find block. this=%p", pointer);
    } else {
        xdbg("xmemcheck_common_t::remove_trace this=%p,add_timestamp=%ld", pointer, iter->second.m_add_timestamp);
        m_alloc_pointers.erase(iter);
    }
}

void xmemcheck_common_t::print_leak_pointers(uint64_t now) {
    if (now < m_last_print_timestamp + m_check_interval) {
        return;
    }
    m_last_print_timestamp = now;
    uint64_t start_leak_time = now - m_leak_time_s;
    uint32_t leak_count = 0;
    for (auto & v : m_alloc_pointers) {
        if (v.second.m_add_timestamp < start_leak_time) {
            base::xobject_t* dataunit = reinterpret_cast<base::xobject_t*>(v.first);
            xwarn("print_leak_pointers obj_type=%d,time=%ld,pointer=%p,dataunit=%s", m_obj_type, now - v.second.m_add_timestamp, v.first, dataunit->dump().c_str());
            print_trace(v.second);
            leak_count++;
        }
    }
    xinfo("print_leak_pointers obj_type=%d,size=%zu,leak_count=%d", m_obj_type, m_alloc_pointers.size(), leak_count);
}

bool xmemcheck_dbgplugin_t::on_object_create(xobject_t* target) {
    //xdbg("xmemcheck_dbgplugin_t::on_object_create this=%p,type=%d", target, target->get_obj_type());
    int mgr_type = obj_type_to_mgr_type(target->get_obj_type());
    xassert(mgr_type >= 0);
    m_all_type_mgr[mgr_type].add_trace((uint64_t)target, "", target->get_obj_type());
    return true;
}

bool xmemcheck_dbgplugin_t::on_object_destroy(xobject_t* target) {
    //xdbg("xmemcheck_dbgplugin_t::on_object_destroy this=%p,type=%d", target, target->get_obj_type());
    int mgr_type = obj_type_to_mgr_type(target->get_obj_type());
    xassert(mgr_type >= 0);
    m_all_type_mgr[mgr_type].remove_trace((uint64_t)target);
    return true;
}

bool xmemcheck_dbgplugin_t::on_object_addref(xobject_t* target) {
    xdbg("xmemcheck_dbgplugin_t::on_object_addref this=%p,type=%d", target, target->get_obj_type());
    return true;
}

bool xmemcheck_dbgplugin_t::on_object_releaseref(xobject_t* target) {
    xdbg("xmemcheck_dbgplugin_t::on_object_releaseref this=%p,type=%d", target, target->get_obj_type());
    return true;
}

void xmemcheck_dbgplugin_t::xmemcheck_dbg_init() {
    xkinfo("xmemcheck_dbgplugin_t::xmemcheck_dbg_init");
    xmemcheck_dbgplugin_t * dbg_plugin = new xmemcheck_dbgplugin_t();
    top::base::xcontext_t::instance().set_debug_modes(top::base::xcontext_t::enum_debug_mode_memory_check);
    top::base::xcontext_t::instance().set_debug_plugin(dbg_plugin);
}

#endif
NS_END2
