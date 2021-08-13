#pragma once
#include "Address.h"
#include "Data.h"

class Transaction {
  public:
    static const uint16_t TRANSACTION_SIZE = 12;
    std::vector<uint8_t> from;
    std::vector<uint8_t> to;
    uint16_t tx_type;
    uint64_t amount;
    uint32_t extra;    
    uint32_t tx_deposit;
    uint16_t source_action_type;
    uint16_t target_action_type;
    uint64_t last_tx_nonce;
    std::vector<uint8_t> m_last_tx_hash;
    std::vector<uint8_t> note;
    /// Transaction signature.
    std::vector<uint8_t> signature;

    Transaction(const Data& from, const Data& to, uint16_t tx_type, uint64_t amount, uint32_t extra, 
                uint32_t tx_deposit, uint16_t source_action_type, uint16_t target_action_type,
                uint64_t last_tx_nonce, const Data& last_tx_hash, const Data& note)
        : from(std::move(from))
        , to(std::move(to))
        , tx_type(std::move(tx_type))
        , amount(std::move(amount))
        , extra(std::move(extra))        
        , tx_deposit(std::move(tx_deposit))
        , source_action_type(std::move(source_action_type))
        , target_action_type(std::move(target_action_type))
        , last_tx_nonce(std::move(last_tx_nonce))
        , m_last_tx_hash(std::move(last_tx_hash))
        , note(std::move(note)) 
        {
            if (m_last_tx_hash.size() % 2 != 0 && m_last_tx_hash.size() > 2)
                m_last_tx_hash.insert(m_last_tx_hash.begin()+2, '0');
        }

  public:
    /// Encodes the transaction.
    Data encode() const;
};

