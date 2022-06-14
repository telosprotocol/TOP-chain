use crate::{Precompile, PrecompileFailure};
use engine_types::types::{Address, EthGas};
use engine_types::{PrecompileResult, RustPrecompileOutput};
use evm::{Context, ExitError};

/// Identity precompile costs.
mod costs {
    use engine_types::types::EthGas;

    /// The base cost of the operation.
    pub(super) const IDENTITY_BASE: EthGas = EthGas::new(15);

    /// The cost per word.
    pub(super) const IDENTITY_PER_WORD: EthGas = EthGas::new(3);
}

mod consts {
    /// Length of the identity word.
    pub(super) const IDENTITY_WORD_LEN: u64 = 32;
}

pub struct Identity;

impl Identity {
    pub(super) const ADDRESS: Address = super::make_address(0, 4);
}

impl Precompile for Identity {
    fn required_gas(input: &[u8]) -> Result<EthGas, PrecompileFailure> {
        Ok(
            (input.len() as u64 + consts::IDENTITY_WORD_LEN - 1) / consts::IDENTITY_WORD_LEN
                * costs::IDENTITY_PER_WORD
                + costs::IDENTITY_BASE,
        )
    }

    /// Takes the input bytes, copies them, and returns it as the output.
    ///
    /// See: https://ethereum.github.io/yellowpaper/paper.pdf
    /// See: https://etherscan.io/address/0000000000000000000000000000000000000004
    fn run(
        &self,
        input: &[u8],
        target_gas: Option<EthGas>,
        _context: &Context,
        _is_static: bool,
    ) -> PrecompileResult {
        let cost = Self::required_gas(input)?;
        if let Some(target_gas) = target_gas {
            if cost > target_gas {
                return Err(PrecompileFailure::Error {
                    exit_status: ExitError::OutOfGas,
                });
            }
        }

        Ok(RustPrecompileOutput::without_logs(cost, input.to_vec()).into())
    }
}
