use engine_types::{Vec, H160, H256};
use rlp::{Decodable, DecoderError, Rlp};

/// Type indicator (per EIP-2718) for access list transactions
pub const EIP_2930_TYPE_BYTE: u8 = 0x01;
pub const EIP_1559_TYPE_BYTE: u8 = 0x02;

#[derive(Debug, Eq, PartialEq, Clone)]
pub struct AccessTuple {
    pub address: H160,
    pub storage_keys: Vec<H256>,
}

impl Decodable for AccessTuple {
    fn decode(rlp: &Rlp<'_>) -> Result<Self, DecoderError> {
        let address = rlp.val_at(0)?;
        let storage_keys = rlp.list_at(1)?;

        Ok(Self {
            address,
            storage_keys,
        })
    }
}
