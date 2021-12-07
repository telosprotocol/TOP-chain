#pragma once

#include "xbase/xlog.h"

NS_BEG2(top, sync)

#undef __MODULE__
#define __MODULE__  "xsync"


#ifdef SYNC_TEST
#define xsync_dbg(fmt, ...) xdbg("vnode_id(%s) " fmt, m_vnode_id.c_str(), ## __VA_ARGS__)
#define xsync_info(fmt, ...) xinfo("vnode_id(%s) " fmt, m_vnode_id.c_str(), ## __VA_ARGS__)
#define xsync_warn(fmt, ...) xwarn("vnode_id(%s) " fmt, m_vnode_id.c_str(), ## __VA_ARGS__)
#define xsync_kinfo(fmt, ...) xkinfo("vnode_id(%s) " fmt, m_vnode_id.c_str(), ## __VA_ARGS__)
#define xsync_error(fmt, ...) xerror("vnode_id(%s) " fmt, m_vnode_id.c_str(), ## __VA_ARGS__)

#else
#define xsync_dbg xdbg
#define xsync_info xinfo
#define xsync_warn xwarn
#define xsync_kinfo xkinfo
#define xsync_error xerror
#endif

NS_END2
