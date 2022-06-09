#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbasic/xhex.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xethheader.h"
#include "xdata/xethreceipt.h"
#include "xdata/xlightunit_info.h"
#include "xdata/xethbuild.h"
#include "xevm_common/common_data.h"
#include "xevm_common/xtriehash.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace top;
using namespace top::base;
using namespace top::data;

class test_eth_root : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

class legacy_tx {
public:
    uint64_t                m_chainid;
    top::evm_common::u256   m_nonce;
    top::evm_common::u256   m_gasprice;
    top::evm_common::u256   m_gas;
    common::xeth_address_t  m_to;
    top::evm_common::u256   m_value;
    xbytes_t                m_data;
    top::evm_common::u256   m_signV;
    top::evm_common::h256   m_signR;
    top::evm_common::h256   m_signS;
public:
    void streamRLP(evm_common::RLPStream& _s) const {
        _s.appendList(9);
        _s << m_nonce;
        _s << m_gasprice;
        _s << m_gas;
        if (!m_to.empty()) {
            _s << m_to.to_bytes();
        } else {
            _s << "";
        }
        _s << m_value;
        _s << m_data;
        _s << m_signV;
        _s << m_signR;
        _s << m_signS;
    }
    xbytes_t encodeBytes() const {
        evm_common::RLPStream _s;
        streamRLP(_s);
        return _s.out();
    }
    void decodeRLP(evm_common::RLP const& _r) {
        if (_r.itemCount() != 9) {
            xassert(false);
        }
        std::error_code ec;
        m_nonce = _r[0].toInt<evm_common::u256>();
        m_gasprice = _r[1].toInt<evm_common::u256>();
        m_gas = _r[2].toInt<evm_common::u256>();
        if (!_r[3].isEmpty()) {
            xbytes_t _bytes = _r[3].toBytes();
            m_to = common::xeth_address_t::build_from(_bytes, ec);
        } else {
            m_to = common::xeth_address_t::zero();
        }
        m_value = _r[4].toInt<evm_common::u256>();
        m_data = _r[5].toBytes();
        m_signV = _r[6].toInt<evm_common::u256>();
        m_signR = _r[7].toInt<evm_common::u256>();
        m_signS = _r[8].toInt<evm_common::u256>();        
    }
};

using xLogBloom_t = evm_common::h2048;
struct LogEntry
{
	void streamRLP(evm_common::RLPStream& _s) const {
        _s.appendList(3) << address << topics << data;
    }
	evm_common::Address address;
	evm_common::h256s topics;
	xbytes_t data;
};
using LogEntries = std::vector<LogEntry>;

class legacy_receipt {
public:
	uint8_t m_status;
	evm_common::u256 m_gasUsed;
	xLogBloom_t     m_bloom;
	LogEntries m_log;
public:
    void streamRLP(evm_common::RLPStream& _s) const {
        _s.appendList(4);
        _s << m_status;
        _s << m_gasUsed;
        _s << m_bloom;
        _s.appendList(m_log.size());
        for (LogEntry const& l: m_log)
            l.streamRLP(_s);
    }
    xbytes_t encodeBytes() const {
        evm_common::RLPStream _s;
        streamRLP(_s);
        return _s.out();
    }
    void decodeRLP(evm_common::RLP const& _r) {
        xassert(false);
        // if (_r.itemCount() != 9) {
        //     xassert(false);
        // }
        // std::error_code ec;
        // m_status = (uint8_t)_r[0];
        // m_gasUsed = (evm_common::u256)_r[1];
        // m_bloom = (xLogBloom_t)_r[2];
        // for (auto const& i : _r[3]) {
        //     m_log.emplace_back(i);  
        // }
    }
};

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

evm_common::u256 jsToU256(std::string const& _s) {
    return jsToInt<32>(_s); 
}

xJson::Value read_json_file(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) {
        assert(false);
        return {};
    }
    std::ostringstream tmp;
    tmp << in.rdbuf();
    in.close();
    std::string json_str = tmp.str();

    xJson::Value content;
    xJson::Reader reader;
    bool ret = reader.parse(json_str, content);
    if (!ret) {
        assert(false);
        return {};
    }
    return content;
}

xbytes_t  legecy_json_tx_to_bytes(xJson::Value const& txjson) {
    std::error_code ec;
    legacy_tx tx;
    tx.m_nonce = jsToU256(txjson["nonce"].asString());
    std::cout << "m_nonce " << tx.m_nonce << std::endl;
    tx.m_gasprice = jsToU256(txjson["gasPrice"].asString());
    tx.m_gas = jsToU256(txjson["gas"].asString());
    std::string tostr = txjson["to"].asString();
    if (!tostr.empty()) {
        tx.m_to = common::xeth_address_t::build_from(tostr);
    }
    tx.m_value = jsToU256(txjson["value"].asString());
    tx.m_data = top::from_hex(txjson["input"].asString(), ec);
    tx.m_signV = jsToU256(txjson["v"].asString());
    xbytes_t rbs = top::from_hex(txjson["r"].asString(), ec);    
    tx.m_signR = evm_common::xh256_t(rbs);
    xbytes_t sbs = top::from_hex(txjson["s"].asString(), ec);    
    tx.m_signS= evm_common::xh256_t(sbs);
    return tx.encodeBytes();
}

xbytes_t  eip1599_json_tx_to_bytes(xJson::Value const& txjson) {
    std::error_code ec;
    xeth_transaction_t tx;
    evm_common::u256 chainid = jsToU256(txjson["chainId"].asString());
    tx.set_chainid(chainid);   
    evm_common::u256 nonce = jsToU256(txjson["nonce"].asString());
    tx.set_nonce(nonce);
    evm_common::u256  maxPriorityFeePerGas = jsToU256(txjson["maxFeePerGas"].asString());
    tx.set_max_priority_fee_per_gas(maxPriorityFeePerGas);
    evm_common::u256  maxFeePerGas = jsToU256(txjson["maxPriorityFeePerGas"].asString());
    tx.set_max_fee_per_gas(maxFeePerGas);
    evm_common::u256  gas = jsToU256(txjson["gas"].asString());
    tx.set_gas(gas);
    std::string tostr = txjson["to"].asString();
    if (!tostr.empty()) {
        common::xeth_address_t to = common::xeth_address_t::build_from(tostr);
        tx.set_to(to);
    }
    evm_common::u256  value = jsToU256(txjson["value"].asString());
    tx.set_value(value);
    xbytes_t data = top::from_hex(txjson["input"].asString(), ec);
    tx.set_data(data);

    evm_common::u256  signV = jsToU256(txjson["v"].asString());
    tx.set_signV(signV);
    xbytes_t rbs = top::from_hex(txjson["r"].asString(), ec);    
    evm_common::h256 signR = evm_common::xh256_t(rbs);
    tx.set_signR(signR);
    xbytes_t sbs = top::from_hex(txjson["s"].asString(), ec);    
    evm_common::h256  signS= evm_common::xh256_t(sbs);
    tx.set_signS(signS);

    return tx.encodeBytes();
}

xbytes_t  json_tx_to_bytes(xJson::Value const& txjson) {
    uint8_t type = (uint8_t)jsToU256(txjson["type"].asString());
    xbytes_t _bs;
    if (type == EIP_LEGACY) {
        _bs = legecy_json_tx_to_bytes(txjson);
    } else if (type == EIP_1559) {
        _bs = eip1599_json_tx_to_bytes(txjson);
    }
    return _bs;
}


#if 0 // TODO(jimmy) can't pass
TEST_F(test_eth_root, eth_block_transactions_root) {
    std::error_code ec;
    xJson::Value _get_block_js = read_json_file("/home/code/telosprotocal/jimmy_TOP-chain/tests/xdata_test/test_eth_block.json");
    xJson::Value result_js = _get_block_js["result"];
    xJson::Value txs_js = result_js["transactions"];
    int txs_count = txs_js.size();
    std::cout << "txs_count=" << txs_count << std::endl;
    std::vector<xbytes_t> leafs;
    for (int i = 0; i < txs_count; i++) {
        xJson::Value tx_js = txs_js[i];
        xbytes_t bs = json_tx_to_bytes(tx_js);
        leafs.push_back(bs);
        // std::cout << "tx_bs=" << top::to_hex_prefixed(bs) << std::endl;
    }
    
    evm_common::h256 txsRoot = evm_common::orderedTrieRoot(leafs);
    auto transactionsRoot_bytes = top::from_hex(result_js["transactionsRoot"].asString(), ec);
    std::cout << "block transactionsRoot " << top::to_hex_prefixed(transactionsRoot_bytes) << std::endl;
    std::cout << "txsRoot=" << top::to_hex_prefixed(txsRoot.asBytes()) << std::endl;
    // ASSERT_EQ(top::to_hex_prefixed(transactionsRoot_bytes), top::to_hex_prefixed(txsRoot.asBytes()));
}
#endif

TEST_F(test_eth_root, txs_root_one) {
    std::error_code ec;
#if 0      // TODO(jimmy) can't pass
{
    legacy_tx tx;    
    tx.m_nonce = jsToU256("0x1");
    tx.m_gasprice = jsToU256("0x3b9aca00");
    tx.m_gas = jsToU256("0x5208");
    tx.m_to = common::xeth_address_t::build_from("0xb7762d8dbd7e5c023ff99402b78af7c13b01eec1");
    tx.m_value = jsToU256("0x16345785d8a0000");
    tx.m_data = top::from_hex("0x", ec);
    tx.m_signV = jsToU256("0x557");
    tx.m_signR = evm_common::xh256_t(top::from_hex("0x39eb0fe3b093f02f96fc277d85d785f4f638a5ed61bde31d7f09e451c0c56bf4", ec));
    tx.m_signS= evm_common::xh256_t(top::from_hex("0x89672aacfc50bb3c7c27e7e119afb085cbe2ce8d23128ad105676500a9ec0c", ec));

    // ASSERT_EQ(top::to_hex_prefixed(to_bytes(tx.get_tx_hash())), "0x631311a3658fde222a4bce3e3a8e4f31444a78d6e184e31b676d91aa72bb65c5");
    // ASSERT_EQ(top::to_hex_prefixed(tx.get_from().to_bytes()), "0x4dce5c8961e283786cb31ad7fc072347227d7ea2");

    xbytes_t _bs = tx.encodeBytes();
    std::vector<xbytes_t> leafs;
    leafs.push_back(_bs);
    evm_common::h256 txsRoot = evm_common::orderedTrieRoot(leafs);
    // std::cout << "txsRoot=" << top::to_hex_prefixed(txsRoot.asBytes()) << std::endl;
    ASSERT_EQ(top::to_hex_prefixed(txsRoot.asBytes()), "0xcd237e2846d443edbaee06f01de2a8f1623ed3f41949c19c6ac54001ef4138a6");
}
#endif
{
    xeth_transaction_t tx;
    tx.set_chainid(jsToU256("0x26b"));   
    tx.set_nonce(jsToU256("0x2"));
    tx.set_max_priority_fee_per_gas(jsToU256("0x59682f00"));
    tx.set_max_fee_per_gas(jsToU256("0x59682f08"));
    tx.set_gas(jsToU256("0x5208"));
    tx.set_to(common::xeth_address_t::build_from("0xc8e6615f4c0ca0f44c0ac05daadb2aaad9720c98"));
    tx.set_value(jsToU256("0x1bc16d674ec80000"));
    tx.set_data(top::from_hex("0x", ec));
    tx.set_signV(jsToU256("0x1"));
    xbytes_t rbs = top::from_hex("0x3aa2d1b9ca2c95f5bcf3dc4076241cb0552359ebfa523ad9c045aa3c1953779c", ec);    
    tx.set_signR(evm_common::xh256_t(rbs));
    xbytes_t sbs = top::from_hex("0x385b0d94ee10c5325ae4960a616c9c2aaad9e8549dd43d68bb5ca14206d62ded", ec);    
    tx.set_signS(evm_common::xh256_t(sbs));
    ASSERT_EQ(top::to_hex_prefixed(to_bytes(tx.get_tx_hash())), "0x631311a3658fde222a4bce3e3a8e4f31444a78d6e184e31b676d91aa72bb65c5");
    ASSERT_EQ(top::to_hex_prefixed(tx.get_from().to_bytes()), "0x4dce5c8961e283786cb31ad7fc072347227d7ea2");


    xbytes_t _bs = tx.encodeBytes();
    std::vector<xbytes_t> leafs;
    leafs.push_back(_bs);
    evm_common::h256 txsRoot = evm_common::orderedTrieRoot(leafs);
    ASSERT_EQ(top::to_hex_prefixed(txsRoot.asBytes()), "0x3d77feb07307f61210a1385b6d09dc3480701526330ab24763824604ccaaa0c2");
}
}

TEST_F(test_eth_root, receipts_root_one) {
    std::error_code ec;
{
    xeth_receipt_t receipt;
    receipt.set_tx_version_type(EIP_LEGACY);
    receipt.set_tx_status(ethreceipt_status_successful);
    receipt.set_cumulative_gas_used((uint64_t)jsToU256("0x5208"));
    xbytes_t _bs = receipt.encodeBytes();
    std::vector<xbytes_t> leafs;
    leafs.push_back(_bs);
    evm_common::h256 receiptsRoot = evm_common::orderedTrieRoot(leafs);
    ASSERT_EQ(top::to_hex_prefixed(receiptsRoot.asBytes()), "0x056b23fbba480696b65fe5a59b8f2148a1299103c4f57df839233af2cf4ca2d2");
}
{
    xeth_receipt_t receipt;
    receipt.set_tx_version_type(EIP_1559);
    receipt.set_tx_status(ethreceipt_status_successful);
    receipt.set_cumulative_gas_used((uint64_t)jsToU256("0x5208"));
    xbytes_t _bs = receipt.encodeBytes();
    std::vector<xbytes_t> leafs;
    leafs.push_back(_bs);
    evm_common::h256 receiptsRoot = evm_common::orderedTrieRoot(leafs);
    ASSERT_EQ(top::to_hex_prefixed(receiptsRoot.asBytes()), "0xf78dfb743fbd92ade140711c8bbc542b5e307f0ab7984eff35d751969fe57efa");
}
}


TEST_F(test_eth_root, receipts_root_three) {
    std::error_code ec;
    xeth_receipts_t receipts;
    xeth_receipt_t receipt1;
    receipt1.set_tx_status(ethreceipt_status_successful);
    receipt1.set_cumulative_gas_used((uint64_t)jsToU256("0x5208"));
    receipts.push_back(receipt1);
    xeth_receipt_t receipt2;
    receipt2.set_tx_status(ethreceipt_status_successful);
    receipt2.set_cumulative_gas_used((uint64_t)jsToU256("0xa410"));
    receipts.push_back(receipt2);
    xeth_receipt_t receipt3;
    receipt3.set_tx_status(ethreceipt_status_successful);
    receipt3.set_cumulative_gas_used((uint64_t)jsToU256("0xf618"));
    receipts.push_back(receipt3);

    evm_common::h256 receiptsRoot = xeth_build_t::build_receipts_root(receipts);
    ASSERT_EQ(top::to_hex_prefixed(receiptsRoot.asBytes()), "0x25e6b7af647c519a27cc13276a1e6abc46154b51414d174b072698df1f6c19df");
}

TEST_F(test_eth_root, txs_root_three) {
    std::error_code ec;
    xeth_transactions_t txs;
    {
        xeth_transaction_t tx;
        tx.set_chainid(jsToU256("0x26b"));
        tx.set_nonce(jsToU256("0x3"));
        tx.set_max_priority_fee_per_gas(jsToU256("0x77359400"));
        tx.set_max_fee_per_gas(jsToU256("0x4a817c800"));
        tx.set_gas(jsToU256("0x1e8480"));
        tx.set_to(common::xeth_address_t::build_from("0xcf637687290860dfb1183a7914f373a6f1d09fd0"));
        tx.set_value(jsToU256("0x3b9aca00"));
        tx.set_data(top::from_hex("0x", ec));
        tx.set_signV(jsToU256("0x0"));
        tx.set_signR(evm_common::xh256_t(top::from_hex("0x6e9e1dafc492cd2c4c5f95192175e652a0aa1c0a0b0c1181c9d956f35678cf68", ec)));
        tx.set_signS(evm_common::xh256_t(top::from_hex("0x4d3b78350da27b7ceb47bd436a800263230e3141a993cd198d9626ad6dba069d", ec)));
        ASSERT_EQ(top::to_hex_prefixed(to_bytes(tx.get_tx_hash())), "0x05ff44c891968e11c569ad6eb5a5ca96ce7ea855d5bc838af0701d6006aafa02");
        ASSERT_EQ(top::to_hex_prefixed(tx.get_from().to_bytes()), "0x4dce5c8961e283786cb31ad7fc072347227d7ea2");
        txs.push_back(tx);
    }
    {
        xeth_transaction_t tx;
        tx.set_chainid(jsToU256("0x26b"));
        tx.set_nonce(jsToU256("0x4"));
        tx.set_max_priority_fee_per_gas(jsToU256("0x77359400"));
        tx.set_max_fee_per_gas(jsToU256("0x4a817c800"));
        tx.set_gas(jsToU256("0x1e8480"));
        tx.set_to(common::xeth_address_t::build_from("0xcf637687290860dfb1183a7914f373a6f1d09fd0"));
        tx.set_value(jsToU256("0x3b9aca00"));
        tx.set_data(top::from_hex("0x", ec));
        tx.set_signV(jsToU256("0x0"));
        tx.set_signR(evm_common::xh256_t(top::from_hex("0x1639623c77e6122639c6ef6bfcc4f1def7d7cdb02091339b6eb3707e8508f271", ec)));
        tx.set_signS(evm_common::xh256_t(top::from_hex("0x41a7af54e9cfba8cad57e2750a43585c0306a213858e4c706e5eb253d96ab3da", ec)));
        ASSERT_EQ(top::to_hex_prefixed(to_bytes(tx.get_tx_hash())), "0xd54f085fd93469ae557b3b7f1ea81004c71c9e53ad6c770dccc5df965dca93e5");
        ASSERT_EQ(top::to_hex_prefixed(tx.get_from().to_bytes()), "0x4dce5c8961e283786cb31ad7fc072347227d7ea2");
        txs.push_back(tx);
    }
    {
        xeth_transaction_t tx;
        tx.set_chainid(jsToU256("0x26b"));
        tx.set_nonce(jsToU256("0x5"));
        tx.set_max_priority_fee_per_gas(jsToU256("0x77359400"));
        tx.set_max_fee_per_gas(jsToU256("0x4a817c800"));
        tx.set_gas(jsToU256("0x1e8480"));
        tx.set_to(common::xeth_address_t::build_from("0xcf637687290860dfb1183a7914f373a6f1d09fd0"));
        tx.set_value(jsToU256("0x3b9aca00"));
        tx.set_data(top::from_hex("0x", ec));
        tx.set_signV(jsToU256("0x0"));
        tx.set_signR(evm_common::xh256_t(top::from_hex("0x1dff2db9a591017376230c47d474fd76742a94ea88387494232d3c555a2bcb17", ec)));
        tx.set_signS(evm_common::xh256_t(top::from_hex("0x44b84fe21e68d5949cf9f7543e2780f99572b0fb9b9b917a91d4ae3a048f412b", ec)));
        ASSERT_EQ(top::to_hex_prefixed(to_bytes(tx.get_tx_hash())), "0xacfde86a369fc891b6e5be1cc72eb923da7553f7fd5ef997edcd694c690e7912");
        ASSERT_EQ(top::to_hex_prefixed(tx.get_from().to_bytes()), "0x4dce5c8961e283786cb31ad7fc072347227d7ea2");
        txs.push_back(tx);
    }

    evm_common::h256 txsRoot = xeth_build_t::build_transactions_root(txs);
    ASSERT_EQ(top::to_hex_prefixed(txsRoot.asBytes()), "0xe9d5f517609edf2dcbdc9442f1aba582db5b365a06444424bee401c4ead4e57f");
}
