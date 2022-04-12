// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xeth_header.h"
#include "difficulty.h"
#include "xstore.h"
#include "util.h"
#include "xevm_common/common.h"
NS_BEG4(top, xvm, system_contracts, xeth)

using namespace top::evm_common;

class xeth_bridge_t {
    #define ETH_CHAIN_ID 1
    #define CONFIRM_HEIGHTS 25
public:
    xeth_bridge_t(){
    }

    bool init_genesis_block_header(std::string headerContent, std::string emitter);
    bool sync_block_header(std::string headerContent);
    uint64_t getCurrentHeightOfMainChain(uint64_t chainID);
    uint8_t* getHashOfMainChainByHeight(uint64_t chainID, int64_t height);
    bool getHeaderIfHeightConfirmed(int64_t height, xeth_block_header_t header, uint64_t chainID);
    bool verify(xeth_block_header_t prev_header, xeth_block_header_t new_header);

private:
    bool validateOwner(std::string owner);
    bool isLondonFork(int64_t height);
    // VerifyGaslimit verifies the header gas limit according increase/decrease
    // in relation to the parent gas limit.
    bool VerifyGaslimit(u256 parentGasLimit, u256 headerGasLimit);
    bigint CalcBaseFee(xeth_block_header_t &parentHeader);
private:
    store m_store;
    std::string owner;
};

NS_END4
