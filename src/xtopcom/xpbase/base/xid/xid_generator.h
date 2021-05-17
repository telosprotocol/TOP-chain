#pragma once

#pragma once

#include <map>
#include <mutex>
#include <memory>
#include <string>

#include "xpbase/base/top_utils.h"
#include "xpbase/base/xid/xid_def.h"

namespace top {

namespace base {

class XIDGenerator {
public:
    XIDGenerator();
    ~XIDGenerator();
    bool CreateXID(
        const XIDType& xid_type);
    bool GetXID(
        XID& xid) const;
    bool GetXIDFromDB(
        const XIDType& xid_type, 
        XID& xid) const;
    void Reset();
    bool DeleteXID(
        const XIDType& xid_type);
    bool SaveXID();
private:
    bool CreatePrivateAndPublicKey(
        std::string& private_key,
        std::string& public_key);
    DISALLOW_COPY_AND_ASSIGN(XIDGenerator);

    XIDType type_;
    XID xid_;
    mutable std::mutex mutex_;
};

typedef std::shared_ptr<XIDGenerator>  XIDGeneratorSptr;

}  // namespace base

}  // namespace top
