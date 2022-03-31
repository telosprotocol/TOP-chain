use crate::{format, String, H160};
use serde::de::Visitor;
use serde::{Deserialize, Serialize};
/// Base Eth Address type
#[derive(Debug, Clone, Copy, Eq, PartialEq, Ord, PartialOrd)]
pub struct Address(H160);

impl Address {
    /// Construct Address from H160
    pub const fn new(val: H160) -> Self {
        Self(val)
    }

    /// Get raw H160 data
    pub fn raw(&self) -> H160 {
        self.0
    }

    /// Encode address to string
    pub fn encode(&self) -> String {
        hex::encode(self.0.as_bytes())
    }

    pub fn decode(address: &str) -> Result<Address, error::AddressError> {
        if address.len() != 40 {
            return Err(error::AddressError::IncorrectLength);
        }
        let mut result = [0u8; 20];
        hex::decode_to_slice(address, &mut result)
            .map_err(|_| error::AddressError::FailedDecodeHex)?;
        Ok(Address::new(H160(result)))
    }

    pub fn as_bytes(&self) -> &[u8] {
        self.0.as_bytes()
    }

    pub fn try_from_slice(raw_addr: &[u8]) -> Result<Self, error::AddressError> {
        if raw_addr.len() != 20 {
            return Err(error::AddressError::IncorrectLength);
        }
        Ok(Self::new(H160::from_slice(raw_addr)))
    }

    pub fn from_array(array: [u8; 20]) -> Self {
        Self(H160(array))
    }

    pub const fn zero() -> Self {
        Address::new(H160([0u8; 20]))
    }
}

impl TryFrom<&[u8]> for Address {
    type Error = error::AddressError;

    fn try_from(raw_addr: &[u8]) -> Result<Self, Self::Error> {
        Self::try_from_slice(raw_addr).map_err(|_| error::AddressError::IncorrectLength)
    }
}

impl Serialize for Address {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: serde::Serializer,
    {
        serializer.serialize_bytes(self.0.as_bytes())
    }
}

impl<'de> Deserialize<'de> for Address {
    fn deserialize<D>(deserializer: D) -> Result<Self, D::Error>
    where
        D: serde::Deserializer<'de>,
    {
        struct H160Visitor;

        impl<'de> Visitor<'de> for H160Visitor {
            type Value = Address;

            fn expecting(&self, formatter: &mut std::fmt::Formatter) -> std::fmt::Result {
                formatter.write_fmt(format_args!("expected a valid Address"))
            }

            fn visit_bytes<E>(self, v: &[u8]) -> Result<Self::Value, E>
            where
                E: serde::de::Error,
            {
                if v.len() != 20 {
                    return Err(serde::de::Error::custom(format!(
                        "{}, len: {}",
                        error::AddressError::IncorrectLength,
                        v.len()
                    )));
                }
                let address = Address(H160::from_slice(&v));
                Ok(address)
            }
        }

        deserializer.deserialize_bytes(H160Visitor)
    }
}

impl Default for Address {
    fn default() -> Self {
        Address::zero()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_address_serializer() {
        let eth_address = "096DE9C2B8A5B8c22cEe3289B101f6960d68E51E";
        let address = Address::new(H160::from_slice(&hex::decode(eth_address).unwrap()[..]));
        let serialized_addr = bincode::serialize(&address).unwrap();

        assert_eq!(serialized_addr.len(), 28); // size(20) 0 0 0 0 0 0 0 + addr(*20)

        let de_addr = bincode::deserialize::<Address>(serialized_addr.as_slice()).unwrap();
        assert_eq!(de_addr, address);

        // let addr = Address::try_from_slice(&serialized_addr).unwrap();
        let addr = Address::try_from_slice(&serialized_addr[8..]).unwrap();
        assert_eq!(addr.encode(), eth_address.to_lowercase());
    }

    #[test]
    fn test_wrong_address_19() {
        let serialized_addr = [0u8; 19];
        let addr = Address::try_from_slice(&serialized_addr);
        let err = addr.unwrap_err();
        matches!(err, error::AddressError::IncorrectLength);
    }
}

pub mod error {
    use crate::{fmt, String};

    #[derive(Eq, Hash, Clone, Debug, PartialEq)]
    pub enum AddressError {
        FailedDecodeHex,
        IncorrectLength,
    }

    impl AsRef<[u8]> for AddressError {
        fn as_ref(&self) -> &[u8] {
            match self {
                Self::FailedDecodeHex => b"FAILED_DECODE_ETH_ADDRESS",
                Self::IncorrectLength => b"ETH_WRONG_ADDRESS_LENGTH",
            }
        }
    }

    impl fmt::Display for AddressError {
        fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
            let msg = String::from_utf8(self.as_ref().to_vec()).unwrap();
            write!(f, "{}", msg)
        }
    }
}
