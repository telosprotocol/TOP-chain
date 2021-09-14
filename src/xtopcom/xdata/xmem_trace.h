// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <atomic>
#include "xbase/xobject.h"
#include "xbase/xns_macro.h"

NS_BEG2(top, data)


class xmemtrace_objinfo_t {
 public:
    std::atomic<uint32_t>    m_obj_count{0};
    std::atomic<uint32_t>    m_obj_opt_count{0};
};

#define XMEMTRACE_XOBJECT_TYPE_MIN  (-500)

class xmemtrace_plugin_t : public base::xdbgplugin_t {
 public:
    xmemtrace_plugin_t() = default;
 protected:
    virtual ~xmemtrace_plugin_t() {}
 private:
    xmemtrace_plugin_t(const xmemtrace_plugin_t &);
    xmemtrace_plugin_t & operator = (const xmemtrace_plugin_t &);
 public:  // subclass need override
    static  void       init();
    int                obj_type_to_mgr_type(int obj_type) {return obj_type - XMEMTRACE_XOBJECT_TYPE_MIN;}
    virtual bool       on_object_create(base::xobject_t* target) override;
    virtual bool       on_object_destroy(xobject_t* target) override;
    virtual bool       on_object_addref(xobject_t* target) override;
    virtual bool       on_object_releaseref(xobject_t* target) override;

 private:
    void               fresh_obj_count(int mgr_type, xobject_t* target);

 private:
    static constexpr int m_obj_type_max{base::enum_xobject_type_max - XMEMTRACE_XOBJECT_TYPE_MIN};  // enum_xobject_type_max enum_xobject_type_min
    xmemtrace_objinfo_t  m_all_type_mgr[m_obj_type_max];
};

#define XENABLE_MEMCHECK_DBG

#if defined XENABLE_MEMCHECK_DBG
# define MEMTRACE_INIT()                            data::xmemtrace_plugin_t::init()
#else
# define MEMCHECK_INIT()
#endif

NS_END2
