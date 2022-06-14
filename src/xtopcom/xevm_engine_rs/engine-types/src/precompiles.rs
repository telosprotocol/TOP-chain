use crate::types::{Address, EthGas};
use crate::Borrowed;
use crate::{proto_precompile, ContractBridgeArgs};
use evm::executor::stack::Log;
use evm::{executor, Context, ExitError, ExitSucceed, ExitFatal};

/// Rust Precompile Contract always has certain return status: ExitSucceed::Return
/// and most of it has no logs.
/// use this struct to make contract written easier.
#[derive(Debug, Default)]
pub struct RustPrecompileOutput {
    pub cost: EthGas,
    pub output: Vec<u8>,
    pub logs: Vec<Log>,
}
impl RustPrecompileOutput {
    pub fn without_logs(cost: EthGas, output: Vec<u8>) -> Self {
        Self {
            cost,
            output,
            logs: Vec::new(),
        }
    }
}

impl From<RustPrecompileOutput> for executor::stack::PrecompileOutput {
    fn from(output: RustPrecompileOutput) -> Self {
        executor::stack::PrecompileOutput {
            exit_status: ExitSucceed::Returned,
            cost: output.cost.as_u64(),
            output: output.output,
            logs: output.logs,
        }
    }
}

impl From<proto_precompile::PrecompileOutput> for executor::stack::PrecompileOutput {
    fn from(output: proto_precompile::PrecompileOutput) -> Self {
        executor::stack::PrecompileOutput {
            exit_status: match output.get_exit_status() {
                1 => ExitSucceed::Stopped,
                2 => ExitSucceed::Returned,
                _ => ExitSucceed::Suicided,
            },
            cost: output.get_cost(),
            output: output.get_output().clone().into(),
            logs: output.get_logs().iter().map(|r| r.clone().into()).collect(),
        }
    }
}

impl From<proto_precompile::PrecompileFailure> for executor::stack::PrecompileFailure {
    fn from(output: proto_precompile::PrecompileFailure) -> Self {
        match output.get_fail_status() {
            1 => executor::stack::PrecompileFailure::Error {
                exit_status: match output.get_minor_status() {
                    0 => ExitError::StackOverflow,
                    1 => ExitError::StackOverflow,
                    2 => ExitError::InvalidJump,
                    3 => ExitError::InvalidRange,
                    4 => ExitError::DesignatedInvalid,
                    5 => ExitError::CallTooDeep,
                    6 => ExitError::CreateCollision,
                    7 => ExitError::CreateContractLimit,
                    8 => ExitError::InvalidCode,
                    9 => ExitError::OutOfOffset,
                    10 => ExitError::OutOfGas,
                    11 => ExitError::OutOfFund,
                    12 => ExitError::PCUnderflow,
                    13 => ExitError::CreateEmpty,
                    _ => ExitError::Other(Borrowed("Other Error")),
                },
            },
            2 => executor::stack::PrecompileFailure::Revert {
                exit_status: evm::ExitRevert::Reverted,
                output: output.get_output().to_vec(),
                cost: output.get_cost(),
            },
            3 => executor::stack::PrecompileFailure::Fatal {
                exit_status: match output.get_minor_status() {
                    1 => ExitFatal::NotSupported,
                    2 => ExitFatal::UnhandledInterrupt,
                    _ => ExitFatal::Other(Borrowed("Other Error")),
                }
            },
            _ => executor::stack::PrecompileFailure::Fatal {
                exit_status: evm::ExitFatal::NotSupported,
            },
        }
    }
}

pub type PrecompileResult =
    Result<executor::stack::PrecompileOutput, executor::stack::PrecompileFailure>;

impl From<proto_precompile::ContractContext> for evm::Context {
    fn from(context: proto_precompile::ContractContext) -> Self {
        evm::Context {
            address: context.get_address().into(),
            caller: context.get_caller().into(),
            apparent_value: context.get_apparent_value().into(),
        }
    }
}

impl Into<proto_precompile::ContractContext> for evm::Context {
    fn into(self) -> proto_precompile::ContractContext {
        let mut proto_context = proto_precompile::ContractContext::new();
        proto_context.set_address(self.address.into());
        proto_context.set_apparent_value(self.apparent_value.into());
        proto_context.set_caller(self.caller.into());
        proto_context
    }
}

impl ContractBridgeArgs {
    pub fn new_call_args(
        address: Address,
        input: &[u8],
        target_gas: Option<EthGas>,
        context: &Context,
        is_static: bool,
    ) -> Self {
        let mut args = ContractBridgeArgs::new();
        args.set_contract_address(address.into());
        args.set_input(input.to_vec());

        args.set_target_gas(target_gas.unwrap_or(EthGas::new(0)).as_u64());
        args.set_context(context.clone().into());
        args.set_is_static(is_static);
        args
    }
}
