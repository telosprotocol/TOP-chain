use crate::types::Address;
use crate::H256;

pub enum VersionPrefix {
    V1 = 0x1,
}

#[derive(Clone, Copy)]
pub enum KeyPrefix {
    Nonce = 0x1,
    Balance = 0x2,
    Code = 0x3,
    Storage = 0x4,
    Generation = 0x5,
}

// pub fn bytes_to_key(prefix: KeyPrefix, bytes: &[u8]) -> Vec<u8> {
//     [&[VersionPrefix::V1 as u8], &[prefix as u8], bytes].concat()
// }

pub fn address_to_key(prefix: KeyPrefix, address: &Address) -> [u8; 22] {
    let mut result = [0u8; 22];
    result[0] = VersionPrefix::V1 as u8;
    result[1] = prefix as u8;
    result[2..22].copy_from_slice(address.as_bytes());
    result
}

pub enum StorageKeyKind {
    Normal([u8; 54]),
    Generation([u8; 58]),
}

impl AsRef<[u8]> for StorageKeyKind {
    fn as_ref(&self) -> &[u8] {
        use StorageKeyKind::*;
        match &self {
            Normal(v) => v,
            Generation(v) => v,
        }
    }
}

pub fn storage_to_key(address: &Address, key: &H256, generation: u32) -> StorageKeyKind {
    if generation == 0 {
        StorageKeyKind::Normal(normal_storage_key(address, key))
    } else {
        StorageKeyKind::Generation(generation_storage_key(address, key, generation))
    }
}

fn normal_storage_key(address: &Address, key: &H256) -> [u8; 54] {
    let mut result = [0u8; 54];
    result[0] = VersionPrefix::V1 as u8;
    result[1] = KeyPrefix::Storage as u8;
    result[2..22].copy_from_slice(address.as_bytes());
    result[22..54].copy_from_slice(&key.0);
    result
}

fn generation_storage_key(address: &Address, key: &H256, generation: u32) -> [u8; 58] {
    let mut result = [0u8; 58];
    result[0] = VersionPrefix::V1 as u8;
    result[1] = KeyPrefix::Storage as u8;
    result[2..22].copy_from_slice(address.as_bytes());
    result[22..26].copy_from_slice(&generation.to_be_bytes());
    result[26..58].copy_from_slice(&key.0);
    result
}
