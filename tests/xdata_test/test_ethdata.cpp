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
        xh256_t value;
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
        evm_common::xbloom9_t _logBloom;
        xbytes_t _logdata = {1,2,3,4};
        _logBloom.add(_logdata);
        _header.set_logBloom(_logBloom);
        std::string root_str = "0x9c09bae2c4a8f1487e11260efd4a19b7cb719ad4dc40fdd4ac461e04fae01aba";
        std::error_code ec;
        xbytes_t root_bs = top::from_hex(root_str, ec);
        xh256_t _root(root_bs);
        _header.set_transactions_root(_root);
        _header.set_receipts_root(_root);
        _header.set_state_root(_root);
        common::xeth_address_t _ethaddr = common::xeth_address_t::build_from("0xb7762d8dbd7e5c023ff99402b78af7c13b01eec1");
        _header.set_coinbase(_ethaddr);

        xbytes_t data_bs = top::from_hex("0x112233", ec);
        _header.set_extra_data(data_bs);

        evm_common::RLPStream s;
        _header.streamRLP(s);
        std::string outstr = top::to_string(s.out());
        std::cout << "ethheader_rlp:" << outstr.size() << std::endl;

        xeth_header_t _header2;
        evm_common::RLP d(s.out());
        _header2.decodeRLP(d, ec);
        if (ec) {
            assert(false);
        }
        ASSERT_EQ(_header2.get_gaslimit(), 10000000);
        ASSERT_EQ(_header2.get_gasused(), 5000000);
        ASSERT_EQ(_header2.get_baseprice(), 10000000000);
        ASSERT_EQ(_header2.get_transactions_root().asBytes(), root_bs);
        ASSERT_EQ(_header2.get_receipts_root().asBytes(), root_bs);
        ASSERT_EQ(_header2.get_state_root().asBytes(), root_bs);
        ASSERT_EQ(_header2.get_coinbase().to_bytes(), _ethaddr.to_bytes());
        ASSERT_EQ(_header2.get_extra_data(), data_bs);
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

TEST_F(test_ethdata, rlp_test_1) {
    struct s1
    {
        s1() = default;
        s1(uint64_t _v0, uint64_t _v1) {
            v0 = _v0;
            v1 = _v1;
        }
        uint64_t v0;
        uint64_t v1;
        void encodeRLP(evm_common::RLPStream& _s) {
            _s.appendList(2);
            _s << v0;
            _s << v1;
        }
    };
    struct s2
    {
        uint64_t v0;
        s1       v1;

        void encodeRLP(evm_common::RLPStream& _s) {
            _s.appendList(2);
            _s << v0;
            v1.encodeRLP(_s);
        }
    };
    struct s3
    {
        uint64_t            v0;
        evm_common::RLP     v1;
        void encodeRLP(evm_common::RLPStream& _s) {
            _s.appendList(2);
            _s << v0;
            _s << v1;
        }
        void decodeRLP(evm_common::RLP const& _r) {
            if (_r.itemCount() != 2 ) {
                xassert(false);
                return;
            }
            v0 = (uint64_t)_r[0];
            v1 = _r[1];
        }
    };

    {
        s1 _s1(0x11, 0x11);
        s2 _s2;
        _s2.v0 = 0x33;
        _s2.v1 = _s1;

        evm_common::RLPStream rlp_s2;
        _s2.encodeRLP(rlp_s2);
        std::string outstr_s2 = top::to_string(rlp_s2.out());
        std::cout << "rlp_s2 = " << top::to_hex(rlp_s2.out()) << std::endl;        

        s3 _s3;
        evm_common::RLP r_s3(rlp_s2.out());
        _s3.decodeRLP(r_s3);
        
        evm_common::RLPStream rlp_s3;
        _s3.encodeRLP(rlp_s3);
        std::string outstr_s3 = top::to_string(rlp_s3.out());
        std::cout << "rlp_s3 = " << top::to_hex(rlp_s3.out()) << std::endl;

        ASSERT_EQ(outstr_s3, outstr_s2);
    }
}

TEST_F(test_ethdata, rlp_test_2) {
    struct s1
    {
        s1(uint64_t _v0, uint64_t _v1) {
            v0 = _v0;
            v1 = _v1;
        }
        uint64_t v0;
        uint64_t v1;
        void encodeRLP(evm_common::RLPStream& _s) {
            _s.appendList(2);
            _s << v0;
            _s << v1;
        }
    };
    struct s2
    {
        std::vector<s1> v0;
        void add_s1(s1 const& _v) {
            v0.push_back(_v);
        }
        void encodeRLP(evm_common::RLPStream& _s) {
            _s.appendList(v0.size());
            for (auto & v : v0) {
                v.encodeRLP(_s);
            }
        }
    };

    struct s3
    {
        uint64_t    v0;
        s2          v1;
        uint64_t    v2;
        void encodeRLP(evm_common::RLPStream& _s) {
            _s.appendList(3);
            _s << v0;
            v1.encodeRLP(_s);
            _s << v2;
        }        
    };

    struct s4
    {
        uint64_t    v0;
        evm_common::RLP     v1;
        uint64_t    v2;
        void encodeRLP(evm_common::RLPStream& _s) {
            _s.appendList(3);
            _s << v0;
            _s << v1;
            _s << v2;
        }
        void decodeRLP(evm_common::RLP const& _r) {
            v0 = (uint64_t)_r[0];
            v1 = _r[1];
            v2 = (uint64_t)_r[2];
        }
    };

    {
        s1 _s1_0(0x11, 0x11);
        s1 _s1_1(0x22, 0x22);
        s2 _s2;
        _s2.add_s1(_s1_0);
        _s2.add_s1(_s1_1);        
        s3 _s3;
        _s3.v0 = 0x55;
        _s3.v1 = _s2;
        _s3.v2 = 0x55;

        evm_common::RLPStream rlp_s3;
        _s3.encodeRLP(rlp_s3);
        std::string outstr_s3 = top::to_string(rlp_s3.out());
        std::cout << "rlp_s3 = " << top::to_hex(rlp_s3.out()) << std::endl;

        evm_common::RLP r_s4(rlp_s3.out());
        s4 _s4;
        _s4.decodeRLP(r_s4);
        ASSERT_EQ(_s4.v0, _s3.v0);
        evm_common::RLPStream rlp_s4;
        _s4.encodeRLP(rlp_s4);
        std::string outstr_s4 = top::to_string(rlp_s4.out());
        std::cout << "rlp_s4 = " << top::to_hex(rlp_s4.out()) << std::endl;

        ASSERT_EQ(outstr_s3, outstr_s4);
    }
}
