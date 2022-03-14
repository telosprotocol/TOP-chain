use crate::{EvmPrecompileResult, Precompile, PrecompileOutput};
use engine_sdk as sdk;
use engine_types::{types::Address, types::EthGas, vec, Borrowed, H256};
use evm::{Context, ExitError};

mod costs {
    use engine_types::types::EthGas;

    pub(super) const ECRECOVER_BASE: EthGas = EthGas::new(3_000);
}

mod consts {
    pub(super) const INPUT_LEN: usize = 128;
    pub(super) const SIGN_LEN: usize = 65;
}

pub fn ecrecover(hash: H256, signature: &[u8]) -> Result<Address, ExitError> {
    assert_eq!(signature.len(), consts::SIGN_LEN);

    #[cfg(feature = "top_crypto")]
    return sdk::ecrecover(hash, signature).map_err(|e| ExitError::Other(Borrowed(e.as_str())));
    
    #[cfg(not(feature = "top_crypto"))]
    internal_impl(hash, signature)
}

fn internal_impl(hash: H256, signature: &[u8]) -> Result<Address, ExitError> {
    use sha3::Digest;

    let hash = secp256k1::Message::parse_slice(hash.as_bytes()).unwrap();
    let v = signature[64];
    let signature = secp256k1::Signature::parse_slice(&signature[0..64]).unwrap();
    let bit = match v {
        0..=26 => v,
        _ => v - 27,
    };

    if let Ok(recovery_id) = secp256k1::RecoveryId::parse(bit) {
        if let Ok(public_key) = secp256k1::recover(&hash, &signature, &recovery_id) {
            // recover returns a 65-byte key, but addresses come from the raw 64-byte key
            let r = sha3::Keccak256::digest(&public_key.serialize()[1..]);
            return Address::try_from_slice(&r[12..])
                .map_err(|_| ExitError::Other(Borrowed("ERR_INCORRECT_ADDRESS")));
        }
    }

    Err(ExitError::Other(Borrowed(sdk::ECRecoverErr.as_str())))
}

pub(super) struct ECRecover;

impl ECRecover {
    pub(super) const ADDRESS: Address = super::make_address(0, 1);
}

impl Precompile for ECRecover {
    fn required_gas(_input: &[u8]) -> Result<EthGas, ExitError> {
        Ok(costs::ECRECOVER_BASE)
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

        let mut input = input.to_vec();
        input.resize(consts::INPUT_LEN, 0);

        let mut hash = [0; 32];
        hash.copy_from_slice(&input[0..32]);

        let mut v = [0; 32];
        v.copy_from_slice(&input[32..64]);

        let mut signature = [0; 65]; // signature is (r, s, v), typed (uint256, uint256, uint8)
        signature[0..32].copy_from_slice(&input[64..96]); // r
        signature[32..64].copy_from_slice(&input[96..128]); // s

        let v_bit = match v[31] {
            27 | 28 if v[..31] == [0; 31] => v[31] - 27,
            _ => {
                return Ok(PrecompileOutput::without_logs(cost, vec![255u8; 32]).into());
            }
        };
        signature[64] = v_bit; // v

        let address_res = ecrecover(H256::from_slice(&hash), &signature);
        let output = match address_res {
            Ok(a) => {
                let mut output = [0u8; 32];
                output[12..32].copy_from_slice(a.as_bytes());
                output.to_vec()
            }
            Err(_) => {
                vec![255u8; 32]
            }
        };

        Ok(PrecompileOutput::without_logs(cost, output).into())
    }
}
