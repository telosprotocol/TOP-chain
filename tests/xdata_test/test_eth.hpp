#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbasic/xhex.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xethheader.h"
#include "xdata/xethreceipt.h"
#include "xdata/xlightunit_info.h"
#include "xcommon/common_data.h"

using namespace top;
using namespace top::base;
using namespace top::data;

template <unsigned N> boost::multiprecision::number<boost::multiprecision::cpp_int_backend<N * 8, N * 8, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>> jsToInt(std::string const& _s)
{
    if (_s.substr(0, 2) == "0x")
        // Hex
        return evm_common::fromBigEndian<boost::multiprecision::number<boost::multiprecision::cpp_int_backend<N * 8, N * 8, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>>(evm_common::fromHex(_s.substr(2)));
    else if (_s.find_first_not_of("0123456789") == std::string::npos)
        // Decimal
        return boost::multiprecision::number<boost::multiprecision::cpp_int_backend<N * 8, N * 8, boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>(_s);
    else
        // Binary
        return 0;			// FAIL
}

class test_ethtx_tool {
 public:
    static evm_common::u256 jsToU256(std::string const& _s) {
        return jsToInt<32>(_s); 
    }

    static xbyte_t jsToByte(std::string const & _s) {
        return jsToInt<1>(_s).convert_to<xbyte_t>();
    }

    static xeth_transaction_t create_test_eth() {
        std::error_code ec;
        auto chainid = jsToU256("0x26b");
        auto nonce = jsToU256("0x2");
        auto max_priority_fee_per_gas = jsToU256("0x59682f00");
        auto max_fee_per_gas = jsToU256("0x59682f08");
        auto gas = jsToU256("0x5208");
        auto to = common::xeth_address_t::build_from("0xc8e6615f4c0ca0f44c0ac05daadb2aaad9720c98");
        auto value = jsToU256("0x1bc16d674ec80000");
        auto data = top::from_hex("0x", ec);
        auto signV = jsToByte("0x1");
        auto signR = xh256_t(top::from_hex("0x3aa2d1b9ca2c95f5bcf3dc4076241cb0552359ebfa523ad9c045aa3c1953779c", ec));
        auto signS = xh256_t(top::from_hex("0x385b0d94ee10c5325ae4960a616c9c2aaad9e8549dd43d68bb5ca14206d62ded", ec));

        xeth_transaction_t tx = xeth_transaction_t::build_eip1559_tx(chainid, nonce, max_priority_fee_per_gas, max_fee_per_gas, gas, to, value, data);
        tx.set_sign(signR, signS, signV);
        return tx;
    }

};

