use crate::{Context, Precompile};
use engine_sdk::io::ContractBridge;
use engine_sdk::runtime::Runtime;
use engine_types::precompiles::*;
use engine_types::proto_precompile::ContractBridgeArgs;
use engine_types::{types::Address, types::EthGas, PrecompileResult};
use evm::executor::stack::PrecompileFailure;
use evm::ExitFatal;

pub struct Erc20TopPrecompile;

impl Erc20TopPrecompile {
    pub(super) const ADDRESS: Address = crate::make_address(0xff000000, 5);
}

impl Precompile for Erc20TopPrecompile {
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

pub struct EthPrecompile;

impl EthPrecompile {
    pub(super) const ADDRESS: Address = crate::make_address(0xff000000, 6);
}

impl Precompile for EthPrecompile {
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
pub struct Erc20UsdtPrecompile;

impl Erc20UsdtPrecompile {
    pub(super) const ADDRESS: Address = crate::make_address(0xff000000, 7);
}

impl Precompile for Erc20UsdtPrecompile {
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

pub struct Erc20UsdcPrecompile;

impl Erc20UsdcPrecompile {
    pub(super) const ADDRESS: Address = crate::make_address(0xff000000, 8);
}

impl Precompile for Erc20UsdcPrecompile {
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
