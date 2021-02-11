#pragma once

#include <string>
#include <memory>

#include "xpbase/base/top_utils.h"

namespace top {

namespace base {

struct XID;

class XIDParser {
public:
    XIDParser();
    ~XIDParser();
    bool ParserFromString(
        const std::string& stream);
    std::shared_ptr<XID> GetXID() const;
private:
    DISALLOW_COPY_AND_ASSIGN(XIDParser);
    std::shared_ptr<XID> sptr_xid_;
};

}  // namespace base

}  // namespace top