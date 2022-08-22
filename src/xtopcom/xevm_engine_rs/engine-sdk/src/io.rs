use crate::error;
use engine_types::{proto_precompile::ContractBridgeArgs, PrecompileResult, U256};

/// The purpose of this trait is to represent a reference to a value that
/// could be obtained by IO, but without eagerly loading it into memory.
pub trait StorageIntermediate: Sized {
    fn len(&self) -> usize;
    fn is_empty(&self) -> bool;
    fn copy_to_slice(&self, buffer: &mut [u8]);

    fn to_vec(&self) -> Vec<u8> {
        let size = self.len();
        let mut buf = vec![0u8; size];
        self.copy_to_slice(&mut buf);
        buf
    }
}

/// Trait for reading/writing values from storage and a generalized `stdin`/`stdout`.
pub trait IO {
    /// A type giving a reference to a value obtained by IO without loading it
    /// into memory.
    type StorageValue: StorageIntermediate;

    /// Read bytes that were passed as input to the process. This can be thought of as a
    /// generalization of `stdin` or command-line arguments.
    fn read_input(&self) -> Self::StorageValue;

    /// Return a value to an external process.
    fn return_output(&mut self, value: &[u8]);

    /// Return error code and used gas;
    fn return_error(&mut self, ec_gas: (u32, u64));

    /// Read the value in storage at the given key, if any.
    fn read_storage(&self, key: &[u8]) -> Option<Self::StorageValue>;

    /// Check if there is a value in storage at the given key, but do not read the value.
    /// Equivalent to `self.read_storage(key).is_some()` but more efficient.
    fn storage_has_key(&self, key: &[u8]) -> bool;

    /// Write the given value to storage under the given key. Returns a reference to the old
    /// value stored at that key (if any).
    fn write_storage(&mut self, key: &[u8], value: &[u8]) -> Option<Self::StorageValue>;

    /// Remove entry from storage and capture the value present at the given key (if any)
    fn remove_storage(&mut self, key: &[u8]) -> Option<Self::StorageValue>;

    /// Read the length of the bytes stored at the given key.
    fn read_storage_len(&self, key: &[u8]) -> Option<usize> {
        self.read_storage(key).map(|s| s.len())
    }

    /// Convenience function to read a 64-bit unsigned integer from storage
    /// (assumes big-endian encoding).
    fn read_u64(&self, key: &[u8]) -> Result<u64, error::ReadU64Error> {
        let value = self
            .read_storage(key)
            .ok_or(error::ReadU64Error::MissingValue)?;

        if value.len() != 8 {
            return Err(error::ReadU64Error::InvalidU64);
        }

        let mut result = [0u8; 8];
        value.copy_to_slice(&mut result);
        Ok(u64::from_be_bytes(result))
    }

    /// Convenience function to read a 256-bit unsigned integer from storage
    /// (assumes rlp encoding).
    fn read_u256(&self, key: &[u8]) -> Result<U256, error::ReadU256Error> {
        let value = self
            .read_storage(key)
            .ok_or(error::ReadU256Error::MissingValue)?;

        rlp::decode(value.to_vec().as_slice()).map_err(|_| error::ReadU256Error::InvalidU256)
    }
}

pub trait ContractBridge {
    type StorageValue: StorageIntermediate;

    fn extern_contract_call(args: ContractBridgeArgs) -> PrecompileResult;

    fn get_result() -> Option<Self::StorageValue>;

    fn get_error() -> Option<Self::StorageValue>;

    fn engine_return(&self, engine_ptr: u64);
    fn executor_return(&self, executor_ptr: u64);
}
