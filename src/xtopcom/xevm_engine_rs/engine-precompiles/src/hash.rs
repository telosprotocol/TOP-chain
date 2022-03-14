use crate::{EvmPrecompileResult, Precompile, PrecompileOutput};
use engine_sdk as sdk;
use engine_types::{types::Address, types::EthGas, vec};
use evm::{Context, ExitError};

mod costs {
    use engine_types::types::EthGas;

    pub(super) const SHA256_BASE: EthGas = EthGas::new(60);

    pub(super) const SHA256_PER_WORD: EthGas = EthGas::new(12);

    pub(super) const RIPEMD160_BASE: EthGas = EthGas::new(600);

    pub(super) const RIPEMD160_PER_WORD: EthGas = EthGas::new(120);
}

mod consts {
    pub(super) const SHA256_WORD_LEN: u64 = 32;

    pub(super) const RIPEMD_WORD_LEN: u64 = 32;
}

/// SHA256 precompile.
pub struct SHA256;

impl SHA256 {
    pub(super) const ADDRESS: Address = super::make_address(0, 2);
}

impl Precompile for SHA256 {
    fn required_gas(input: &[u8]) -> Result<EthGas, ExitError> {
        Ok(
            (input.len() as u64 + consts::SHA256_WORD_LEN - 1) / consts::SHA256_WORD_LEN
                * costs::SHA256_PER_WORD
                + costs::SHA256_BASE,
        )
    }

    /// See: https://ethereum.github.io/yellowpaper/paper.pdf
    /// See: https://docs.soliditylang.org/en/develop/units-and-global-variables.html#mathematical-and-cryptographic-functions
    /// See: https://etherscan.io/address/0000000000000000000000000000000000000002
    fn run(
        &self,
        input: &[u8],
        target_gas: Option<EthGas>,
        _context: &Context,
        _is_static: bool,
    ) -> EvmPrecompileResult {
        use sha2::Digest;

        let cost = Self::required_gas(input)?;
        if let Some(target_gas) = target_gas {
            if cost > target_gas {
                return Err(ExitError::OutOfGas);
            }
        }

        #[cfg(not(feature = "top_crypto"))]
        let output = sha2::Sha256::digest(input).to_vec();

        #[cfg(feature = "top_crypto")]
        let output = sdk::sha256(input).as_bytes().to_vec();

        Ok(PrecompileOutput::without_logs(cost, output).into())
    }
}

/// RIPEMD160 precompile.
pub struct RIPEMD160;

impl RIPEMD160 {
    pub(super) const ADDRESS: Address = super::make_address(0, 3);

    fn internal_impl(input: &[u8]) -> [u8; 20] {
        use ripemd160::Digest;
        let hash = ripemd160::Ripemd160::digest(input);
        let mut output = [0u8; 20];
        output.copy_from_slice(&hash);
        output
    }
}

impl Precompile for RIPEMD160 {
    fn required_gas(input: &[u8]) -> Result<EthGas, ExitError> {
        Ok(
            (input.len() as u64 + consts::RIPEMD_WORD_LEN - 1) / consts::RIPEMD_WORD_LEN
                * costs::RIPEMD160_PER_WORD
                + costs::RIPEMD160_BASE,
        )
    }

    fn run(
        &self,
        input: &[u8],
        target_gas: Option<EthGas>,
        _context: &Context,
        _is_static: bool,
    ) -> EvmPrecompileResult {
        let cost = Self::required_gas(input)?;
        if let Some(target_gas) = target_gas {
            if cost > target_gas {
                return Err(ExitError::OutOfGas);
            }
        }

        #[cfg(not(feature = "top_crypto"))]
        let hash = Self::internal_impl(input);

        #[cfg(feature = "top_crypto")]
        let hash = sdk::ripemd160(input);

        // The result needs to be padded with leading zeros because it is only 20 bytes, but
        // the evm works with 32-byte words.
        let mut output = vec![0u8; 32];
        output[12..].copy_from_slice(&hash);
        Ok(PrecompileOutput::without_logs(cost, output).into())
    }
}
