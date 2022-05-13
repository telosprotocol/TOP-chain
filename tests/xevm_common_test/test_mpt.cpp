// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

//#include "xevm_common/common_data.h"
#include "xevm_common/xborsh.hpp"
//#include "xevm_common/xtriecommon.h"
#include "xevm_common/xtriehash.h"
#include "xevm_common/rlp.h"

#include <iostream>
#include <limits>
#include <fstream>
#include <gtest/gtest.h>
#include "xpbase/base/top_utils.h"

using namespace top::evm_common;
using namespace top::evm_common::rlp;



TEST(test_mpt, test_mpt_test) {
 
    {
        const  h256 goResult{"92b220a2a01703e1fd89ab22f00d6983631852090209416f1508b84175455c3d"};
        std::vector<bytes> mpt_data { {0xc0, 0x41} };
        h256  hashResult = orderedTrieRoot(mpt_data);
        std::cout << " MPT Root Hash: " << hashResult.hex() << std::endl;
        ASSERT_EQ(hashResult, goResult);
    }

    {
        const h256 goResult{"92b220a2a01703e1fd89ab22f00d6983631852090209416f1508b84175455c3d"};
        std::string str1 ="c041";
        std::string hexdata = top::HexDecode(str1);
        std::vector<bytes> mpt_data { bytes(hexdata.begin(), hexdata.end())};
        h256  hashResult = orderedTrieRoot(mpt_data);
        std::cout << "MPT Root Hash: " <<  hashResult.hex()  << std::endl;
        ASSERT_EQ(hashResult, goResult);
        //go result  
    }

    {
        const h256 goResult{"40fbc1f71e148f995456177f25366e04f31d618b16272394601774e4cf69d249"};
        std::string str2[] = {"f04cf757812428b0763112efb33b6f4fad7deb445e", "f04cf757812428b0763112efb33b6f4fad7deb445e"};
        std::vector<bytes> mpt_data;
        for (auto  str: str2) {
             std::string hexdata = top::HexDecode(str);
             mpt_data.push_back( bytes(hexdata.begin(), hexdata.end()));
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
        std::vector<bytes> mpt_data;
        for (auto  str: str2) {
            std::string hexdata = top::HexDecode(str);
            mpt_data.push_back( bytes(hexdata.begin(), hexdata.end()));
        }
        h256  hashResult = orderedTrieRoot(mpt_data);
        std::cout << "MPT Root Hash: " <<  hashResult.hex()  << std::endl;
        ASSERT_EQ(hashResult, goResult);
        //go result 0xb72d55c76bd8f477f4b251763c33f75e6f5f5dd8af071e711e0cb9b2accc70ea
    }

    {

        std::string hasharray[] ={
        "92b220a2a01703e1fd89ab22f00d6983631852090209416f1508b84175455c3d",
        "34141241241412341289ab22f00d6983631852090209416f1508b84175455c3d",
        "334141241244323423423b22f00d6983631852090209416f1508b84175455c3d",
        "1ce12341a244323423423b22f00d6983631852090209416f1508b84175455c3d"};

        std::string  addressArray[] ={
            "4708fDB6D749EdB1d7848c0bb14B85da17dc4Dd2",
            "6248e54F072Fc61745AF93D4EF95bC338E1c4Ef8",
            "5C8Dce7268C796832C9F664486ad718731f73bEa",
            "40D21280d5399Af7aE6507ba48643eaE47c618cE"};

         std::string blockHashArray[] = {
            "d38c0c4e84de118cfdcc775130155d83b8bbaaf23dc7f3c83a626b10473213bd",
            "fb3aa5c655c2ec9d40609401f88d505d1da61afaa550e36ef5da0509ada257ba",
            "8e54a4494fe5da016bfc01363f4f6cdc91013bb5434bd2a4a3359f13a23afa2f",
            "0684ac65a9fa32414dda56996f4183597d695987fdb82b145d722743891a6fe8" };
        
        
        typedef  struct  receipt_def{
            uint8_t     Type;
            //bytes       PostState;
            uint64_t    Status;
            uint64_t    CumulativeGasUsed;
            h2048     Bloom;    //2048;
            h256        TxHash;
            h160     Address;   
            uint64_t    GasUsed;
            h256        BlockHash;
            uint        TransactionIndex;
        }receipt_def;

         const h256 goResult{"06d13153a5d7562c6d79f3079687164ad79cf705e67830938f9a4f9670e1fb22"};
        std::vector<bytes> receipts;
        for (int i = 0; i < 4; i++)
        {
           receipt_def receipt;
           receipt.Type =  i;
          // receipt.PostState =  h256.data();
            receipt.Status = i;
            receipt.CumulativeGasUsed = i*2;
            receipt.Bloom =  h2048{"00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"};
            receipt.TxHash = h256{hasharray[i]};
            receipt.Address = h160{addressArray[i]};
            receipt.GasUsed = i*123;
            receipt.BlockHash = h256{blockHashArray[i]};
            receipt.TransactionIndex = i;
            
            bytes encoded = bytes();
            append(encoded, RLP::encode( receipt.Type));
            append(encoded, RLP::encode(receipt.Status));
            append(encoded, RLP::encode(receipt.CumulativeGasUsed));
            append(encoded, RLP::encode(bytes(receipt.Bloom.begin(), receipt.Bloom.end())));
            append(encoded, RLP::encode(bytes(receipt.TxHash.begin(), receipt.TxHash.end())));
            append(encoded, RLP::encode(bytes(receipt.Address.begin(),receipt.Address.end())));
            append(encoded, RLP::encode(receipt.GasUsed ));
            append(encoded, RLP::encode(bytes(receipt.BlockHash.begin(), receipt.BlockHash.end())));
            append(encoded, RLP::encode(receipt.TransactionIndex));
            receipts.push_back(encoded);
        }

        h256 receiptsRoot = orderedTrieRoot(receipts);
        std::cout << "MPT receipts Hash: " <<  receiptsRoot.hex()  << std::endl;
        ASSERT_EQ(receiptsRoot, goResult);

    }

}