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
