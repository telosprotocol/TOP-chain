pub mod address;
pub mod balance;
pub mod gas;
pub mod wei;

pub use address::*;
pub use balance::*;
pub use gas::*;
pub use wei::*;

pub type RawU256 = [u8; 32];
// Big-endian large integer type.
pub type RawH256 = [u8; 32]; // Unformatted binary data of fixed length.

// pub type StorageUsage = u64;
