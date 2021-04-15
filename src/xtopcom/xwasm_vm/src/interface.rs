use std::ffi::CStr;
use std::fs::File;
use std::io::Read;
use std::os::raw::c_char;

use crate::wasm_backend::compile;

#[no_mangle]
pub extern "C" fn validate_wasm_with_content(s: *const u8, size: u32) -> bool {
    let ptr = s;
    let mut wasm = Vec::<u8>::new();

    for i in 0..size as usize {
        unsafe {
            let iter = ((ptr as usize) + i) as *const u8;
            wasm.push(*iter);
            // println!("{}:{:?} : {:?}", i, iter, *iter);
        }
    }
    println!("wasm: {:?}", wasm);

    match compile(&wasm, None) {
        Ok(_) => true,
        Err(err) => {
            println!("compile wasm failed {}", err);
            false
        }
    }
}

#[no_mangle]
pub extern "C" fn validate_wasm_with_path(s: *const c_char) -> bool {
    let path;
    unsafe {
        // todo unwrap()
        path = CStr::from_ptr(s).to_str().unwrap();
        println!("here is wasm file path: {:?}", path);
    }

    match do_validate_wasm_with_path(path) {
        Some(_) => true,
        None => false,
    }
}

use wasmer::Module;
fn do_validate_wasm_with_path(path: &str) -> Option<Module> {
    let mut file = match File::open(path) {
        Ok(file) => file,
        Err(err) => {
            println!("couldn't open file {} {}", path, err);
            return None;
        }
    };

    // Read wasm
    let mut wasm = Vec::<u8>::new();
    // todo unwrap()
    file.read_to_end(&mut wasm).unwrap();

    return match compile(&wasm, None) {
        Ok(module) => Some(module),
        Err(err) => {
            println!("compile wasm failed {}", err);
            None
        }
    };
}

// #[no_mangle]
// pub extern "C" fn get_module(s: *const u8, size: u32) -> Box<Module> {
//     let ptr = s;
//     let mut wasm = Vec::<u8>::new();

//     for i in 0..size as usize {
//         unsafe {
//             let iter = ((ptr as usize) + i) as *const u8;
//             wasm.push(*iter);
//             // println!("{}:{:?} : {:?}", i, iter, *iter);
//         }
//     }
//     println!("wasm: {:?}", wasm);

//     Box::new(compile(&wasm, None).unwrap())
// }

// // pub enum OpaqueStruct{}

// #[no_mangle]
// pub extern "C" fn use_module(args: Box<Module>) -> bool {
//     let module = *args;
//     let import_object = wasmer::imports! {};
//     println!("Instantiating module...");
//     // Let's instantiate the Wasm module.
//     let instance = wasmer::Instance::new(&module, &import_object).unwrap();
//     wasmer_middlewares::metering::set_remaining_points(&instance, 100);

//     let add = instance
//         .exports
//         .get_function("add")
//         .unwrap()
//         .native::<(i32, i32), i32>()
//         .unwrap();

//     println!("Calling `add` function once...");
//     println!("{}", add.call(1, 2).unwrap());

//     true
// }

use crate::backend::{Backend, BackendApi, Querier, Storage};
use crate::instance::{Instance, InstanceOptions};
use wasmer::Val;

const DEFAULT_GAS_LIMIT: u64 = 400_000;
const DEFAULT_INSTANCE_OPTIONS: InstanceOptions = InstanceOptions {
    gas_limit: DEFAULT_GAS_LIMIT,
    print_debug: false,
};

#[derive(Copy, Clone)]
pub struct MockApi {}
impl BackendApi for MockApi {}

pub struct MockStorage {}
impl Storage for MockStorage {}

pub struct MockQuerier {}
impl Querier for MockQuerier {}

pub fn mock_backend() -> Backend<MockApi, MockStorage, MockQuerier> {
    Backend {
        api: MockApi {},
        storage: MockStorage {},
        querier: MockQuerier {},
    }
}

#[no_mangle]
pub extern "C" fn get_instance(
    s: *const u8,
    size: u32,
) -> Box<Instance<MockApi, MockStorage, MockQuerier>> {
    let ptr = s;
    let mut wasm = Vec::<u8>::new();

    for i in 0..size as usize {
        unsafe {
            let iter = ((ptr as usize) + i) as *const u8;
            wasm.push(*iter);
            // println!("{}:{:?} : {:?}", i, iter, *iter);
        }
    }
    println!("wasm: {:?}", wasm);

    Box::new(Instance::from_code(&wasm, mock_backend(), DEFAULT_INSTANCE_OPTIONS, None).unwrap())
}

#[no_mangle]
pub extern "C" fn use_instance(args: Box<Instance<MockApi, MockStorage, MockQuerier>>) -> bool {
    let instance = *args;

    let res = instance
        .call_function1("add", &[Val::I32(1), Val::I32(2)])
        .unwrap();
    assert_eq!(res, Val::I32(3));

    true
}
