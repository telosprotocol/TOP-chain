use core::panic;

use crate::{
    io::IO,
    prelude::{Address, H256},
};

pub fn keccak(input: &[u8]) -> H256 {
    unsafe {
        super::exports::evm_keccak256(input.len() as u64, input.as_ptr() as u64, 1);
        let bytes = H256::zero();
        super::exports::evm_read_register(1, bytes.0.as_ptr() as *const u64 as u64);
        bytes
    }
}

pub fn top_account_to_evm_address(addr: &[u8]) -> Address {
    if (addr.len() == 20) {
        Address::try_from_slice(&addr).unwrap()
    } else if (addr.len() == 22) { // suppose 0x... or T6...
        Address::try_from_slice(&addr[2..]).unwrap()
    } else {
        unreachable!()
    }
    // Address::try_from_slice(&keccak(addr)[12..]).unwrap()
}

// # sdk_expect()
pub trait SdkExpect<T> {
    fn sdk_expect(self, msg: &str) -> T;
}
impl<T> SdkExpect<T> for Option<T> {
    fn sdk_expect(self, msg: &str) -> T {
        match self {
            Some(t) => t,
            None => crate::panic_utf8(msg.as_ref()),
        }
    }
}
impl<T, E> SdkExpect<T> for core::result::Result<T, E> {
    fn sdk_expect(self, msg: &str) -> T {
        match self {
            Ok(t) => t,
            Err(_) => crate::panic_utf8(msg.as_ref()),
        }
    }
}

// # sdk_unwrap()
pub trait SdkUnwrap<T> {
    fn sdk_unwrap(self) -> T;
}
impl<T> SdkUnwrap<T> for Option<T> {
    fn sdk_unwrap(self) -> T {
        match self {
            Some(t) => t,
            None => crate::panic_utf8("ERR_UNWRAP".as_bytes()),
        }
    }
}
impl<T, E: AsRef<[u8]>> SdkUnwrap<T> for core::result::Result<T, E> {
    fn sdk_unwrap(self) -> T {
        match self {
            Ok(t) => t,
            Err(e) => crate::panic_utf8(e.as_ref()),
        }
    }
}

// # sdk_process
pub trait SdkProcess<T> {
    fn sdk_process(self);
}
impl<T: AsRef<[u8]>, E: AsRef<[u8]>> SdkProcess<T> for Result<T, E> {
    fn sdk_process(self) {
        match self {
            Ok(r) => crate::runtime::Runtime.return_output(r.as_ref()),
            Err(e) => crate::panic_utf8(e.as_ref()),
        }
    }
}
