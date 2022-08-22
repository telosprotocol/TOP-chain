#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbasic/xhex.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xethheader.h"
#include "xdata/xethreceipt.h"
#include "xdata/xlightunit_info.h"
#include "xevm_common/common_data.h"

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

    static xeth_transaction_t create_test_eth() {
        xeth_transaction_t tx;
        std::error_code ec;
        tx.set_tx_version(EIP_1559);
        tx.set_chainid(jsToU256("0x26b"));   
        tx.set_nonce(jsToU256("0x2"));
        tx.set_max_priority_fee_per_gas(jsToU256("0x59682f00"));
        tx.set_max_fee_per_gas(jsToU256("0x59682f08"));
        tx.set_gas(jsToU256("0x5208"));
        tx.set_to(common::xeth_address_t::build_from("0xc8e6615f4c0ca0f44c0ac05daadb2aaad9720c98"));
        tx.set_value(jsToU256("0x1bc16d674ec80000"));
        tx.set_data(top::from_hex("0x", ec));
        tx.set_signV(jsToU256("0x1"));
        tx.set_signR(evm_common::xh256_t(top::from_hex("0x3aa2d1b9ca2c95f5bcf3dc4076241cb0552359ebfa523ad9c045aa3c1953779c", ec)));
        tx.set_signS(evm_common::xh256_t(top::from_hex("0x385b0d94ee10c5325ae4960a616c9c2aaad9e8549dd43d68bb5ca14206d62ded", ec)));    
        return tx;
    }

};

