#include "xbase/xutl.h"
#include "xdata/xserial_transfrom.h"
#include "xdata/xtransaction.h"

using namespace std;
using namespace top::evm_common;

namespace top {
namespace data {
int serial_transfrom::eth_to_top(string strEth, uint8_t nEipVersion, string & strTop) {
    top::base::xstream_t stream(top::base::xcontext_t::instance());
    stream.write_compact_var(nEipVersion);
    stream.write_compact_var(strEth);
    strTop.append((char *)stream.data(), stream.size());
    return 0;
}

int serial_transfrom::top_to_eth(string strTop, uint8_t & nEipVersion, string & strEth) {
    top::base::xstream_t stream(top::base::xcontext_t::instance(), (uint8_t *)strTop.data(), strTop.size());
    stream.read_compact_var(nEipVersion);
    stream.read_compact_var(strEth);
    xdbg("serial_transfrom::top_to_eth Eth:%s", top::base::xstring_utl::to_hex(strEth).c_str());
    return 0;
}

}  // namespace data
}  // namespace top
