mod vm_error;

pub use vm_error::VmError;

pub type VmResult<T> = core::result::Result<T, VmError>;
