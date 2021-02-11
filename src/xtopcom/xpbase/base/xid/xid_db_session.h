#pragma once

#include <string>

#include "xpbase/base/xid/xid_def.h"

namespace top {

namespace base {

struct XIdDBSession {
    static bool Insert(
        const std::string& xid_key,
        const XID& xid);
    static bool Select(
        const std::string& xid_key,
        XIDSptr& xid);
    static bool Delete(
        const std::string& xid_key);
};

}  // namespace base

}  // namespace top
