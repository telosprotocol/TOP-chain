use std::env;
use std::process::Command;
use std::path::Path;
 
fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();
 
    // Command::new("cd").args(&[".."]);
    Command::new("g++").args(&["xchain_capi_demo/main.cpp", "-O3","-c", "-fPIC", "-o"])
        .arg(&format!("{}/demo.o", out_dir))
        .status().unwrap();
 
    Command::new("ar").args(&["crus", "libdemocapi.a", "demo.o"])
        .current_dir(&Path::new(&out_dir))
        .status().unwrap();
 
    println!("cargo:rustc-link-search=native={}", out_dir);
    println!("cargo:rustc-link-lib=static=democapi");
}