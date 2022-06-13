#![allow(unused)]
use std::env;

use std::process::Command;

#[cfg(feature = "build_as_xtop_lib")]
fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();

    Command::new("cp")
        .arg("-f")
        .arg(&format!(
            "{}/../../../../../../../lib/Linux/libxwasmer_runner.a",
            out_dir
        ))
        .arg(&format!("{}", out_dir))
        .status()
        .unwrap();

    println!("cargo:rustc-link-search=native={}", out_dir);
    println!("cargo:rustc-link-lib=static=xwasmer_runner");
}

#[cfg(not(feature = "build_as_xtop_lib"))]
fn main() {}
