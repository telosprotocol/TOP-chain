use crate::runtime::{process_gas_info, GasInfo, Runtime};

#[link(name = "xcontract_api")]
extern "C" {
    fn c_depoly(ptr: i64, gas_ptr: i64) -> i32;
    fn c_call(ptr: i64, gas_ptr: i64) -> i32;
}

pub fn native_c_depoly(runtime: &Runtime, ptr: i64, gas_ptr: i64) -> i32 {
    println!(
        "rust_vm::native_c_depoly import get parm: {} {}",
        ptr, gas_ptr
    );
    let res = unsafe { c_depoly(ptr, gas_ptr) };
    println!("used gas: {}", unsafe { *(gas_ptr as *mut i32) });
    // process_gas_info(runtime, gas as u64);
    match process_gas_info(
        runtime,
        GasInfo::with_cost(unsafe { *(gas_ptr as *mut i32) as u64 }),
    ) {
        Ok(_) => res,
        Err(err) => {
            println!("gas err after c_depoly {:?}", err);
            -2
        }
    }
}

pub fn native_c_call(runtime: &Runtime, ptr: i64, gas_ptr: i64) -> i32 {
    println!("rust_vm::native_c_call import get parm: {}", ptr);
    let res = unsafe { c_call(ptr, gas_ptr) };
    println!("used gas: {}", unsafe { *(gas_ptr as *mut i32) });
    match process_gas_info(
        runtime,
        GasInfo::with_cost(unsafe { *(gas_ptr as *mut i32) as u64 }),
    ) {
        Ok(_) => res,
        Err(err) => {
            println!("gas err after c_call {:?}", err);
            -2
        }
    }
}
