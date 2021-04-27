use std::env;
// use std::path::Path;
use std::process::Command;

fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();
    // // Command::new("cd").args(&[".."]);
    // Command::new("g++")
    //     .args(&["xchain_capi_demo/main.cpp", "-O3", "-c", "-fPIC", "-o"])
    //     .arg(&format!("{}/demo.o", out_dir))
    //     .status()
    //     .unwrap();

    // Command::new("ar")
    //     .args(&["crus", "libdemocapi.a", "demo.o"])
    //     .current_dir(&Path::new(&out_dir))
    //     .status()
    //     .unwrap();

    // Command::new("cp")
    //     .args(&["/home/charles/Eluvk-project/TOP-chain/cbuild/lib/Linux/libxcontract_api.a"])
    //     .arg(&format!("{}/libxcontract_api.a", out_dir))
    //     .status()
    //     .unwrap();

    Command::new("cp")
        .args(&["/home/charles/Eluvk-project/TOP-chain/cbuild/lib/Linux/libxcontract_api.so"])
        .arg(&format!("{}/libxcontract_api.so", out_dir))
        .status()
        .unwrap();

    println!("cargo:rustc-link-search=native={}", out_dir);
    // println!("cargo:rustc-link-lib=static=democapi");
    // println!("cargo:rustc-link-lib=static=xcontract_api");
    println!("cargo:rustc-link-lib=dylib=xcontract_api");
}
