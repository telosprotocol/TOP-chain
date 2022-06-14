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
    #[error("Ran out of gas during contract execution")]
    GasDepletion {
        #[cfg(feature = "backtraces")]
        backtrace: Backtrace,
    },
    #[error("Error instantiating a Wasm module: {msg}")]
    InstantiationErr {
        msg: String,
        #[cfg(feature = "backtraces")]
        backtrace: Backtrace,
    },

    #[error("Error resolving Wasm function: {}", msg)]
    ResolveErr {
        msg: String,
        #[cfg(feature = "backtraces")]
        backtrace: Backtrace,
    },
    #[error(
        "Unexpected number of result values when calling '{}'. Expected: {}, actual: {}.",
        function_name,
        expected,
        actual
    )]
    ResultMismatch {
        function_name: String,
        expected: usize,
        actual: usize,
        #[cfg(feature = "backtraces")]
        backtrace: Backtrace,
    },
    #[error("Error executing Wasm: {}", msg)]
    RuntimeErr {
        msg: String,
        #[cfg(feature = "backtraces")]
        backtrace: Backtrace,
    },
    #[error("Uninitialized Context Data: {}", kind)]
    UninitializedContextData {
        kind: String,
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
    pub(crate) fn gas_depletion() -> Self {
        VmError::GasDepletion {
            #[cfg(feature = "backtraces")]
            backtrace: Backtrace::capture(),
        }
    }

    pub(crate) fn instantiation_err<S: Into<String>>(msg: S) -> Self {
        VmError::InstantiationErr {
            msg: msg.into(),
            #[cfg(feature = "backtraces")]
            backtrace: Backtrace::capture(),
        }
    }

    pub(crate) fn resolve_err<S: Into<String>>(msg: S) -> Self {
        VmError::ResolveErr {
            msg: msg.into(),
            #[cfg(feature = "backtraces")]
            backtrace: Backtrace::capture(),
        }
    }
    pub(crate) fn result_mismatch<S: Into<String>>(
        function_name: S,
        expected: usize,
        actual: usize,
    ) -> Self {
        VmError::ResultMismatch {
            function_name: function_name.into(),
            expected,
            actual,
            #[cfg(feature = "backtraces")]
            backtrace: Backtrace::capture(),
        }
    }
    pub(crate) fn runtime_err<S: Into<String>>(msg: S) -> Self {
        VmError::RuntimeErr {
            msg: msg.into(),
            #[cfg(feature = "backtraces")]
            backtrace: Backtrace::capture(),
        }
    }

    pub(crate) fn uninitialized_context_data<S: Into<String>>(kind: S) -> Self {
        VmError::UninitializedContextData {
            kind: kind.into(),
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
impl From<wasmer::ExportError> for VmError {
    fn from(original: wasmer::ExportError) -> Self {
        VmError::resolve_err(format!("Could not get export: {}", original))
    }
}
impl From<wasmer::RuntimeError> for VmError {
    fn from(original: wasmer::RuntimeError) -> Self {
        VmError::runtime_err(format!("Wasmer runtime error: {}", original))
    }
}
