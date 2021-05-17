use crate::errors::{VmError, VmResult};
use crate::instance::{Instance, InstanceOptions};
use wasmer::Val;

// const DEFAULT_GAS_LIMIT: u64 = 400_000;
const DEFAULT_GAS_LIMIT: u64 = 0;
const DEFAULT_INSTANCE_OPTIONS: InstanceOptions = InstanceOptions {
    gas_limit: DEFAULT_GAS_LIMIT,
    print_debug: false,
};

pub fn vm_res_handle(vm_res: VmResult<Val>) -> i32 {
    match vm_res {
        Ok(res) => {
            println!("rustvm execute ok {}", res.unwrap_i32());
            0
        }
        Err(err) => match err {
            VmError::GasDepletion {} => {
                println!("gas depletion");
                -2
            }
            _ => {
                println!("unknown {:?}", err);
                -1
            }
        },
    }
}

#[no_mangle]
pub extern "C" fn get_erc20_instance(s: *const u8, size: u32) -> Box<Instance> {
    let ptr = s;
    let mut wasm = Vec::<u8>::new();

    for i in 0..size as usize {
        unsafe {
            let iter = ((ptr as usize) + i) as *const u8;
            wasm.push(*iter);
        }
    }
    // println!("wasm: {:?}", wasm);

    Box::new(Instance::from_code(&wasm, DEFAULT_INSTANCE_OPTIONS, None).unwrap())
}

#[no_mangle]
pub extern "C" fn depoly_erc20(
    ins: &Instance,
    // symbol: *const c_char,
    // total_allowance: u64,
    params: i64,
) -> i32 {
    let gas: i32 = 0;
    vm_res_handle(ins.call_function1(
        "depoly",
        &[Val::I64(params), Val::I64(&gas as *const i32 as i64)],
    ))
}

#[no_mangle]
pub extern "C" fn call_erc20(
    ins: &Instance,
    // function_name:*const c_char,
    // function_args:i32,
    params: i64,
) -> i32 {
    let gas: i32 = 0;
    vm_res_handle(ins.call_function1(
        "call",
        &[Val::I64(params), Val::I64(&gas as *const i32 as i64)],
    ))
}

#[no_mangle]
pub extern "C" fn get_gas_left(ins: &Instance) -> u64 {
    ins.get_gas_left()
}

#[no_mangle]
pub extern "C" fn set_gas_left(ins: &Instance, gas_limit: u64) {
    ins.set_gas_left(gas_limit);
}

#[no_mangle]
pub extern "C" fn release_instance(_ins: Box<Instance>) {}
