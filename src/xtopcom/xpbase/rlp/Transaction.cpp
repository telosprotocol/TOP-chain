// Copyright © 2017-2020 Trust Wallet.
//
// This file is part of Trust. The full Trust copyright notice, including
// terms governing use, modification, and redistribution, is contained in the
// file LICENSE at the root of the source code distribution tree.

#include "Transaction.h"
#include "RLP.h"

Data Transaction::encode() const {
    auto encoded = Data();
    append(encoded, Ethereum::RLP::encode(from));
    append(encoded, Ethereum::RLP::encode(to));
    append(encoded, Ethereum::RLP::encode(tx_type));
    append(encoded, Ethereum::RLP::encode(amount));
    append(encoded, Ethereum::RLP::encode(extra));
    append(encoded, Ethereum::RLP::encode(tx_deposit));
    append(encoded, Ethereum::RLP::encode(source_action_type));
    append(encoded, Ethereum::RLP::encode(target_action_type));
    append(encoded, Ethereum::RLP::encode(last_tx_nonce));
    append(encoded, Ethereum::RLP::encode(m_last_tx_hash));
    append(encoded, Ethereum::RLP::encode(note));
    if (!signature.empty()) {
        append(encoded, Ethereum::RLP::encode(signature));
    }
    return Ethereum::RLP::encodeList(encoded);
}
