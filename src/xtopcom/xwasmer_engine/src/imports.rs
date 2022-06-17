#![allow(unused)]

use crate::{
    errors::{VmError, VmResult},
    memory::write_region,
    runtime::{process_gas_info, GasInfo, Runtime},
};

mod exports {
    extern "C" {
        pub fn wasmer_block_index() -> u64;
        pub fn get_args_bytes_ptr() -> *const u8;
        pub fn get_args_bytes_size() -> u32;
    }
}

pub fn do_block_index(_runtime: &Runtime) -> u64 {
    unsafe { exports::wasmer_block_index() }
}

pub fn do_get_args(_runtime: &Runtime) -> VmResult<u32> {
    let mut args = Vec::<u8>::new();
    unsafe {
        let ptr = exports::get_args_bytes_ptr();
        let sz = exports::get_args_bytes_size();
        for i in 0..sz as usize {
            let iter = ((ptr as usize) + i) as *const u8;
            args.push(*iter);
        }
    }
    println!("{:?}", args);
    write_to_contract(_runtime, args.as_slice())
}

fn write_to_contract(runtime: &Runtime, input: &[u8]) -> VmResult<u32> {
    let out_size = input.len() as u32;
    let result = runtime.call_function1("allocate", &[out_size.into()])?;

    let target_ptr = ref_to_u32(&result)?;

    if target_ptr == 0 {
        return Err(VmError::runtime_err("write_to_contract msg 0"));
    }
    println!(
        "write_to_contract 1, target_ptr: {}, input: {:?}",
        target_ptr, input
    );
    write_region(&runtime.memory(), target_ptr, input)?;
    Ok(target_ptr)
}

/// Safely converts input of type &T to u32.
/// Errors with a cosmwasm_vm::errors::VmError::ConversionErr if conversion cannot be done.
pub fn ref_to_u32<T: TryInto<u32> + ToString + Clone>(input: &T) -> VmResult<u32> {
    input
        .clone()
        .try_into()
        .map_err(|_| VmError::runtime_err("msg"))
}
