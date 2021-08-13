// Copyright © 2017-2021 Trust Wallet.
//
// This file is part of Trust. The full Trust copyright notice, including
// terms governing use, modification, and redistribution, is contained in the
// file LICENSE at the root of the source code distribution tree.

#include "Address.h"
#include "xpbase/base/top_utils.h"

bool Address::isValid(const std::string& string) {
    // TODO: Finalize implementation
    if (string.size() != Address::size*2 + TOP_PREFIX.size() || string.substr(0, TOP_PREFIX.size()) != TOP_PREFIX) {
        return false;
    }
    return true;    
}

Address::Address(const std::string& string) {
    // TODO: Finalize implementation
    if (!isValid(string)) {
        throw std::invalid_argument("Invalid address data");
    }    
    std::string data = top::HexDecode(string.substr(TOP_PREFIX.size()));
    std::copy(data.begin(), data.end(), bytes.begin());    
}

std::string Address::string() const {
    // TODO: Finalize implementation
    return TOP_PREFIX + top::HexEncode(std::string((char*)bytes.data(), bytes.size()));
}
