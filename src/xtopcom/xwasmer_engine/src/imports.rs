#![allow(unused)]
use crate::runtime::{process_gas_info, GasInfo, Runtime};


mod exports {
    extern "C" {
        pub fn wasmer_block_index() -> u64;
    }
}

pub fn do_block_index(_runtime: &Runtime) -> u64 {
    unsafe { exports::wasmer_block_index() }
}
