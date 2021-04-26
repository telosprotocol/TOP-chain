use crate::runtime::{process_gas_info, GasInfo, Runtime};

#[link(name = "democapi")]
extern "C" {
    fn c_call(ptr: i64) -> i32;
    fn c_depoly(ptr: i64) -> i32;
}

pub fn native_c_depoly(runtime: &Runtime, ptr: i64) -> i32 {
    println!("rust_vm::native_c_depoly import get parm: {}", ptr);
    match process_gas_info(runtime, GasInfo::with_cost(unsafe { c_depoly(ptr) } as u64)) {
        Ok(_) => 0,
        Err(err) => {
            println!("gas err after c_depoly {:?}", err);
            -1
        }
    }
}

pub fn native_c_call(runtime: &Runtime, ptr: i64) -> i32 {
    println!("rust_vm::native_c_call import get parm: {}", ptr);
    match process_gas_info(runtime, GasInfo::with_cost(unsafe { c_call(ptr) } as u64)) {
        Ok(_) => 0,
        Err(err) => {
            println!("gas err after c_call {:?}", err);
            -1
        }
    }
}
