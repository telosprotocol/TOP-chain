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
            match res.ty() {
                wasmer::ValType::I32 => println!("rustvm execute ok {}", res.unwrap_i32()),
                wasmer::ValType::I64 => println!("rustvm execute ok {}", res.unwrap_i64()),
                wasmer::ValType::F32 => println!("rustvm execute ok {}", res.unwrap_f32()),
                wasmer::ValType::F64 => println!("rustvm execute ok {}", res.unwrap_f64()),
                wasmer::ValType::V128 => println!("rustvm execute ok {}", res.unwrap_v128()),
                _ => (),
            }

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
pub extern "C" fn deploy_contract(code_ptr: *const u8, size: u32) -> Box<Instance> {
    let mut wasm = Vec::<u8>::new();

    for i in 0..size as usize {
        unsafe {
            let iter = ((code_ptr as usize) + i) as *const u8;
            wasm.push(*iter);
        }
    }
    Box::new(Instance::from_code(&wasm, DEFAULT_INSTANCE_OPTIONS, None).unwrap())
}

#[no_mangle]
pub extern "C" fn call_contract_1(ins: &Instance, a: i32, b: i32) -> i32 {
    vm_res_handle(ins.call_function1("add", &[Val::I32(a), Val::I32(b)]))
}

#[no_mangle]
pub extern "C" fn call_contract_2(ins: &Instance) -> i32 {
    vm_res_handle(ins.call_function1("blk_index", &[]))
}
#[no_mangle]
pub extern "C" fn call_contract_3(ins: &Instance) -> i32 {
    vm_res_handle(ins.call_function1("nothing", &[]))
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
