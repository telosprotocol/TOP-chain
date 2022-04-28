use engine_types::basic::*;
use engine_types::proto_basic::{ProtoAddress, RawU256, ResultLog};

use crate::proto_parameters::*;

/// The status of a transaction.
/// same as TransactionStatus in `xevm_common/xevm_transaction_result.h`
#[derive(Debug, PartialEq, Eq)]
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

    pub fn as_u32(&self) -> u32 {
        match self {
            TransactionStatus::Succeed(_) => 0,
            TransactionStatus::Revert(_) => 1,
            TransactionStatus::OutOfGas => 2,
            TransactionStatus::OutOfFund => 3,
            TransactionStatus::OutOfOffset => 4,
        }
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

impl SubmitResult {
    const CURRENT_CALL_ARGS_VERSION: u32 = 1;

    pub fn new_proto(status: TransactionStatus, gas_used: u64, logs: Vec<ResultLog>) -> Self {
        let mut res = SubmitResult::new();
        res.set_version(Self::CURRENT_CALL_ARGS_VERSION);
        res.set_transaction_status(status.as_u32());

        match status {
            TransactionStatus::Succeed(data) => res.set_status_data(data),
            TransactionStatus::Revert(data) => res.set_status_data(data),
            _ => (),
        }
        res.set_gas_used(gas_used);
        res.set_logs(logs.into());
        res
    }
}
