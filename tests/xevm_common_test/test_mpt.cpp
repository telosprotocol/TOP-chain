// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//#include "xcommon/common_data.h"
#include "xcommon/rlp.h"
#include "xevm_common/xtriehash.h"
#include <iostream>
#include <limits>
#include <fstream>
#include <gtest/gtest.h>
#include "xpbase/base/top_utils.h"

using namespace top::evm_common;




TEST(test_mpt, test_mpt_test) {
 
    {
        const  h256 goResult{"92b220a2a01703e1fd89ab22f00d6983631852090209416f1508b84175455c3d"};
        std::vector<top::xbytes_t> mpt_data{{0xc0, 0x41}};
        h256  hashResult = orderedTrieRoot(mpt_data);
        std::cout << " MPT Root Hash: " << hashResult.hex() << std::endl;
        ASSERT_EQ(hashResult, goResult);
    }

    {
        const h256 goResult{"92b220a2a01703e1fd89ab22f00d6983631852090209416f1508b84175455c3d"};
        std::string str1 ="c041";
        std::string hexdata = top::HexDecode(str1);
        std::vector<top::xbytes_t> mpt_data{top::xbytes_t(hexdata.begin(), hexdata.end())};
        h256  hashResult = orderedTrieRoot(mpt_data);
        std::cout << "MPT Root Hash: " <<  hashResult.hex()  << std::endl;
        ASSERT_EQ(hashResult, goResult);
        //go result  
    }

    {
        const h256 goResult{"40fbc1f71e148f995456177f25366e04f31d618b16272394601774e4cf69d249"};
        std::string str2[] = {"f04cf757812428b0763112efb33b6f4fad7deb445e", "f04cf757812428b0763112efb33b6f4fad7deb445e"};
        std::vector<top::xbytes_t> mpt_data;
        for (auto  str: str2) {
             std::string hexdata = top::HexDecode(str);
            mpt_data.push_back(top::xbytes_t(hexdata.begin(), hexdata.end()));
        }
        h256  hashResult = orderedTrieRoot(mpt_data);
        std::cout << "MPT Root Hash: " <<  hashResult.hex()  << std::endl;
        ASSERT_EQ(hashResult, goResult);
        //go result  0x40fbc1f71e148f995456177f25366e04f31d618b16272394601774e4cf69d249
    }

    {
       const h256 goResult{"b72d55c76bd8f477f4b251763c33f75e6f5f5dd8af071e711e0cb9b2accc70ea"};
        std::string str2[] = { "ca410605310cdc3bb8d4977ae4f0143df54a724ed873457e2272f39d66e0460e971d9d",
                                "6cd850eca0a7ac46bb1748d7b9cb88aa3bd21c57d852c28198ad8fa422c4595032e88a4494b4778b36b944fe47a52b8c5cd312910139dfcb4147ab8e972cc456bcb063f25dd78f54c4d34679e03142c42c662af52947d45bdb6e555751334ace76a5080ab5a0256a1d259855dfc5c0b8023b25befbb13fd3684f9f755cbd3d63544c78ee2001452dd54633a7593ade0b183891a0a4e9c7844e1254005fbe592b1b89149a502c24b6e1dca44c158aebedf01beae9c30cabe16a",
                                "14abd5c47c0be87b0454596baad2",
                                "ca410605310cdc3bb8d4977ae4f0143df54a724ed873457e2272f39d66e0460e971d9d" };
       std::vector<top::xbytes_t> mpt_data;
        for (auto  str: str2) {
            std::string hexdata = top::HexDecode(str);
            mpt_data.push_back(top::xbytes_t(hexdata.begin(), hexdata.end()));
        }
        h256  hashResult = orderedTrieRoot(mpt_data);
        std::cout << "MPT Root Hash: " <<  hashResult.hex()  << std::endl;
        ASSERT_EQ(hashResult, goResult);
        //go result 0xb72d55c76bd8f477f4b251763c33f75e6f5f5dd8af071e711e0cb9b2accc70ea
    }
    
    {
        struct xrelay_receipt_log {
            //address of the contract that transaction execution
            h160                m_contract_address;
            // supplied by the contract, usually ABI-encoded
            top::xbytes_t m_data;
            // list of topics provided by the contract
            h256s               m_topics;
        };

        struct  xrelay_receipt{
            //status code,check transactions status
            uint8_t                         m_status;
            //gas of block used
            u256                m_gasUsed;
            // bloom filter for the logs of the block
            h2048               m_logsBloom;
            //receipt log
            std::vector<xrelay_receipt_log> m_logs;
        };

        const h256 goResult{"9c21c8bb362995ccb4c0058d55d1b7c354e092f807728afae0b417109ff3b649"};
        
        xrelay_receipt receipt;
        receipt.m_status = 0x1;
        receipt.m_gasUsed = u256 {0xccde};
        receipt.m_logsBloom = h2048{"00000001000000004000000000000000000000000000000000000000000000000000000000041000000000000000008000000000000080000000000000200000000000000000000000000008000000000000000000008000000000000000000010000000020000000004000100000800000000040000000000000012000000000000000020000000008000000000000000000000000000000000000000000000420000000000000000000000000000000000000000080000000000000000000000000002000000200000000000000000000008002000000000000000000020000010000200000000000000000000000000000000000000000000002000000000"};
        //add logs
        xrelay_receipt_log log1;
        log1.m_contract_address = h160 {"e22c0e020c99e9aed339618fdcea2871d678ef38"};
        log1.m_topics.push_back(h256 {"8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925"});
        log1.m_topics.push_back(h256 {"000000000000000000000000f39fd6e51aad88f6f4ce6ab8827279cfffb92266"});
        log1.m_topics.push_back(h256 {"000000000000000000000000009b5f068bc20a5b12030fcb72975d8bddc4e84c"});
        std::string str1 = "00000000000000000000000000000000000000000000000000000000000003de";
        std::string log_data1 = top::HexDecode(str1);
        log1.m_data = top::xbytes_t(log_data1.begin(), log_data1.end());
        receipt.m_logs.push_back(log1);

        xrelay_receipt_log log2;
        log2.m_contract_address = h160 {"e22c0e020c99e9aed339618fdcea2871d678ef38"};
        log2.m_topics.push_back(h256 {"ddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef"});
        log2.m_topics.push_back(h256 {"000000000000000000000000f39fd6e51aad88f6f4ce6ab8827279cfffb92266"});
        log2.m_topics.push_back(h256 {"0000000000000000000000000000000000000000000000000000000000000000"});
        std::string   str2 = "000000000000000000000000000000000000000000000000000000000000000a";
        std::string log_data2 = top::HexDecode(str2);
        log2.m_data = top::xbytes_t(log_data2.begin(), log_data2.end());
        receipt.m_logs.push_back(log2);

        xrelay_receipt_log log3;
        log3.m_contract_address = h160 {"009b5f068bc20a5b12030fcb72975d8bddc4e84c"};
        log3.m_topics.push_back(h256 {"4f89ece0f576ba3986204ba19a44d94601604b97cf3baa922b010a758d303842"});
        log3.m_topics.push_back(h256 {"000000000000000000000000e22c0e020c99e9aed339618fdcea2871d678ef38"});
        log3.m_topics.push_back(h256 {"000000000000000000000000f3b23b373dc8854cc2936f4ab4b8e782011ccf87"});
        log3.m_topics.push_back(h256 {"000000000000000000000000f39fd6e51aad88f6f4ce6ab8827279cfffb92266"});
        std::string str3 = "000000000000000000000000000000000000000000000000000000000000000a000000000000000000000000a4ba11f3f36b12c71f2aef775583b306a3cf784a";
        std::string log_data3 = top::HexDecode(str3);
        log3.m_data = top::xbytes_t(log_data3.begin(), log_data3.end());
        receipt.m_logs.push_back(log3);

        std::vector<top::xbytes_t> receipt_vector;

        //RLP receipt
        RLPStream receiptrlp;
        receiptrlp.appendList(4);
        receiptrlp << receipt.m_status;
        receiptrlp << receipt.m_gasUsed << receipt.m_logsBloom;
        receiptrlp.appendList(receipt.m_logs.size());
        for (auto &log : receipt.m_logs) {
            receiptrlp.appendList(3)  << log.m_contract_address << log.m_topics << log.m_data;
        }
        //get rlp bytes
        top::xbytes_t receiptOut = receiptrlp.out();
       // std::cout << "encoded  " << toHex(receiptOut.begin(), receiptOut.end(), "") <<std::endl;
        receipt_vector.push_back(receiptOut);
        
        h256 receiptsRoot = orderedTrieRoot(receipt_vector);
        std::cout << "receipts  test  Hash: " <<  receiptsRoot.hex()  << std::endl;
        ASSERT_EQ(receiptsRoot, goResult);
    }

}