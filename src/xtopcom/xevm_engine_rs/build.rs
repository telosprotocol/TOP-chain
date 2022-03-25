#![allow(unused)]
use std::env;

use std::process::Command;

#[cfg(feature = "build_as_xtop_lib")]
fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();

    // from: target/release/build/xevm_engine-e40834640f8d4de0/out/
    // to :|-  evm_engine_rs/debug/...
    //     |-  lib/linux/*.a
    // Command::new("cp")
    //     .arg("-f")
    //     .arg(&format!(
    //         "{}/../../../../../lib/Linux/libxevm_runner.a",
    //         out_dir
    //     ))
    //     .arg(&format!("{}", out_dir))
    //     .status()
    //     .unwrap();
    
    // product: 
    // from: cbuild/src/xtopcom/xevm_engine_rs/debug/build/xevm_engine-94e9d92524952b17/out/
    // to  : cbuild/lib/Linux
    Command::new("cp")
        .arg("-f")
        .arg(&format!(
            "{}/../../../../../../../lib/Linux/libxevm_runner.a",
            out_dir
        ))
        .arg(&format!("{}", out_dir))
        .status()
        .unwrap();

    println!("cargo:rustc-link-search=native={}", out_dir);
    println!("cargo:rustc-link-lib=static=xevm_runner");
}

#[cfg(not(feature = "build_as_xtop_lib"))]
fn main() {}
