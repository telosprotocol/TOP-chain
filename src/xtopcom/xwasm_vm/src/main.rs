#[link(name = "democapi")]
extern "C" {
    fn c_call(ptr: i64) -> i32;
    fn c_depoly(ptr: i64) -> i32;
}

mod errors;
mod imports;
mod instance;
mod interface;
mod middleware;
mod runtime;
mod size;
mod wasm_backend;

use crate::instance::{Instance, InstanceOptions};
use std::fs::File;
use std::io::Read;
const DEFAULT_GAS_LIMIT: u64 = 400_000;
const DEFAULT_INSTANCE_OPTIONS: InstanceOptions = InstanceOptions {
    gas_limit: DEFAULT_GAS_LIMIT,
    print_debug: false,
};

use wasmer::Val;

fn main() {
    // unsafe {
    //     println!("c_call res: {}", c_call(1));
    //     println!("c_depoly res: {}", c_depoly(2));
    // }
    let path = String::from(
        "/home/charles/Eluvk-project/TOP-chain/src/xtopcom/xwasm_vm/test_rust_in_c/test_erc20.wasm",
    );
    let mut file = match File::open(path) {
        Ok(file) => file,
        Err(err) => {
            // println!("couldn't open file {} {}", path, err);
            return;
        }
    };

    // Read wasm
    let mut wasm = Vec::<u8>::new();
    // todo unwrap()
    file.read_to_end(&mut wasm).unwrap();

    let ins = Box::new(Instance::from_code(&wasm, DEFAULT_INSTANCE_OPTIONS, None)).unwrap();

    println!("{:?}", ins.get_gas_left());
    ins.call_function1("call", &[Val::I64(140730243188608)]).unwrap();
    println!("{:?}", ins.get_gas_left());
}
