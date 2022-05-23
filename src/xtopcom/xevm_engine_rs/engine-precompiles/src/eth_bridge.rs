use crate::{Context, Precompile};
use engine_sdk::io::ContractBridge;
use engine_sdk::runtime::Runtime;
use engine_types::precompiles::*;
use engine_types::proto_precompile::ContractBridgeArgs;
use engine_types::{types::Address, types::EthGas, PrecompileResult};
use evm::executor::stack::PrecompileFailure;
use evm::ExitFatal;

pub struct EthBridgePrecompile;

impl EthBridgePrecompile {
    pub(super) const ADDRESS: Address = crate::make_address(0xff000000, 2);
}

impl Precompile for EthBridgePrecompile {
    fn required_gas(input: &[u8]) -> Result<EthGas, PrecompileFailure> {
        Err(PrecompileFailure::Fatal {
            exit_status: ExitFatal::NotSupported,
        })
    }

    fn run(
        &self,
        input: &[u8],
        target_gas: Option<EthGas>,
        context: &Context,
        is_static: bool,
    ) -> PrecompileResult {
        Runtime::extern_contract_call(ContractBridgeArgs::new_call_args(
            Self::ADDRESS,
            input,
            target_gas,
            context,
            is_static,
        ))
    }
}
