#include "xdata/xserial_transfrom.h"
#include "xdata/xtransaction.h"

bool isZeroSignature(u256 const & _r, u256 const & _s) {
    return !_r && !_s;
}

bool SignatureStruct::isValid() const noexcept {
    static const h256 s_max{"0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141"};
    static const h256 s_zero;

    return (v <= 1 && r > s_zero && s > s_zero && r < s_max && s < s_max);
}
namespace top {
namespace data {
int serial_transfrom::eth_to_top(string strEth, string & strTop) {
    top::base::xstream_t stream(top::base::xcontext_t::instance());
    stream.write_compact_var(strEth);
    strTop.append((char *)stream.data(), stream.size());
    return 0;
}

int serial_transfrom::top_to_eth(string strTop, string & strEth) {
    top::base::xstream_t stream(top::base::xcontext_t::instance(), (uint8_t *)strTop.data(), strTop.size());
    stream.read_compact_var(strEth);
    xdbg("serial_transfrom::top_to_eth Eth:%s", top::base::xstring_utl::to_hex(strEth).c_str());
    return 0;
}

}  // namespace data
}  // namespace top
