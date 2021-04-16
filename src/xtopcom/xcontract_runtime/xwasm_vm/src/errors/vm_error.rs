use thiserror::Error;

#[derive(Error, Debug)]
#[non_exhaustive]
pub enum VmError {
    #[error("Error compiling Wasm: {msg}")]
    CompileErr {
        msg: String,
        #[cfg(feature = "backtraces")]
        backtrace: Backtrace,
    },
}

impl VmError {
    pub(crate) fn compile_err<S: Into<String>>(msg: S) -> Self {
        VmError::CompileErr {
            msg: msg.into(),
            #[cfg(feature = "backtraces")]
            backtrace: Backtrace::capture(),
        }
    }
}
impl From<wasmer::CompileError> for VmError {
    fn from(original: wasmer::CompileError) -> Self {
        VmError::compile_err(format!("Could not compile: {}", original))
    }
}
