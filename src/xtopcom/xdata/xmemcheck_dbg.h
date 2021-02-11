// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include "xbase/xobject.h"
#include "xbasic/xns_macro.h"

NS_BEG2(top, data)
#if defined XENABLE_MEMCHECK_DBG
class xmemcheck_common_t final {
 protected:
    struct xmemcheck_trace_info_t {
        std::string                 m_tag;
        std::vector<std::string>    m_traces;
    };
    struct xmemcheck_common_info_t {
        uint64_t    m_this_pointer;
        uint64_t    m_add_timestamp;
        std::vector<xmemcheck_trace_info_t> m_backtraces;
    };

 public:
    xmemcheck_common_t();
    ~xmemcheck_common_t();

    static xmemcheck_common_t& get_instance() {
        static xmemcheck_common_t singleton;
        return singleton;
    }

    void remove_trace(uint64_t pointer);
    void add_trace(uint64_t pointer, const std::string & tag, int obj_type);

 private:
    void print_leak_pointers(uint64_t now);
    void print_trace(const xmemcheck_common_info_t & trace_info);

 private:
    int                                         m_obj_type{10000};
    std::map<uint64_t, xmemcheck_common_info_t>  m_alloc_pointers;
    uint64_t                                    m_last_print_timestamp{0};
    std::mutex                                  m_lock;
    static constexpr uint64_t                   m_check_interval{60*60};  // print every check interval
    static constexpr uint64_t                   m_leak_time_s{2*60*60};  // print all block over leak time
};

class xmemcheck_dbgplugin_t : public base::xdbgplugin_t {
 public:
    xmemcheck_dbgplugin_t() = default;
 protected:
    virtual ~xmemcheck_dbgplugin_t() {}
 private:
    xmemcheck_dbgplugin_t(const xmemcheck_dbgplugin_t &);
    xmemcheck_dbgplugin_t & operator = (const xmemcheck_dbgplugin_t &);
 public:  // subclass need override
    static  void       xmemcheck_dbg_init();
    int                obj_type_to_mgr_type(int obj_type) {return obj_type + 255;}
    virtual bool       on_object_create(base::xobject_t* target) override;
    virtual bool       on_object_destroy(xobject_t* target) override;
    virtual bool       on_object_addref(xobject_t* target) override;
    virtual bool       on_object_releaseref(xobject_t* target) override;
 private:
    static constexpr int m_obj_type_max{4096};  // enum_xobject_type_max enum_xobject_type_min
    xmemcheck_common_t  m_all_type_mgr[m_obj_type_max];
};



# define MEMCHECK_INIT()                            data::xmemcheck_dbgplugin_t::xmemcheck_dbg_init()
# define MEMCHECK_REMOVE_TRACE(_pointer_)           //xmemcheck_common_t::get_instance().remove_trace((uint64_t)_pointer_)
# define MEMCHECK_ADD_TRACE(_pointer_, _tag)        //xmemcheck_common_t::get_instance().add_trace((uint64_t)_pointer_, _tag)
#else
# define MEMCHECK_INIT()
# define MEMCHECK_REMOVE_TRACE(_pointer_)
# define MEMCHECK_ADD_TRACE(_pointer_, _tag)
#endif  // DEBUG

NS_END2
