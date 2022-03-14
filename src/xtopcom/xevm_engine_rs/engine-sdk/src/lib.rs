#![allow(unused)]

use crate::prelude::Address;
use crate::prelude::H256;

pub mod dup_cache;
pub mod env;
pub mod error;
pub mod io;
mod prelude;
pub mod runtime;
pub mod types;

use runtime::exports;

#[cfg(not(feature = "top_crypto"))]
use sha3::{Digest, Keccak256};

const ECRECOVER_MESSAGE_SIZE: u64 = 32;
const ECRECOVER_SIGNATURE_LENGTH: u64 = 64;
const ECRECOVER_MALLEABILITY_FLAG: u64 = 1;

pub fn panic_utf8(bytes: &[u8]) -> ! {
    unsafe {
        exports::evm_log_utf8(bytes.len() as u64, bytes.as_ptr() as u64);
    }
    unreachable!()
}

pub fn log_utf8(bytes: &[u8]) {
    unsafe {
        exports::evm_log_utf8(bytes.len() as u64, bytes.as_ptr() as u64);
    }
}

pub fn log(data: &str) {
    log_utf8(data.as_bytes())
}

pub fn sha256(input: &[u8]) -> H256 {
    unsafe {
        exports::evm_sha256(input.len() as u64, input.as_ptr() as u64, 1);
        let bytes = H256::zero();
        exports::evm_read_register(1, bytes.0.as_ptr() as *const u64 as u64);
        bytes
    }
}

#[cfg(feature = "top_crypto")]
pub fn keccak(input: &[u8]) -> H256 {
    unsafe {
        exports::evm_keccak256(input.len() as u64, input.as_ptr() as u64, 1);
        let bytes = H256::zero();
        exports::evm_read_register(1, bytes.0.as_ptr() as *const u64 as u64);
        bytes
    }
}

#[cfg(not(feature = "top_crypto"))]
pub fn keccak(data: &[u8]) -> H256 {
    H256::from_slice(Keccak256::digest(data).as_slice())
}

pub fn ripemd160(input: &[u8]) -> [u8; 20] {
    unsafe {
        const REGISTER_ID: u64 = 1;
        exports::evm_ripemd160(input.len() as u64, input.as_ptr() as u64, REGISTER_ID);
        let bytes = [0u8; 20];
        exports::evm_read_register(REGISTER_ID, bytes.as_ptr() as u64);
        bytes
    }
}

pub fn ecrecover(hash: H256, signature: &[u8]) -> Result<Address, ECRecoverErr> {
    unsafe {
        let hash_ptr = hash.as_ptr() as u64;
        let sig_ptr = signature.as_ptr() as u64;
        const RECOVER_REGISTER_ID: u64 = 1;
        const KECCACK_REGISTER_ID: u64 = 2;
        let result = exports::evm_ecrecover(
            ECRECOVER_MESSAGE_SIZE,
            hash_ptr,
            ECRECOVER_SIGNATURE_LENGTH,
            sig_ptr,
            signature[64] as u64,
            ECRECOVER_MALLEABILITY_FLAG,
            RECOVER_REGISTER_ID,
        );
        if result == (true as u64) {
            // The result from the ecrecover call is in a register; we can use this
            // register directly for the input to keccak256. This is why the length is
            // set to `u64::MAX`.
            exports::evm_keccak256(u64::MAX, RECOVER_REGISTER_ID, KECCACK_REGISTER_ID);
            let keccak_hash_bytes = [0u8; 32];
            exports::evm_read_register(KECCACK_REGISTER_ID, keccak_hash_bytes.as_ptr() as u64);
            Ok(Address::try_from_slice(&keccak_hash_bytes[12..]).map_err(|_| ECRecoverErr)?)
        } else {
            Err(ECRecoverErr)
        }
    }
}

#[macro_export]
macro_rules! log {
    ($e: expr) => {
        #[cfg(feature = "log")]
        $crate::log($e)
    };
}

pub struct ECRecoverErr;

impl ECRecoverErr {
    pub fn as_str(&self) -> &'static str {
        "ERR_ECRECOVER"
    }
}

impl AsRef<[u8]> for ECRecoverErr {
    fn as_ref(&self) -> &[u8] {
        self.as_str().as_bytes()
    }
}
