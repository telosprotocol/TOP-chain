use engine_precompiles::secp256k1::ecrecover;
use engine_sdk as sdk;
use engine_types::types::{Address, Wei};
use engine_types::{Vec, U256};
use rlp::{Decodable, DecoderError, Encodable, Rlp, RlpStream};

#[derive(Debug, Eq, PartialEq, Clone)]
pub struct TransactionLegacy {
    /// A monotonically increasing transaction counter for this sender
    pub nonce: U256,
    /// The fee the sender pays per unit of gas
    pub gas_price: U256,
    /// The maximum amount of gas units consumed by the transaction
    pub gas_limit: U256,
    /// The receiving address (`None` for the zero address)
    pub to: Option<Address>,
    /// The amount of ETH to transfer
    pub value: Wei,
    /// Arbitrary binary data for a contract call invocation
    pub data: Vec<u8>,
}

impl TransactionLegacy {
    pub fn rlp_append_unsigned(&self, s: &mut RlpStream, chain_id: Option<u64>) {
        s.begin_list(if chain_id.is_none() { 6 } else { 9 });
        s.append(&self.nonce);
        s.append(&self.gas_price);
        s.append(&self.gas_limit);
        match self.to.as_ref() {
            None => s.append(&""),
            Some(address) => s.append(&address.raw()),
        };
        s.append(&self.value.raw());
        s.append(&self.data);
        if let Some(chain_id) = chain_id {
            s.append(&chain_id);
            s.append(&0u8);
            s.append(&0u8);
        }
    }

    /// Returns self.gas as a u64, or None if self.gas > u64::MAX
    #[allow(unused)]
    pub fn get_gas_limit(&self) -> Option<u64> {
        self.gas_limit.try_into().ok()
    }

    pub fn normalize(self) -> super::NormalizedEthTransaction {
        super::NormalizedEthTransaction {
            address: None,
            chain_id: None,
            nonce: self.nonce,
            gas_limit: self.gas_limit,
            max_priority_fee_per_gas: self.gas_price,
            max_fee_per_gas: self.gas_price,
            to: self.to,
            value: self.value,
            data: self.data,
            access_list: Vec::new(),
        }
    }
}

#[derive(Debug, Eq, PartialEq, Clone)]
pub struct LegacyEthSignedTransaction {
    /// The unsigned transaction data
    pub transaction: TransactionLegacy,
    /// The ECDSA recovery ID
    pub v: u64,
    /// The first ECDSA signature output
    pub r: U256,
    /// The second ECDSA signature output
    pub s: U256,
}

impl LegacyEthSignedTransaction {
    /// Returns sender of given signed transaction by doing ecrecover on the signature.
    pub fn sender(&self) -> Option<Address> {
        let mut rlp_stream = RlpStream::new();
        // See details of CHAIN_ID computation here - https://github.com/ethereum/EIPs/blob/master/EIPS/eip-155.md#specification
        let (chain_id, rec_id) = match self.v {
            0..=26 => return None,
            27..=28 => (None, (self.v - 27) as u8),
            29..=34 => return None,
            _ => (Some((self.v - 35) / 2), ((self.v - 35) % 2) as u8),
        };
        self.transaction
            .rlp_append_unsigned(&mut rlp_stream, chain_id);
        let message_hash = sdk::keccak(rlp_stream.as_raw());
        ecrecover(message_hash, &super::vrs_to_arr(rec_id, self.r, self.s)).ok()
    }

    /// Returns chain id encoded in `v` parameter of the signature if that was done, otherwise None.
    pub fn chain_id(&self) -> Option<u64> {
        match self.v {
            0..=34 => None,
            _ => Some((self.v - 35) / 2),
        }
    }
}

impl Encodable for LegacyEthSignedTransaction {
    fn rlp_append(&self, s: &mut RlpStream) {
        s.begin_list(9);
        s.append(&self.transaction.nonce);
        s.append(&self.transaction.gas_price);
        s.append(&self.transaction.gas_limit);
        match self.transaction.to.as_ref() {
            None => s.append(&""),
            Some(address) => s.append(&address.raw()),
        };
        s.append(&self.transaction.value.raw());
        s.append(&self.transaction.data);
        s.append(&self.v);
        s.append(&self.r);
        s.append(&self.s);
    }
}

impl Decodable for LegacyEthSignedTransaction {
    fn decode(rlp: &Rlp<'_>) -> Result<Self, DecoderError> {
        if rlp.item_count() != Ok(9) {
            return Err(rlp::DecoderError::RlpIncorrectListLen);
        }
        let nonce = rlp.val_at(0)?;
        let gas_price = rlp.val_at(1)?;
        let gas = rlp.val_at(2)?;
        let to = super::rlp_extract_to(rlp, 3)?;
        let value = Wei::new(rlp.val_at(4)?);
        let data = rlp.val_at(5)?;
        let v = rlp.val_at(6)?;
        let r = rlp.val_at(7)?;
        let s = rlp.val_at(8)?;
        Ok(Self {
            transaction: TransactionLegacy {
                nonce,
                gas_price,
                gas_limit: gas,
                to,
                value,
                data,
            },
            v,
            r,
            s,
        })
    }
}
