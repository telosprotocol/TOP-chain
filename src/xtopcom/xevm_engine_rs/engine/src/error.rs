use evm::{ExitError, ExitFatal};

#[derive(Debug)]
pub enum EngineStateError {
    NotFound,
    DeserializationFailed,
}

impl AsRef<[u8]> for EngineStateError {
    fn as_ref(&self) -> &[u8] {
        match self {
            Self::NotFound => b"ERR_STATE_NOT_FOUND",
            Self::DeserializationFailed => b"ERR_STATE_CORRUPTED",
        }
    }
}

#[derive(Debug, Clone, Eq, PartialEq)]
pub struct EngineError {
    pub kind: EngineErrorKind,
    pub gas_used: u64,
}

impl From<EngineErrorKind> for EngineError {
    fn from(kind: EngineErrorKind) -> Self {
        Self { kind, gas_used: 0 }
    }
}

impl AsRef<[u8]> for EngineError {
    fn as_ref(&self) -> &[u8] {
        self.kind.as_bytes()
    }
}

/// todo add notes
impl engine_sdk::types::ErrOutput for EngineError {
    fn err_output(&self) -> (u32, u64) {
        (self.kind.as_error_code(), self.gas_used)
    }
}

/// #### Errors with the EVM engine.
/// here is regular error code exit from rust evm interface
/// should be defined in same order as `xcontract_runtime/xerror/xerror.h` `enum xenum_errc`
/// use `as_error_code()` to convert into u32 than pass to C and add `evm_vm_ec_begin`
#[derive(Debug, Clone, Eq, PartialEq)]
pub enum EngineErrorKind {
    EvmError(ExitError),
    EvmFatal(ExitFatal),
    IncorrectArgs,
    IncorrectNonce,
}

impl EngineErrorKind {
    pub fn with_gas_used(self, gas_used: u64) -> EngineError {
        EngineError {
            kind: self,
            gas_used,
        }
    }

    pub fn as_bytes(&self) -> &[u8] {
        use EngineErrorKind::*;
        match self {
            EvmError(ExitError::StackUnderflow) => b"ERR_STACK_UNDERFLOW",
            EvmError(ExitError::StackOverflow) => b"ERR_STACK_OVERFLOW",
            EvmError(ExitError::InvalidJump) => b"ERR_INVALID_JUMP",
            EvmError(ExitError::InvalidRange) => b"ERR_INVALID_RANGE",
            EvmError(ExitError::DesignatedInvalid) => b"ERR_DESIGNATED_INVALID",
            EvmError(ExitError::CallTooDeep) => b"ERR_CALL_TOO_DEEP",
            EvmError(ExitError::CreateCollision) => b"ERR_CREATE_COLLISION",
            EvmError(ExitError::CreateContractLimit) => b"ERR_CREATE_CONTRACT_LIMIT",
            EvmError(ExitError::OutOfOffset) => b"ERR_OUT_OF_OFFSET",
            EvmError(ExitError::OutOfGas) => b"ERR_OUT_OF_GAS",
            EvmError(ExitError::OutOfFund) => b"ERR_OUT_OF_FUND",
            EvmError(ExitError::Other(m)) => m.as_bytes(),
            EvmError(_) => unreachable!(), // unused misc
            EvmFatal(ExitFatal::NotSupported) => b"ERR_NOT_SUPPORTED",
            EvmFatal(ExitFatal::UnhandledInterrupt) => b"ERR_UNHANDLED_INTERRUPT",
            EvmFatal(ExitFatal::Other(m)) => m.as_bytes(),
            EvmFatal(_) => unreachable!(), // unused misc
            IncorrectNonce => b"ERR_INCORRECT_NONCE",
            IncorrectArgs => b"ERR_INCORRECT_ARGS",
        }
    }

    pub fn as_error_code(&self) -> u32 {
        use EngineErrorKind::*;
        match self {
            EvmError(_) => 1,
            EvmFatal(_) => 2,
            IncorrectArgs => 3,
            &IncorrectNonce => 4,
        }
    }
}

impl AsRef<[u8]> for EngineErrorKind {
    fn as_ref(&self) -> &[u8] {
        self.as_bytes()
    }
}

impl From<ExitError> for EngineErrorKind {
    fn from(e: ExitError) -> Self {
        EngineErrorKind::EvmError(e)
    }
}

impl From<ExitFatal> for EngineErrorKind {
    fn from(e: ExitFatal) -> Self {
        EngineErrorKind::EvmFatal(e)
    }
}

#[derive(Debug, Clone, Eq, PartialEq)]
pub struct BalanceOverflow;

impl AsRef<[u8]> for BalanceOverflow {
    fn as_ref(&self) -> &[u8] {
        b"ERR_BALANCE_OVERFLOW"
    }
}
/// Errors resulting from trying to pay for gas
#[derive(Debug, Clone, Eq, PartialEq)]
pub enum GasPaymentError {
    /// Overflow adding ETH to an account balance (should never happen)
    BalanceOverflow(BalanceOverflow),
    /// Overflow in gas * gas_price calculation
    EthAmountOverflow,
    /// Not enough balance for account to cover the gas cost
    OutOfFund,
}

impl AsRef<[u8]> for GasPaymentError {
    fn as_ref(&self) -> &[u8] {
        match self {
            Self::BalanceOverflow(overflow) => overflow.as_ref(),
            Self::EthAmountOverflow => b"ERR_GAS_ETH_AMOUNT_OVERFLOW",
            Self::OutOfFund => b"ERR_OUT_OF_FUND",
        }
    }
}

impl From<BalanceOverflow> for GasPaymentError {
    fn from(overflow: BalanceOverflow) -> Self {
        Self::BalanceOverflow(overflow)
    }
}
