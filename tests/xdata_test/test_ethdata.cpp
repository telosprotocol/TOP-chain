#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xethheader.h"
#include "xdata/xethreceipt.h"
#include "xdata/xlightunit_info.h"

using namespace top;
using namespace top::base;
using namespace top::data;

class test_ethdata : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_ethdata, basic_type_rlp) {
    {
        uint64_t value = 0;
        evm_common::RLPStream s;
        s << value;
        std::string outstr = top::to_string(s.out());
        std::cout << "empty uint64_t:" << outstr.size() << std::endl;
    }
    {
        evm_common::u256 value = 0;
        evm_common::RLPStream s;
        s << value;
        std::string outstr = top::to_string(s.out());
        std::cout << "empty u256:" << outstr.size() << std::endl;
    }
    {
        evm_common::h2048 value;
        evm_common::RLPStream s;
        s << value;
        std::string outstr = top::to_string(s.out());
        std::cout << "empty h2048:" << outstr.size() << std::endl;
    }    
    {
        evm_common::xbloom9_t value;
        evm_common::RLPStream s;
        s << value.get_data();
        std::string outstr = top::to_string(s.out());
        std::cout << "empty xbloom9_t:" << outstr.size() << std::endl;
    }
    {
        evm_common::h256 value;
        evm_common::RLPStream s;
        s << value;
        std::string outstr = top::to_string(s.out());
        std::cout << "empty h256:" << outstr.size() << std::endl;
    }        
    {
        evm_common::xh256_t value;
        evm_common::RLPStream s;
        s << value;
        std::string outstr = top::to_string(s.out());
        std::cout << "empty xh256_t:" << outstr.size() << std::endl;
    }    
    {
        xbytes_t value;
        evm_common::RLPStream s;
        s << value;
        std::string outstr = top::to_string(s.out());
        std::cout << "empty xbytes_t:" << outstr.size() << std::endl;
    }    
}

TEST_F(test_ethdata, ethheader_rlp) {
    {
        xeth_header_t _header;
        evm_common::RLPStream s;
        _header.streamRLP(s);
        std::string outstr = top::to_string(s.out());
        std::cout << "empty ethheader_rlp:" << outstr.size() << std::endl;

        xeth_header_t _header2;
        evm_common::RLP d(s.out());
        std::error_code ec;
        _header2.decodeRLP(d, ec);
        if (ec) {
            assert(false);
        }
    }
    {
        xeth_header_t _header;
        _header.set_gaslimit(10000000);
        _header.set_gasused(5000000);
        _header.set_baseprice(10000000000);

        evm_common::RLPStream s;
        _header.streamRLP(s);
        std::string outstr = top::to_string(s.out());
        std::cout << "ethheader_rlp:" << outstr.size() << std::endl;

        xeth_header_t _header2;
        evm_common::RLP d(s.out());
        std::error_code ec;
        _header2.decodeRLP(d, ec);
        if (ec) {
            assert(false);
        }        
    }
}

TEST_F(test_ethdata, ethreceipt_store_serialize) {
    {
        xeth_store_receipt_t _receipt;
        _receipt.set_tx_status(enum_ethreceipt_status::ethreceipt_status_successful);
        xbytes_t _bytes = _receipt.encodeBytes();
        std::string _bin_str = top::to_string(_bytes);

        std::error_code ec;
        xbytes_t _bytes2 = top::to_bytes(_bin_str);
        xeth_store_receipt_t _receipt2;
        _receipt2.decodeBytes(_bytes2, ec);
        if (ec) {assert(false);}
        ASSERT_EQ(_receipt2.get_tx_status(), enum_ethreceipt_status::ethreceipt_status_successful);
    }
}

