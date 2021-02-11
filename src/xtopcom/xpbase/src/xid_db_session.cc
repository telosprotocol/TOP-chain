#include "xpbase/base/xid/xid_db_session.h"

#include "xpbase/base/xid/xid_def.h"
#include "xpbase/base/xid/xid_parser.h"

namespace top {

namespace base {

bool XIdDBSession::Insert(
        const std::string& xid_key,
        const XID& xid) {
    return true;
//     std::string field("0");
//     std::string xid_value = xid.ToString();
//     TOP_INFO("Insert xid_value:%s!", HexEncode(xid_value).c_str());
//     if (kadmlia::kKadSuccess != 
//         storage::XLedgerDB::Instance()->map_set(xid_key, field, xid_value)) {
//         return false;
//     }
//    return true;
}

bool XIdDBSession::Select(
        const std::string& xid_key,
        XIDSptr& xid) {
    return false;
//     std::string field("0");
//     std::string xid_value ;
//     if (kadmlia::kKadSuccess != 
//         storage::XLedgerDB::Instance()->map_get(xid_key, field, xid_value)) {
//         return false;
//     }
//     XIDParser parser;
//     parser.ParserFromString(xid_value);
//     xid = parser.GetXID();
//    return true;
}


bool XIdDBSession::Delete(
        const std::string& xid_key) {
    return true;
//     if (kadmlia::kKadSuccess != storage::XLedgerDB::Instance()->remove(xid_key)) {
//         return false;
//     }
//    return true;
}

}  // namespace base

}  // namespace top
