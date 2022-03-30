use crate::prelude::*;
use evm::backend::Log;

/// Borsh-encoded log for use in a `SubmitResult`.
#[derive(Debug, BorshSerialize, BorshDeserialize)]
pub struct ResultLog {
    pub address: Address,
    pub topics: Vec<RawU256>,
    pub data: Vec<u8>,
}

impl From<Log> for ResultLog {
    fn from(log: Log) -> Self {
        let topics = log
            .topics
            .into_iter()
            .map(|topic| topic.0)
            .collect::<Vec<_>>();
        ResultLog {
            address: Address::new(log.address),
            topics,
            data: log.data,
        }
    }
}

/// The status of a transaction.
#[derive(Debug, BorshSerialize, BorshDeserialize, PartialEq, Eq)]
pub enum TransactionStatus {
    Succeed(Vec<u8>),
    Revert(Vec<u8>),
    OutOfGas,
    OutOfFund,
    OutOfOffset,
}

impl TransactionStatus {
    pub fn is_ok(&self) -> bool {
        matches!(*self, TransactionStatus::Succeed(_))
    }

    pub fn is_revert(&self) -> bool {
        matches!(*self, TransactionStatus::Revert(_))
    }

    pub fn is_fail(&self) -> bool {
        *self == TransactionStatus::OutOfGas
            || *self == TransactionStatus::OutOfFund
            || *self == TransactionStatus::OutOfOffset
    }
}

impl AsRef<[u8]> for TransactionStatus {
    fn as_ref(&self) -> &[u8] {
        match self {
            Self::Succeed(_) => b"SUCCESS",
            Self::Revert(_) => b"ERR_REVERT",
            Self::OutOfGas => b"ERR_OUT_OF_GAS",
            Self::OutOfFund => b"ERR_OUT_OF_FUNDS",
            Self::OutOfOffset => b"ERR_OUT_OF_OFFSET",
        }
    }
}

/// Borsh-encoded parameters for the `call`, `call_with_args`, `deploy_code`,
/// and `deploy_with_input` methods.
#[derive(Debug, BorshSerialize, BorshDeserialize)]
pub struct SubmitResult {
    version: u8,
    pub status: TransactionStatus,
    pub gas_used: u64,
    pub logs: Vec<ResultLog>,
}

impl SubmitResult {
    /// Must be incremented when making breaking changes to the SubmitResult ABI.
    /// The current value of 7 is chosen because previously a `TransactionStatus` object
    /// was first in the serialization, which is an enum with less than 7 variants.
    /// Therefore, no previous `SubmitResult` would have began with a leading 7 byte,
    /// and this can be used to distinguish the new ABI (with version byte) from the old.
    const VERSION: u8 = 7;

    pub fn new(status: TransactionStatus, gas_used: u64, logs: Vec<ResultLog>) -> Self {
        Self {
            version: Self::VERSION,
            status,
            gas_used,
            logs,
        }
    }
}

/// Borsh-encoded parameters for the engine `call` function.
#[derive(BorshSerialize, BorshDeserialize, Debug, PartialEq, Eq, Clone)]
pub struct FunctionCallArgsV2 {
    pub contract: Address,
    /// Wei compatible Borsh-encoded value field to attach an ETH balance to the transaction
    pub value: WeiU256,
    pub input: Vec<u8>,
}

// impl FunctionCallArgsV2 {
//     pub fn deserialize(bytes: &[u8]) -> Option<Self> {
//         if let Ok(value) = Self::try_from_slice(bytes) {
//             Some(value)
//         } else {
//             None
//         }
//     }
// }
#[derive(BorshSerialize, BorshDeserialize, Debug, PartialEq, Eq, Clone)]
pub enum CallArgs {
    V2(FunctionCallArgsV2),
    // V1(FunctionCallArgsV1),
}

impl CallArgs {
    pub fn deserialize(bytes: &[u8]) -> Option<Self> {
        // For handling new input format (wrapped into call args enum) - for data structures with new arguments,
        // made for flexibility and extensibility.
        if let Ok(value) = Self::try_from_slice(bytes) {
            Some(value)
            // Fallback, for handling old input format,
            // i.e. input, formed as a raw (not wrapped into call args enum) data structure with legacy arguments,
            // made for backward compatibility.
            // } else if let Ok(value) = FunctionCallArgsV1::try_from_slice(bytes) {
            //     Some(Self::V1(value))
            // Dealing with unrecognized input should be handled and result as an exception in a call site.
        } else {
            None
        }
    }
}
/// Borsh-encoded parameters for the `view` function.
#[derive(BorshSerialize, BorshDeserialize, Debug, Eq, PartialEq)]
pub struct ViewCallArgs {
    pub sender: Address,
    pub address: Address,
    pub amount: RawU256,
    pub input: Vec<u8>,
}

/// Borsh-encoded parameters for the `get_storage_at` function.
#[derive(BorshSerialize, BorshDeserialize)]
pub struct GetStorageAtArgs {
    pub address: Address,
    pub key: RawH256,
}
