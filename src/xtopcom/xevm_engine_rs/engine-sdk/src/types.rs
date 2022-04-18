use crate::{io::IO, prelude::H256};

pub fn keccak(input: &[u8]) -> H256 {
    unsafe {
        super::exports::evm_keccak256(input.len() as u64, input.as_ptr() as u64, 1);
        let bytes = H256::zero();
        super::exports::evm_read_register(1, bytes.0.as_ptr() as *const u64 as u64);
        bytes
    }
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

pub trait ErrOutput {
    fn err_output(&self) -> (u32, u64);
}

// # sdk_process
// process Result<>
// return rust evm execute result back to C
// use by interface extern "C" pub function
// Ok -  true - success
// Err - false - error
pub trait SdkProcess<T> {
    fn sdk_process(self) -> bool;
}
impl<T, E> SdkProcess<T> for Result<T, E>
where
    T: AsRef<[u8]>,
    E: AsRef<[u8]> + ErrOutput,
{
    fn sdk_process(self) -> bool {
        match self {
            Ok(r) => {
                crate::runtime::Runtime.return_output(r.as_ref());
                true
            }
            Err(e) => {
                crate::log_utf8(e.as_ref());
                crate::runtime::Runtime.return_error(e.err_output());
                false
            }
        }
    }
}
