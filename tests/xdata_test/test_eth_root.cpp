#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbasic/xhex.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xethheader.h"
#include "xdata/xethreceipt.h"
#include "xdata/xlightunit_info.h"
#include "xdata/xethbuild.h"
#include "xcommon/common_data.h"
#include "xevm_common/xtriehash.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "tests/xdata_test/test_eth.hpp"

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
        if (!m_to.is_zero()) {
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

Json::Value read_json_file(const std::string& filepath) {
    std::ifstream in(filepath);
    if (!in.is_open()) {
        assert(false);
        return {};
    }
    std::ostringstream tmp;
    tmp << in.rdbuf();
    in.close();
    std::string json_str = tmp.str();

    Json::Value content;
    Json::Reader reader;
    bool ret = reader.parse(json_str, content);
    if (!ret) {
        assert(false);
        return {};
    }
    return content;
}

xbytes_t  legecy_json_tx_to_bytes(Json::Value const& txjson) {
    std::error_code ec;
    legacy_tx tx;
    tx.m_nonce = test_ethtx_tool::jsToU256(txjson["nonce"].asString());
    std::cout << "m_nonce " << tx.m_nonce << std::endl;
    tx.m_gasprice = test_ethtx_tool::jsToU256(txjson["gasPrice"].asString());
    tx.m_gas = test_ethtx_tool::jsToU256(txjson["gas"].asString());
    std::string tostr = txjson["to"].asString();
    if (!tostr.empty()) {
        tx.m_to = common::xeth_address_t::build_from(tostr);
    }
    tx.m_value = test_ethtx_tool::jsToU256(txjson["value"].asString());
    tx.m_data = top::from_hex(txjson["input"].asString(), ec);
    tx.m_signV = test_ethtx_tool::jsToU256(txjson["v"].asString());
    xbytes_t rbs = top::from_hex(txjson["r"].asString(), ec);    
    tx.m_signR = xh256_t(rbs);
    xbytes_t sbs = top::from_hex(txjson["s"].asString(), ec);    
    tx.m_signS= xh256_t(sbs);
    return tx.encodeBytes();
}

xbytes_t  eip1599_json_tx_to_bytes(Json::Value const& txjson) {
    std::error_code ec;    
    uint64_t txversion = (uint64_t)test_ethtx_tool::jsToU256(txjson["type"].asString());
    assert((enum_ethtx_version)txversion == enum_ethtx_version::EIP_1559);
    evm_common::u256 chainid = test_ethtx_tool::jsToU256(txjson["chainId"].asString());
    evm_common::u256 nonce = test_ethtx_tool::jsToU256(txjson["nonce"].asString());
    evm_common::u256  maxPriorityFeePerGas = test_ethtx_tool::jsToU256(txjson["maxFeePerGas"].asString());
    evm_common::u256  maxFeePerGas = test_ethtx_tool::jsToU256(txjson["maxPriorityFeePerGas"].asString());
    evm_common::u256  gas = test_ethtx_tool::jsToU256(txjson["gas"].asString());
    evm_common::u256  value = test_ethtx_tool::jsToU256(txjson["value"].asString());
    xbytes_t data = top::from_hex(txjson["input"].asString(), ec);
    xbyte_t  signV = test_ethtx_tool::jsToByte(txjson["v"].asString());
    xbytes_t rbs = top::from_hex(txjson["r"].asString(), ec);
    evm_common::h256 signR = xh256_t(rbs);
    xbytes_t sbs = top::from_hex(txjson["s"].asString(), ec);
    evm_common::h256  signS= xh256_t(sbs);

    std::string tostr = txjson["to"].asString();
    if (!tostr.empty()) {
        common::xeth_address_t to = common::xeth_address_t::build_from(tostr);
        xeth_transaction_t tx = xeth_transaction_t::build_eip1559_tx(chainid, nonce, maxPriorityFeePerGas, maxFeePerGas, gas, to, value, data);
        tx.set_sign(signR, signS, signV);
        return tx.encodeBytes();
    } else {
        xeth_transaction_t tx = xeth_transaction_t::build_eip1559_tx(chainid, nonce, maxPriorityFeePerGas, maxFeePerGas, gas, value, data);
        tx.set_sign(signR, signS, signV);
        return tx.encodeBytes();
    }
}

xbytes_t  json_tx_to_bytes(Json::Value const& txjson) {
    uint8_t type = (uint8_t)test_ethtx_tool::jsToU256(txjson["type"].asString());
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
    Json::Value _get_block_js = read_json_file("/home/code/telosprotocal/jimmy_TOP-chain/tests/xdata_test/test_eth_block.json");
    Json::Value result_js = _get_block_js["result"];
    Json::Value txs_js = result_js["transactions"];
    int txs_count = txs_js.size();
    std::cout << "txs_count=" << txs_count << std::endl;
    std::vector<xbytes_t> leafs;
    for (int i = 0; i < txs_count; i++) {
        Json::Value tx_js = txs_js[i];
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
    tx.m_nonce = test_ethtx_tool::jsToU256("0x1");
    tx.m_gasprice = test_ethtx_tool::jsToU256("0x3b9aca00");
    tx.m_gas = test_ethtx_tool::jsToU256("0x5208");
    tx.m_to = common::xeth_address_t::build_from("0xb7762d8dbd7e5c023ff99402b78af7c13b01eec1");
    tx.m_value = test_ethtx_tool::jsToU256("0x16345785d8a0000");
    tx.m_data = top::from_hex("0x", ec);
    tx.m_signV = test_ethtx_tool::jsToU256("0x557");
    tx.m_signR = xh256_t(top::from_hex("0x39eb0fe3b093f02f96fc277d85d785f4f638a5ed61bde31d7f09e451c0c56bf4", ec));
    tx.m_signS= xh256_t(top::from_hex("0x89672aacfc50bb3c7c27e7e119afb085cbe2ce8d23128ad105676500a9ec0c", ec));

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
    evm_common::u256 chainid = test_ethtx_tool::jsToU256("0x26b");
    evm_common::u256 nonce = test_ethtx_tool::jsToU256("0x2");
    evm_common::u256 max_priority_fee_per_gas = test_ethtx_tool::jsToU256("0x59682f00");
    evm_common::u256 max_fee_per_gas = test_ethtx_tool::jsToU256("0x59682f08");
    evm_common::u256 gaslimit = test_ethtx_tool::jsToU256("0x5208");
    common::xeth_address_t to = common::xeth_address_t::build_from("0xc8e6615f4c0ca0f44c0ac05daadb2aaad9720c98");
    evm_common::u256 value = test_ethtx_tool::jsToU256("0x1bc16d674ec80000");
    xbytes_t data = top::from_hex("0x", ec);
    xbyte_t v = test_ethtx_tool::jsToByte("0x1");
    xh256_t r = xh256_t(top::from_hex("0x3aa2d1b9ca2c95f5bcf3dc4076241cb0552359ebfa523ad9c045aa3c1953779c", ec));
    xh256_t s = xh256_t(top::from_hex("0x385b0d94ee10c5325ae4960a616c9c2aaad9e8549dd43d68bb5ca14206d62ded", ec));

    xeth_transaction_t tx = xeth_transaction_t::build_eip1559_tx(chainid, nonce, max_priority_fee_per_gas, max_fee_per_gas, gaslimit, to, value, data);
    tx.set_sign(r, s, v);
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
    receipt.set_cumulative_gas_used((uint64_t)test_ethtx_tool::jsToU256("0x5208"));
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
    receipt.set_cumulative_gas_used((uint64_t)test_ethtx_tool::jsToU256("0x5208"));
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
    receipt1.set_cumulative_gas_used((uint64_t)test_ethtx_tool::jsToU256("0x5208"));
    receipts.push_back(receipt1);
    xeth_receipt_t receipt2;
    receipt2.set_tx_status(ethreceipt_status_successful);
    receipt2.set_cumulative_gas_used((uint64_t)test_ethtx_tool::jsToU256("0xa410"));
    receipts.push_back(receipt2);
    xeth_receipt_t receipt3;
    receipt3.set_tx_status(ethreceipt_status_successful);
    receipt3.set_cumulative_gas_used((uint64_t)test_ethtx_tool::jsToU256("0xf618"));
    receipts.push_back(receipt3);

    evm_common::h256 receiptsRoot = xeth_build_t::build_receipts_root(receipts);
    ASSERT_EQ(top::to_hex_prefixed(receiptsRoot.asBytes()), "0x25e6b7af647c519a27cc13276a1e6abc46154b51414d174b072698df1f6c19df");
}

TEST_F(test_eth_root, transfer_three_tx) {
    std::error_code ec;
    xeth_transactions_t txs;
    xeth_receipts_t receipts;
    {
        auto chainid = test_ethtx_tool::jsToU256("0x26b");
        auto nonce = test_ethtx_tool::jsToU256("0x3");
        auto max_priority_fee_per_gas = test_ethtx_tool::jsToU256("0x77359400");
        auto max_fee_per_gas = test_ethtx_tool::jsToU256("0x4a817c800");
        auto gaslimit = test_ethtx_tool::jsToU256("0x1e8480");
        auto to = common::xeth_address_t::build_from("0xcf637687290860dfb1183a7914f373a6f1d09fd0");
        auto value = test_ethtx_tool::jsToU256("0x3b9aca00");
        auto data = top::from_hex("0x", ec);
        auto signV = test_ethtx_tool::jsToByte("0x0");
        auto signR = xh256_t(top::from_hex("0x6e9e1dafc492cd2c4c5f95192175e652a0aa1c0a0b0c1181c9d956f35678cf68", ec));
        auto signS = xh256_t(top::from_hex("0x4d3b78350da27b7ceb47bd436a800263230e3141a993cd198d9626ad6dba069d", ec));

        xeth_transaction_t tx = xeth_transaction_t::build_eip1559_tx(chainid, nonce, max_priority_fee_per_gas, max_fee_per_gas, gaslimit, to, value, data);
        tx.set_sign(signR, signS, signV);

        ASSERT_EQ(top::to_hex_prefixed(to_bytes(tx.get_tx_hash())), "0x05ff44c891968e11c569ad6eb5a5ca96ce7ea855d5bc838af0701d6006aafa02");
        ASSERT_EQ(top::to_hex_prefixed(tx.get_from().to_bytes()), "0x4dce5c8961e283786cb31ad7fc072347227d7ea2");
        txs.push_back(tx);
    }
    {
        auto chainid = test_ethtx_tool::jsToU256("0x26b");
        auto nonce = test_ethtx_tool::jsToU256("0x4");
        auto max_priority_fee_per_gas = test_ethtx_tool::jsToU256("0x77359400");
        auto max_fee_per_gas = test_ethtx_tool::jsToU256("0x4a817c800");
        auto gaslimit = test_ethtx_tool::jsToU256("0x1e8480");
        auto to = common::xeth_address_t::build_from("0xcf637687290860dfb1183a7914f373a6f1d09fd0");
        auto value = test_ethtx_tool::jsToU256("0x3b9aca00");
        auto data = top::from_hex("0x", ec);
        auto signV = test_ethtx_tool::jsToByte("0x0");
        auto signR = xh256_t(top::from_hex("0x1639623c77e6122639c6ef6bfcc4f1def7d7cdb02091339b6eb3707e8508f271", ec));
        auto signS = xh256_t(top::from_hex("0x41a7af54e9cfba8cad57e2750a43585c0306a213858e4c706e5eb253d96ab3da", ec));

        xeth_transaction_t tx = xeth_transaction_t::build_eip1559_tx(chainid, nonce, max_priority_fee_per_gas, max_fee_per_gas, gaslimit, to, value, data);
        tx.set_sign(signR, signS, signV);
        ASSERT_EQ(top::to_hex_prefixed(to_bytes(tx.get_tx_hash())), "0xd54f085fd93469ae557b3b7f1ea81004c71c9e53ad6c770dccc5df965dca93e5");
        ASSERT_EQ(top::to_hex_prefixed(tx.get_from().to_bytes()), "0x4dce5c8961e283786cb31ad7fc072347227d7ea2");
        txs.push_back(tx);
    }
    {
        auto chainid = test_ethtx_tool::jsToU256("0x26b");
        auto nonce = test_ethtx_tool::jsToU256("0x5");
        auto max_priority_fee_per_gas = test_ethtx_tool::jsToU256("0x77359400");
        auto max_fee_per_gas = test_ethtx_tool::jsToU256("0x4a817c800");
        auto gaslimit = test_ethtx_tool::jsToU256("0x1e8480");
        auto to = common::xeth_address_t::build_from("0xcf637687290860dfb1183a7914f373a6f1d09fd0");
        auto value = test_ethtx_tool::jsToU256("0x3b9aca00");
        auto data = top::from_hex("0x", ec);
        auto signV = test_ethtx_tool::jsToByte("0x0");
        auto signR = xh256_t(top::from_hex("0x1dff2db9a591017376230c47d474fd76742a94ea88387494232d3c555a2bcb17", ec));
        auto signS = xh256_t(top::from_hex("0x44b84fe21e68d5949cf9f7543e2780f99572b0fb9b9b917a91d4ae3a048f412b", ec));

        xeth_transaction_t tx = xeth_transaction_t::build_eip1559_tx(chainid, nonce, max_priority_fee_per_gas, max_fee_per_gas, gaslimit, to, value, data);
        tx.set_sign(signR, signS, signV);
        ASSERT_EQ(top::to_hex_prefixed(to_bytes(tx.get_tx_hash())), "0xacfde86a369fc891b6e5be1cc72eb923da7553f7fd5ef997edcd694c690e7912");
        ASSERT_EQ(top::to_hex_prefixed(tx.get_from().to_bytes()), "0x4dce5c8961e283786cb31ad7fc072347227d7ea2");
        txs.push_back(tx);
    }
    evm_common::h256 txsRoot = xeth_build_t::build_transactions_root(txs);
    ASSERT_EQ(top::to_hex_prefixed(txsRoot.asBytes()), "0xe9d5f517609edf2dcbdc9442f1aba582db5b365a06444424bee401c4ead4e57f");

    {
        xeth_receipt_t receipt1;
        receipt1.set_tx_status(ethreceipt_status_successful);
        receipt1.set_cumulative_gas_used((uint64_t)test_ethtx_tool::jsToU256("0x5208"));
        receipts.push_back(receipt1);
        xeth_receipt_t receipt2;
        receipt2.set_tx_status(ethreceipt_status_successful);
        receipt2.set_cumulative_gas_used((uint64_t)test_ethtx_tool::jsToU256("0xa410"));
        receipts.push_back(receipt2);
        xeth_receipt_t receipt3;
        receipt3.set_tx_status(ethreceipt_status_successful);
        receipt3.set_cumulative_gas_used((uint64_t)test_ethtx_tool::jsToU256("0xf618"));
        receipts.push_back(receipt3);

        evm_common::h256 receiptsRoot = xeth_build_t::build_receipts_root(receipts);
        ASSERT_EQ(top::to_hex_prefixed(receiptsRoot.asBytes()), "0x25e6b7af647c519a27cc13276a1e6abc46154b51414d174b072698df1f6c19df");        
    }

    xeth_header_t ethheader;
    xethheader_para_t header_para;
    header_para.m_gaslimit = (uint64_t)test_ethtx_tool::jsToU256("0x7a1200");
    header_para.m_baseprice = test_ethtx_tool::jsToU256("0x7");
    header_para.m_coinbase = common::xeth_address_t::build_from("0x0000000000000000000000000000000000000000");
    xeth_build_t::build_ethheader(header_para, txs, receipts, {}, ethheader);
    ASSERT_EQ(top::to_hex_prefixed(ethheader.get_receipts_root().asBytes()), "0x25e6b7af647c519a27cc13276a1e6abc46154b51414d174b072698df1f6c19df");
    ASSERT_EQ(top::to_hex_prefixed(ethheader.get_transactions_root().asBytes()), "0xe9d5f517609edf2dcbdc9442f1aba582db5b365a06444424bee401c4ead4e57f");
    ASSERT_EQ(top::to_hex_prefixed(ethheader.get_logBloom().to_bytes()), "0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    ASSERT_EQ(ethheader.get_gaslimit(), header_para.m_gaslimit);
    ASSERT_EQ(ethheader.get_gasused(), (uint64_t)test_ethtx_tool::jsToU256("0xf618"));
    ASSERT_EQ(ethheader.get_baseprice(), test_ethtx_tool::jsToU256("0x7"));
    ASSERT_EQ(ethheader.get_coinbase().to_hex_string(), "0x0000000000000000000000000000000000000000");
}

TEST_F(test_eth_root, contract_deploy_one_tx) {
    {
        std::error_code ec;
        xeth_receipts_t receipts;
        xeth_receipt_t receipt1;
        receipt1.set_tx_status(ethreceipt_status_successful);
        receipt1.set_cumulative_gas_used((uint64_t)test_ethtx_tool::jsToU256("0x702fa"));
        receipts.push_back(receipt1);

        evm_common::h256 receiptsRoot = xeth_build_t::build_receipts_root(receipts);
        ASSERT_EQ(top::to_hex_prefixed(receiptsRoot.asBytes()), "0x3a0ebd4fbeac45cb7daf178da5b9cefba9b7f5c94a9fcea5ff9d66b0952c77b0");
    }
    {
        std::error_code ec;
        xeth_transactions_t txs;

        auto chainid = test_ethtx_tool::jsToU256("0x26b");
        auto nonce = test_ethtx_tool::jsToU256("0x6");
        auto max_priority_fee_per_gas = test_ethtx_tool::jsToU256("0x9502f900");
        auto max_fee_per_gas = test_ethtx_tool::jsToU256("0x9502f90e");
        auto gas = test_ethtx_tool::jsToU256("0x702fa");
        auto value = test_ethtx_tool::jsToU256("0x0");
        auto data = top::from_hex("0x60806040523480156200001157600080fd5b5060405162000a8238038062000a82833981810160405281019062000037919062000205565b83600090805190602001906200004f929190620000a9565b50826001908051906020019062000068929190620000a9565b5081600260006101000a81548160ff021916908360ff16021790555080600260016101000a81548160ff021916908360f81c021790555050505050620004a6565b828054620000b79062000383565b90600052602060002090601f016020900481019282620000db576000855562000127565b82601f10620000f657805160ff191683800117855562000127565b8280016001018555821562000127579182015b828111156200012657825182559160200191906001019062000109565b5b5090506200013691906200013a565b5090565b5b80821115620001555760008160009055506001016200013b565b5090565b6000620001706200016a84620002de565b620002b5565b9050828152602081018484840111156200018f576200018e62000452565b5b6200019c8482856200034d565b509392505050565b600081519050620001b58162000472565b92915050565b600082601f830112620001d357620001d26200044d565b5b8151620001e584826020860162000159565b91505092915050565b600081519050620001ff816200048c565b92915050565b600080600080608085870312156200022257620002216200045c565b5b600085015167ffffffffffffffff81111562000243576200024262000457565b5b6200025187828801620001bb565b945050602085015167ffffffffffffffff81111562000275576200027462000457565b5b6200028387828801620001bb565b93505060406200029687828801620001ee565b9250506060620002a987828801620001a4565b91505092959194509250565b6000620002c1620002d4565b9050620002cf8282620003b9565b919050565b6000604051905090565b600067ffffffffffffffff821115620002fc57620002fb6200041e565b5b620003078262000461565b9050602081019050919050565b60007fff0000000000000000000000000000000000000000000000000000000000000082169050919050565b600060ff82169050919050565b60005b838110156200036d57808201518184015260208101905062000350565b838111156200037d576000848401525b50505050565b600060028204905060018216806200039c57607f821691505b60208210811415620003b357620003b2620003ef565b5b50919050565b620003c48262000461565b810181811067ffffffffffffffff82111715620003e657620003e56200041e565b5b80604052505050565b7f4e487b7100000000000000000000000000000000000000000000000000000000600052602260045260246000fd5b7f4e487b7100000000000000000000000000000000000000000000000000000000600052604160045260246000fd5b600080fd5b600080fd5b600080fd5b600080fd5b6000601f19601f8301169050919050565b6200047d8162000314565b81146200048957600080fd5b50565b620004978162000340565b8114620004a357600080fd5b50565b6105cc80620004b66000396000f3fe608060405234801561001057600080fd5b50600436106100505760003560e01c806306fdde0314610156578063313ce5671461017457806395d89b4114610192578063bb07e85d146101b057610051565b5b6000600260019054906101000a900460f81b600036604051602001610078939291906103d4565b604051602081830303815290604052905060008073ff0000000000000000000000000000000000000173ffffffffffffffffffffffffffffffffffffffff16836040516100c591906103fe565b600060405180830381855af49150503d8060008114610100576040519150601f19603f3d011682016040523d82523d6000602084013e610105565b606091505b509150915081819061014d576040517f08c379a00000000000000000000000000000000000000000000000000000000081526004016101449190610430565b60405180910390fd5b50805160208201f35b61015e6101ce565b60405161016b9190610430565b60405180910390f35b61017c61025c565b6040516101899190610452565b60405180910390f35b61019a61026f565b6040516101a79190610430565b60405180910390f35b6101b86102fd565b6040516101c59190610415565b60405180910390f35b600080546101db9061051a565b80601f01602080910402602001604051908101604052809291908181526020018280546102079061051a565b80156102545780601f1061022957610100808354040283529160200191610254565b820191906000526020600020905b81548152906001019060200180831161023757829003601f168201915b505050505081565b600260009054906101000a900460ff1681565b6001805461027c9061051a565b80601f01602080910402602001604051908101604052809291908181526020018280546102a89061051a565b80156102f55780601f106102ca576101008083540402835291602001916102f5565b820191906000526020600020905b8154815290600101906020018083116102d857829003601f168201915b505050505081565b600260019054906101000a900460f81b81565b6103198161049f565b82525050565b61033061032b8261049f565b61054c565b82525050565b60006103428385610483565b935061034f8385846104d8565b82840190509392505050565b60006103668261046d565b6103708185610483565b93506103808185602086016104e7565b80840191505092915050565b600061039782610478565b6103a1818561048e565b93506103b18185602086016104e7565b6103ba81610585565b840191505092915050565b6103ce816104cb565b82525050565b60006103e0828661031f565b6001820191506103f1828486610336565b9150819050949350505050565b600061040a828461035b565b915081905092915050565b600060208201905061042a6000830184610310565b92915050565b6000602082019050818103600083015261044a818461038c565b905092915050565b600060208201905061046760008301846103c5565b92915050565b600081519050919050565b600081519050919050565b600081905092915050565b600082825260208201905092915050565b60007fff0000000000000000000000000000000000000000000000000000000000000082169050919050565b600060ff82169050919050565b82818337600083830152505050565b60005b838110156105055780820151818401526020810190506104ea565b83811115610514576000848401525b50505050565b6000600282049050600182168061053257607f821691505b6020821081141561054657610545610556565b5b50919050565b6000819050919050565b7f4e487b7100000000000000000000000000000000000000000000000000000000600052602260045260246000fd5b6000601f19601f830116905091905056fea26469706673582212201c41c7119aac7d56d426fbbc21ff7c0341d9b6f299af2667e722c6d4d74d847764736f6c63430008070033000000000000000000000000000000000000000000000000000000000000008000000000000000000000000000000000000000000000000000000000000000c0000000000000000000000000000000000000000000000000000000000000000601000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000003544f5000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000003544f500000000000000000000000000000000000000000000000000000000000", ec);
        auto signV = test_ethtx_tool::jsToByte("0x0");
        auto signR = xh256_t(top::from_hex("0x6f25240dbaa494fc9cd77e8ff392ea32a719ed72ec26cd45d9f79a72b16192a2", ec));
        auto signS = xh256_t(top::from_hex("0x79d98d28b9a33013a294c2eafc1faad9317b6a24620b111d3d874819c89bb05d", ec));

        xeth_transaction_t tx = xeth_transaction_t::build_eip1559_tx(chainid, nonce, max_priority_fee_per_gas, max_fee_per_gas, gas, value, data);
        tx.set_sign(signR, signS, signV);
        ASSERT_EQ(top::to_hex_prefixed(to_bytes(tx.get_tx_hash())), "0xa61f1db0b476226b38e6275bcd0886e606662cc77075ad46ad123329282da7ee");
        ASSERT_EQ(top::to_hex_prefixed(tx.get_from().to_bytes()), "0x4dce5c8961e283786cb31ad7fc072347227d7ea2");
        txs.push_back(tx);

        evm_common::h256 txsRoot = xeth_build_t::build_transactions_root(txs);
        ASSERT_EQ(top::to_hex_prefixed(txsRoot.asBytes()), "0x1393c702fa7fcab4d1590f0559b46b0ce121dae312aed0d4f8f9eb2613a99db0");
    }
}

TEST_F(test_eth_root, contract_call_one_tx_1) {
    {
        std::error_code ec;
        xeth_receipts_t receipts;
        xeth_receipt_t receipt1;
        receipt1.set_tx_status(ethreceipt_status_successful);
        receipt1.set_cumulative_gas_used((uint64_t)test_ethtx_tool::jsToU256("0xaad8"));
        receipts.push_back(receipt1);

        evm_common::h256 receiptsRoot = xeth_build_t::build_receipts_root(receipts);
        ASSERT_EQ(top::to_hex_prefixed(receiptsRoot.asBytes()), "0xe8ca3b23d3103f754321acc7dd175efcafc7f57211c48279e39fbe785cac230a");
    }
    {
        std::error_code ec;
        xeth_transactions_t txs;

        auto chainid = test_ethtx_tool::jsToU256("0x26b");
        auto nonce = test_ethtx_tool::jsToU256("0x8");
        auto max_priority_fee_per_gas = test_ethtx_tool::jsToU256("0x9502f900");
        auto max_fee_per_gas = test_ethtx_tool::jsToU256("0x9502f90e");
        auto gas = test_ethtx_tool::jsToU256("0xaad8");
        auto to = common::xeth_address_t::build_from("0x34edcccfee15977e34b8faf6d8e55ec6282e4946");
        auto value = test_ethtx_tool::jsToU256("0x0");
        auto data = top::from_hex("0x6057361d0000000000000000000000000000000000000000000000000000000000000bb8", ec);
        auto signV = test_ethtx_tool::jsToByte("0x0");
        auto signR = xh256_t(top::from_hex("0x46fe988216d598ea9501aa61dd7d8f9092c6c649726e0e5207c94f80c4d00eb5", ec));
        auto signS = xh256_t(top::from_hex("0x4a607d1f987c9c7e8236e0794c331109b23bb470d4b0c358227327799f2d865a", ec));

        xeth_transaction_t tx = xeth_transaction_t::build_eip1559_tx(chainid, nonce, max_priority_fee_per_gas, max_fee_per_gas, gas, to, value, data);
        tx.set_sign(signR, signS, signV);
        ASSERT_EQ(top::to_hex_prefixed(to_bytes(tx.get_tx_hash())), "0x567ecbbd02d3f640b278470becbec136b595912f1b6420a1f6ec14336d674556");
        ASSERT_EQ(top::to_hex_prefixed(tx.get_from().to_bytes()), "0x4dce5c8961e283786cb31ad7fc072347227d7ea2");
        txs.push_back(tx);

        evm_common::h256 txsRoot = xeth_build_t::build_transactions_root(txs);
        ASSERT_EQ(top::to_hex_prefixed(txsRoot.asBytes()), "0x0cfd2438bac53941ad970b4d496eff1b01bf6d3ea10a27b8d7c3f985e5cd420e");
    }
}

TEST_F(test_eth_root, contract_call_one_tx_2) {
    std::error_code ec;   
    xeth_receipts_t receipts;
    xeth_transactions_t txs;

{
    xeth_receipt_t receipt1;
    receipt1.set_tx_status(ethreceipt_status_successful);
    receipt1.set_cumulative_gas_used((uint64_t)test_ethtx_tool::jsToU256("0x57506"));

    common::xeth_address_t log_address = common::xeth_address_t::build_from("0x7618e07ed0bd6d2e2d7dd044200c1b44f7ae33ef");
    xh256s_t log_topics;
    log_topics.push_back(xh256_t(top::from_hex("0x342827c97908e5e2f71151c08502a66d44b6f758e3ac2f1de95f02eb95f0a735", ec)));
    log_topics.push_back(xh256_t(top::from_hex("0x0000000000000000000000000000000000000000000000000000000000000000", ec)));
    log_topics.push_back(xh256_t(top::from_hex("0x0000000000000000000000004dce5c8961e283786cb31ad7fc072347227d7ea2", ec)));
    xbytes_t log_data;
    evm_common::xevm_log_t log(log_address, log_topics, log_data);
    evm_common::xevm_logs_t logs;
    logs.push_back(log);
    receipt1.set_logs(logs);
    receipt1.create_bloom();
    receipts.push_back(receipt1);
    ASSERT_EQ(receipt1.get_logsBloom().to_hex_string(),
        "0x00000000000000000000010000000000000000000400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002000000000000800000000000000000000000000000000000000000000000020000000000000000000800000000000000000000000000000000000000000000000000000400000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000040000002000000000000000000020004000000000000000000000000000000000000000000000000000000000000000");
    evm_common::h256 receiptsRoot = xeth_build_t::build_receipts_root(receipts);
    ASSERT_EQ(top::to_hex_prefixed(receiptsRoot.asBytes()), "0x860bf9acb9f14c96d685c6f040a4339c755a35a3d59a634ba02aec2ba578d026");
}
{
    auto chainid = test_ethtx_tool::jsToU256("0x26b");
    auto nonce = test_ethtx_tool::jsToU256("0x9");
    auto max_priority_fee_per_gas = test_ethtx_tool::jsToU256("0x9502f900");
    auto max_fee_per_gas = test_ethtx_tool::jsToU256("0x9502f90e");
    auto gas = test_ethtx_tool::jsToU256("0x57506");
    auto value = test_ethtx_tool::jsToU256("0x0");
    auto data = top::from_hex("0x608060405234801561001057600080fd5b5061005a6040518060400160405280601b81526020017f4f776e657220636f6e7472616374206465706c6f7965642062793a00000000008152503361011a60201b6101e91760201c565b336000806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555060008054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16600073ffffffffffffffffffffffffffffffffffffffff167f342827c97908e5e2f71151c08502a66d44b6f758e3ac2f1de95f02eb95f0a73560405160405180910390a36102ef565b6101b882826040516024016101309291906102bf565b6040516020818303038152906040527f319af333000000000000000000000000000000000000000000000000000000007bffffffffffffffffffffffffffffffffffffffffffffffffffffffff19166020820180517bffffffffffffffffffffffffffffffffffffffffffffffffffffffff83818316178352505050506101bc60201b60201c565b5050565b60008151905060006a636f6e736f6c652e6c6f679050602083016000808483855afa5050505050565b600081519050919050565b600082825260208201905092915050565b60005b8381101561021f578082015181840152602081019050610204565b8381111561022e576000848401525b50505050565b6000601f19601f8301169050919050565b6000610250826101e5565b61025a81856101f0565b935061026a818560208601610201565b61027381610234565b840191505092915050565b600073ffffffffffffffffffffffffffffffffffffffff82169050919050565b60006102a98261027e565b9050919050565b6102b98161029e565b82525050565b600060408201905081810360008301526102d98185610245565b90506102e860208301846102b0565b9392505050565b6104d3806102fe6000396000f3fe608060405234801561001057600080fd5b50600436106100365760003560e01c8063893d20e81461003b578063a6f9dae114610059575b600080fd5b610043610075565b60405161005091906102ef565b60405180910390f35b610073600480360381019061006e919061033b565b61009e565b005b60008060009054906101000a900473ffffffffffffffffffffffffffffffffffffffff16905090565b60008054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff163373ffffffffffffffffffffffffffffffffffffffff161461012c576040517f08c379a0000000000000000000000000000000000000000000000000000000008152600401610123906103c5565b60405180910390fd5b8073ffffffffffffffffffffffffffffffffffffffff1660008054906101000a900473ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff167f342827c97908e5e2f71151c08502a66d44b6f758e3ac2f1de95f02eb95f0a73560405160405180910390a3806000806101000a81548173ffffffffffffffffffffffffffffffffffffffff021916908373ffffffffffffffffffffffffffffffffffffffff16021790555050565b61028182826040516024016101ff92919061046d565b6040516020818303038152906040527f319af333000000000000000000000000000000000000000000000000000000007bffffffffffffffffffffffffffffffffffffffffffffffffffffffff19166020820180517bffffffffffffffffffffffffffffffffffffffffffffffffffffffff8381831617835250505050610285565b5050565b60008151905060006a636f6e736f6c652e6c6f679050602083016000808483855afa5050505050565b600073ffffffffffffffffffffffffffffffffffffffff82169050919050565b60006102d9826102ae565b9050919050565b6102e9816102ce565b82525050565b600060208201905061030460008301846102e0565b92915050565b600080fd5b610318816102ce565b811461032357600080fd5b50565b6000813590506103358161030f565b92915050565b6000602082840312156103515761035061030a565b5b600061035f84828501610326565b91505092915050565b600082825260208201905092915050565b7f43616c6c6572206973206e6f74206f776e657200000000000000000000000000600082015250565b60006103af601383610368565b91506103ba82610379565b602082019050919050565b600060208201905081810360008301526103de816103a2565b9050919050565b600081519050919050565b60005b8381101561040e5780820151818401526020810190506103f3565b8381111561041d576000848401525b50505050565b6000601f19601f8301169050919050565b600061043f826103e5565b6104498185610368565b93506104598185602086016103f0565b61046281610423565b840191505092915050565b600060408201905081810360008301526104878185610434565b905061049660208301846102e0565b939250505056fea2646970667358221220ab4ee28980438881db8aa0657f4663b8278261f3fcc4567ad786d8bdfa2897bc64736f6c634300080e0033");
    auto signV = test_ethtx_tool::jsToByte("0x0");
    auto signR = xh256_t(top::from_hex("0xb3a1141d92735d39cf9960d43862e02a3aab10ffd7a8ee1b85cc4a2062bba571", ec));
    auto signS = xh256_t(top::from_hex("0x57313f27ef76cac1b5363a5628d5d413d752fde3334afd4ecd3c107291b18952", ec));

    xeth_transaction_t tx = xeth_transaction_t::build_eip1559_tx(chainid, nonce, max_priority_fee_per_gas, max_fee_per_gas, gas, value, data);
    tx.set_sign(signR, signS, signV);
    ASSERT_EQ(top::to_hex_prefixed(to_bytes(tx.get_tx_hash())), "0x5ab38261338ca9c7d093654a8c1afd0933fabd65324e8bf722bcb750ef791531");
    ASSERT_EQ(top::to_hex_prefixed(tx.get_from().to_bytes()), "0x4dce5c8961e283786cb31ad7fc072347227d7ea2");
    txs.push_back(tx);
    evm_common::h256 txsRoot = xeth_build_t::build_transactions_root(txs);
    ASSERT_EQ(top::to_hex_prefixed(txsRoot.asBytes()), "0x36c818f84049135b96ab9e5e424349ec5b904c6378fc95277eec7f1cac353f9f");
}

    xeth_header_t ethheader;
    xethheader_para_t header_para;
    header_para.m_gaslimit = (uint64_t)test_ethtx_tool::jsToU256("0x7a1200");
    header_para.m_baseprice = test_ethtx_tool::jsToU256("0x7");
    header_para.m_coinbase = common::xeth_address_t::build_from("0x0000000000000000000000000000000000000000");
    xeth_build_t::build_ethheader(header_para, txs, receipts, {}, ethheader);
    ASSERT_EQ(top::to_hex_prefixed(ethheader.get_receipts_root().asBytes()), "0x860bf9acb9f14c96d685c6f040a4339c755a35a3d59a634ba02aec2ba578d026");
    ASSERT_EQ(top::to_hex_prefixed(ethheader.get_transactions_root().asBytes()), "0x36c818f84049135b96ab9e5e424349ec5b904c6378fc95277eec7f1cac353f9f");
    ASSERT_EQ(top::to_hex_prefixed(ethheader.get_logBloom().to_bytes()), "0x00000000000000000000010000000000000000000400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002000000000000800000000000000000000000000000000000000000000000020000000000000000000800000000000000000000000000000000000000000000000000000400000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000040000002000000000000000000020004000000000000000000000000000000000000000000000000000000000000000");
    ASSERT_EQ(ethheader.get_gaslimit(), header_para.m_gaslimit);
    ASSERT_EQ(ethheader.get_gasused(), (uint64_t)test_ethtx_tool::jsToU256("0x57506"));
    ASSERT_EQ(ethheader.get_baseprice(), test_ethtx_tool::jsToU256("0x7"));
    ASSERT_EQ(ethheader.get_coinbase().to_hex_string(), "0x0000000000000000000000000000000000000000");
}
