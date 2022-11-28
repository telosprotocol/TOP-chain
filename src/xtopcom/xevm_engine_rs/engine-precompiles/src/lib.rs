#![allow(unused)]
// #![allow(dead_code)]
// #![cfg_attr(not(feature = "std"), no_std)]
// #![cfg_attr(not(feature = "std"), feature(alloc_error_handler))]
// #![cfg_attr(feature = "log", feature(panic_info_message))]

pub mod blake2;
pub mod bn128;
pub mod hash;
pub mod identity;
pub mod modexp;
// pub mod native;
// pub mod random;
pub mod secp256k1;

pub mod erc20;
pub mod eth_bridge;
pub mod heco_client;
pub mod bsc_client;
pub mod eth2_client;

use crate::blake2::Blake2F;
use crate::bn128::{Bn128Add, Bn128Mul, Bn128Pair};
use crate::hash::{RIPEMD160, SHA256};
use crate::identity::Identity;
use crate::modexp::ModExp;
// use crate::random::RandomSeed;
use crate::erc20::Erc20TopPrecompile;
use crate::erc20::Erc20UsdcPrecompile;
use crate::erc20::Erc20UsdtPrecompile;
use crate::erc20::EthPrecompile;
use crate::eth_bridge::EthBridgePrecompile;
use crate::heco_client::HecoClientPrecompile;
use crate::bsc_client::BscClientPrecompile;
use crate::eth2_client::Eth2ClientPrecompile;
use crate::secp256k1::ECRecover;
use engine_types::{types::Address, types::EthGas, vec, BTreeMap, Borrowed, Box, Vec, H160, H256};
use evm::executor::stack::PrecompileFailure;
use evm::{backend::Log, executor, Context, ExitError, ExitSucceed};

/// A precompiled function for use in the EVM.
pub trait Precompile {
    /// The required gas in order to run the precompile function.
    fn required_gas(input: &[u8]) -> Result<EthGas, PrecompileFailure>
    where
        Self: Sized;

    /// Runs the precompile function.
    fn run(
        &self,
        input: &[u8],
        target_gas: Option<EthGas>,
        context: &Context,
        is_static: bool,
    ) -> engine_types::PrecompileResult;
}

/// Hard fork marker.
pub trait HardFork {}
/// Homestead hard fork marker.
pub struct Berlin;

impl HardFork for Berlin {}

pub struct Precompiles(pub BTreeMap<Address, Box<dyn Precompile>>);

impl executor::stack::PrecompileSet for Precompiles {
    fn execute(
        &self,
        address: H160,
        input: &[u8],
        gas_limit: Option<u64>,
        context: &Context,
        is_static: bool,
    ) -> Option<Result<executor::stack::PrecompileOutput, executor::stack::PrecompileFailure>> {
        self.0.get(&Address::new(address)).map(|p| {
            p.run(input, gas_limit.map(EthGas::new), context, is_static)
            // .map_err(|exit_status| executor::stack::PrecompileFailure::Error { exit_status })
        })
    }

    fn is_precompile(&self, address: H160) -> bool {
        self.0.contains_key(&Address::new(address))
    }
}

pub struct PrecompileConstructorContext {
    pub random_seed: H256,
}

impl Precompiles {
    pub fn new_berlin(ctx: PrecompileConstructorContext) -> Self {
        let addresses = vec![
            ECRecover::ADDRESS,
            SHA256::ADDRESS,
            RIPEMD160::ADDRESS,
            Identity::ADDRESS,
            ModExp::<Berlin>::ADDRESS,
            Bn128Add::<Berlin>::ADDRESS,
            Bn128Mul::<Berlin>::ADDRESS,
            Bn128Pair::<Berlin>::ADDRESS,
            Blake2F::ADDRESS,
            Erc20TopPrecompile::ADDRESS,
            EthPrecompile::ADDRESS,
            Erc20UsdtPrecompile::ADDRESS,
            Erc20UsdcPrecompile::ADDRESS,
            EthBridgePrecompile::ADDRESS,
            HecoClientPrecompile::ADDRESS,
            BscClientPrecompile::ADDRESS,
            Eth2ClientPrecompile::ADDRESS,
        ];
        let fun: Vec<Box<dyn Precompile>> = vec![
            Box::new(ECRecover),
            Box::new(SHA256),
            Box::new(RIPEMD160),
            Box::new(Identity),
            Box::new(ModExp::<Berlin>::new()),
            Box::new(Bn128Add::<Berlin>::new()),
            Box::new(Bn128Mul::<Berlin>::new()),
            Box::new(Bn128Pair::<Berlin>::new()),
            Box::new(Blake2F),
            Box::new(Erc20TopPrecompile),
            Box::new(EthPrecompile),
            Box::new(Erc20UsdtPrecompile),
            Box::new(Erc20UsdcPrecompile),
            Box::new(EthBridgePrecompile),
            Box::new(HecoClientPrecompile),
            Box::new(BscClientPrecompile),
            Box::new(Eth2ClientPrecompile),
        ];
        let map: BTreeMap<Address, Box<dyn Precompile>> = addresses.into_iter().zip(fun).collect();

        Precompiles(map)
    }

    pub fn new_london(ctx: PrecompileConstructorContext) -> Self {
        // no precompile changes in London HF
        Self::new_berlin(ctx)
    }
}

/// fn for making an address by concatenating the bytes from two given numbers,
/// Note that 32 + 128 = 160 = 20 bytes (the length of an address). This function is used
/// as a convenience for specifying the addresses of the various precompiles.
pub const fn make_address(x: u32, y: u128) -> Address {
    let x_bytes = x.to_be_bytes();
    let y_bytes = y.to_be_bytes();
    Address::new(H160([
        x_bytes[0],
        x_bytes[1],
        x_bytes[2],
        x_bytes[3],
        y_bytes[0],
        y_bytes[1],
        y_bytes[2],
        y_bytes[3],
        y_bytes[4],
        y_bytes[5],
        y_bytes[6],
        y_bytes[7],
        y_bytes[8],
        y_bytes[9],
        y_bytes[10],
        y_bytes[11],
        y_bytes[12],
        y_bytes[13],
        y_bytes[14],
        y_bytes[15],
    ]))
}

const fn make_h256(x: u128, y: u128) -> H256 {
    let x_bytes = x.to_be_bytes();
    let y_bytes = y.to_be_bytes();
    H256([
        x_bytes[0],
        x_bytes[1],
        x_bytes[2],
        x_bytes[3],
        x_bytes[4],
        x_bytes[5],
        x_bytes[6],
        x_bytes[7],
        x_bytes[8],
        x_bytes[9],
        x_bytes[10],
        x_bytes[11],
        x_bytes[12],
        x_bytes[13],
        x_bytes[14],
        x_bytes[15],
        y_bytes[0],
        y_bytes[1],
        y_bytes[2],
        y_bytes[3],
        y_bytes[4],
        y_bytes[5],
        y_bytes[6],
        y_bytes[7],
        y_bytes[8],
        y_bytes[9],
        y_bytes[10],
        y_bytes[11],
        y_bytes[12],
        y_bytes[13],
        y_bytes[14],
        y_bytes[15],
    ])
}
