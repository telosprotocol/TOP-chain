// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xmem_trace.h"
#include "xbase/xcontext.h"
#include "xmetrics/xmetrics.h"

NS_BEG2(top, data)

void xmemtrace_plugin_t::fresh_obj_count(int mgr_type, xobject_t* target) {
#if 0
    XMETRICS_COUNTER_SET("xbaseobject_" + std::to_string(target->get_obj_type()), m_all_type_mgr[mgr_type].m_obj_count);
#else  // optimize metrics record performance
    if (m_all_type_mgr[mgr_type].m_obj_opt_count++ > 100) {
        m_all_type_mgr[mgr_type].m_obj_opt_count = 0;
        XMETRICS_COUNTER_SET("xbaseobject_" + std::to_string(target->get_obj_type()), m_all_type_mgr[mgr_type].m_obj_count);
    }
#endif
}

bool xmemtrace_plugin_t::on_object_create(xobject_t* target) {
    if (target->get_obj_type() < XMEMTRACE_XOBJECT_TYPE_MIN) {
        return false;
    }

    //xdbg("xmemcheck_dbgplugin_t::on_object_create this=%p,type=%d", target, target->get_obj_type());
    int mgr_type = obj_type_to_mgr_type(target->get_obj_type());    
    m_all_type_mgr[mgr_type].m_obj_count++;
    fresh_obj_count(mgr_type, target);
    
    return true;
}

bool xmemtrace_plugin_t::on_object_destroy(xobject_t* target) {
    if (target->get_obj_type() < XMEMTRACE_XOBJECT_TYPE_MIN) {
        return false;
    }
    //xdbg("xmemcheck_dbgplugin_t::on_object_destroy this=%p,type=%d", target, target->get_obj_type());
    int mgr_type = obj_type_to_mgr_type(target->get_obj_type());
    m_all_type_mgr[mgr_type].m_obj_count--;
    fresh_obj_count(mgr_type, target);
    return true;
}



bool xmemtrace_plugin_t::on_object_addref(xobject_t* target) {
    return true;
}

bool xmemtrace_plugin_t::on_object_releaseref(xobject_t* target) {
    return true;
}


void xmemtrace_plugin_t::init() {
    xkinfo("xmemtrace_plugin_t::init");
    xmemtrace_plugin_t * dbg_plugin = new xmemtrace_plugin_t();
    top::base::xcontext_t::instance().set_debug_modes(top::base::xcontext_t::enum_debug_mode_memory_check);
    top::base::xcontext_t::instance().set_debug_plugin(dbg_plugin);
}

NS_END2
