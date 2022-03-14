use crate::prelude::H256;
use crate::error;
use engine_types::account_id::AccountId;

/// Timestamp represented by the number of nanoseconds since the Unix Epoch.
#[derive(Debug, Copy, Clone, Eq, PartialEq, PartialOrd, Ord)]
pub struct Timestamp(u64);

impl Timestamp {
    pub fn new(ns: u64) -> Self {
        Self(ns)
    }

    pub fn nanos(&self) -> u64 {
        self.0
    }

    pub fn millis(&self) -> u64 {
        self.0 / 1_000_000
    }

    pub fn secs(&self) -> u64 {
        self.0 / 1_000_000_000
    }
}

pub trait Env {
    // fn signer_account_id(&self) -> AccountId;
    // fn current_account_id(&self) -> AccountId;
    fn predecessor_account_id(&self) -> AccountId;

    fn block_height(&self) -> u64;
    fn block_timestamp(&self) -> Timestamp;
    // fn attached_deposit(&self) -> u128; // todo
    fn random_seed(&self) -> H256;
    // fn prepaid_gas(&self) -> NearGas; // todo
}
