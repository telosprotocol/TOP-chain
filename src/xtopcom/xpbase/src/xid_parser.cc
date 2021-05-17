#include "xpbase/base/xid/xid_parser.h"

#include <stdint.h>
#include <string.h>

#include "xpbase/base/xid/xid_def.h"
#include "xpbase/base/top_log.h"

namespace top {

namespace base {

bool XIDParser::ParserFromString(
        const std::string& stream) {
    std::string str = stream;
    uint32_t high;
    memcpy(&high, str.c_str(), sizeof(high));
    sptr_xid_->xnetwork_id_ = high >> 8;
    sptr_xid_->zone_id_ = high & 0xFF;
    uint32_t public_key_size = str.size() - sizeof(high);
    TOP_DEBUG("public_key_size:%d", public_key_size);
    sptr_xid_->public_key_.resize(public_key_size);
    memcpy(
        (char*)sptr_xid_->public_key_.c_str(), 
        str.c_str() + sizeof(high), 
        public_key_size);
    return true;
}

std::shared_ptr<XID> XIDParser::GetXID() const {
    return sptr_xid_;
}

XIDParser::XIDParser()
    :sptr_xid_(std::make_shared<XID>()) {}

XIDParser::~XIDParser() {}

}  // namespace base

}  // namespace top
